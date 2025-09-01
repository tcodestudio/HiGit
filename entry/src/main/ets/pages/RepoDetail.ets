import { BreakpointConstants } from "../constants/BreakpointConstants"
import { getRepoById } from "../services/AppService";
import { CommitItemView } from "../views/CommitItemVIew";
import { hilog } from "@kit.PerformanceAnalysisKit";
import { common } from '@kit.AbilityKit'
import { PopupLoading } from "../views/PopupLoading";
import { PromptAction } from "@kit.ArkUI"
import { truncateUrl } from "../utils/Utils";
import { JSON } from "@kit.ArkTS";
import { CommitItem } from '../data/Commit'
import { ToastHook } from "../utils/ToastHook";
import RepoDetailViewModel from "../viewModel/RepoDetailViewModel";

interface SelectOption {
  value: string;
}

@Builder
export function RepoDetailBuilder() {
  RepoDetail();
}

@Component
struct RepoDetail {
  private uiContext: UIContext = this.getUIContext();
  private context = this.uiContext.getHostContext() as common.UIAbilityContext;
  private promptAction: PromptAction = this.uiContext.getPromptAction();
  private toastHook: ToastHook = new ToastHook(this.uiContext);
  @State viewModel: RepoDetailViewModel = new RepoDetailViewModel();
  @StorageLink('currentBreakpoint') currentBreakpoint: string = BreakpointConstants.BREAKPOINT_LG;
  pathStack: NavPathStack = new NavPathStack();

  aboutToAppear() {
    this.viewModel.setToastHook(this.toastHook);
    this.viewModel.setPopHandler(() => {
      this.pathStack.pop(true);
    })
  }

  ready() {
    this.viewModel.openRepo(this.context);
  }

  private getRepoInitial(): string {
    if (!this.viewModel.repo || !this.viewModel.repo.name) {
      return 'R'
    }
    const trimmed = this.viewModel.repo.name.trim()
    if (trimmed.length === 0) {
      return 'R'
    }
    return trimmed.charAt(0).toUpperCase()
  }

  @Builder
  menus() {
    Row() {
      Button({ type: ButtonType.Circle, stateEffect: true }) {
        Text() {
          SymbolSpan($r('sys.symbol.folder_badge_eye'))
            .fontSize(24)
            .renderingStrategy(SymbolRenderingStrategy.SINGLE)
            .effectStrategy(SymbolEffectStrategy.SCALE)
        }
      }
      .width(40)
      .height(40)
      .backgroundColor($r('sys.color.comp_background_tertiary'))
      .margin({
        top: 8,
        right: 16,
      })
      .onClick(() => {
        if (this.viewModel.isLoading) {
          return;
        }
        if (this.viewModel.commitList.length === 0) {
          this.promptAction.showToast({
            message: "请先拉取该分支代码"
          })
          return;
        }
        this.pathStack.pushPathByName("Files",
          JSON.stringify({ id: this.viewModel.repo!.id, branch: this.viewModel.selectedBranch }),
          true)
      })

      Button({ type: ButtonType.Circle, stateEffect: true }) {
        Text() {
          SymbolSpan($r('sys.symbol.trash'))
            .fontSize(24)
            .renderingStrategy(SymbolRenderingStrategy.SINGLE)
            .effectStrategy(SymbolEffectStrategy.SCALE)
        }
      }
      .width(40)
      .height(40)
      .backgroundColor($r('sys.color.comp_background_tertiary'))
      .margin({
        top: 8,
        right: 16,
      })
      .onClick(() => {
        this.getUIContext().showAlertDialog(
          {
            title: '提醒',
            message: '确认移除该仓库？',
            autoCancel: true,
            alignment: DialogAlignment.Center,
            offset: { dx: 0, dy: -20 },
            gridCount: 3,
            buttons: [{
              value: '取消',
              action: () => {
              }
            },
              {
                enabled: true,
                defaultFocus: true,
                style: DialogButtonStyle.HIGHLIGHT,
                value: '确认',
                action: () => {
                  this.viewModel.deleteRepo(this.context);
                }
              }],
          });
      })
    }
  }

  @Builder
  errorView() {
    Column() {
      Text(this.viewModel.errorMessage)
        .fontColor($r('sys.color.font_primary'))
        .margin(16)
        .alignSelf(ItemAlign.Start)
    }
    .width('100%')
  }

