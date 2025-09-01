//
// Created on 2025/8/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "core.h"
#include "global.h"
#include "hilog/log.h"
#include "utils/utils.hpp"
#include <repo_manager.h>

bool Core::InitApp(napi_env env, napi_value exports) noexcept {
    OH_LOG_INFO(LOG_APP, "Core::InitApp");
    napi_property_descriptor desc[] = {
        {
            .utf8name = "initSystem",
            .name = nullptr,
            .method = &Core::InitSystem,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "initRepo",
            .name = nullptr,
            .method = &Core::InitRepo,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "getBranches",
            .name = nullptr,
            .method = &Core::GetBranches,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "getTags",
            .name = nullptr,
            .method = &Core::GetTags,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "fetch",
            .name = nullptr,
            .method = &Core::Fetch,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "history",
            .name = nullptr,
            .method = &Core::GetHistory,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "getSSHKey",
            .name = nullptr,
            .method = &Core::GetSSHKey,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "generateSSHKey",
            .name = nullptr,
            .method = &Core::GenerateSSHKey,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "deleteRepo",
            .name = nullptr,
            .method = &Core::DeleteRepo,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "getFileTree",
            .name = nullptr,
            .method = &Core::GetFileTree,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
        {
            .utf8name = "readFile",
            .name = nullptr,
            .method = &Core::ReadFile,
            .getter = nullptr,
            .setter = nullptr,
            .value = nullptr,
            .attributes = napi_default,
            .data = nullptr,
        },
    };

    return Utils::checkNAPIResult(napi_define_properties(env, exports, std::size(desc), desc), env, "Core::InitApp",
                                  "Can't export methods.");
}

void Core::StoreRepoManager(const std::string &repoUrl, std::unique_ptr<RepoManager> manager) {
    repo_registry_.emplace(repoUrl, std::move(manager));
}

RepoManager *Core::FindRepoManager(const std::string &repoUrl) {
    auto it = repo_registry_.find(repoUrl);
    if (it == repo_registry_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void Core::DeleteRepoManager(const std::string &repoUrl) { repo_registry_.erase(repoUrl); }

void Core::InitSSH(std::string const &basePath) {
    ssh_manager_ = std::make_unique<SSHManager>();

    std::string sshPath = basePath + "/ssh/id_rsa";
    std::string publicKeyPath = basePath + "/ssh/id_rsa.pub";

    if (std::filesystem::exists(sshPath)) {
        OH_LOG_INFO(LOG_APP, "SSH key pair already exists at %{public}s", sshPath.c_str());
        bool loaded = ssh_manager_->loadKeyPair(sshPath, publicKeyPath, "");
        if (!loaded) {
            OH_LOG_ERROR(LOG_APP, "Failed to load SSH key pair, error: %{public}s",
                         ssh_manager_->getLastError().c_str());
            return;
        }
        return;
    }

    OH_LOG_INFO(LOG_APP, "Generating SSH key pair at %{public}s", sshPath.c_str());
    bool success = ssh_manager_->generateKeyPair(4096, "higit", "");
    if (!success) {
        OH_LOG_ERROR(LOG_APP, "Failed to generate SSH key pair, error: %{public}s",
                     ssh_manager_->getLastError().c_str());
        return;
    }

    if (!ssh_manager_->validateKeyPair()) {
        OH_LOG_ERROR(LOG_APP, "Failed to validate SSH key pair, error: %{public}s",
                     ssh_manager_->getLastError().c_str());
        return;
    }

    bool saved = ssh_manager_->saveKeyPair(basePath + "/ssh/id_rsa", basePath + "/ssh/id_rsa.pub", "");
    if (!saved) {
        OH_LOG_ERROR(LOG_APP, "Failed to save SSH key pair, error: %{public}s", ssh_manager_->getLastError().c_str());
        return;
    }

    OH_LOG_INFO(LOG_APP, "SSH key pair generated and saved at %{public}s", sshPath.c_str());
}

std::string Core::GetSSHKey() const { return ssh_manager_->getPublicKey(); }

std::string Core::GenerateSSHKey() const {
    std::string sshPath = Globals::files_directory + "/ssh/id_rsa";
    std::string publicKeyPath = Globals::files_directory + "/ssh/id_rsa.pub";

    // 如果存在则删除
    if (std::filesystem::exists(sshPath)) {
        std::filesystem::remove(sshPath);
    }
    if (std::filesystem::exists(publicKeyPath)) {
        std::filesystem::remove(publicKeyPath);
    }

    // 生成新的 SSH 密钥
    OH_LOG_INFO(LOG_APP, "Generating SSH key pair at %{public}s", sshPath.c_str());
    bool success = ssh_manager_->generateKeyPair(4096, "higit", "");
    if (!success) {
        OH_LOG_ERROR(LOG_APP, "Failed to generate SSH key pair, error: %{public}s",
                     ssh_manager_->getLastError().c_str());
        return "";
    }

    if (!ssh_manager_->validateKeyPair()) {
        OH_LOG_ERROR(LOG_APP, "Failed to validate SSH key pair, error: %{public}s",
                     ssh_manager_->getLastError().c_str());
        return "";
    }

    bool saved = ssh_manager_->saveKeyPair(sshPath, publicKeyPath, "");
    if (!saved) {
        OH_LOG_ERROR(LOG_APP, "Failed to save SSH key pair, error: %{public}s", ssh_manager_->getLastError().c_str());
        return "";
    }

    OH_LOG_INFO(LOG_APP, "SSH key pair generated and saved at %{public}s", sshPath.c_str());
    return ssh_manager_->getPublicKey();
}