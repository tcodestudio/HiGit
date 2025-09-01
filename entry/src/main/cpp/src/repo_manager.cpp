#include "repo_manager.h"
#include "git2/common.h"
#include "global.h"
#include <cstring>
#include <ctime>
#include <git2.h>
#include <hilog/log.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

// 在文件开头添加SSH主机密钥验证回调
static int certificate_check_cb(git_cert *cert, int valid, const char *host, void *payload) {
    if (valid) {
        OH_LOG_INFO(LOG_APP, "SSH host key is valid for %{public}s", host);
        return 0; // 接受有效的主机密钥
    }

    // 对于无效的主机密钥，可以选择接受或拒绝
    // 在生产环境中应该更严格，这里为了开发方便选择接受
    OH_LOG_WARN(LOG_APP, "SSH host key is invalid for %{public}s, but accepting for development", host);
    return 0; // 0表示接受，非0表示拒绝

    // 如果需要更严格的安全策略，可以这样处理：
    // if (valid) {
    //     return 0; // 接受
    // } else {
    //     OH_LOG_ERROR(LOG_APP, "Rejecting invalid SSH host key for %{public}s", host);
    //     return -1; // 拒绝
    // }
}

static int credentials_cb(git_credential **out, const char *url, const char *username_from_url,
                          unsigned int allowed_types, void *payload) {
    if (allowed_types & GIT_CREDENTIAL_SSH_KEY) {
        std::string public_key = Globals::files_directory + "/ssh/id_rsa.pub";
        std::string private_key = Globals::files_directory + "/ssh/id_rsa";

        OH_LOG_INFO(LOG_APP, "SSH authentication requested for URL: %{public}s", url);
        int result =
            git_credential_ssh_key_new(out, username_from_url, public_key.c_str(), private_key.c_str(), nullptr);

        if (result == 0) {
            OH_LOG_INFO(LOG_APP, "SSH credential created successfully");
        } else {
            OH_LOG_ERROR(LOG_APP, "Failed to create SSH credential, error: %{public}d", result);
        }

        return result;
    } else if (allowed_types & GIT_CREDENTIAL_USERNAME) {
        OH_LOG_INFO(LOG_APP, "Username credential requested");
        return git_credential_username_new(out, username_from_url);
    }

    OH_LOG_ERROR(LOG_APP, "No suitable credential type for SSH, allowed_types: %{public}u", allowed_types);
    return GIT_EUSER;
}


RepoManager::RepoManager() : repository_(nullptr), remote_(nullptr) {
    OH_LOG_INFO(LOG_APP, "RepoManager::RepoManager");
    // 初始化libgit2库
    git_libgit2_init();

    git_libgit2_opts(GIT_OPT_ENABLE_STRICT_HASH_VERIFICATION, 0);
    git_libgit2_opts(GIT_OPT_SET_MWINDOW_SIZE, 128 * 1024 * 1024);        // 128MB
    git_libgit2_opts(GIT_OPT_SET_MWINDOW_MAPPED_LIMIT, 64 * 1024 * 1024); // 64M
    int timeout_ms = 30 * 1000;                                           // 60 seconds
    int error = git_libgit2_opts(GIT_OPT_SET_SERVER_TIMEOUT, timeout_ms);

    int features = git_libgit2_features();
    if (features & GIT_FEATURE_SSH) {
        OH_LOG_INFO(LOG_APP, "SSH support is enabled");
    } else {
        OH_LOG_ERROR(LOG_APP, "SSH support is not enabled");
    }

    std::string certPath = Globals::files_directory + "/cert.pem";
    if (access(certPath.c_str(), F_OK) == 0) {
        git_libgit2_opts(GIT_OPT_SET_SSL_CERT_LOCATIONS, certPath.c_str(), nullptr);
    } else {
        OH_LOG_WARN(LOG_APP, "cert.pem not found, use default SSL certificate path %{public}s", certPath.c_str());
    }
}

RepoManager::~RepoManager() {
    freeResources();
    // 关闭libgit2库
    git_libgit2_shutdown();
}

void RepoManager::freeResources() {
    if (remote_) {
        git_remote_free(remote_);
        remote_ = nullptr;
    }

    if (repository_) {
        git_repository_free(repository_);
        repository_ = nullptr;
    }
}

void RepoManager::setError(const std::string &error) { lastError_ = error; }

bool RepoManager::checkError(int error, const std::string &operation) {
    if (error < 0) {
        OH_LOG_ERROR(LOG_APP, "Error: %{public}d, operation: %{public}s", error, operation.c_str());
        const git_error *e = git_error_last();
        if (e) {
            // 获取libgit2的原始错误消息
            std::string libgit2_message = e->message ? e->message : "未知错误";

            // 构建详细的错误信息
            std::string detailed_error = "操作失败\n";
            detailed_error += "错误分类: " + getErrorClassDescription(static_cast<git_error_t>(e->klass)) + "\n";
            detailed_error += "具体错误: " + getErrorCodeDescription(error) + "\n";
            detailed_error += "详细信息: " + libgit2_message + "\n";
            detailed_error += "解决方案: " + getErrorSolution(static_cast<git_error_t>(e->klass), error);

            setError(detailed_error);
        } else {
            setError(operation + " 操作失败，错误码: " + std::to_string(error));
        }
        return false;
    }
    return true;
}

