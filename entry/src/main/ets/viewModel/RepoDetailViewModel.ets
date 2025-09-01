import { CommitItem } from "../data/Commit";
import {
  getBranches,
  initRepo,
  getTags,
  getCommits,
  fetchBranch,
  parseCommitList,
  deleteRepo
} from "../services/GitService";
import { taskpool } from "@kit.ArkTS";
import { Result } from "../data/Result";
import { RepoItem } from "../data/RepoItem";
import { updateRepo } from "../services/AppService";
import { sortBranches } from "../utils/Utils";
import { ToastHook } from "../utils/ToastHook";
import { emitter } from "@kit.BasicServicesKit";
import { hilog } from "@kit.PerformanceAnalysisKit";
import { removeRepo } from "../services/AppService";

@Observed
export default class RepoDetailViewModel {
  private toastHook?: ToastHook;
  // 加载状态
  @Track isLoading: boolean = false;
  @Track loadingMessage: string = "";
  @Track showErrorMessage: boolean = false;
  @Track errorMessage: string = "";
  // 数据管理
  @Track repo: RepoItem | null = null;
  @Track branches: string[] = [];
  @Track tags: string[] = [];
  @Track commitList: CommitItem[] = []
  @Track selectedBranch: string = '';
  @Track selectedBranchIndex: number = 0;
  // 回调
  private popHandler?: () => void;

  setPopHandler(pop: () => void) {
    this.popHandler = pop;
  }

  pop() {
    if (this.popHandler) {
      this.popHandler();
    }
  }

  setToastHook(hook: ToastHook) {
    this.toastHook = hook;
  }

  updateRepoItem(repo: RepoItem) {
    this.repo = repo;
  }

  async openRepo(context: Context) {
    this.loadingMessage = "正在打开仓库..."
    this.isLoading = true;
    taskpool.execute(initRepo, context.filesDir, this.repo!.url, this.repo!.name, this.repo!.provider)
      .then((data) => {
        let result = data as Result;
        if (!result.success) {
          this.errorMessage = result.message;
          this.showErrorMessage = true;
        } else {
          this.loadGitBranches(context);
          this.loadGitTags(context);
        }
        this.isLoading = false;
      })
  }

  saveRepo(context: Context) {
    updateRepo(context, this.repo!.url, this.repo!);
  }

  loadGitBranches(context: Context) {
    this.loadingMessage = "加载中..."
    this.isLoading = true;
    taskpool.execute(getBranches, this.repo!.url).then((data) => {
      this.isLoading = false;
      let result = data as Result;
      if (result.success) {
        let branches = JSON.parse(result.data) as Array<string>;
        this.branches = sortBranches(branches);
        if (this.branches.length > 0) {
          this.selectedBranch = this.branches[0];
          this.repo!.branch = this.selectedBranch;
          this.repo!.branches = this.branches.length;
          this.saveRepo(context);
          this.loadGitCommits(context);
        }
      } else {
        this.toastHook?.showToast(result.message);
        this.pop();
      }
    })
  }

  loadGitTags(context: Context) {
    taskpool.execute(getTags, this.repo!.url).then((data) => {
      let result = data as Result;
      if (result.success) {
        this.tags = JSON.parse(result.data) as Array<string>;
        this.repo!.tags = this.tags.length;
        this.saveRepo(context);
      } else {
        this.toastHook?.showToast(result.message);
      }
    })
  }

  loadGitCommits(context: Context) {
    this.loadingMessage = "加载中..."
    this.isLoading = true;
    taskpool.execute(getCommits, this.repo!.url, this.selectedBranch, 5, 0).then((data) => {
      let result = data as Result;
      if (result.success) {
        // 使用 parseCommitList 解析提交数据
        this.commitList = parseCommitList(result.data);
        if (this.commitList.length > 0) {
          if (this.commitList[0].timestamp > this.repo!.time) {
            this.repo!.time = this.commitList[0].timestamp;
            this.saveRepo(context);
          }
        }
        hilog.info(0x0000, "appTag", "get %{public}d commits", this.commitList.length);
      } else {
        this.toastHook?.showToast(result.message);
      }
      this.isLoading = false;
    })
  }

  gitFetch(context: Context) {
    this.loadingMessage = "分支拉取中..."
    this.isLoading = true;
    let task: taskpool.LongTask = new taskpool.LongTask(fetchBranch, this.repo!.url, this.selectedBranch)
    // 先移除之前的监听器，避免重复监听
    emitter.off("fetch");
    // 注册新的监听器
    emitter.on("fetch", (data: emitter.EventData) => {
      if (data.data?.message == "end") {
        this.loadingMessage = "拉取完成..."
      } else if (data.data?.message == "processing") {
        this.loadingMessage = data.data?.process + "/" + data.data?.total
      }
    })
    taskpool.execute(task).then((_) => {
      emitter.off("fetch");
      this.loadGitCommits(context);
    })
  }

  deleteRepo(context: Context) {
    this.isLoading = true;
    this.loadingMessage = "删除中..."
    taskpool.execute(deleteRepo, context.filesDir, this.repo!.url,
      this.repo!.name,
      this.repo!.provider).then((data) => {
      let result = data as Result;
      if (!result.success) {
        this.errorMessage = result.message;
        this.showErrorMessage = true;
      } else {
        removeRepo(context, this.repo!.url);
        this.toastHook?.showToast(result.message);
        this.pop();
      }
      this.isLoading = false;
    })
  }
}