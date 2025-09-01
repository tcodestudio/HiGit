import { RepoItem } from '../data/RepoItem';
import { initGit } from '../services/GitService';
import { AboutView } from '../views/About';
import { CreateRepo } from '../views/CreateRepo';
import { HelpView } from '../views/Help';
import { PopupLoading } from '../views/PopupLoading';
import { RepoItemView } from '../views/RepoItemView';
import { SettingsView } from '../views/Settings';
import { common } from '@kit.AbilityKit'
import { taskpool } from '@kit.ArkTS';
import { getAllRepos, initApp } from '../services/AppService';
import { hilog } from '@kit.PerformanceAnalysisKit';

@Concurrent
async function init(context: Context) {
  initApp(context);
}

@Concurrent
async function listRepos(context: Context) {
  return getAllRepos(context)
}

@Entry
@Component
struct Index {
  private context = this.getUIContext().getHostContext() as common.UIAbilityContext;
  pathStack: NavPathStack = new NavPathStack();
  @State showAboutSheet: boolean = false;
  @State showHelpSheet: boolean = false;
  @State showSettingSheet: boolean = false;
  @State showCreateRepo: boolean = false;
  @State repos: RepoItem[] = [];
  @State isLoading: boolean = false;
  @State loadingMessage: string = "";

  loadRepoList() {
    hilog.info(0x0000, "appTag", "index get repo list");
    this.loadingMessage = "加载中...";
    this.isLoading = true;
    this.repos = [];
    taskpool.execute(listRepos, this.context).then((data) => {
      hilog.info(0x0000, "appTag", "list repos success");
      this.repos = data as RepoItem[];
      this.repos.forEach((item) => {
        hilog.info(0x0000, "appTag", "%{public}s %{public}s %{public}d", item.name, item.branch, item.branches);
      })
      this.isLoading = false;
    })
  }

  aboutToAppear() {
    taskpool.execute(init, this.context).then((_) => {
      this.context.eventHub.on('reload_app_list', (_: string) => {
        this.repos = [];
        this.loadRepoList();
      });
      this.context.eventHub.on('add_repo_success', (_: string) => {
        this.showCreateRepo = false;
      })
      taskpool.execute(initGit, this.context.filesDir).then((_) => {
        this.loadRepoList();
      });
    });
  }

  @Builder
  aboutSheet() {
    AboutView()
  }

  @Builder
  helpSheet() {
    HelpView()
  }

  @Builder
  settingSheet() {
    SettingsView()
  }

  @Builder
  createRepoSheet() {
    CreateRepo()
  }

  @Builder
  settingMenuList() {
    Menu() {
      MenuItem({ content: "应用设置" })
        .onClick(() => {
          this.showSettingSheet = true;
        })

      MenuItem({ content: "使用指南" })
        .onClick(() => {
          this.showHelpSheet = true;
        })

      MenuItem({ content: "关于应用" })
        .onClick(() => {
          this.showAboutSheet = true;
        })

    }
  }

  @Builder
  menus() {
    Row() {
      Button({ type: ButtonType.Circle, stateEffect: true }) {
        Text() {
          SymbolSpan($r('sys.symbol.plus'))
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
        right: 8,
      })
      .onClick(() => {
        this.showCreateRepo = true;
      })

      Button({ type: ButtonType.Circle, stateEffect: true }) {
        Text() {
          SymbolSpan($r('sys.symbol.dot_grid_2x2'))
            .fontSize(24)
            .renderingStrategy(SymbolRenderingStrategy.SINGLE)
            .effectStrategy(SymbolEffectStrategy.SCALE)
        }
      }
      .width(40)
      .height(40)
      .bindMenu(this.settingMenuList)
      .backgroundColor($r('sys.color.comp_background_tertiary'))
      .margin({
        top: 8,
        right: 16,
      })
    }
  }

  build() {
    Column() {
      Navigation(this.pathStack) {

        RelativeContainer() {

          // 仓库列表
          List() {
            ForEach(this.repos, (r: RepoItem) => {
              RepoItemView({
                repo: r
              }).onClick(() => {
                this.pathStack.clear(true);
                this.pathStack.replacePathByName("RepoDetail", r.id, true)
              })

            }, (r: RepoItem) => JSON.stringify(r))
          }
          .alignRules({
            bottom: { anchor: "__container__", align: VerticalAlign.Bottom },
          })
          .width('100%')
          .height('100%')
          .scrollBar(BarState.Off)
          .edgeEffect(EdgeEffect.Spring, { alwaysEnabled: true })
          .bindSheet(this.showCreateRepo, this.createRepoSheet, {
            showClose: true,
            detents: [SheetSize.LARGE, SheetSize.MEDIUM],
            dragBar: true,
            blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
            title: {
              title: "创建仓库",
            },
            onDisappear: () => {
              this.showCreateRepo = false;
            },
            backgroundColor: 125831006,
            preferType: SheetType.CENTER,
          })

          PopupLoading({ visible: this.isLoading, modalSize: 160, message: this.loadingMessage })

        }
        .bindSheet(this.showAboutSheet, this.aboutSheet, {
          showClose: true,
          detents: [SheetSize.LARGE],
          dragBar: true,
          blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
          title: {
            title: "关于",
          },
          onDisappear: () => {
            this.showAboutSheet = false;
          },
          backgroundColor: 125831006,
          preferType: SheetType.CENTER,
        })
      }
      .navBarWidth(360)
      .hideTitleBar(false)
      .title("HiGit")
      .hideBackButton(true)
      .titleMode(NavigationTitleMode.Free)
      .mode(NavigationMode.Auto)
      .menus(this.menus)
      .onNavigationModeChange((_) => {
      })
      .bindSheet(this.showHelpSheet, this.helpSheet, {
        showClose: true,
        detents: [SheetSize.LARGE, SheetSize.MEDIUM],
        dragBar: true,
        blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
        title: {
          title: "欢迎使用HiGit",
        },
        onDisappear: () => {
          this.showHelpSheet = false;
        },
        backgroundColor: 125831006,
        preferType: SheetType.CENTER,
      })

    }
    .width('100%')
    .height('100%')
    .backgroundColor($r('sys.color.background_primary'))
    .bindSheet(this.showSettingSheet, this.settingSheet, {
      showClose: true,
      detents: [SheetSize.LARGE, SheetSize.MEDIUM],
      dragBar: true,
      blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
      title: {
        title: "设置",
      },
      onDisappear: () => {
        this.showSettingSheet = false;
      },
      backgroundColor: 125831006,
      preferType: SheetType.CENTER,
    })
  }
}