// 获取错误分类的描述
std::string RepoManager::getErrorClassDescription(git_error_t error_class) {
    switch (error_class) {
    case GIT_ERROR_NOMEMORY:
        return "内存不足";
    case GIT_ERROR_OS:
        return "操作系统错误";
    case GIT_ERROR_INVALID:
        return "无效操作或参数";
    case GIT_ERROR_REFERENCE:
        return "引用错误";
    case GIT_ERROR_ZLIB:
        return "数据压缩错误";
    case GIT_ERROR_REPOSITORY:
        return "仓库错误";
    case GIT_ERROR_CONFIG:
        return "配置错误";
    case GIT_ERROR_REGEX:
        return "正则表达式错误";
    case GIT_ERROR_ODB:
        return "对象数据库错误";
    case GIT_ERROR_INDEX:
        return "索引错误";
    case GIT_ERROR_OBJECT:
        return "Git对象错误";
    case GIT_ERROR_NET:
        return "网络错误";
    case GIT_ERROR_TAG:
        return "标签错误";
    case GIT_ERROR_TREE:
        return "树对象错误";
    case GIT_ERROR_INDEXER:
        return "索引器错误";
    case GIT_ERROR_SSL:
        return "SSL/TLS错误";
    case GIT_ERROR_SUBMODULE:
        return "子模块错误";
    case GIT_ERROR_THREAD:
        return "线程错误";
    case GIT_ERROR_STASH:
        return "暂存错误";
    case GIT_ERROR_CHECKOUT:
        return "检出错误";
    case GIT_ERROR_FETCHHEAD:
        return "获取头信息错误";
    case GIT_ERROR_MERGE:
        return "合并错误";
    case GIT_ERROR_SSH:
        return "SSH连接错误";
    case GIT_ERROR_FILTER:
        return "过滤器错误";
    case GIT_ERROR_REVERT:
        return "回退错误";
    case GIT_ERROR_CALLBACK:
        return "回调函数错误";
    case GIT_ERROR_CHERRYPICK:
        return "樱桃选择错误";
    case GIT_ERROR_DESCRIBE:
        return "描述错误";
    case GIT_ERROR_REBASE:
        return "变基错误";
    case GIT_ERROR_FILESYSTEM:
        return "文件系统错误";
    case GIT_ERROR_PATCH:
        return "补丁错误";
    case GIT_ERROR_WORKTREE:
        return "工作树错误";
    case GIT_ERROR_SHA:
        return "SHA哈希错误";
    case GIT_ERROR_HTTP:
        return "HTTP错误";
    case GIT_ERROR_INTERNAL:
        return "内部错误";
    case GIT_ERROR_GRAFTS:
        return "嫁接错误";
    default:
        return "未知错误类型";
    }
}

// 获取具体错误码的描述
std::string RepoManager::getErrorCodeDescription(int error_code) {
    switch (error_code) {
    case GIT_OK:
        return "操作成功";
    case GIT_ERROR:
        return "一般错误";
    case GIT_ENOTFOUND:
        return "请求的对象未找到";
    case GIT_EEXISTS:
        return "对象已存在，阻止操作";
    case GIT_EAMBIGUOUS:
        return "多个对象匹配";
    case GIT_EBUFS:
        return "输出缓冲区太短";
    case GIT_EUSER:
        return "用户回调返回错误";
    case GIT_EBAREREPO:
        return "裸仓库不允许此操作";
    case GIT_EUNBORNBRANCH:
        return "HEAD指向没有提交的分支";
    case GIT_EUNMERGED:
        return "合并进行中，阻止操作";
    case GIT_ENONFASTFORWARD:
        return "引用不能快进";
    case GIT_EINVALIDSPEC:
        return "名称/引用规范格式无效";
    case GIT_ECONFLICT:
        return "检出冲突阻止操作";
    case GIT_ELOCKED:
        return "锁文件阻止操作";
    case GIT_EMODIFIED:
        return "引用值与期望不匹配";
    case GIT_EAUTH:
        return "认证错误";
    case GIT_ECERTIFICATE:
        return "服务器证书无效";
    case GIT_EAPPLIED:
        return "补丁/合并已应用";
    case GIT_EPEEL:
        return "请求的peel操作不可能";
    case GIT_EEOF:
        return "意外的EOF";
    case GIT_EINVALID:
        return "无效操作或输入";
    case GIT_EUNCOMMITTED:
        return "索引中有未提交的更改";
    case GIT_EDIRECTORY:
        return "操作对目录无效";
    case GIT_EMERGECONFLICT:
        return "存在合并冲突无法继续";
    case GIT_PASSTHROUGH:
        return "用户配置的回调拒绝操作";
    case GIT_ITEROVER:
        return "迭代器迭代结束";
    case GIT_RETRY:
        return "内部重试";
    case GIT_EMISMATCH:
        return "对象哈希和不匹配";
    case GIT_EINDEXDIRTY:
        return "索引中有未保存的更改";
    case GIT_EAPPLYFAIL:
        return "补丁应用失败";
    case GIT_EOWNER:
        return "对象不属于当前用户";
    case GIT_TIMEOUT:
        return "操作超时";
    case GIT_EUNCHANGED:
        return "没有更改";
    case GIT_ENOTSUPPORTED:
        return "不支持此选项";
    case GIT_EREADONLY:
        return "对象是只读的";
    default:
        return "未知错误码: " + std::to_string(error_code);
    }
}

