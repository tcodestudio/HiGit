import { LengthUnit, PromptAction } from "@kit.ArkUI"
import { taskpool } from "@kit.ArkTS";
import { generateSSHKey, getSSHKey } from "../services/GitService";
import { pasteboard } from '@kit.BasicServicesKit';
import { Result } from "../data/Result";

@Component
export struct SettingsView {
  private uiContext: UIContext = this.getUIContext();
  private promptAction: PromptAction = this.uiContext.getPromptAction();
  @State sshKey: string = "";

  aboutToAppear() {
    taskpool.execute(getSSHKey).then((key) => {
      this.sshKey = key.toString();
    });
  }

  build() {
    Scroll() {
      Column() {
        Text("SSH 公钥")
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor($r('sys.color.font_primary'))
          .margin({ top: 16 })
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("用于通过 SSH 访问私有或受限仓库。复制公钥并添加到托管平台(如 GitHub、GitCode) 的 SSH Keys。\n")
          Span("如需更换，可点击下方“重新生成”按钮，并在托管平台更新公钥。")
        }
        .lineSpacing({ value: 6, unit: LengthUnit.VP })
        .fontSize(14)
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Column() {
          Text(this.sshKey)
            .fontSize(13)
            .fontFamily("monospace")
            .lineSpacing({ value: 4, unit: LengthUnit.VP })
            .textAlign(TextAlign.Start)
            .textOverflow({ overflow: TextOverflow.Clip })
            .fontColor($r('sys.color.font_primary'))
        }
        .width('100%')
        .padding({
          top: 12,
          bottom: 12,
          left: 12,
          right: 12
        })
        .backgroundColor($r('sys.color.comp_background_tertiary'))
        .borderRadius(12)
        .margin({ top: 12 })

        Row() {
          Button("复制公钥")
            .layoutWeight(1)
            .type(ButtonType.Capsule)
            .fontSize(14)
            .height(40)
            .onClick(async () => {
              let pasteData = pasteboard.createData(pasteboard.MIMETYPE_TEXT_PLAIN, this.sshKey);
              let systemPasteboard = pasteboard.getSystemPasteboard();
              await systemPasteboard.setData(pasteData);
              this.promptAction.showToast({
                message: '复制成功',
              })
            })
            .margin({ right: 8 })

          Button("重新生成")
            .layoutWeight(1)
            .type(ButtonType.Capsule)
            .fontSize(14)
            .height(40)
            .onClick(() => {
              taskpool.execute(generateSSHKey).then((result) => {
                const data = result as Result;
                if (data.success) {
                  this.sshKey = data.data;
                  this.promptAction.showToast({
                    message: '生成成功',
                  })
                } else {
                  this.promptAction.showToast({
                    message: '生成失败',
                  })
                }
              })
            })
            .margin({ left: 8 })
        }
        .margin({ top: 16 })

        Blank()
      }
      .height('100%')
      .width('100%')
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


