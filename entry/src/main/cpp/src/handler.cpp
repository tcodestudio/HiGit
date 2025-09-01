#include "global.h"
#include "utils/messages.hpp"
#include "utils/utils.hpp"
#include <core.h>
#include <filesystem>
#include <hilog/log.h>
#include <js_native_api_types.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <node_api_types.h>
#include <rawfile/raw_file_manager.h>
#include <repo_manager.h>

Core Core::instance_{};

napi_value Core::InitSystem(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::InitSystem-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::InitSystem-NAPI =================");

    constexpr size_t expectedParams = 1U;
    constexpr size_t basePathIdx = 0U;

    size_t argc = expectedParams;
    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const basePath = Utils::extractString(env, argv[basePathIdx], "Can't extract basePath", from);
    if (!basePath.has_value()) {
        return nullptr;
    }

    Globals::files_directory = basePath.value();

    OH_LOG_INFO(LOG_APP, "basePath: %{public}s", basePath.value().c_str());

    // init ssh
    Core::GetInstance()->InitSSH(basePath.value());

    return Messages::NewResultMessage(env, true, "初始化系统成功");
}

napi_value Core::InitRepo(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::InitRepo-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::InitRepo-NAPI =================");
    constexpr size_t expectedParams = 4U;
    constexpr size_t basePathIdx = 0U;
    constexpr size_t repoURLIdx = 1U;
    constexpr size_t repoNameIdx = 2U;
    constexpr size_t providerIdx = 3U;

    size_t argc = expectedParams;
    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const basePath = Utils::extractString(env, argv[basePathIdx], "Can't extract basePath", from);
    if (!basePath.has_value()) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const repoName = Utils::extractString(env, argv[repoNameIdx], "Can't extract repoName", from);
    if (!repoName.has_value()) {
        return nullptr;
    }

    auto const provider = Utils::extractString(env, argv[providerIdx], "Can't extract provider", from);
    if (!provider.has_value()) {
        return nullptr;
    }

    OH_LOG_INFO(LOG_APP, "basePath: %{public}s, repoName: %{public}s", basePath.value().c_str(),
                repoName.value().c_str());
    OH_LOG_INFO(LOG_APP, "repoURL: %{public}s", repoURL.value().c_str());

    auto repoDir = basePath.value() + "/repos/" + provider.value() + "/" + repoName.value();

    if (Core::GetInstance()->FindRepoManager(repoURL.value()) != nullptr) {
        OH_LOG_INFO(LOG_APP, "RepoManager already exists for url: %{public}s", repoURL.value().c_str());
        return Messages::NewResultMessage(env, true, "初始化仓库成功");
    }

    // 新建 RepoManager 并按目录是否存在决定 open 或 init
    auto manager = std::make_unique<RepoManager>();

    bool ok = false;
    if (std::filesystem::exists(repoDir)) {
        // 目录存在，视为已创建过仓库：打开
        OH_LOG_INFO(LOG_APP, "Open existing repo at: %{public}s", repoDir.c_str());
        ok = manager->openRepository(repoDir);
        ok = manager->connectRemote(repoURL.value(), repoDir);
    } else {
        // 目录不存在，创建目录并初始化裸仓库
        if (std::filesystem::create_directories(repoDir)) {
            OH_LOG_INFO(LOG_APP, "Create directory success: %{public}s", repoDir.c_str());
        } else {
            OH_LOG_ERROR(LOG_APP, "Create directory failed: %{public}s", repoDir.c_str());
            return Messages::NewResultMessage(env, false, "创建目录失败");
        }
        OH_LOG_INFO(LOG_APP, "Init new bare repo at: %{public}s", repoDir.c_str());
        ok = manager->connectRemote(repoURL.value(), repoDir);
    }

    if (!ok) {
        OH_LOG_ERROR(LOG_APP, "Init/open repository failed at: %{public}s, error: %{public}s", repoDir.c_str(),
                     manager->getLastError().c_str());
        return Messages::NewResultMessage(env, false, manager->getLastError());
    }
    OH_LOG_INFO(LOG_APP, "Init/open Repo success");
    // 放入 Core 的内存缓存，key 为 repoURL
    Core::GetInstance()->StoreRepoManager(repoURL.value(), std::move(manager));

    return Messages::NewResultMessage(env, true, "初始化仓库成功");
}