// 获取错误的解决方案建议
std::string RepoManager::getErrorSolution(git_error_t error_class, int error_code) {
    // 优先根据具体错误码提供解决方案
    switch (error_code) {
    case GIT_TIMEOUT:
        return "1) 检查网络连接是否稳定 2) 增加超时时间设置 3) 尝试重新执行操作";
    case GIT_ECERTIFICATE:
        return "1) 检查证书文件是否存在 2) 验证证书是否过期 3) 确认系统时间是否正确";
    case GIT_EAUTH:
        return "1) 检查用户名和密码是否正确 2) 验证SSH密钥是否有效 3) 确认访问权限";
    case GIT_ENOTFOUND:
        return "1) 检查目标对象是否存在 2) 验证引用名称是否正确 3) 尝试同步远程仓库";
    case GIT_EEXISTS:
        return "1) 检查目标是否已存在 2) 使用强制选项覆盖 3) 先删除现有对象";
    case GIT_ECONFLICT:
        return "1) 解决文件冲突 2) 使用git status查看冲突文件 3) 手动编辑冲突后重新提交";
    case GIT_EUNCOMMITTED:
        return "1) 提交或暂存当前更改 2) 使用git stash保存更改 3) 重置工作目录";
    case GIT_ELOCKED:
        return "1) 等待其他进程完成 2) 检查是否有其他Git操作在运行 3) 重启应用程序";
    case GIT_EMODIFIED:
        return "1) 同步远程仓库 2) 检查本地引用状态 3) 重新获取最新数据";
    case GIT_EMERGECONFLICT:
        return "1) 解决合并冲突 2) 使用git status查看冲突 3) 手动解决后继续合并";
    case GIT_ENONFASTFORWARD:
        return "1) 先拉取最新更改 2) 解决冲突后重新推送 3) 使用--force-with-lease选项";
    case GIT_EINVALIDSPEC:
        return "1) 检查引用名称格式 2) 验证分支或标签名称 3) 使用正确的命名规范";
    case GIT_EBUFS:
        return "1) 增加缓冲区大小 2) 分批处理大量数据 3) 检查系统内存";
    case GIT_EAMBIGUOUS:
        return "1) 使用完整的提交ID 2) 明确指定分支名称 3) 检查引用是否唯一";
    case GIT_EBAREREPO:
        return "1) 使用非裸仓库 2) 检查仓库初始化方式 3) 重新创建仓库";
    case GIT_EUNBORNBRANCH:
        return "1) 创建第一个提交 2) 检查HEAD引用状态 3) 初始化仓库内容";
    case GIT_EUNMERGED:
        return "1) 完成当前合并 2) 解决合并冲突 3) 提交合并结果";
    case GIT_EPEEL:
        return "1) 检查对象类型 2) 验证peel操作是否支持 3) 使用正确的对象引用";
    case GIT_EEOF:
        return "1) 检查网络连接 2) 验证数据完整性 3) 重新获取数据";
    case GIT_EINVALID:
        return "1) 检查操作参数 2) 验证当前状态 3) 使用正确的操作顺序";
    case GIT_EDIRECTORY:
        return "1) 检查目标是否为文件 2) 验证操作对目录是否有效 3) 使用正确的操作";
    case GIT_PASSTHROUGH:
        return "1) 检查用户配置 2) 验证回调函数 3) 确认操作权限";
    case GIT_EMISMATCH:
        return "1) 验证对象完整性 2) 重新获取对象数据 3) 检查仓库状态";
    case GIT_EINDEXDIRTY:
        return "1) 保存当前索引状态 2) 提交或暂存更改 3) 重置索引";
    case GIT_EAPPLYFAIL:
        return "1) 检查补丁格式 2) 验证目标文件状态 3) 手动应用补丁";
    case GIT_EOWNER:
        return "1) 检查文件权限 2) 确认用户身份 3) 使用sudo或管理员权限";
    case GIT_EUNCHANGED:
        return "1) 检查是否有实际更改 2) 验证操作目标 3) 确认操作必要性";
    case GIT_ENOTSUPPORTED:
        return "1) 检查Git版本 2) 验证功能支持 3) 使用替代方案";
    case GIT_EREADONLY:
        return "1) 检查文件权限 2) 确认写入权限 3) 修改文件属性";
    default:
        // 如果具体错误码没有特殊处理，则根据错误分类提供通用建议
        switch (error_class) {
        case GIT_ERROR_NOMEMORY:
            return "1) 关闭其他占用内存的应用程序 2) 检查系统可用内存 3) 重启应用程序";
        case GIT_ERROR_OS:
            return "1) 检查文件权限设置 2) 确认磁盘空间充足 3) 检查文件是否被占用";
        case GIT_ERROR_NET:
            return "1) 检查网络连接状态 2) 验证远程仓库地址 3) 检查防火墙和代理设置";
        case GIT_ERROR_SSL:
            return "1) 检查SSL证书配置 2) 验证系统时间是否正确 3) 更新证书文件";
        case GIT_ERROR_SSH:
            return "1) 检查SSH密钥配置 2) 验证公钥是否已添加 3) 检查SSH配置文件";
        case GIT_ERROR_REPOSITORY:
            return "1) 检查仓库完整性 2) 验证.git目录状态 3) 尝试重新初始化";
        case GIT_ERROR_CONFIG:
            return "1) 检查配置文件格式 2) 验证配置项值 3) 重置为默认配置";
        case GIT_ERROR_FILESYSTEM:
            return "1) 检查磁盘空间 2) 验证文件权限 3) 检查文件系统状态";
        case GIT_ERROR_HTTP:
            return "1) 检查HTTP代理设置 2) 验证网络连接 3) 检查服务器状态";
        default:
            return "1) 检查操作参数是否正确 2) 验证当前状态是否允许操作 3) 尝试重新执行操作";
        }
    }
}

bool RepoManager::openRepository(const std::string &path) {
    // 释放之前的资源
    freeResources();

    // 检查路径是否存在
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        setError("Repository path does not exist: " + path);
        return false;
    }

    // 打开仓库
    if (!checkError(git_repository_open(&repository_, path.c_str()), "Open repository")) {
        setError(lastError_.c_str());
        OH_LOG_ERROR(LOG_APP, "Open repository failed: %{public}s, error: %{public}s", path.c_str(),
                     lastError_.c_str());
        return false;
    }

    repoPath_ = path;
    return true;
}

bool RepoManager::createRepository(const std::string &path, bool isBare) {
    // 释放之前的资源
    freeResources();

    // 创建仓库
    unsigned int flags = 0;
    if (isBare) {
        flags |= GIT_REPOSITORY_INIT_BARE;
    }

    git_repository_init_options init_opts = GIT_REPOSITORY_INIT_OPTIONS_INIT;
    init_opts.flags = flags;

    if (!checkError(git_repository_init_ext(&repository_, path.c_str(), &init_opts), "Create repository")) {
        OH_LOG_ERROR(LOG_APP, "Create repository failed: %{public}s", path.c_str());
        return false;
    }

    repoPath_ = path;
    return true;
}