  build() {
    NavDestination() {
      RelativeContainer() {
        if (this.viewModel.repo == null) {
          PopupLoading({ visible: true, modalSize: 160, message: '初始化中…' })
        } else {
          Column() {
            // 概览卡片
            Column() {
              // 顶部信息行
              Row({ space: 6 }) {
                // 头像圆标：更大尺寸与更鲜明的品牌色
                Row() {
                  Text(this.getRepoInitial())
                    .fontSize(16)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#0B1220')
                }
                .width(36)
                .height(36)
                .borderRadius(18)
                .backgroundColor('#93C5FD')
                .justifyContent(FlexAlign.Center)
                .alignItems(VerticalAlign.Center)

                // 名称、提供方、URL（标题与链接左对齐，链接前不放徽章）
                Column() {
                  Text(this.viewModel.repo.name || 'Repository')
                    .fontSize(18)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#0F172A')

                  // URL 独占一行，与标题左对齐，使用智能截断
                  Text(this.viewModel.repo.url && this.viewModel.repo.url.length > 0 ?
                  truncateUrl(this.viewModel.repo.url, 45) :
                    'git@github.com:hntvdam/dam.git')
                    .fontSize(12)
                    .fontColor('#334155')
                    .margin({ top: 4 })
                    .maxLines(1)
                    .textOverflow({ overflow: TextOverflow.Ellipsis })
                }
                .alignItems(HorizontalAlign.Start)
                .margin({ left: 4 })
              }
              .width('100%')
              .alignItems(VerticalAlign.Top)

              // 细线分隔
              Row() {
              }
              .height(1)
              .backgroundColor('#0000000A')
              .margin({ top: 12, bottom: 10 })

              // 统计信息：分支 / 标签 / 提交
              Row({ space: 12 }) {
                Column() {
                  Row({ space: 6 }) {
                    Text('🌿').fontSize(12)
                    Text('分支').fontSize(12).fontColor('#64748B')
                  }

                  Text(this.viewModel.branches.length.toString())
                    .fontSize(16)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#0F172A')
                    .margin({ top: 4 })
                }
                .padding(10)
                .backgroundColor('#F8FAFC')
                .borderRadius(12)
                .border({ width: 1, color: '#0000000D' })
                .layoutWeight(1)

                Column() {
                  Row({ space: 6 }) {
                    Text('🏷️').fontSize(12)
                    Text('标签').fontSize(12).fontColor('#64748B')
                  }

                  Text(`${this.viewModel.repo.tags}`)
                    .fontSize(16)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#0F172A')
                    .margin({ top: 4 })
                }
                .padding(10)
                .backgroundColor('#F8FAFC')
                .borderRadius(12)
                .border({ width: 1, color: '#0000000D' })
                .layoutWeight(1)

                Column() {
                  Row({ space: 6 }) {
                    Text('⛏️').fontSize(12)
                    Text('提交').fontSize(12).fontColor('#64748B')
                  }

                  Text(`${this.viewModel.repo.commits}`)
                    .fontSize(16)
                    .fontWeight(FontWeight.Medium)
                    .fontColor('#0F172A')
                    .margin({ top: 4 })
                }
                .padding(10)
                .backgroundColor('#F8FAFC')
                .borderRadius(12)
                .border({ width: 1, color: '#0000000D' })
                .layoutWeight(1)
              }
            }
            .padding(16)
            .backgroundColor($r('app.color.white'))
            .border({ width: 1, color: '#0000000D' })
            .borderRadius(16)
            .margin({ left: 12, right: 12, top: 10 })
            .shadow({ radius: 2, color: $r('sys.color.gray_02') })


            // 分支选择（胶囊按钮）
            Row({ space: 8 }) {
              Text('分支').fontSize(12).fontColor('#64748B')

              Select(this.viewModel.branches.map((branch: string): SelectOption => ({ value: branch })))
                .selected(this.viewModel.selectedBranchIndex)
                .value(this.viewModel.selectedBranch)
                .font({ size: 16, weight: 500 })
                .fontColor('#182431')
                .selectedOptionFont({ size: 16, weight: 400 })
                .optionFont({ size: 16, weight: 400 })
                .space(8)
                .arrowPosition(ArrowPosition.END)
                .menuAlign(MenuAlignType.START, { dx: 0, dy: 0 })
                .optionWidth(200)
                .optionHeight(300)
                .controlSize(ControlSize.SMALL)
                .onSelect((index: number, text?: string | undefined) => {
                  this.viewModel.selectedBranchIndex = index;
                  if (text) {
                    this.viewModel.selectedBranch = text;
                    this.viewModel.loadGitCommits(this.context);
                  }
                })

              Blank()

              Text() {
                SymbolSpan($r('sys.symbol.arrow_clockwise'))
                  .fontSize(16)
                  .fontColor([$r('sys.color.font_secondary')])
              }
              .onClick(() => {
                this.viewModel.gitFetch(this.context);
              })
              .margin({ right: 16 })
            }
            .width('100%')
            .padding({ left: 12, right: 12, top: 12 })

            Column() {
              if (this.viewModel.commitList.length == 0) {
                // 空态提示（尚未获取该分支的提交数据）
                Column() {

                  Button() {
                    Text('请求该分支的数据').fontSize(12).fontColor($r('app.color.white'))
                  }
                  .padding({
                    left: 14,
                    right: 14,
                    top: 8,
                    bottom: 8
                  })
                  .borderRadius(999)
                  .backgroundColor('#0EA5E9')
                  .margin({ top: 12 })
                  .onClick(() => {
                    this.viewModel.gitFetch(this.context);
                  })
                }
                .justifyContent(FlexAlign.Center)
                .alignItems(HorizontalAlign.Center)
                .width('100%')
                .padding({ top: 24, bottom: 24 })
              } else {
                ForEach(this.viewModel.commitList, (c: CommitItem) => {
                  CommitItemView({
                    commit: c,
                  })
                }, (c: CommitItem) => c.id)

                // 加载更多（pill 按钮）
                Row() {
                  Row() {
                    Text('查看更多提交').fontSize(12)
                  }
                  .padding({
                    left: 14,
                    right: 14,
                    top: 8,
                    bottom: 8
                  })
                  .borderRadius(999)
                  .backgroundColor('#F1F5F9')
                  .border({ width: 1, color: '#00000010' })
                  .onClick(() => {
                    this.pathStack.pushPathByName("CommitHistory",
                      JSON.stringify({ id: this.viewModel.repo!.id, branch: this.viewModel.selectedBranch }),
                      true)
                  })
                }
                .justifyContent(FlexAlign.Center)
                .width('100%')
                .padding(12)
              }
            }
            .backgroundColor($r('app.color.white'))
            .border({ width: 1, color: '#0000000D' })
            .borderRadius(16)
            .shadow({ radius: 2, color: $r('sys.color.gray_02') })
            .margin({
              left: 12,
              right: 12,
              top: 12,
              bottom: 24
            })
            .alignItems(HorizontalAlign.Start)
          }
          .width('100%')
          .height('100%')
          .backgroundColor($r('sys.color.comp_background_focus'))
          .alignItems(HorizontalAlign.Start)
        }

        PopupLoading({ visible: this.viewModel.isLoading, modalSize: 160, message: this.viewModel.loadingMessage })

      }
    }
    .title('仓库详情')
    .width('100%')
    .height('100%')
    .backgroundColor('#F8FAFC')
    .onReady((context: NavDestinationContext) => {
      this.pathStack = context.pathStack;
      const raw = this.pathStack.getParamByName('RepoDetail') as string[];
      if (!raw || raw.length == 0 || !raw[0]) {
        this.pathStack.pop(true);
        return;
      }
      try {
        hilog.info(0x0000, "appTag", "get repo id %{public}s", raw[0]);
        let data = getRepoById(this.context, raw[0]);
        if (data) {
          this.viewModel.repo = data;
          this.ready();
        } else {
          this.pathStack.pop(true);
        }
      } catch (error) {
        hilog.info(0x0000, "appTag", error);
      }
    })
    .menus(this.menus)
    .onBackPressed(() => {
      this.pathStack.pop(true);
      return true;
    })
    .bindSheet(this.viewModel.showErrorMessage, this.errorView, {
      showClose: true,
      detents: [SheetSize.MEDIUM],
      dragBar: true,
      blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
      title: {
        title: "错误信息",
      },
      onDisappear: () => {
        this.viewModel.showErrorMessage = false;
        this.pathStack.pop(true);
      },
      backgroundColor: 125831006,
      preferType: SheetType.CENTER,
    })
  }
}

