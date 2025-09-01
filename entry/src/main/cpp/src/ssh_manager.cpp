#include "ssh_manager.h"
#include "global.h"
#include <cstring>
#include <fstream>
#include <hilog/log.h>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// mbedtls 头文件
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/ecp.h>
#include <mbedtls/entropy.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <mbedtls/sha256.h>

SSHManager::SSHManager() : session_(nullptr) {
    OH_LOG_INFO(LOG_APP, "SSHManager::SSHManager");

    // 初始化libssh2库
    int result = libssh2_init(0);
    if (result != 0) {
        setError("Failed to initialize libssh2 library");
        OH_LOG_ERROR(LOG_APP, "Failed to initialize libssh2 library: %{public}d", result);
        return;
    }

    // 创建SSH会话
    session_ = libssh2_session_init();
    if (!session_) {
        setError("Failed to create SSH session");
        OH_LOG_ERROR(LOG_APP, "Failed to create SSH session");
        return;
    }

    // 设置会话选项
    libssh2_session_set_blocking(session_, 1);
}

SSHManager::~SSHManager() {
    freeResources();
    libssh2_exit();
}

void SSHManager::freeResources() {
    if (session_) {
        libssh2_session_disconnect(session_, "Goodbye");
        libssh2_session_free(session_);
        session_ = nullptr;
    }
}

void SSHManager::setError(const std::string &error) {
    lastError_ = error;
    OH_LOG_ERROR(LOG_APP, "SSHManager error: %{public}s", error.c_str());
}

bool SSHManager::checkError(int result, const std::string &operation) {
    if (result < 0) {
        char *sshError = nullptr;
        int sshErrorLen = 0;
        libssh2_session_last_error(session_, &sshError, &sshErrorLen, 0);

        if (sshError && sshErrorLen > 0) {
            setError(operation + " failed: " + std::string(sshError, sshErrorLen));
        } else {
            setError(operation + " failed with unknown error");
        }
        return false;
    }
    return true;
}

bool SSHManager::generateKeyPair(int bits, const std::string &comment, const std::string &passphrase) {
    OH_LOG_INFO(LOG_APP, "Generating SSH key pair: bits=%{public}d", bits);

    keyInfo_.keyType = "ssh-rsa";
    keyInfo_.comment = comment.empty() ? "higit@openharmony" : comment;
    return generateRSAKeyPairWithMbedTLS(bits, passphrase);
}