bool RepoManager::connectRemote(const std::string &url, const std::string &localPath) {
    // 释放之前的资源
    freeResources();

    remoteUrl_ = url;

    // 检查本地路径是否存在仓库
    if (git_repository_open(&repository_, localPath.c_str()) == 0) {
        // 成功打开现有仓库
        repoPath_ = localPath;
    } else {
        // 创建新的裸仓库
        if (!createRepository(localPath, true)) {
            OH_LOG_ERROR(LOG_APP, "Create repository failed: %{public}s, error: %{public}s", localPath.c_str(),
                         lastError_.c_str());
            return false;
        }
    }

    // 添加或更新远程仓库连接
    if (!addRemote("origin", url)) {
        // 注意：这里不调用freeResources()，因为我们可能想保留已打开的仓库
        // 只清理remote_成员变量
        if (remote_) {
            git_remote_free(remote_);
            remote_ = nullptr;
        }
        OH_LOG_ERROR(LOG_APP, "Add remote failed: %{public}s, error: %{public}s", url.c_str(), lastError_.c_str());
        return false;
    }

    // 获取远程仓库对象
    if (!checkError(git_remote_lookup(&remote_, repository_, "origin"), "Lookup remote")) {
        // 注意：这里不调用freeResources()，因为我们可能想保留已打开的仓库
        // 只清理remote_成员变量
        if (remote_) {
            git_remote_free(remote_);
            remote_ = nullptr;
        }
        OH_LOG_ERROR(LOG_APP, "Lookup remote failed: %{public}s, error: %{public}s", "origin", lastError_.c_str());
        return false;
    }

    // 连接远程仓库
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    callbacks.credentials = credentials_cb;
    callbacks.certificate_check = certificate_check_cb;
    if (!checkError(git_remote_connect(remote_, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr),
                    "Connect to remote repository")) {
        // 注意：这里不调用freeResources()，因为我们可能想保留已打开的仓库
        // 只清理remote_成员变量
        if (remote_) {
            git_remote_free(remote_);
            remote_ = nullptr;
        }
        setError(lastError_.c_str());
        OH_LOG_ERROR(LOG_APP, "Connect to remote repository failed: %{public}s, error: %{public}s", "origin",
                     lastError_.c_str());
        return false;
    }

    OH_LOG_INFO(LOG_APP, "Successfully connected to remote repository");
    return true;
}

bool RepoManager::shallowClone(const std::string &url, const std::string &localPath, int depth) {
    // 释放之前的资源
    freeResources();

    // 设置克隆选项
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    // 设置为裸仓库以避免检出文件
    clone_opts.bare = true;
    clone_opts.fetch_opts.depth = depth > 0 ? depth : 1; // 浅克隆深度

    // 执行克隆操作
    if (!checkError(git_clone(&repository_, url.c_str(), localPath.c_str(), &clone_opts), "Shallow clone repository")) {
        OH_LOG_ERROR(LOG_APP, "Shallow clone repository failed: %{public}s, error: %{public}s", url.c_str(),
                     lastError_.c_str());
        return false;
    }

    remoteUrl_ = url;
    repoPath_ = localPath;
    return true;
}

bool RepoManager::cloneRepository(const std::string &url, const std::string &localPath) {
    // 释放之前的资源
    freeResources();

    // 设置克隆选项
    git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
    // 设置为裸仓库以避免检出文件
    clone_opts.bare = true;
    // 不设置depth，获取完整历史

    // 执行克隆操作
    if (!checkError(git_clone(&repository_, url.c_str(), localPath.c_str(), &clone_opts), "Clone repository")) {
        return false;
    }

    remoteUrl_ = url;
    repoPath_ = localPath;
    return true;
}

bool RepoManager::addRemote(const std::string &name, const std::string &url) {
    if (!repository_) {
        setError("No repository opened");
        return false;
    }

    git_remote *remote = nullptr;
    int error = git_remote_create(&remote, repository_, name.c_str(), url.c_str());

    if (error == GIT_EEXISTS) {
        // 远程仓库已存在，更新URL
        if (!checkError(git_remote_set_url(repository_, name.c_str(), url.c_str()), "Update remote URL")) {
            return false;
        }
        return true;
    } else if (!checkError(error, "Add remote")) {
        return false;
    }

    git_remote_free(remote);
    return true;
}

