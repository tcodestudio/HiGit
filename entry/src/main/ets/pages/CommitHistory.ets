import { RepoItem } from "../data/RepoItem";
import { getCommits, parseCommitList } from "../services/GitService";
import { CommitItemView } from "../views/CommitItemVIew";
import { hilog } from "@kit.PerformanceAnalysisKit";
import { getRepoById } from "../services/AppService";
import { common } from "@kit.AbilityKit";
import { PromptAction } from "@kit.ArkUI";
import { JSON, taskpool } from "@kit.ArkTS";
import { Result } from "../data/Result";
import { CommitItem } from '../data/Commit'

interface ParamData {
  id: string;
  branch: string;
}

@Builder
export function CommitHistoryBuilder() {
  CommitHistory();
}

@Component
struct CommitHistory {
  private uiContext: UIContext = this.getUIContext();
  private context = this.uiContext.getHostContext() as common.UIAbilityContext;
  private promptAction: PromptAction = this.uiContext.getPromptAction();
  pathStack: NavPathStack = new NavPathStack();
  @State isRefreshing: boolean = false;
  @State isLoading: boolean = false;
  @State selectedBranch: string = 'main';
  @State refreshing: boolean = false;
  @State refreshOffset: number = 0;
  @State refreshState: RefreshStatus = RefreshStatus.Inactive;
  // 分页数据
  private pageSize: number = 20;
  @State currentPage: number = 0;
  @State isLastPage: boolean = false;
  // 数据
  @State commits: CommitItem[] = [];
  @State repo: RepoItem | null = null;

  ready() {
    this.loadCommits();
  }

  async loadCommits() {
    if (this.isLastPage) {
      this.isLoading = false;
      this.refreshing = false;
      return;
    }
    this.isLoading = true;
    hilog.info(0x0000, "appTag", "get commit page %{public}d", this.currentPage);
    taskpool.execute(getCommits, this.repo!.url, this.selectedBranch, this.pageSize, this.currentPage * this.pageSize)
      .then((data) => {
        let result = data as Result;
        if (result.success) {
          // 使用 parseCommitList 解析提交数据
          let list = parseCommitList(result.data);
          if (list.length < this.pageSize) {
            this.isLastPage = true;
          }
          this.commits.push(...list);
          hilog.info(0x0000, "appTag", "get %{public}d commits", this.commits.length);
        } else {
          this.promptAction.showToast({
            message: result.message
          })
        }
        this.isLoading = false;
        this.isRefreshing = false;
      })
  }

  @Builder
  refreshBuilder() {
    Stack({ alignContent: Alignment.Bottom }) {
      if (this.refreshState != RefreshStatus.Inactive && this.refreshState != RefreshStatus.Done) {
        Progress({ value: this.refreshOffset, total: 64, type: ProgressType.Ring })
          .width(32).height(32)
          .style({ status: this.refreshing ? ProgressStatus.LOADING : ProgressStatus.PROGRESSING })
          .margin(10)
      }
    }
    .clip(true)
    .height("100%")
    .width("100%")
  }

  @Builder
  loadingFooter() {
    Row() {
      LoadingProgress().height(32).width(48)
      Text("加载中")
    }.width("100%")
    .height(64)
    .justifyContent(FlexAlign.Center)
    .visibility(this.isLoading ? Visibility.Visible : Visibility.Hidden)
  }

  initializeFromParams(context: NavDestinationContext): void {
    this.pathStack = context.pathStack;
    const raw = this.pathStack.getParamByName('CommitHistory') as string[];
    if (!raw || raw.length == 0 || !raw[0]) {
      this.pathStack.pop(true);
      return;
    }

    try {
      let param = JSON.parse(raw[0]) as ParamData
      let repoData = getRepoById(this.context, param.id);

      if (repoData) {
        this.repo = repoData;
        this.selectedBranch = param.branch;
        this.ready();
      } else {
        this.promptAction.showToast({ message: "仓库不存在" });
        this.pathStack.pop(true);
      }
    } catch (error) {
      this.promptAction.showToast({ message: "参数解析失败" });
      this.pathStack.pop(true);
    }
  }

  build() {
    NavDestination() {
      Refresh({ refreshing: $$this.isRefreshing, builder: this.refreshBuilder() }) {
        List() {
          ForEach(this.commits, (c: CommitItem) => {
            ListItem() {
              CommitItemView({
                commit: c,
              })
            }
          }, (c: CommitItem) => c.id)

          ListItem() {
            this.loadingFooter();
          }

          ListItem() {
            Text("没有更多数据了^_^")
              .fontColor($r('sys.color.font_secondary'))
          }
          .width('100%')
          .align(Alignment.Center)
          .visibility(this.isLastPage ? Visibility.Visible : Visibility.Hidden)
        }
        .width('100%')
        .height('100%')
        .onScrollIndex((start: number, end: number) => {
          if (end >= this.commits.length - 1 && end != 0) {
            this.isLoading = true;
            this.currentPage = this.currentPage + 1;
            this.loadCommits();
            hilog.info(0x0000, "appTag", "%{public}d-%{public}d", start, end, this.commits.length);
          } else {
            hilog.info(0x0000, "appTag", "else %{public}d-%{public}d", start, end, this.commits.length);
          }
        })
        .scrollBar(BarState.Off)
        // 开启边缘滑动效果。
        .edgeEffect(EdgeEffect.Spring, { alwaysEnabled: true })
      }
      .onOffsetChange((offset: number) => {
        this.refreshOffset = offset;
      })
      .onStateChange((state: RefreshStatus) => {
        this.refreshState = state;
      })
      .onRefreshing(() => {
        this.currentPage = 0;
        this.commits = [];
        this.isLastPage = false;
        this.loadCommits();
      })
      .refreshOffset(64)
      .pullToRefresh(true)
    }
    .title('提交历史')
    .width('100%')
    .height('100%')
    .hideTitleBar(false)
    .onReady((context: NavDestinationContext) => {
      this.initializeFromParams(context);
    })
    .onBackPressed(() => {
      this.pathStack.pop(true);
      return true;
    })
  }
}