napi_value Core::GetBranches(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GetBranches-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GetBranches-NAPI =================");

    auto repoManager = Utils::FindRepoManager(env, info, from);
    if (repoManager == nullptr) {
        OH_LOG_INFO(LOG_APP, "仓库未初始化");
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }

    auto branches = repoManager->getRemoteBranches();
    if (branches.empty()) {
        OH_LOG_ERROR(LOG_APP, "GetBranches failed: %{public}s", repoManager->getLastError().c_str());
        return Messages::NewResultMessage(env, false, repoManager->getLastError());
    }
    nlohmann::json json;
    for (auto &branch : branches) {
        json.push_back(branch.name);
    }
    OH_LOG_INFO(LOG_APP, "GetBranches success");
    return Messages::NewResultMessage(env, true, "GetBranches success", json.dump());
}

napi_value Core::GetTags(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GetTags-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GetTags-NAPI =================");

    auto repoManager = Utils::FindRepoManager(env, info, from);
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "仓库未初始化");
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }

    auto tags = repoManager->getRemoteTags();
    if (tags.empty()) {
        return Messages::NewResultMessage(env, true, "GetTags success", "[]");
    }
    nlohmann::json json;
    for (auto &tag : tags) {
        json.push_back(tag.name);
    }
    OH_LOG_INFO(LOG_APP, "GetTags success");
    return Messages::NewResultMessage(env, true, "GetTags success", json.dump());
}

struct FetchCallbackData {
    unsigned int process;
    unsigned int total;
    std::string message;
};

napi_value Core::Fetch(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::Fetch-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::Fetch-NAPI =================");

    constexpr size_t expectedParams = 3U;
    constexpr size_t repoURLIdx = 0U;
    constexpr size_t branchIdx = 1U;
    constexpr size_t callbackIdx = 2U;

    size_t argc = expectedParams;

    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const branch = Utils::extractString(env, argv[branchIdx], "Can't extract branch", from);
    if (!branch.has_value()) {
        return nullptr;
    }

    auto const repoManager = Core::GetInstance()->FindRepoManager(repoURL.value());
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "RepoManager not found for url: %{public}s", repoURL.value().c_str());
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }
    OH_LOG_INFO(LOG_APP, "Fetching branch: %{public}s", branch.value().c_str());

    napi_value startCallbackData[3];
    napi_create_int32(env, 0, &startCallbackData[0]);
    napi_create_int32(env, 0, &startCallbackData[1]);
    napi_create_string_utf8(env, "start", NAPI_AUTO_LENGTH, &startCallbackData[2]);
    napi_status startStatus = napi_call_function(env, nullptr, argv[callbackIdx], 3, startCallbackData, nullptr);
    if (startStatus != napi_ok) {
        OH_LOG_ERROR(LOG_APP, "Core::callMessageCallback napi_call_threadsafe_function failed with status: %{public}d",
                     startStatus);
    }

    auto fetchResult = repoManager->fetch(
        "origin", {branch.value()}, 0, [env, argv](unsigned int receivedObjects, unsigned int totalObjects) {
            napi_value data[3];
            napi_create_int32(env, receivedObjects, &data[0]);
            napi_create_int32(env, totalObjects, &data[1]);
            napi_create_string_utf8(env, "processing", NAPI_AUTO_LENGTH, &data[2]);
            napi_status status = napi_call_function(env, nullptr, argv[callbackIdx], 3, data, nullptr);
            if (status != napi_ok) {
                OH_LOG_ERROR(LOG_APP,
                             "Core::callMessageCallback napi_call_threadsafe_function failed with status: %{public}d",
                             status);
            }
        });

    napi_value endCallbackData[3];
    napi_create_int32(env, 0, &endCallbackData[0]);
    napi_create_int32(env, 0, &endCallbackData[1]);
    napi_create_string_utf8(env, "end", NAPI_AUTO_LENGTH, &endCallbackData[2]);
    napi_status endStatus = napi_call_function(env, nullptr, argv[callbackIdx], 3, endCallbackData, nullptr);
    if (endStatus != napi_ok) {
        OH_LOG_ERROR(LOG_APP, "Core::callMessageCallback napi_call_threadsafe_function failed with status: %{public}d",
                     endStatus);
    }

    if (!fetchResult) {
        OH_LOG_ERROR(LOG_APP, "Fetch failed: %{public}s", repoManager->getLastError().c_str());
        return Messages::NewResultMessage(env, false, repoManager->getLastError());
    }

    return Messages::NewResultMessage(env, true, "拉取分支成功");
}