bool RepoManager::fetch(const std::string &remoteName, const std::vector<std::string> &branchRefs, int depth,
                        FetchProgressCallback progressCallback) {
    if (!repository_) {
        setError("仓库未初始化");
        return false;
    }

    OH_LOG_INFO(LOG_APP, "Starting fetch for remote: %{public}s, branches: %{public}zu, depth: %{public}d",
                remoteName.c_str(), branchRefs.size(), depth);

    git_remote *remote = nullptr;
    if (!checkError(git_remote_lookup(&remote, repository_, remoteName.c_str()), "Lookup remote")) {
        OH_LOG_ERROR(LOG_APP, "Lookup remote failed: %{public}s, error: %{public}s", remoteName.c_str(),
                     lastError_.c_str());
        return false;
    }

    // 设置 fetch 选项
    git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;

    // 设置获取深度
    if (depth > 0) {
        fetch_opts.depth = depth;
        OH_LOG_INFO(LOG_APP, "Setting fetch depth to: %{public}d", depth);
    }
    fetch_opts.prune = GIT_FETCH_PRUNE;

    // 不下载标签以减少数据传输
    fetch_opts.download_tags = GIT_REMOTE_DOWNLOAD_TAGS_NONE;

    // 设置传输选项
    fetch_opts.follow_redirects = GIT_REMOTE_REDIRECT_NONE;

    // 准备引用规格数组
    git_strarray refspecs = {0};
    std::vector<std::string> refspec_strings; // 保持字符串的生命周期
    std::vector<const char *> refspec_ptrs;

    if (!branchRefs.empty()) {
        // 使用指定的分支引用
        for (const auto &branch : branchRefs) {
            // 转换分支名为引用规格
            std::string refspec = "refs/heads/" + branch + ":refs/remotes/origin/" + branch;
            refspec_strings.push_back(refspec);                     // 存储字符串
            refspec_ptrs.push_back(refspec_strings.back().c_str()); // 获取指针
            OH_LOG_DEBUG(LOG_APP, "Adding refspec: %{public}s", refspec.c_str());
        }
        refspecs.strings = const_cast<char **>(refspec_ptrs.data());
        refspecs.count = refspec_ptrs.size();
    } else {
        // 如果没有指定分支，获取所有主要分支但限制对象数量
        OH_LOG_INFO(LOG_APP, "No specific branches specified, will fetch with depth limit");
    }

    // 添加回调以监控进度和错误
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;

    // 如果提供了进度回调函数，则设置进度回调
    if (progressCallback) {
        // 使用成员变量来存储进度回调函数，避免lambda捕获问题
        static FetchProgressCallback *current_callback = nullptr;
        current_callback = &progressCallback;

        callbacks.transfer_progress = [](const git_indexer_progress *stats, void *payload) -> int {
            // 调用当前设置的进度回调函数
            if (current_callback && *current_callback) {
                try {
                    (*current_callback)(stats->received_objects, stats->total_objects);
                } catch (...) {
                    // 忽略回调函数中的异常，避免影响fetch操作
                    OH_LOG_WARN(LOG_APP, "Progress callback threw an exception, ignoring");
                }
            }

            // 保持原有的日志输出
            if (stats->indexed_objects > 0) {
                OH_LOG_INFO(LOG_APP, "Indexing progress: %{public}d/%{public}d", stats->indexed_objects,
                            stats->total_objects);
            }

            return 0; // 继续
        };
    }

    callbacks.credentials = credentials_cb;
    callbacks.certificate_check = certificate_check_cb;

    fetch_opts.callbacks = callbacks;

    int result;
    if (!branchRefs.empty()) {
        OH_LOG_INFO(LOG_APP, "Fetching with %{public}zu refspecs:", refspecs.count);
        for (size_t i = 0; i < refspecs.count; ++i) {
            OH_LOG_INFO(LOG_APP, "  Refspec %{public}zu: %{public}s", i, refspecs.strings[i]);
        }
        result = git_remote_fetch(remote, &refspecs, &fetch_opts, "fetching specific branches");
    } else {
        result = git_remote_fetch(remote, nullptr, &fetch_opts, "fetching with limits");
    }

    bool success = checkError(result, "Fetch from remote");

    if (success) {
        OH_LOG_INFO(LOG_APP, "Fetch completed successfully");
    } else {
        const git_error *e = git_error_last();
        if (e) {
            OH_LOG_ERROR(LOG_APP, "Fetch failed with error: %{public}s", e->message);
            // 打印更详细的错误信息
            OH_LOG_ERROR(LOG_APP, "Error class: %d, Error message: %{public}s", e->klass, e->message);
        } else {
            OH_LOG_ERROR(LOG_APP, "Fetch failed with unknown error, result code: %d", result);
        }
    }

    git_remote_free(remote);
    return success;
}

std::vector<BranchInfo> RepoManager::getRemoteBranches(const std::string &remoteName) {
    std::vector<BranchInfo> branches;

    if (!repository_) {
        setError("仓库未初始化");
        return branches;
    }

    // 获取远程仓库对象
    git_remote *remote = nullptr;
    if (!checkError(git_remote_lookup(&remote, repository_, remoteName.c_str()), "Lookup remote")) {
        return branches;
    }

    // 连接远程仓库以获取最新的引用
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    callbacks.credentials = credentials_cb;
    callbacks.certificate_check = certificate_check_cb;
    if (git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr) == 0) {
        // 获取远程引用
        const git_remote_head **remote_heads;
        size_t heads_len;

        if (checkError(git_remote_ls(&remote_heads, &heads_len, remote), "List remote references")) {
            for (size_t i = 0; i < heads_len; ++i) {
                const git_remote_head *head = remote_heads[i];

                // 只处理分支引用(以refs/heads/开头)
                if (strncmp(head->name, "refs/heads/", 11) == 0) {
                    BranchInfo branch;
                    branch.name = head->name + 11; // 去掉"refs/heads/"前缀
                    branch.id = git_oid_tostr_s(&head->oid);
                    branch.isRemote = true;
                    branch.isCurrent = false;
                    branches.push_back(branch);
                }
            }
        }
        git_remote_disconnect(remote);
    }

    git_remote_free(remote);
    if (branches.empty()) {
        setError("获取远程分支失败，请检查：1) 远程仓库是否可访问 2) 远程分支是否存在 3) 网络连接是否稳定");
    }
    return branches;
}

std::vector<TagInfo> RepoManager::getRemoteTags(const std::string &remoteName) {
    std::vector<TagInfo> tags;

    if (!repository_) {
        setError("仓库未初始化");
        return tags;
    }

    // 获取远程仓库对象
    git_remote *remote = nullptr;
    if (!checkError(git_remote_lookup(&remote, repository_, remoteName.c_str()), "Lookup remote")) {
        return tags;
    }

    // 连接远程仓库以获取最新的引用
    git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
    callbacks.credentials = credentials_cb;
    callbacks.certificate_check = certificate_check_cb;
    if (git_remote_connect(remote, GIT_DIRECTION_FETCH, &callbacks, nullptr, nullptr) == 0) {
        // 获取远程引用
        const git_remote_head **remote_heads;
        size_t heads_len;

        if (checkError(git_remote_ls(&remote_heads, &heads_len, remote), "List remote references")) {
            for (size_t i = 0; i < heads_len; ++i) {
                const git_remote_head *head = remote_heads[i];
                // 只处理标签引用(以refs/tags/开头)
                if (strncmp(head->name, "refs/tags/", 10) == 0) {
                    TagInfo tag;
                    tag.name = head->name + 10; // 去掉"refs/tags/"前缀
                    tag.id = git_oid_tostr_s(&head->oid);

                    // 检查是否有剥离后的目标提交ID
                    if (!git_oid_is_zero(&head->loid)) {
                        tag.peeledId = git_oid_tostr_s(&head->loid);
                        tag.isAnnotated = true; // 有剥离ID说明是附注标签
                    } else {
                        tag.peeledId = "";
                        tag.isAnnotated = false; // 没有剥离ID可能是轻量标签
                    }

                    tags.push_back(tag);
                }
            }
        }
        git_remote_disconnect(remote);
    }

    git_remote_free(remote);
    if (tags.empty()) {
        setError("获取远程标签失败，请检查：1) 远程仓库是否可访问 2) 远程标签是否存在 3) 网络连接是否稳定");
    }
    return tags;
}