bool SSHManager::generateRSAKeyPairWithMbedTLS(int bits, const std::string &passphrase) {
    OH_LOG_INFO(LOG_APP, "Generating RSA key pair with %{public}d bits using mbedtls", bits);

    // 初始化 mbedtls 结构
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // 设置随机数生成器
    const char *pers = "higit_ssh_keygen";
    int ret =
        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        setError("Failed to seed random number generator: " + std::to_string(ret));
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 生成 RSA 密钥对
    ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));
    if (ret != 0) {
        setError("Failed to setup RSA key: " + std::to_string(ret));
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &ctr_drbg, bits, 65537);
    if (ret != 0) {
        setError("Failed to generate RSA key pair: " + std::to_string(ret));
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 将私钥转换为 PEM 格式
    unsigned char private_key_buffer[8192];

    ret = mbedtls_pk_write_key_pem(&pk, private_key_buffer, sizeof(private_key_buffer));
    if (ret != 0) {
        setError("Failed to write private key to PEM: " + std::to_string(ret));
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 将公钥转换为 PEM 格式
    unsigned char public_key_buffer[8192];

    ret = mbedtls_pk_write_pubkey_pem(&pk, public_key_buffer, sizeof(public_key_buffer));
    if (ret != 0) {
        setError("Failed to write public key to PEM: " + std::to_string(ret));
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 保存密钥内容
    keyInfo_.privateKey = std::string((char *)private_key_buffer);
    std::string rawPublicKey = std::string((char *)public_key_buffer);

    // 转换为 OpenSSH 格式
    keyInfo_.publicKey = convertToOpenSSHFormat(rawPublicKey, keyInfo_.comment);

    // 计算指纹
    keyInfo_.fingerprint = calculateFingerprint(keyInfo_.publicKey);

    OH_LOG_INFO(LOG_APP, "RSA key pair generated successfully using mbedtls");

    // 清理资源
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return true;
}

std::string SSHManager::convertToOpenSSHFormat(const std::string &pemPublicKey, const std::string &comment) {
    // 从 PEM 格式的公钥中提取 RSA 公钥数据
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    // 解析 PEM 格式的公钥
    int ret = mbedtls_pk_parse_public_key(&pk, (const unsigned char *)pemPublicKey.c_str(), pemPublicKey.length() + 1);
    if (ret != 0) {
        mbedtls_pk_free(&pk);
        setError("Failed to parse PEM public key: " + std::to_string(ret));
        return "";
    }

    // 检查是否是 RSA 密钥
    if (mbedtls_pk_get_type(&pk) != MBEDTLS_PK_RSA) {
        mbedtls_pk_free(&pk);
        setError("Public key is not RSA type");
        return "";
    }

    // 获取 RSA 上下文
    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(pk);

    // 导出公钥数据
    mbedtls_mpi N, E;
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&E);

    ret = mbedtls_rsa_export(rsa, &N, NULL, NULL, NULL, &E);
    if (ret != 0) {
        mbedtls_mpi_free(&N);
        mbedtls_mpi_free(&E);
        mbedtls_pk_free(&pk);
        setError("Failed to export RSA public key: " + std::to_string(ret));
        return "";
    }

    // 计算指数和模数长度
    size_t exponent_len = mbedtls_mpi_size(&E);
    size_t modulus_len = mbedtls_mpi_size(&N);

    // 处理前导零（OpenSSH 要求正数前加 0 字节）
    bool add_leading_zero_e = (exponent_len > 0 && mbedtls_mpi_get_bit(&E, (exponent_len * 8) - 1) == 1);
    bool add_leading_zero_n = (modulus_len > 0 && mbedtls_mpi_get_bit(&N, (modulus_len * 8) - 1) == 1);
    size_t exponent_data_len = exponent_len + (add_leading_zero_e ? 1 : 0);
    size_t modulus_data_len = modulus_len + (add_leading_zero_n ? 1 : 0);

    // 计算各部分长度
    size_t ssh_rsa_len = 7;        // "ssh-rsa" 字符串长度
    size_t ssh_rsa_len_bytes = 4;  // 密钥类型长度字段
    size_t exponent_len_bytes = 4; // 指数长度字段
    size_t modulus_len_bytes = 4;  // 模数长度字段

    // 计算总长度
    size_t total_len =
        ssh_rsa_len_bytes + ssh_rsa_len + exponent_len_bytes + exponent_data_len + modulus_len_bytes + modulus_data_len;

    std::vector<unsigned char> ssh_key_data(total_len);
    size_t offset = 0;

    // 写入 "ssh-rsa" 字符串长度 (4 bytes, big-endian)
    ssh_key_data[offset++] = 0;
    ssh_key_data[offset++] = 0;
    ssh_key_data[offset++] = 0;
    ssh_key_data[offset++] = ssh_rsa_len;

    memcpy(&ssh_key_data[offset], "ssh-rsa", ssh_rsa_len);
    offset += ssh_rsa_len;

    // 写入指数长度 (4 bytes, big-endian)
    ssh_key_data[offset++] = exponent_data_len >> 24;
    ssh_key_data[offset++] = exponent_data_len >> 16;
    ssh_key_data[offset++] = exponent_data_len >> 8;
    ssh_key_data[offset++] = exponent_data_len & 0xFF;

    // 写入指数数据（如果需要前导零）
    if (add_leading_zero_e) {
        ssh_key_data[offset++] = 0;
    }
    mbedtls_mpi_write_binary(&E, &ssh_key_data[offset], exponent_len);
    offset += exponent_len;

    // 写入模数长度 (4 bytes, big-endian)
    ssh_key_data[offset++] = modulus_data_len >> 24;
    ssh_key_data[offset++] = modulus_data_len >> 16;
    ssh_key_data[offset++] = modulus_data_len >> 8;
    ssh_key_data[offset++] = modulus_data_len & 0xFF;

    // 写入模数数据（如果需要前导零）
    if (add_leading_zero_n) {
        ssh_key_data[offset++] = 0;
    }
    mbedtls_mpi_write_binary(&N, &ssh_key_data[offset], modulus_len);
    offset += modulus_len;

    // 验证长度
    if (offset != total_len) {
        mbedtls_mpi_free(&N);
        mbedtls_mpi_free(&E);
        mbedtls_pk_free(&pk);
        setError("Failed to construct OpenSSH public key: incorrect length");
        return "";
    }

    // 转换为 Base64
    std::string base64_key = base64Encode(ssh_key_data);
    if (base64_key.empty()) {
        mbedtls_mpi_free(&N);
        mbedtls_mpi_free(&E);
        mbedtls_pk_free(&pk);
        setError("Failed to encode public key to base64");
        return "";
    }

    // 清理资源
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&E);
    mbedtls_pk_free(&pk);

    return "ssh-rsa " + base64_key + " " + comment;
}

