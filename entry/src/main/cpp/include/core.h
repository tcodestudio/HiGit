//
// Created on 2025/8/10.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef HIGIT_CORE_H
#define HIGIT_CORE_H

#include "ssh_manager.h"
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <memory>
#include <repo_manager.h>
#include <string>
#include <unordered_map>

class Core final {
public:
    Core(){};
    ~Core(){};

    Core(Core const &) = delete;
    Core &operator=(Core const &) = delete;

    Core(Core &&) = delete;
    Core &operator=(Core &&) = delete;

    static Core *GetInstance() { return &Core::instance_; }

    void SetEnv(napi_env env) { core_env = env; }
    [[nodiscard]] static bool InitApp(napi_env env, napi_value exports) noexcept;
    // 初始化应用
    [[nodiscard]] static napi_value InitSystem(napi_env env, napi_callback_info info) noexcept;
    // 初始化仓库
    [[nodiscard]] static napi_value InitRepo(napi_env env, napi_callback_info info) noexcept;
    // 获取分支
    [[nodiscard]] static napi_value GetBranches(napi_env env, napi_callback_info info) noexcept;
    // 获取标签
    [[nodiscard]] static napi_value GetTags(napi_env env, napi_callback_info info) noexcept;
    // 拉取
    [[nodiscard]] static napi_value Fetch(napi_env env, napi_callback_info info) noexcept;
    // 获取历史
    [[nodiscard]] static napi_value GetHistory(napi_env env, napi_callback_info info) noexcept;
    // 获取 SSH Key
    [[nodiscard]] static napi_value GetSSHKey(napi_env env, napi_callback_info info) noexcept;
    // 生成 SSH Key
    [[nodiscard]] static napi_value GenerateSSHKey(napi_env env, napi_callback_info info) noexcept;
    // 删除仓库
    [[nodiscard]] static napi_value DeleteRepo(napi_env env, napi_callback_info info) noexcept;
    // 获取文件树
    [[nodiscard]] static napi_value GetFileTree(napi_env env, napi_callback_info info) noexcept;
    // 读取文件
    [[nodiscard]] static napi_value ReadFile(napi_env env, napi_callback_info info) noexcept;

    void StoreRepoManager(const std::string &repoUrl, std::unique_ptr<RepoManager> manager);
    RepoManager *FindRepoManager(const std::string &repoUrl);
    void DeleteRepoManager(const std::string &repoUrl);

private:
    static Core instance_;
    std::unique_ptr<SSHManager> ssh_manager_;
    napi_env core_env;

    std::unordered_map<std::string, std::unique_ptr<RepoManager>> repo_registry_;

    void InitSSH(std::string const &basePath);

    std::string GetSSHKey() const;

    std::string GenerateSSHKey() const;
};


#endif // HIGIT_CORE_H