CommitInfo RepoManager::convertToCommitInfo(git_commit *commit) {
    CommitInfo info;

    // 获取提交ID
    const git_oid *oid = git_commit_id(commit);
    info.id = git_oid_tostr_s(oid);

    // 获取短ID（前7个字符）
    char short_id[GIT_OID_HEXSZ + 1] = {0};
    git_oid_tostr(short_id, 8, oid); // 8是为了包含结尾的'\0'
    info.shortId = short_id;

    // 获取作者信息
    const git_signature *author = git_commit_author(commit);
    if (author) {
        info.author = author->name ? author->name : "";
        info.email = author->email ? author->email : "";
        info.timestamp = author->when.time;
    }

    // 获取提交信息
    const char *message = git_commit_message(commit);
    if (message) {
        info.message = message;
        // 获取简短信息（第一行）
        const char *newline = strchr(message, '\n');
        if (newline) {
            info.shortMessage = std::string(message, newline - message);
        } else {
            info.shortMessage = message;
        }
    }

    // 获取父提交
    unsigned int parent_count = git_commit_parentcount(commit);
    for (unsigned int i = 0; i < parent_count; ++i) {
        git_commit *parent;
        if (git_commit_parent(&parent, commit, i) == 0) {
            const git_oid *parent_oid = git_commit_id(parent);
            info.parentIds.push_back(git_oid_tostr_s(parent_oid));
            git_commit_free(parent);
        }
    }

    return info;
}

std::vector<CommitInfo> RepoManager::getCommitHistory(const std::string &branch, int count) {
    std::vector<CommitInfo> commits;

    if (!repository_) {
        setError("No repository opened");
        return commits;
    }

    // 解析分支或提交ID
    git_oid oid;
    if (!resolveReference(oid, branch)) {
        return commits;
    }

    // 创建提交遍历器
    git_revwalk *walk;
    if (!checkError(git_revwalk_new(&walk, repository_), "Create revision walker")) {
        return commits;
    }

    // 添加起始提交
    if (!checkError(git_revwalk_push(walk, &oid), "Push commit to walker")) {
        git_revwalk_free(walk);
        return commits;
    }

    // 设置排序方式（时间倒序）
    git_revwalk_sorting(walk, GIT_SORT_TIME);

    // 遍历提交历史
    git_oid commit_oid;
    int commit_count = 0;
    while (git_revwalk_next(&commit_oid, walk) == 0 && commit_count < count) {
        git_commit *commit;
        if (git_commit_lookup(&commit, repository_, &commit_oid) == 0) {
            commits.push_back(convertToCommitInfo(commit));
            git_commit_free(commit);
            commit_count++;
        }
    }

    git_revwalk_free(walk);
    return commits;
}

std::vector<CommitInfo> RepoManager::getCommitHistory(const std::string &branch, int count, int offset) {
    std::vector<CommitInfo> commits;

    if (!repository_) {
        setError("仓库未初始化");
        return commits;
    }

    // 解析分支或提交ID
    git_oid oid;
    if (!resolveReference(oid, branch)) {
        setError("获取提交历史失败，请检查：1) 分支是否存在 2) 提交ID是否正确 3) 网络连接是否稳定");
        return commits;
    }

    // 创建提交遍历器
    git_revwalk *walk;
    if (!checkError(git_revwalk_new(&walk, repository_), "Create revision walker")) {
        return commits;
    }

    // 添加起始提交
    if (!checkError(git_revwalk_push(walk, &oid), "Push commit to walker")) {
        git_revwalk_free(walk);
        return commits;
    }

    // 设置排序方式（时间倒序）
    git_revwalk_sorting(walk, GIT_SORT_TIME);

    // 跳过前offset个提交
    git_oid commit_oid;
    int skipped = 0;
    while (skipped < offset && git_revwalk_next(&commit_oid, walk) == 0) {
        skipped++;
    }

    // 遍历提交历史
    int commit_count = 0;
    while (git_revwalk_next(&commit_oid, walk) == 0 && commit_count < count) {
        git_commit *commit;
        if (git_commit_lookup(&commit, repository_, &commit_oid) == 0) {
            commits.push_back(convertToCommitInfo(commit));
            git_commit_free(commit);
            commit_count++;
        }
    }

    git_revwalk_free(walk);
    if (commits.empty()) {
        setError("获取提交历史失败，请检查：1) 分支是否存在 2) 提交ID是否正确 3) 网络连接是否稳定");
    }
    return commits;
}

CommitInfo RepoManager::getCommitDetails(const std::string &commitId) {
    CommitInfo info;

    if (!repository_) {
        setError("No repository opened");
        return info;
    }

    // 解析提交ID
    git_oid oid;
    if (!resolveReference(oid, commitId)) {
        return info;
    }

    // 查找提交
    git_commit *commit;
    if (!checkError(git_commit_lookup(&commit, repository_, &oid), "Lookup commit")) {
        return info;
    }

    info = convertToCommitInfo(commit);
    git_commit_free(commit);

    return info;
}

