import { common, OpenLinkOptions } from '@kit.AbilityKit'
import { router } from '@kit.ArkUI'
import { hilog } from '@kit.PerformanceAnalysisKit';
import { preferences } from '@kit.ArkData';

@Entry
@Component
struct Safe {
  private context = this.getUIContext().getHostContext() as common.UIAbilityContext;

  build() {
    Column() {
      Image($r('app.media.icon'))
        .height(72)
        .width(72)
        .margin({ top: 96 })

      Text("欢迎使用")
        .fontSize(14)
        .fontColor($r('sys.color.ohos_id_color_text_secondary'))
        .fontWeight(FontWeight.Bold)
        .margin({ top: 24, bottom: 4 })

      Text("HiGit")
        .fontSize(32)
        .fontColor($r('sys.color.ohos_id_color_text_primary'))
        .fontWeight(FontWeight.Bold)

      Blank()

      Image($r('app.media.ic_safe'))
        .width(24)
        .height(24)

      Text() {
        Span("本应用仅需")
        Span("联网").fontWeight(FontWeight.Bold)
        Span("获取基本的应用运行日志以改善用户使用体验。")
        Span("有关隐私的详细可参阅关于")
        Span(" 《HiGit隐私政策》 ").fontColor(Color.Blue)
          .onClick(() => {
            try {
              let openLinkOptions: OpenLinkOptions = {
                appLinkingOnly: false,
              }
              this.context.openLink("https://api.tcodestudio.com/api/html/hi_git_privacy", openLinkOptions);
            } catch (e) {
              hilog.info(0x0000, 'appTag', '%{public}s', 'Ability::onWindowStageCreate');
            }
          })
        Span("。")
        Span("\n\n")
        Span("请仔细阅读以上声明。点击\"同意\"，既代表您同意HiGit用户协议。")
      }
      .fontSize(14)
      .padding({
        left: 24,
        right: 24,
        top: 12,
        bottom: 24
      })
      .fontColor($r('sys.color.ohos_id_color_text_secondary'))

      Row() {
        Button("取消")
          .layoutWeight(1)
          .backgroundColor(0x0D000000)
          .fontColor(0x0A59F0)
          .onClick(() => {
            let context: common.UIAbilityContext = getContext(this) as common.UIAbilityContext;
            context.terminateSelf();
          })
          .margin({ left: 16, right: 12 })
        Button("同意")
          .fontColor(Color.White)
          .layoutWeight(1)
          .onClick(() => {
            let dataPreferences = preferences.getPreferencesSync(this.context, {
              name: "App"
            });
            dataPreferences.putSync("agree_app", true);
            dataPreferences.flush();
            router.replaceUrl({ url: 'pages/Index' })
          })
          .margin({ right: 16, left: 12 })
      }
      .justifyContent(FlexAlign.SpaceBetween)
      .margin({ bottom: 32 })

    }
    .width('100%')
    .height('100%')
  }
}