napi_value Core::GetHistory(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GetHistory-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GetHistory-NAPI =================");

    constexpr size_t expectedParams = 4U;
    constexpr size_t repoURLIdx = 0U;
    constexpr size_t branchIdx = 1U;
    constexpr size_t countIdx = 2U;
    constexpr size_t offsetIdx = 3U;

    size_t argc = expectedParams;

    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const branch = Utils::extractString(env, argv[branchIdx], "Can't extract branch", from);
    if (!branch.has_value()) {
        return nullptr;
    }

    auto const count = Utils::extractInteger(env, argv[countIdx], "Can't extract count", from);
    if (!count.has_value()) {
        return nullptr;
    }

    auto const offset = Utils::extractInteger(env, argv[offsetIdx], "Can't extract offset", from);
    if (!offset.has_value()) {
        return nullptr;
    }

    auto const repoManager = Core::GetInstance()->FindRepoManager(repoURL.value());
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "RepoManager not found for url: %{public}s", repoURL.value().c_str());
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }

    auto const history = repoManager->getCommitHistory(branch.value(), count.value(), offset.value());
    if (history.empty()) {
        return Messages::NewResultMessage(env, true, "获取提交历史成功", "[]");
    }
    nlohmann::json json;
    for (auto &commit : history) {
        json.push_back({
            {"id", commit.id},
            {"shortId", commit.shortId},
            {"author", commit.author},
            {"email", commit.email},
            {"timestamp", commit.timestamp},
            {"message", commit.message},
            {"shortMessage", commit.shortMessage},
        });
    }
    return Messages::NewResultMessage(env, true, "获取提交历史成功", json.dump());
}

napi_value Core::GetSSHKey(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GetSSHKey-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GetSSHKey-NAPI =================");

    auto sshKey = Core::GetInstance()->GetSSHKey();
    return Messages::NewResultMessage(env, true, "获取 SSH 密钥成功", sshKey);
}

napi_value Core::GenerateSSHKey(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GenerateSSHKey-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GenerateSSHKey-NAPI =================");

    auto sshKey = Core::GetInstance()->GenerateSSHKey();
    if (sshKey.empty()) {
        return Messages::NewResultMessage(env, false, "生成 SSH 密钥失败");
    }
    return Messages::NewResultMessage(env, true, "生成 SSH 密钥成功", sshKey);
}

napi_value Core::DeleteRepo(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::DeleteRepo-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::DeleteRepo-NAPI =================");

    constexpr size_t expectedParams = 4U;
    constexpr size_t basePathIdx = 0U;
    constexpr size_t repoURLIdx = 1U;
    constexpr size_t repoNameIdx = 2U;
    constexpr size_t providerIdx = 3U;

    size_t argc = expectedParams;
    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const basePath = Utils::extractString(env, argv[basePathIdx], "Can't extract basePath", from);
    if (!basePath.has_value()) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const repoName = Utils::extractString(env, argv[repoNameIdx], "Can't extract repoName", from);
    if (!repoName.has_value()) {
        return nullptr;
    }

    auto const provider = Utils::extractString(env, argv[providerIdx], "Can't extract provider", from);
    if (!provider.has_value()) {
        return nullptr;
    }

    OH_LOG_INFO(LOG_APP, "basePath: %{public}s, repoName: %{public}s", basePath.value().c_str(),
                repoName.value().c_str());
    OH_LOG_INFO(LOG_APP, "repoURL: %{public}s", repoURL.value().c_str());

    auto repoDir = basePath.value() + "/repos/" + provider.value() + "/" + repoName.value();

    if (Core::GetInstance()->FindRepoManager(repoURL.value()) != nullptr) {
        Core::GetInstance()->DeleteRepoManager(repoURL.value());
    }

    // 删除目录
    if (std::filesystem::exists(repoDir)) {
        if (std::filesystem::remove_all(repoDir)) {
            OH_LOG_INFO(LOG_APP, "Delete directory success: %{public}s", repoDir.c_str());
        } else {
            OH_LOG_ERROR(LOG_APP, "Delete directory failed: %{public}s", repoDir.c_str());
            return Messages::NewResultMessage(env, false, "删除目录失败");
        }
    }

    return Messages::NewResultMessage(env, true, "删除仓库成功");
}