std::vector<BranchInfo> RepoManager::getLocalBranches() {
    std::vector<BranchInfo> branches;

    if (!repository_) {
        setError("No repository opened");
        return branches;
    }

    // 获取所有引用
    git_branch_iterator *iter;
    if (!checkError(git_branch_iterator_new(&iter, repository_, GIT_BRANCH_LOCAL), "Create branch iterator")) {
        return branches;
    }

    git_reference *ref;
    git_branch_t branch_type;
    while (git_branch_next(&ref, &branch_type, iter) == 0) {
        BranchInfo branch;

        // 获取分支名称
        const char *name;
        if (git_branch_name(&name, ref) == 0) {
            branch.name = name;
        }

        // 获取分支指向的提交ID
        const git_oid *target_oid = git_reference_target(ref);
        if (target_oid) {
            branch.id = git_oid_tostr_s(target_oid);
        }

        branch.isRemote = false;

        // 检查是否为当前分支
        git_reference *head_ref;
        if (git_repository_head(&head_ref, repository_) == 0) {
            branch.isCurrent = git_reference_cmp(ref, head_ref) == 0;
            git_reference_free(head_ref);
        } else {
            branch.isCurrent = false;
        }

        branches.push_back(branch);
        git_reference_free(ref);
    }

    git_branch_iterator_free(iter);
    return branches;
}

bool RepoManager::checkoutBranch(const std::string &branchName) {
    if (!repository_) {
        setError("No repository opened");
        return false;
    }

    // 查找分支引用
    git_reference *ref;
    std::string ref_name = "refs/heads/" + branchName;
    if (!checkError(git_reference_lookup(&ref, repository_, ref_name.c_str()), "Lookup branch reference")) {
        return false;
    }

    // 获取分支指向的提交
    git_object *target;
    if (!checkError(git_reference_peel(&target, ref, GIT_OBJECT_COMMIT), "Peel reference to commit")) {
        git_reference_free(ref);
        return false;
    }

    // 执行检出操作
    git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
    checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;

    if (!checkError(git_checkout_tree(repository_, target, &checkout_opts), "Checkout tree")) {
        git_object_free(target);
        git_reference_free(ref);
        return false;
    }

    // 设置HEAD引用
    if (!checkError(git_repository_set_head(repository_, ref_name.c_str()), "Set HEAD reference")) {
        git_object_free(target);
        git_reference_free(ref);
        return false;
    }

    git_object_free(target);
    git_reference_free(ref);
    return true;
}

bool RepoManager::createBranch(const std::string &branchName, const std::string &commitId) {
    if (!repository_) {
        setError("No repository opened");
        return false;
    }

    // 解析提交ID
    git_oid oid;
    if (!checkError(git_oid_fromstr(&oid, commitId.c_str()), "Parse commit ID")) {
        return false;
    }

    // 创建分支
    git_commit *commit;
    if (!checkError(git_commit_lookup(&commit, repository_, &oid), "Lookup commit")) {
        return false;
    }

    git_reference *ref;
    std::string ref_name = "refs/heads/" + branchName;
    bool result = checkError(git_branch_create(&ref, repository_, branchName.c_str(), commit, 0), "Create branch");

    if (ref) {
        git_reference_free(ref);
    }
    git_commit_free(commit);

    return result;
}

bool RepoManager::isConnected() const { return remote_ != nullptr && git_remote_connected(remote_); }

bool RepoManager::isOpen() const { return repository_ != nullptr; }

std::string RepoManager::getLastError() const { return lastError_; }

std::string RepoManager::getCurrentBranch() const {
    if (!repository_) {
        return "";
    }

    git_reference *head;
    if (git_repository_head(&head, repository_) != 0) {
        return "";
    }

    const char *branchName;
    if (git_branch_name(&branchName, head) != 0) {
        git_reference_free(head);
        return "";
    }

    std::string result = branchName;
    git_reference_free(head);
    return result;
}

std::string RepoManager::getRemoteUrl(const std::string &remoteName) const {
    if (!repository_) {
        return "";
    }

    git_remote *remote = nullptr;
    if (git_remote_lookup(&remote, repository_, remoteName.c_str()) != 0) {
        return "";
    }

    const char *url = git_remote_url(remote);
    std::string result = url ? url : "";

    git_remote_free(remote);
    return result;
}

bool RepoManager::resolveReference(git_oid &oid, const std::string &ref) {
    if (!repository_) {
        setError("No repository opened");
        return false;
    }

    // 首先尝试作为分支名查找
    if (git_reference_name_to_id(&oid, repository_, ("refs/heads/" + ref).c_str()) == 0) {
        OH_LOG_DEBUG(LOG_APP, "Resolved as local branch: refs/heads/%{public}s", ref.c_str());
        return true;
    }

    // 尝试作为远程分支名查找
    if (git_reference_name_to_id(&oid, repository_, ("refs/remotes/origin/" + ref).c_str()) == 0) {
        OH_LOG_DEBUG(LOG_APP, "Resolved as remote branch: refs/remotes/origin/%{public}s", ref.c_str());
        return true;
    }

    // 尝试作为完整的远程分支引用查找
    if (git_reference_name_to_id(&oid, repository_, ref.c_str()) == 0) {
        OH_LOG_DEBUG(LOG_APP, "Resolved as full reference: %{public}s", ref.c_str());
        return true;
    }

    // 如果还不是远程分支名，尝试直接解析为提交ID
    if (git_oid_fromstr(&oid, ref.c_str()) == 0) {
        OH_LOG_DEBUG(LOG_APP, "Resolved as commit ID: %{public}s", ref.c_str());
        return true;
    }

    // 所有尝试都失败了
    setError("Cannot resolve reference: " + ref);
    return false;
}

std::string RepoManager::getRepositoryPath() const { return repoPath_; }

