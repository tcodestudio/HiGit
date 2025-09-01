@Component
export struct FileDetail {
  @Require @Prop fileContent: string;

  build() {
    Column() {
      Scroll() {
        Column() {
          Text(this.fileContent)
            .fontSize(14)
            .fontColor('#374151')
            .fontFamily('monospace')
            .textAlign(TextAlign.Start)
            .width('100%')
            .padding(20)
            .align(Alignment.TopStart)
            .textSelectable(TextSelectableMode.SELECTABLE_FOCUSABLE)
            .draggable(true)
            .copyOption(CopyOptions.LocalDevice)
        }
        .width('100%')
      }
      .width('100%')
      .layoutWeight(1)
      .backgroundColor('#f9fafb')
      .scrollBar(BarState.Auto)
    }
    .width('100%')
    .height('100%')
    .backgroundColor($r('sys.color.comp_background_primary'))
    .borderRadius({ topLeft: 12, topRight: 12 })
  }
}