napi_value Core::GetFileTree(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::GetFileTree-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::GetFileTree-NAPI =================");

    constexpr size_t expectedParams = 2U;
    constexpr size_t repoURLIdx = 0U;
    constexpr size_t branchIdx = 1U;

    size_t argc = expectedParams;

    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const branch = Utils::extractString(env, argv[branchIdx], "Can't extract branch", from);
    if (!branch.has_value()) {
        return nullptr;
    }

    auto const repoManager = Core::GetInstance()->FindRepoManager(repoURL.value());
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "RepoManager not found for url: %{public}s", repoURL.value().c_str());
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }

    auto const fileTree = repoManager->getBranchFileTree(branch.value());
    if (fileTree.empty()) {
        return Messages::NewResultMessage(env, true, "获取文件树成功", "[]");
    }
    nlohmann::json json;
    for (auto &file : fileTree) {
        json.push_back({
            {"id", file.id},
            {"parentId", file.parentId},
            {"name", file.name},
            {"path", file.path},
            {"isDirectory", file.isDirectory},
            {"fileId", file.fileId},
            {"mode", file.mode},
            {"size", file.size},
            {"extension", file.extension},
        });
    }
    return Messages::NewResultMessage(env, true, "获取文件树成功", json.dump());
}

napi_value Core::ReadFile(napi_env env, napi_callback_info info) noexcept {
    char const *from = "Core::ReadFile-NAPI";
    OH_LOG_INFO(LOG_APP, "================= Core::ReadFile-NAPI =================");

    constexpr size_t expectedParams = 3U;
    constexpr size_t repoURLIdx = 0U;
    constexpr size_t branchIdx = 1U;
    constexpr size_t pathIdx = 2U;

    size_t argc = expectedParams;

    napi_value argv[expectedParams]{};

    bool const result = Utils::extractParameters(env, info, expectedParams, &argc, argv, from);
    if (!result) {
        return nullptr;
    }

    auto const repoURL = Utils::extractString(env, argv[repoURLIdx], "Can't extract repoURL", from);
    if (!repoURL.has_value()) {
        return nullptr;
    }

    auto const branch = Utils::extractString(env, argv[branchIdx], "Can't extract branch", from);
    if (!branch.has_value()) {
        return nullptr;
    }

    auto const path = Utils::extractString(env, argv[pathIdx], "Can't extract path", from);
    if (!path.has_value()) {
        return nullptr;
    }

    auto const repoManager = Core::GetInstance()->FindRepoManager(repoURL.value());
    if (repoManager == nullptr) {
        OH_LOG_ERROR(LOG_APP, "RepoManager not found for url: %{public}s", repoURL.value().c_str());
        return Messages::NewResultMessage(env, false, "仓库未初始化");
    }

    auto const fileContent = repoManager->readFile(branch.value(), path.value());
    if (!fileContent.exists) {
        return Messages::NewResultMessage(env, false, "文件不存在");
    }
    if (fileContent.isBinary) {
        return Messages::NewResultMessage(env, false, "无法读取二进制文件");
    }
    return Messages::NewResultMessage(env, true, "读取文件成功", fileContent.content);
}