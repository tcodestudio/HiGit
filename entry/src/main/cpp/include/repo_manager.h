#ifndef HIGIT_REPO_MANAGER_H
#define HIGIT_REPO_MANAGER_H

#include <functional>
#include <git2.h>
#include <string>
#include <vector>

/**
 * @brief 提交信息结构体
 * 存储Git提交的相关信息
 */
struct CommitInfo {
    std::string id;                     ///< 完整提交ID
    std::string shortId;                ///< 短提交ID（前7位）
    std::string author;                 ///< 作者名称
    std::string email;                  ///< 作者邮箱
    long long timestamp;                ///< 提交时间戳
    std::string message;                ///< 完整提交信息
    std::string shortMessage;           ///< 简短提交信息（第一行）
    std::vector<std::string> parentIds; ///< 父提交ID列表
};

/**
 * @brief 分支信息结构体
 * 存储Git分支的相关信息
 */
struct BranchInfo {
    std::string name; ///< 分支名称
    std::string id;   ///< 分支指向的提交ID
    bool isRemote;    ///< 是否为远程分支
    bool isCurrent;   ///< 是否为当前分支
};

/**
 * @brief 标签信息结构体
 * 存储Git标签的相关信息
 */
struct TagInfo {
    std::string name;     ///< 标签名称（不包含refs/tags/前缀）
    std::string id;       ///< 标签指向的提交ID
    std::string peeledId; ///< 剥离后的目标提交ID（如果是附注标签）
    bool isAnnotated;     ///< 是否为附注标签
};

/**
 * @brief Fetch进度回调函数类型
 * @param receivedObjects 已接收的对象数量
 * @param totalObjects 总对象数量
 */
using FetchProgressCallback = std::function<void(unsigned int receivedObjects, unsigned int totalObjects)>;

/**
 * @brief 文件信息结构体
 * 存储文件的基本信息，用于文件树显示
 */
struct FileTreeNode {
    int id;                ///< 节点ID
    int parentId;          ///< 父节点ID，根节点为-1
    std::string name;      ///< 文件/目录名
    std::string path;      ///< 完整路径
    bool isDirectory;      ///< 是否为目录
    std::string fileId;    ///< 文件的Git对象ID
    uint32_t mode;         ///< 文件模式
    size_t size;           ///< 文件大小（目录为0）
    std::string extension; ///< 文件扩展名
};

/**
 * @brief 文件内容结构体
 * 存储文件的内容
 */
struct FileContent {
    bool exists;         ///< 文件是否存在
    bool isBinary;       ///< 是否为二进制文件
    std::string content; ///< 文件内容
};

/**
 * @brief Git仓库管理类
 * 封装了libgit2库的常用操作，提供简化的接口来管理Git仓库
 *
 * 使用说明：
 * 1. 创建RepoManager实例
 * 2. 使用openRepository()打开现有仓库或createRepository()创建新仓库
 * 3. 使用connectRemote()连接到远程仓库
 * 4. 使用getCommitHistory()获取提交历史等操作
 *
 * 注意：该类不支持拷贝构造和赋值操作
 */
class RepoManager {
public:
    /**
     * @brief 构造函数
     * 初始化libgit2库
     */
    RepoManager();

    /**
     * @brief 析构函数
     * 释放所有资源并关闭libgit2库
     */
    ~RepoManager();

    // 禁止拷贝
    RepoManager(const RepoManager &) = delete;
    RepoManager &operator=(const RepoManager &) = delete;

    /**
     * @brief 打开本地Git仓库
     * @param path 仓库路径
     * @return 成功返回true，失败返回false
     */
    bool openRepository(const std::string &path);

    /**
     * @brief 创建新的Git仓库
     * @param path 仓库路径
     * @param isBare 是否创建裸仓库（无工作目录）
     * @return 成功返回true，失败返回false
     */
    bool createRepository(const std::string &path, bool isBare = false);

    /**
     * @brief 连接到远程Git仓库
     * 如果本地路径已存在仓库则打开，否则创建新仓库并添加远程连接
     * @param url 远程仓库URL
     * @param localPath 本地仓库路径
     * @return 成功返回true，失败返回false
     */
    bool connectRemote(const std::string &url, const std::string &localPath);

    /**
     * @brief 克隆远程仓库
     * 获取完整的提交历史记录
     * @param url 远程仓库URL
     * @param localPath 本地仓库路径
     * @return 成功返回true，失败返回false
     */
    bool cloneRepository(const std::string &url, const std::string &localPath);

    /**
     * @brief 浅克隆远程仓库
     * 只克隆最新的提交记录，不包含完整历史
     * @param url 远程仓库URL
     * @param localPath 本地仓库路径
     * @param depth 克隆深度，默认为1（只克隆最新提交）
     * @return 成功返回true，失败返回false
     */
    bool shallowClone(const std::string &url, const std::string &localPath, int depth = 1);

    /**
     * @brief 添加或更新远程仓库
     * @param name 远程仓库名称
     * @param url 远程仓库URL
     * @return 成功返回true，失败返回false
     */
    bool addRemote(const std::string &name, const std::string &url);

    /**
     * @brief 从远程仓库获取更新
     * @param remoteName 远程仓库名称，默认为"origin"
     * @param branchRefs 要获取的分支引用列表，为空则获取所有分支
     * @param depth 获取深度，0表示获取完整历史，默认为10
     * @param progressCallback 进度回调函数，用于报告fetch操作的进度
     * @return 成功返回true，失败返回false
     */
    bool fetch(const std::string &remoteName = "origin", const std::vector<std::string> &branchRefs = {},
               int depth = 10, FetchProgressCallback progressCallback = nullptr);