std::string SSHManager::base64Encode(const std::vector<unsigned char> &data) {
    // 使用 mbedtls 内置的 Base64 编码函数
    size_t output_len;
    int ret = mbedtls_base64_encode(nullptr, 0, &output_len, data.data(), data.size());
    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        return "";
    }

    std::vector<unsigned char> output(output_len);
    ret = mbedtls_base64_encode(output.data(), output.size(), &output_len, data.data(), data.size());
    if (ret != 0) {
        return "";
    }

    return std::string((char *)output.data(), output_len);
}

std::string SSHManager::generateRandomBase64(int length) {
    // 使用 mbedtls 的随机数生成器生成真实的随机数据
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 63);

    const std::string charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    result.reserve(length);

    for (int i = 0; i < length; ++i) {
        result += charset[dis(gen)];
    }

    return result;
}

bool SSHManager::loadKeyPair(const std::string &privateKeyPath, const std::string &publicKeyPath,
                             const std::string &passphrase) {
    OH_LOG_INFO(LOG_APP, "Loading SSH key pair from: %{public}s", privateKeyPath.c_str());

    try {
        // 读取私钥文件
        std::ifstream privateKeyFile(privateKeyPath, std::ios::binary);
        if (!privateKeyFile.is_open()) {
            setError("Failed to open private key file: " + privateKeyPath);
            return false;
        }

        std::stringstream privateKeyBuffer;
        privateKeyBuffer << privateKeyFile.rdbuf();
        keyInfo_.privateKey = privateKeyBuffer.str();
        privateKeyFile.close();

        // 读取公钥文件（如果提供）
        if (!publicKeyPath.empty()) {
            std::ifstream publicKeyFile(publicKeyPath, std::ios::binary);
            if (publicKeyFile.is_open()) {
                std::stringstream publicKeyBuffer;
                publicKeyBuffer << publicKeyFile.rdbuf();
                keyInfo_.publicKey = publicKeyBuffer.str();
                publicKeyFile.close();
            }
        }

        // 尝试从私钥中提取公钥信息（如果公钥文件未提供）
        if (keyInfo_.publicKey.empty()) {
            if (!extractPublicKeyFromPrivateKey(passphrase)) {
                setError("Failed to extract public key from private key");
                return false;
            }
        }

        // 设置密钥类型
        if (keyInfo_.privateKey.find("RSA") != std::string::npos ||
            keyInfo_.publicKey.find("ssh-rsa") != std::string::npos) {
            keyInfo_.keyType = "ssh-rsa";
        } else if (keyInfo_.privateKey.find("OPENSSH") != std::string::npos ||
                   keyInfo_.publicKey.find("ssh-ed25519") != std::string::npos) {
            keyInfo_.keyType = "ssh-ed25519";
        } else {
            keyInfo_.keyType = "unknown";
        }

        // 设置注释
        keyInfo_.comment = "Loaded from " + privateKeyPath;

        // 计算指纹
        keyInfo_.fingerprint = calculateFingerprint(keyInfo_.publicKey);

        OH_LOG_INFO(LOG_APP, "SSH key pair loaded successfully");
        return true;
    } catch (const std::exception &e) {
        setError("Failed to load key pair: " + std::string(e.what()));
        return false;
    }
}

bool SSHManager::extractPublicKeyFromPrivateKey(const std::string &passphrase) {
    // 使用 mbedtls 从私钥中提取公钥
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    // 设置随机数生成器
    const char *pers = "higit_ssh_keygen";
    int ret =
        mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers));
    if (ret != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 尝试解析私钥，使用随机数生成器
    ret = mbedtls_pk_parse_key(&pk, (const unsigned char *)keyInfo_.privateKey.c_str(), keyInfo_.privateKey.length(),
                               passphrase.empty() ? nullptr : (const unsigned char *)passphrase.c_str(),
                               passphrase.length(), mbedtls_ctr_drbg_random, &ctr_drbg);

    if (ret != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 将公钥转换为 PEM 格式
    unsigned char public_key_buffer[8192];
    ret = mbedtls_pk_write_pubkey_pem(&pk, public_key_buffer, sizeof(public_key_buffer));

    if (ret != 0) {
        mbedtls_pk_free(&pk);
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        return false;
    }

    // 转换为 OpenSSH 格式
    keyInfo_.publicKey = convertToOpenSSHFormat(std::string((char *)public_key_buffer), keyInfo_.comment);

    // 清理资源
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);

    return !keyInfo_.publicKey.empty();
}

