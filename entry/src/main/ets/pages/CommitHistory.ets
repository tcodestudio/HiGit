import { CommitItemView } from "../views/CommitItemVIew";
import { getRepoById } from "../services/AppService";
import { common } from "@kit.AbilityKit";
import { PromptAction } from "@kit.ArkUI";
import { JSON } from "@kit.ArkTS";
import { CommitItem } from '../data/Commit'
import CommitHistoryViewModel from "../viewModel/CommitHistoryViewModel";
import { ToastHook } from "../utils/ToastHook";

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
  private toastHook: ToastHook = new ToastHook(this.uiContext);
  @State viewModel: CommitHistoryViewModel = new CommitHistoryViewModel();
  pathStack: NavPathStack = new NavPathStack();
  @State refreshOffset: number = 0;
  @State refreshState: RefreshStatus = RefreshStatus.Inactive;

  aboutToAppear() {
    this.viewModel.setToastHook(this.toastHook);
  }

  ready() {
    this.viewModel.loadCommits();
  }

  @Builder
  refreshBuilder() {
    Stack({ alignContent: Alignment.Bottom }) {
      if (this.refreshState != RefreshStatus.Inactive && this.refreshState != RefreshStatus.Done) {
        Progress({ value: this.refreshOffset, total: 64, type: ProgressType.Ring })
          .width(32).height(32)
          .style({ status: this.viewModel.refreshing ? ProgressStatus.LOADING : ProgressStatus.PROGRESSING })
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
    .visibility(this.viewModel.isLoading ? Visibility.Visible : Visibility.Hidden)
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
        this.viewModel.updateRepo(repoData);
        this.viewModel.updateBranch(param.branch);
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
      Refresh({ refreshing: $$this.viewModel.isRefreshing, builder: this.refreshBuilder() }) {
        List() {
          LazyForEach(this.viewModel.data, (c: CommitItem) => {
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
          .visibility(this.viewModel.isLastPage ? Visibility.Visible : Visibility.Hidden)
        }
        .width('100%')
        .height('100%')
        .onScrollIndex((start: number, end: number) => {
          if (end >= this.viewModel.data.totalCount() - 1 && end != 0) {
            this.viewModel.isLoading = true;
            this.viewModel.currentPage = this.viewModel.currentPage + 1;
            this.viewModel.loadCommits();
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
        this.viewModel.reset();
        this.viewModel.loadCommits();
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