    /**
     * @brief 获取远程分支列表
     * @param remoteName 远程仓库名称，默认为"origin"
     * @return 分支信息列表
     */
    std::vector<BranchInfo> getRemoteBranches(const std::string &remoteName = "origin");

    /**
     * @brief 获取远程标签列表
     * @param remoteName 远程仓库名称，默认为"origin"
     * @return 标签信息列表
     */
    std::vector<TagInfo> getRemoteTags(const std::string &remoteName = "origin");

    /**
     * @brief 获取提交历史记录
     * @param branch 分支名称或提交ID，默认为"HEAD"
     * @param count 获取的提交数量，默认为50
     * @return 提交信息列表
     */
    std::vector<CommitInfo> getCommitHistory(const std::string &branch = "HEAD", int count = 50);

    /**
     * @brief 获取提交历史记录（分页版本）
     * @param branch 分支名称或提交ID，默认为"HEAD"
     * @param count 获取的提交数量
     * @param offset 偏移量（跳过的提交数量）
     * @return 提交信息列表
     */
    std::vector<CommitInfo> getCommitHistory(const std::string &branch, int count, int offset);

    /**
     * @brief 获取特定提交的详细信息
     * @param commitId 提交ID
     * @return 提交信息
     */
    CommitInfo getCommitDetails(const std::string &commitId);

    /**
     * @brief 获取本地分支列表
     * @return 分支信息列表
     */
    std::vector<BranchInfo> getLocalBranches();

    /**
     * @brief 切换到指定分支
     * @param branchName 分支名称
     * @return 成功返回true，失败返回false
     */
    bool checkoutBranch(const std::string &branchName);

    /**
     * @brief 创建新分支
     * @param branchName 分支名称
     * @param commitId 分支指向的提交ID
     * @return 成功返回true，失败返回false
     */
    bool createBranch(const std::string &branchName, const std::string &commitId);

    /**
     * @brief 获取当前分支名称
     * @return 当前分支名称，如果未设置则返回空字符串
     */
    std::string getCurrentBranch() const;

    /**
     * @brief 获取指定分支的文件树结构
     * @param branch 分支名称或提交ID，默认为"HEAD"
     * @param rootPath 根路径，默认为空（仓库根目录）
     * @return 文件树节点列表，按TreeView格式组织
     */
    std::vector<FileTreeNode> getBranchFileTree(const std::string &branch = "HEAD", const std::string &rootPath = "");

    /**
     * @brief 获取远程仓库URL
     * @param remoteName 远程仓库名称，默认为"origin"
     * @return 远程仓库URL，如果未设置则返回空字符串
     */
    std::string getRemoteUrl(const std::string &remoteName = "origin") const;

    /**
     * @brief 检查是否连接到远程仓库
     * @return 已连接返回true，未连接返回false
     */
    bool isConnected() const;

    /**
     * @brief 检查仓库是否已打开
     * @return 已打开返回true，未打开返回false
     */
    bool isOpen() const;

    /**
     * @brief 获取最后的错误信息
     * @return 错误信息字符串
     */
    std::string getLastError() const;

    /**
     * @brief 获取当前仓库路径
     * @return 仓库路径字符串
     */
    std::string getRepositoryPath() const;

    /**
     * @brief 读取文件
     * @param branch 分支名称或提交ID，默认为"HEAD"
     * @param path 文件路径
     * @return 文件内容
     */
    FileContent readFile(const std::string &branch = "HEAD", const std::string &path = "");

private:
    // 私有成员变量
    git_repository *repository_; ///< Git仓库对象
    git_remote *remote_;         ///< 远程仓库对象
    std::string lastError_;      ///< 最后错误信息
    std::string remoteUrl_;      ///< 远程仓库URL
    std::string repoPath_;       ///< 本地仓库路径

    // 辅助方法
    /**
     * @brief 设置错误信息
     * @param error 错误信息
     */
    void setError(const std::string &error);

    /**
     * @brief 检查libgit2操作的返回值
     * @param error 操作返回值
     * @param operation 操作描述
     * @return 操作成功返回true，失败返回false
     */
    bool checkError(int error, const std::string &operation);

    /**
     * @brief 将git_commit转换为CommitInfo结构体
     * @param commit git_commit对象
     * @return CommitInfo结构体
     */
    CommitInfo convertToCommitInfo(git_commit *commit);

    /**
     * @brief 解析分支或提交ID为git_oid
     * @param oid 输出的git_oid
     * @param ref 分支名称或提交ID
     * @return 成功返回true，失败返回false
     */
    bool resolveReference(git_oid &oid, const std::string &ref);

    /**
     * @brief 释放所有资源
     */
    void freeResources();

    /**
     * @brief 递归遍历Git树对象，构建文件树结构
     * @param tree Git树对象
     * @param basePath 基础路径
     * @param parentId 父节点ID
     * @param nextId 下一个可用的节点ID（引用传递）
     * @param fileTree 文件树列表（引用传递）
     */
    void traverseTree(git_tree *tree, const std::string &basePath, int parentId, int &nextId,
                      std::vector<FileTreeNode> &fileTree);

    /**
     * @brief 获取错误分类的描述
     * @param error_class 错误分类枚举值
     * @return 错误分类的中文描述
     */
    std::string getErrorClassDescription(git_error_t error_class);

    /**
     * @brief 获取具体错误码的描述
     * @param error_code 错误码
     * @return 具体错误的中文描述
     */
    std::string getErrorCodeDescription(int error_code);

    /**
     * @brief 获取错误的解决方案建议
     * @param error_class 错误分类
     * @param error_code 具体错误码
     * @return 解决方案建议
     */
    std::string getErrorSolution(git_error_t error_class, int error_code);
};

#endif // HIGIT_REPO_MANAGER_H