bool SSHManager::saveKeyPair(const std::string &privateKeyPath, const std::string &publicKeyPath,
                             const std::string &passphrase) {
    OH_LOG_INFO(LOG_APP, "Saving SSH key pair to: %{public}s and %{public}s", privateKeyPath.c_str(),
                publicKeyPath.c_str());

    try {
        // 创建目录（如果不存在）
        std::string dir = privateKeyPath.substr(0, privateKeyPath.find_last_of('/'));
        if (!dir.empty() && access(dir.c_str(), F_OK) != 0) {
            if (mkdir(dir.c_str(), 0700) != 0) {
                setError("Failed to create directory: " + dir);
                return false;
            }
        }

        // 保存私钥
        std::ofstream privateKeyFile(privateKeyPath, std::ios::binary | std::ios::trunc);
        if (!privateKeyFile.is_open()) {
            setError("Failed to create private key file: " + privateKeyPath);
            return false;
        }
        privateKeyFile << keyInfo_.privateKey;
        privateKeyFile.close();

        // 设置私钥文件权限为 600
        if (chmod(privateKeyPath.c_str(), S_IRUSR | S_IWUSR) != 0) {
            setError("Failed to set private key file permissions");
            return false;
        }

        // 保存公钥
        std::ofstream publicKeyFile(publicKeyPath, std::ios::binary | std::ios::trunc);
        if (!publicKeyFile.is_open()) {
            setError("Failed to create public key file: " + publicKeyPath);
            return false;
        }
        publicKeyFile << keyInfo_.publicKey << "\n";
        publicKeyFile.close();

        // 设置公钥文件权限为 644
        if (chmod(publicKeyPath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) != 0) {
            setError("Failed to set public key file permissions");
            return false;
        }

        OH_LOG_INFO(LOG_APP, "SSH key pair saved successfully");
        return true;
    } catch (const std::exception &e) {
        setError("Failed to save key pair: " + std::string(e.what()));
        return false;
    }
}

std::string SSHManager::getPublicKey() const { return keyInfo_.publicKey; }

std::string SSHManager::getPrivateKey() const { return keyInfo_.privateKey; }

std::string SSHManager::getFingerprint() const { return keyInfo_.fingerprint; }

std::string SSHManager::getKeyType() const { return keyInfo_.keyType; }

std::string SSHManager::getComment() const { return keyInfo_.comment; }

bool SSHManager::validateKeyPair() const {
    // 验证密钥对的有效性
    if (keyInfo_.publicKey.empty() || keyInfo_.privateKey.empty()) {
        return false;
    }

    // 检查私钥格式
    if (keyInfo_.privateKey.find("-----BEGIN") == std::string::npos) {
        return false;
    }

    // 检查公钥格式
    if (keyInfo_.publicKey.find("ssh-") == std::string::npos) {
        return false;
    }

    return true;
}

bool SSHManager::setComment(const std::string &comment) {
    keyInfo_.comment = comment;
    return true;
}

bool SSHManager::changePassphrase(const std::string &oldPassphrase, const std::string &newPassphrase) {
    OH_LOG_INFO(LOG_APP, "Changing passphrase for SSH key");

    // 重新生成密钥对（使用新密码）
    std::string oldKeyType = keyInfo_.keyType;
    std::string oldComment = keyInfo_.comment;

    if (!generateKeyPair(4096, oldComment, newPassphrase)) {
        setError("Failed to regenerate key pair with new passphrase");
        return false;
    }

    return true;
}

std::string SSHManager::getLastError() const { return lastError_; }

std::string SSHManager::calculateFingerprint(const std::string &publicKey) {
    // 提取公钥的实际内容（去掉类型和注释部分）
    std::string keyData = publicKey;
    size_t firstSpace = keyData.find(' ');
    if (firstSpace != std::string::npos) {
        keyData = keyData.substr(firstSpace + 1);
        size_t secondSpace = keyData.find(' ');
        if (secondSpace != std::string::npos) {
            keyData = keyData.substr(0, secondSpace);
        }
    }

    // 解码 base64 数据为二进制数据
    size_t decoded_len;
    int ret = mbedtls_base64_decode(nullptr, 0, &decoded_len, (const unsigned char *)keyData.c_str(), keyData.length());
    if (ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL) {
        return "SHA256:ERROR";
    }

    std::vector<unsigned char> decoded_data(decoded_len);
    ret = mbedtls_base64_decode(decoded_data.data(), decoded_data.size(), &decoded_len,
                                (const unsigned char *)keyData.c_str(), keyData.length());
    if (ret != 0) {
        return "SHA256:ERROR";
    }

    // 使用 mbedtls 计算 SHA256 指纹
    unsigned char hash[32]; // SHA256 输出长度
    mbedtls_sha256_context sha256_ctx;

    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0); // 0 = SHA256
    mbedtls_sha256_update(&sha256_ctx, decoded_data.data(), decoded_len);
    mbedtls_sha256_finish(&sha256_ctx, hash);
    mbedtls_sha256_free(&sha256_ctx);

    // 转换为十六进制字符串
    std::stringstream ss;
    ss << "SHA256:";
    for (int i = 0; i < 32; i++) {
        ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(hash[i]);
        if (i < 31) {
            ss << ":";
        }
    }

    return ss.str();
}