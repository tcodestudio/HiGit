#ifndef HIGIT_SSH_MANAGER_H
#define HIGIT_SSH_MANAGER_H

#include <libssh2.h>
#include <string>
#include <vector>

// Forward declarations for mbedtls
struct mbedtls_pk_context;
struct mbedtls_entropy_context;
struct mbedtls_ctr_drbg_context;

/**
 * @brief SSH密钥信息结构体
 */
struct SSHKeyInfo {
    std::string publicKey;   ///< 公钥内容
    std::string privateKey;  ///< 私钥内容（加密存储）
    std::string keyType;     ///< 密钥类型（ssh-rsa, ssh-ed25519等）
    std::string comment;     ///< 密钥注释
    std::string fingerprint; ///< 密钥指纹
};

/**
 * @brief SSH管理类
 * 封装了libssh2库的常用操作，提供SSH密钥管理和认证功能
 */
class SSHManager {
public:
    /**
     * @brief 构造函数
     * 初始化libssh2库
     */
    SSHManager();

    /**
     * @brief 析构函数
     * 释放所有资源并关闭libssh2库
     */
    ~SSHManager();

    // 禁止拷贝
    SSHManager(const SSHManager &) = delete;
    SSHManager &operator=(const SSHManager &) = delete;

    /**
     * @brief 生成新的SSH密钥对
     * @param keyType 密钥类型（"ssh-rsa", "ssh-ed25519"）
     * @param bits 密钥位数（仅对RSA有效）
     * @param comment 密钥注释
     * @param passphrase 保护私钥的密码（可选）
     * @return 成功返回true，失败返回false
     */
    bool generateKeyPair(int bits = 4096, const std::string &comment = "", const std::string &passphrase = "");

    /**
     * @brief 从文件加载SSH密钥对
     * @param privateKeyPath 私钥文件路径
     * @param publicKeyPath 公钥文件路径（可选）
     * @param passphrase 私钥密码（如果有的话）
     * @return 成功返回true，失败返回false
     */
    bool loadKeyPair(const std::string &privateKeyPath, const std::string &publicKeyPath = "",
                     const std::string &passphrase = "");

    /**
     * @brief 保存SSH密钥对到文件
     * @param privateKeyPath 私钥保存路径
     * @param publicKeyPath 公钥保存路径
     * @param passphrase 私钥密码（可选）
     * @return 成功返回true，失败返回false
     */
    bool saveKeyPair(const std::string &privateKeyPath, const std::string &publicKeyPath,
                     const std::string &passphrase = "");

    /**
     * @brief 获取公钥内容
     * @return 公钥内容字符串
     */
    std::string getPublicKey() const;

    /**
     * @brief 获取私钥内容
     * @return 私钥内容字符串
     */
    std::string getPrivateKey() const;

    /**
     * @brief 获取密钥指纹
     * @return 密钥指纹字符串
     */
    std::string getFingerprint() const;

    /**
     * @brief 获取密钥类型
     * @return 密钥类型字符串
     */
    std::string getKeyType() const;

    /**
     * @brief 获取密钥注释
     * @return 密钥注释字符串
     */
    std::string getComment() const;

    /**
     * @brief 验证密钥对是否有效
     * @return 有效返回true，无效返回false
     */
    bool validateKeyPair() const;

    /**
     * @brief 设置密钥注释
     * @param comment 新的注释
     * @return 成功返回true，失败返回false
     */
    bool setComment(const std::string &comment);

    /**
     * @brief 更改私钥密码
     * @param oldPassphrase 旧密码
     * @param newPassphrase 新密码
     * @return 成功返回true，失败返回false
     */
    bool changePassphrase(const std::string &oldPassphrase, const std::string &newPassphrase);

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息字符串
     */
    std::string getLastError() const;

private:
    // 私有成员变量
    LIBSSH2_SESSION *session_; ///< SSH会话对象
    SSHKeyInfo keyInfo_;       ///< SSH密钥信息
    std::string lastError_;    ///< 最后错误信息

    // 辅助方法
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const std::string &error);

    /**
     * @brief 释放所有资源
     */
    void freeResources();

    /**
     * @brief 计算密钥指纹
     * @param publicKey 公钥数据
     * @return 指纹字符串
     */
    std::string calculateFingerprint(const std::string &publicKey);

    /**
     * @brief 检查libssh2操作的返回值
     * @param result 操作返回值
     * @param operation 操作描述
     * @return 操作成功返回true，失败返回false
     */
    bool checkError(int result, const std::string &operation);

    /**
     * @brief 使用mbedtls生成RSA密钥对
     * @param bits 密钥位数
     * @param passphrase 私钥密码
     * @return 成功返回true，失败返回false
     */
    bool generateRSAKeyPairWithMbedTLS(int bits, const std::string &passphrase);

    /**
     * @brief 将PEM格式的公钥转换为OpenSSH格式
     * @param pemPublicKey PEM格式的公钥内容
     * @param comment 密钥注释
     * @return OpenSSH格式的公钥字符串
     */
    std::string convertToOpenSSHFormat(const std::string &pemPublicKey, const std::string &comment);

    /**
     * @brief Base64编码函数
     * @param data 要编码的二进制数据
     * @return Base64编码后的字符串
     */
    std::string base64Encode(const std::vector<unsigned char> &data);

    /**
     * @brief 生成指定长度的随机Base64字符串
     * @param length 字符串长度
     * @return 随机Base64字符串
     */
    std::string generateRandomBase64(int length);

    /**
     * @brief 从私钥中提取公钥
     * @param passphrase 私钥密码
     * @return 成功返回true，失败返回false
     */
    bool extractPublicKeyFromPrivateKey(const std::string &passphrase);
};

#endif // HIGIT_SSH_MANAGER_H