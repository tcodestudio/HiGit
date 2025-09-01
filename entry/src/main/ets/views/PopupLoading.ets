@Component
struct WaveBar {
  @Prop waveHeight: number;
  @Prop color: string;
  @Prop delay: number = 0;
  @State private scaleY: number = 0.6;
  @State private alpha: number = 0.6;

  build() {
    Row() {}
    .width(6)
    .height(this.waveHeight)
    .borderRadius(3)
    .backgroundColor(this.color)
    .scale({ x: 1, y: this.scaleY })
    .opacity(this.alpha)
    .animation({ duration: 600, curve: Curve.EaseInOut, iterations: -1, playMode: PlayMode.Alternate, delay: this.delay })
    .onAppear(() => {
      // 触发一次状态变化，进入无限交替
      this.scaleY = 1.0;
      this.alpha = 1.0;
    })
  }
}


@Component
export struct PopupLoading {
  @Prop visible: boolean = false;
  @Prop modalSize: number = 140;
  @Prop message: string = '加载中…';

  @State private popupOpacity: number = 0;
  @State private popupScale: number = 0.96;

  aboutToAppear() {
    this.popupOpacity = 1;
    this.popupScale = 1;
  }

  aboutToDisappear() {
    this.popupOpacity = 0;
    this.popupScale = 0.96;
  }


  build() {
    Stack() {
      // 背景遮罩
      Row() {}
        .width('100%')
        .height('100%')
        .backgroundColor('#00000066')

      // 弹窗主体
      Column({ space: 12 }) {
        // 波浪条形动画（无中心 icon）
        Row({ space: 8 }) {
          // 错峰交替的条形动画（无定时器）
          WaveBar({ waveHeight: this.modalSize * 0.22, color: '#E5E7EB', delay: 0 })
          WaveBar({ waveHeight: this.modalSize * 0.30, color: '#D1D5DB', delay: 100 })
          WaveBar({ waveHeight: this.modalSize * 0.40, color: '#E5E7EB', delay: 200 })
          WaveBar({ waveHeight: this.modalSize * 0.30, color: '#D1D5DB', delay: 300 })
          WaveBar({ waveHeight: this.modalSize * 0.22, color: '#E5E7EB', delay: 400 })
        }
        .justifyContent(FlexAlign.Center)
        .alignItems(VerticalAlign.Bottom)

        // 说明文案（可选）
        if (this.message && this.message.length > 0) {
          Text(this.message)
            .fontSize(14)
            .fontColor('#F3F4F6')
            .opacity(0.9)
        }
      }
        .padding({ top: 18, bottom: 18, left: 20, right: 20 })
        .width(this.modalSize)
        .backgroundColor('#2F3136')
        .borderRadius(16)
        .shadow({ radius: 24, color: '#00000055', offsetX: 0, offsetY: 10 })
        .border({ width: 1, color: '#FFFFFF12' })
        .opacity(this.popupOpacity)
        .scale({ x: this.popupScale, y: this.popupScale })
        .animation({ duration: 220, curve: Curve.EaseOut })

    }
      .width('100%')
      .height('100%')
      .alignContent(Alignment.Center)
      .opacity(this.visible ? 1 : 0)
      .visibility(this.visible ? Visibility.Visible : Visibility.Hidden)
  }
}