std::vector<FileTreeNode> RepoManager::getBranchFileTree(const std::string &branch, const std::string &rootPath) {
    std::vector<FileTreeNode> fileTree;

    if (!repository_) {
        setError("仓库未初始化");
        return fileTree;
    }

    // 解析分支或提交ID
    git_oid oid;
    if (!resolveReference(oid, branch)) {
        setError("获取文件树失败，请检查：1) 分支是否存在 2) 提交ID是否正确 3) 网络连接是否稳定");
        return fileTree;
    }

    // 查找提交对象
    git_commit *commit = nullptr;
    if (!checkError(git_commit_lookup(&commit, repository_, &oid), "Lookup commit")) {
        return fileTree;
    }

    // 获取提交的树对象
    git_tree *tree = nullptr;
    if (!checkError(git_commit_tree(&tree, commit), "Get commit tree")) {
        git_commit_free(commit);
        return fileTree;
    }

    // 如果指定了根路径，导航到该目录
    git_tree *target_tree = tree;
    if (!rootPath.empty()) {
        git_tree_entry *entry = nullptr;
        if (checkError(git_tree_entry_bypath(&entry, tree, rootPath.c_str()), "Find root path in tree")) {
            if (git_tree_entry_type(entry) == GIT_OBJECT_TREE) {
                git_tree *subtree = nullptr;
                if (checkError(git_tree_lookup(&subtree, repository_, git_tree_entry_id(entry)), "Lookup subtree")) {
                    git_tree_free(tree);
                    target_tree = subtree;
                }
            }
            git_tree_entry_free(entry);
        }
    }

    // 递归遍历文件树
    int nextId = 1;
    traverseTree(target_tree, rootPath, -1, nextId, fileTree);

    // 清理资源
    git_tree_free(target_tree);
    git_commit_free(commit);

    return fileTree;
}

void RepoManager::traverseTree(git_tree *tree, const std::string &basePath, int parentId, int &nextId,
                               std::vector<FileTreeNode> &fileTree) {
    if (!tree)
        return;

    size_t entry_count = git_tree_entrycount(tree);
    for (size_t i = 0; i < entry_count; ++i) {
        const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
        if (!entry)
            continue;

        FileTreeNode node;
        node.id = nextId++;
        node.parentId = parentId;
        node.name = git_tree_entry_name(entry);
        node.path = basePath.empty() ? node.name : basePath + "/" + node.name;
        node.isDirectory = (git_tree_entry_type(entry) == GIT_OBJECT_TREE);
        node.fileId = git_oid_tostr_s(git_tree_entry_id(entry));
        node.mode = git_tree_entry_filemode(entry);
        node.size = 0;

        // 获取文件扩展名
        if (!node.isDirectory) {
            size_t dot_pos = node.name.find_last_of('.');
            if (dot_pos != std::string::npos && dot_pos < node.name.length() - 1) {
                node.extension = node.name.substr(dot_pos + 1);
            }

            // 获取文件大小（如果是blob对象）
            if (git_tree_entry_type(entry) == GIT_OBJECT_BLOB) {
                git_blob *blob = nullptr;
                if (git_blob_lookup(&blob, repository_, git_tree_entry_id(entry)) == 0) {
                    node.size = git_blob_rawsize(blob);
                    git_blob_free(blob);
                }
            }
        }

        fileTree.push_back(node);

        // 如果是目录，递归遍历
        if (node.isDirectory) {
            git_tree *subtree = nullptr;
            if (git_tree_lookup(&subtree, repository_, git_tree_entry_id(entry)) == 0) {
                traverseTree(subtree, node.path, node.id, nextId, fileTree);
                git_tree_free(subtree);
            }
        }
    }
}

FileContent RepoManager::readFile(const std::string &branch, const std::string &path) {
    FileContent result;
    result.exists = false;
    result.isBinary = false;
    result.content = "";

    if (!repository_) {
        setError("仓库未打开");
        return result;
    }

    if (path.empty()) {
        setError("文件路径不能为空");
        return result;
    }

    // 解析分支或提交ID
    git_oid oid;
    if (!resolveReference(oid, branch)) {
        setError("读取文件失败，请检查：1) 分支是否存在 2) 提交ID是否正确 3) 网络连接是否稳定");
        return result;
    }

    // 查找提交对象
    git_commit *commit = nullptr;
    if (!checkError(git_commit_lookup(&commit, repository_, &oid), "Lookup commit")) {
        return result;
    }

    // 获取提交的树对象
    git_tree *tree = nullptr;
    if (!checkError(git_commit_tree(&tree, commit), "Get commit tree")) {
        git_commit_free(commit);
        return result;
    }

    // 查找文件对应的树条目
    git_tree_entry *entry = nullptr;
    int error = git_tree_entry_bypath(&entry, tree, path.c_str());
    if (error != 0) {
        if (error == GIT_ENOTFOUND) {
            setError("文件未找到: " + path);
        } else {
            checkError(error, "Find file in tree");
        }
        git_tree_free(tree);
        git_commit_free(commit);
        return result;
    }

    // 检查条目类型，确保是文件而不是目录
    if (git_tree_entry_type(entry) != GIT_OBJECT_BLOB) {
        setError("路径指向的不是文件: " + path);
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return result;
    }

    // 获取blob对象
    git_blob *blob = nullptr;
    if (!checkError(git_blob_lookup(&blob, repository_, git_tree_entry_id(entry)), "Lookup blob")) {
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return result;
    }

    // 文件存在，设置标志
    result.exists = true;

    // 获取文件内容
    const void *rawdata = git_blob_rawcontent(blob);
    size_t size = git_blob_rawsize(blob);

    if (rawdata && size > 0) {
        // 使用libgit2内置的二进制文件检测
        result.isBinary = git_blob_is_binary(blob) == 1;

        if (result.isBinary) {
            // 二进制文件，返回提示信息
            result.content = "[Binary file, size: " + std::to_string(size) + " bytes]";
        } else {
            // 文本文件，直接返回内容
            const auto *data = static_cast<const char *>(rawdata);
            result.content = std::string(data, size);
        }
    } else {
        // 空文件但存在
        result.isBinary = false;
        result.content = "";
    }

    // 清理资源
    git_blob_free(blob);
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    return result;
}