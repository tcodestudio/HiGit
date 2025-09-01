import { bundleManager, common, OpenLinkOptions, Want } from "@kit.AbilityKit"
import { productViewManager } from '@kit.AppGalleryKit';

@Component
export struct AboutView {
  private context = this.getUIContext().getHostContext() as common.UIAbilityContext;
  @State versionName: string = '';

  aboutToAppear(): void {
    this.loadInfo()
  }

  loadInfo() {
    let bundleInfo: bundleManager.BundleInfo =
      bundleManager.getBundleInfoForSelfSync(bundleManager.BundleFlag.GET_BUNDLE_INFO_WITH_APPLICATION)
    this.versionName = bundleInfo.versionName;
  }

  build() {
    Scroll() {
      Column() {
        Image($r('app.media.icon'))
          .height(72)
          .width(72)

        Text("HiGit")
          .fontSize(22)
          .fontColor(Color.Black)
          .fontWeight(FontWeight.Bold)
          .margin({ top: 12, bottom: 24 })

        Column() {
          Row() {
            Text("版本信息")
              .fontColor(Color.Gray)
            Blank()
            Text(this.versionName)
              .fontColor(Color.Gray)
          }
          .padding(16)
          .width('100%')
          .clickEffect({
            level: ClickEffectLevel.HEAVY
          })

          Divider()
          Row() {
            Text("意见反馈")
              .fontColor(Color.Gray)
            Blank()
            Text("pp@tcodestudio.com")
              .fontColor(Color.Gray)
          }
          .padding(16)
          .width('100%')
          .clickEffect({
            level: ClickEffectLevel.HEAVY
          })

          Divider()
          Row() {
            Text("前往评分")
              .fontColor(Color.Gray)
            Blank()
            Text() {
              SymbolSpan($r('sys.symbol.arrow_right'))
                .fontWeight(FontWeight.Normal)
                .fontColor([Color.Gray])
            }
          }
          .padding(16)
          .width('100%')
          .clickEffect({
            level: ClickEffectLevel.HEAVY
          })
          .onClick(() => {
            const wantParam: Want = {
              parameters: {
                bundleName: 'com.tcodestudio.app.higit'
              }
            }
            productViewManager.loadProduct(this.context, wantParam, {});
          })

          Divider()


          Row() {
            Text("安全和隐私概况")
              .fontColor(Color.Gray)
            Blank()
            Text() {
              SymbolSpan($r('sys.symbol.arrow_right'))
                .fontWeight(FontWeight.Normal)
                .fontColor([Color.Gray])
            }
          }
          .padding(16)
          .width('100%')
          .clickEffect({
            level: ClickEffectLevel.HEAVY
          })
          .onClick(() => {
            try {
              let openLinkOptions: OpenLinkOptions = {
                appLinkingOnly: false,
              }
              this.context.openLink("https://api.tcodestudio.com/api/html/hi_git_privacy", openLinkOptions);
            } catch (e) {

            }
          })
        }
        .width('90%')
        .borderRadius(8)
        .backgroundColor(Color.White)

        Blank()

        Text("沪ICP备18003223号-9A")
          .fontColor(Color.Orange)
          .fontSize(14)
        Text("版权所有©️2023-2025 尼奥巴兰 保留一切权利")
          .fontSize(14)
          .fontColor($r('sys.color.font_secondary'))
          .margin({ bottom: 36, top: 4 })
      }
      .height('100%')
    }
  }
}