import {
  TreeController,
  TreeListener,
  TreeListenerManager,
  TreeListenType,
  NodeParam,
  TreeView,
  CallbackParam,
  PromptAction,
} from '@kit.ArkUI';
import { RepoItem } from '../data/RepoItem';
import { common } from '@kit.AbilityKit';
import { getRepoById } from '../services/AppService';
import { hilog } from '@kit.PerformanceAnalysisKit';
import { taskpool } from '@kit.ArkTS';
import { getFileTree, readFile } from '../services/GitService';
import { Result } from "../data/Result";
import { PopupLoading } from '../views/PopupLoading';
import { FileInfo } from '../data/File';
import { formatFileSize } from '../utils/Utils'
import { FileDetail } from '../views/FileDetail';

interface ParamData {
  id: string;
  branch: string;
}

@Builder
export function FilesBuilder() {
  Files();
}

@Component
export struct Files {
  private uiContext: UIContext = this.getUIContext();
  private context = this.uiContext.getHostContext() as common.UIAbilityContext;
  private promptAction: PromptAction = this.uiContext.getPromptAction();
  @State repo: RepoItem | null = null;
  @State selectedBranch: string = 'main';
  pathStack: NavPathStack = new NavPathStack();
  private treeController: TreeController = new TreeController();
  private treeListener: TreeListener = TreeListenerManager.getInstance().getTreeListener();
  @State selectedNodeId: number = -1;
  @State selectedFile: FileInfo | null = null;
  @State showFileModal: boolean = false;
  @State fileContent: string = "";
  @State isLoading: boolean = false;
  @State loadingMessage: string = "加载中...";
  private fileData: FileInfo[] = [];
  // 防误触：滚动手势期间抑制点击
  private suppressClickUntil: number = 0;
  private touchStartY: number = 0;
  private didMoveDuringTouch: boolean = false;

  aboutToDisappear(): void {
    this.treeListener.off(TreeListenType.NODE_CLICK, undefined);
  }

  ready() {
    this.isLoading = true;
    taskpool.execute(getFileTree, this.repo!.url, this.selectedBranch).then((data) => {
      let result = data as Result;
      if (result.success) {
        try {
          const parsed = JSON.parse(result.data) as FileInfo[];
          const getDepth =
            (fileId: number, fileMap: Map<number, FileInfo>, depthCache: Map<number, number>): number => {
              if (depthCache.has(fileId)) {
                return depthCache.get(fileId)!;
              }

              const file = fileMap.get(fileId);
              if (!file || file.parentId === -1 || file.parentId === 0) {
                depthCache.set(fileId, 0);
                return 0;
              }

              const depth = 1 + getDepth(file.parentId, fileMap, depthCache);
              depthCache.set(fileId, depth);
              return depth;
            };

          const fileMap = new Map<number, FileInfo>();
          parsed.forEach(file => fileMap.set(file.id, file));
          const depthCache = new Map<number, number>();

          // 2. 排序
          parsed.sort((a, b) => {
            const depthA = getDepth(a.id, fileMap, depthCache);
            const depthB = getDepth(b.id, fileMap, depthCache);

            // 首先按层级深度排序（父级在前）
            if (depthA !== depthB) {
              return depthA - depthB;
            }

            // 同层级且同父目录下，目录优先于文件
            if (a.parentId === b.parentId) {
              if (a.isDirectory !== b.isDirectory) {
                return a.isDirectory ? -1 : 1; // 目录在前
              }
              // 同类型按名称排序
              return a.name.localeCompare(b.name);
            }

            // 不同父目录，按路径排序
            return a.path.localeCompare(b.path);
          });

          this.fileData = parsed;
          this.buildFileTree();
        } catch (e) {
          hilog.error(0x0000, "appTag", `parse error %{public}s`, `${e}`);
          this.promptAction.showToast({
            message: '解析文件列表失败'
          });
        }
      } else {
        this.promptAction.showToast({
          message: result.message
        })
      }
      this.isLoading = false;
    })
  }

  aboutToAppear(): void {
    this.treeListener.on(TreeListenType.NODE_CLICK, (callbackParam: CallbackParam) => {
      if (Date.now() < this.suppressClickUntil) {
        return;
      }
      this.selectedNodeId = callbackParam.currentNodeId;
      const fileInfo = this.fileData.find(f => f.id === callbackParam.currentNodeId);
      if (fileInfo && !fileInfo.isDirectory) {
        this.isLoading = true;
        this.selectedFile = fileInfo;
        taskpool.execute(readFile, this.repo!.url, this.selectedBranch, fileInfo.path).then((data) => {
          let result = data as Result;
          if (result.success) {
            try {
              this.fileContent = result.data;
              this.showFileModal = true;
            } catch (e) {
              this.promptAction.showToast({
                message: '获取文件失败'
              });
            }
          } else {
            this.promptAction.showToast({
              message: result.message
            })
          }
          this.isLoading = false;
        });
      }
    });
  }

  // 构建文件树
  private buildFileTree() {
    this.fileData.forEach(file => {
      const nodeParam: NodeParam = {
        parentNodeId: file.parentId,
        currentNodeId: file.id,
        isFolder: file.isDirectory,
        primaryTitle: file.name,
        icon: file.isDirectory ? $r('app.media.folder') : $r('app.media.doc'),
        selectedIcon: file.isDirectory ? $r('app.media.folder') : $r('app.media.doc')
      };
      this.treeController.addNode(nodeParam);
    });

    this.treeController.buildDone();
  }

  @Builder
  fileModalContent() {
    FileDetail({
      fileContent: this.fileContent,
    })
  }

  initializeFromParams(context: NavDestinationContext): void {
    this.pathStack = context.pathStack;
    const raw = this.pathStack.getParamByName('Files') as string[];
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
      RelativeContainer() {

        Column() {
          TreeView({ treeController: this.treeController })
            .width('100%')
            .layoutWeight(1)
        }
        .width('100%')
        .height('100%')
      }
      .onTouch((event: TouchEvent) => {
        if (!event || !event.touches || event.touches.length === 0) {
          return;
        }
        const y = event.touches[0].y;
        switch (event.type) {
          case TouchType.Down:
            this.touchStartY = y;
            this.didMoveDuringTouch = false;
            break;
          case TouchType.Move:
            if (Math.abs(y - this.touchStartY) > 8) { // 阈值像素，按需调整
              this.didMoveDuringTouch = true;
              this.suppressClickUntil = Date.now() + 180; // 抑制窗口 ms
            }
            break;
          case TouchType.Up:
          case TouchType.Cancel:
            if (this.didMoveDuringTouch) {
              this.suppressClickUntil = Date.now() + 180;
            }
            break;
        }
      })

      PopupLoading({ visible: this.isLoading, modalSize: 160, message: this.loadingMessage })

    }
    .title('仓库浏览器')
    .backgroundColor($r('sys.color.comp_background_primary'))
    .bindSheet(this.showFileModal, this.fileModalContent, {
      showClose: true,
      detents: [SheetSize.LARGE, SheetSize.MEDIUM],
      dragBar: true,
      blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
      title: {
        title: this.selectedFile?.name || '文件预览',
        subtitle: `${this.selectedFile?.path || ''}` + " " + formatFileSize(this.selectedFile?.size)
      },
      onDisappear: () => {
        this.showFileModal = false;
        this.selectedFile = null;
      },
      backgroundColor: $r('sys.color.comp_background_primary'),
      preferType: SheetType.CENTER
    })
    .onReady((context: NavDestinationContext) => {
      this.initializeFromParams(context);
    })
  }
}