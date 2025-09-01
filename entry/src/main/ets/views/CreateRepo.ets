import { LengthUnit, PromptAction } from "@kit.ArkUI"
import { common } from '@kit.AbilityKit'
import { addRepo, getRepoByUrl } from "../services/AppService";
import { validateGitUrl } from "../utils/Utils";

@Component
export struct CreateRepo {
  private uiContext: UIContext = this.getUIContext();
  private context = this.uiContext.getHostContext() as common.UIAbilityContext;
  private promptAction: PromptAction = this.uiContext.getPromptAction();
  @State repoUrl: string = ""

  addRepo() {
    // 检查 repoUrl 是否为空
    if (!this.repoUrl || this.repoUrl.trim() === "") {
      this.promptAction.showToast({
        message: "请输入仓库地址",
        duration: 2000
      });
      return;
    }

    const trimmedUrl = this.repoUrl.trim();

    // 检查是否是有效的 Git 仓库地址格式
    const isValidGitUrl = validateGitUrl(trimmedUrl);

    if (!isValidGitUrl) {
      this.promptAction.showToast({
        message: "请输入有效的 Git 仓库地址\n支持格式：\n• https://github.com/owner/repo.git\n• git@github.com:owner/repo.git",
        duration: 3000
      });
      return;
    }

    const repo = getRepoByUrl(this.context, trimmedUrl);
    if (repo) {
      this.promptAction.showToast({
        message: "仓库已存在，请勿重复添加",
        duration: 3000
      });
      return;
    }

    addRepo(this.context, trimmedUrl)

    this.promptAction.showToast({
      message: "添加成功",
    })

    this.context.eventHub.emit("add_repo_success")

  }

  build() {
    Scroll() {
      Column() {
        Text("请输入远程 Git 仓库地址")
          .fontSize(14)
          .fontColor($r('sys.color.font_secondary'))
          .margin({ top: 6 })
          .alignSelf(ItemAlign.Start)

        Column() {
          TextInput({
            placeholder: "例如: https(git)://github.com/owner/repo.git",
            text: this.repoUrl
          })
            .onChange((v: string) => {
              this.repoUrl = v
            })
            .fontSize(16)
            .height(44)
        }
        .width('100%')
        .borderRadius(12)
        .margin({ top: 12 })

        Text() {
          Span("使用 SSH 前：").fontWeight(FontWeight.Bold)
          Span("请先前往 “菜单 → 设置 → SSH 公钥” 复制你的公钥，")
          Span("并到托管平台（如 GitHub/GitCode/GitLab/Gitee）账户的 SSH Keys 中添加。")
          Span("完成添加后，再使用 SSH 地址访问私有或受限仓库。")
        }
        .lineSpacing({ value: 6, unit: LengthUnit.VP })
        .fontSize(13)
        .fontColor($r('sys.color.font_secondary'))
        .margin({
          top: 16,
        })
        .alignSelf(ItemAlign.Start)

        Row() {
          Button("确认")
            .layoutWeight(1)
            .type(ButtonType.Capsule)
            .height(40)
            .onClick(() => {
              this.addRepo();
            })
            .margin({ left: 8 })
        }
        .margin({ top: 16 })

        Blank()
      }
      .width('100%')
      .height('100%')
      .padding({
        left: 16,
        right: 16,
        top: 4,
        bottom: 24
      })
    }
    .height('100%')
    .scrollBar(BarState.Off)
    .edgeEffect(EdgeEffect.Spring)
  }
}