import { RepoItem } from "../data/RepoItem";
import { hilog } from "@kit.PerformanceAnalysisKit";
import { taskpool } from "@kit.ArkTS";
import { getAllRepos, initApp } from "../services/AppService";
import { initGit } from "../services/GitService";


@Concurrent
async function init(context: Context) {
  initApp(context);
}

@Concurrent
async function listRepos(context: Context) {
  return getAllRepos(context)
}

@Observed
export default class IndexViewModel {
  @Track isLoading: boolean = false;
  @Track loadingMessage: string = "";
  @Track repos: RepoItem[] = [];

  /**
   * 初始化项目
   * @param context
   */
  init(context: Context) {
    taskpool.execute(init, context).then((_) => {
      context.eventHub.on('reload_app_list', (_: string) => {
        this.loadRepoList(context);
      });
      taskpool.execute(initGit, context.filesDir).then((_) => {
        this.loadRepoList(context);
      });
    });
  }

  /**
   * 加载仓库列表
   * @param context
   */
  async loadRepoList(context: Context) {
    hilog.info(0x0000, "appTag", "index get repo list");
    this.loadingMessage = "加载中...";
    this.isLoading = true;
    this.repos = [];
    taskpool.execute(listRepos, context).then((data) => {
      hilog.info(0x0000, "appTag", "list repos success");
      this.repos = data as RepoItem[];
      this.isLoading = false;
    })
  }
}