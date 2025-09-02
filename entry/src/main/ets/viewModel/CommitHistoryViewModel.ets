import { CommitItem } from "../data/Commit";
import { RepoItem } from "../data/RepoItem";
import { BasicDataSource } from "../utils/BasicDataSource";
import { taskpool } from "@kit.ArkTS";
import { getCommits, parseCommitList } from "../services/GitService";
import { Result } from "../data/Result";
import BaseViewModel from "../views/BaseViewModel";

@Observed
export default class CommitHistoryViewModel extends BaseViewModel {
  @Track data: BasicDataSource<CommitItem> = new BasicDataSource<CommitItem>();
  // 数据
  @Track repo: RepoItem | null = null;
  // 分页数据
  private pageSize: number = 20;
  @Track currentPage: number = 0;
  @Track isLastPage: boolean = false;
  // 刷新状态
  @Track isRefreshing: boolean = false;
  @Track isLoading: boolean = false;
  @Track selectedBranch: string = 'main';
  @Track refreshing: boolean = false;

  updateRepo(repo: RepoItem) {
    this.repo = repo;
  }

  updateBranch(branch: string) {
    this.selectedBranch = branch;
  }

  push(list: CommitItem[]) {
    list.forEach((item) => {
      this.data.pushData(item);
    })
  }

  clean() {
    this.data.cleanData();
  }

  reset() {
    this.currentPage = 0;
    this.clean();
    this.isLastPage = false;
  }

  async loadCommits() {
    if (this.isLastPage) {
      this.isLoading = false;
      this.refreshing = false;
      return;
    }
    this.isLoading = true;
    taskpool.execute(getCommits, this.repo!.url, this.selectedBranch, this.pageSize, this.currentPage * this.pageSize)
      .then((data) => {
        let result = data as Result;
        if (result.success) {
          let list = parseCommitList(result.data);
          if (list.length < this.pageSize) {
            this.isLastPage = true;
          }
          this.push(list);
        } else {
          this.toastHook?.showToast(result.message);
        }
        this.isLoading = false;
        this.isRefreshing = false;
      })
  }
}