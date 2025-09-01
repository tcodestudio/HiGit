import { CommitItem } from '../data/Commit'
import { formatTimestamp } from "../utils/Utils";
import { CommitDetail } from "./CommitDetail";

@Component
export struct CommitItemView {
  @Require @Prop commit: CommitItem;
  @State private isPressed: boolean = false;
  @State private appearOpacity: number = 0;
  @State showCommitDetail: boolean = false;

  @Builder
  showCommitDetailView() {
    CommitDetail({
      commit: this.commit,
    })
  }

  aboutToAppear() {
    this.appearOpacity = 1;
  }

  private getAvatarChar(): string {
    const author: string = this.commit.author ?? '';
    return author.length > 0 ? author.substring(0, 1).toUpperCase() : 'C';
  }

  build() {
    // 头像 + 文本信息 + 哈希徽章
    Row() {
      // 左侧：头像圆标 + 文本信息
      Row() {
        Row() {
          Text(this.getAvatarChar())
            .fontSize(12)
            .fontWeight(FontWeight.Medium)
            .fontColor('#0F172A')
        }
        .width(28)
        .height(28)
        .justifyContent(FlexAlign.Center)
        .alignItems(VerticalAlign.Center)
        .borderRadius(14)
        .backgroundColor('#E2E8F0')

        Column() {
          // 标题
          Text(this.commit.shortMessage)
            .fontSize(15)
            .fontWeight(FontWeight.Medium)
            .fontColor('#0F172A')
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })

          // 次级信息：作者 · 时间
          Row() {
            Text(this.commit.author)
              .fontSize(12)
              .fontColor('#475569')

            Text('·')
              .fontSize(12)
              .fontColor('#94A3B8')
              .padding({ left: 6, right: 6 })

            Text(formatTimestamp(this.commit.timestamp))
              .fontSize(12)
              .fontColor('#64748B')
          }
          .margin({ top: 6 })
          .alignItems(VerticalAlign.Center)
        }
        .alignItems(HorizontalAlign.Start)
        .margin({ left: 10 })
        .layoutWeight(1)
      }
      .alignItems(VerticalAlign.Center)
      .layoutWeight(1)

      Blank().width(8)

      Row() {
        Text(this.commit.shortId.toUpperCase())
          .fontSize(11)
          .fontWeight(FontWeight.Medium)
          .fontColor('#1E293B')
          .fontFamily('monospace')
      }
      .padding({
        left: 10,
        right: 10,
        top: 5,
        bottom: 5
      })
      .borderRadius(10)
      .backgroundColor('#F1F5F9')
      .border({ width: 1, color: '#00000010' })
      .flexShrink(0)
    }
    .animation({ duration: 200, curve: Curve.EaseOut })
    .opacity(this.appearOpacity)
    .shadow({ radius: 2, color: $r('sys.color.gray_02') })
    .scale({ x: this.isPressed ? 0.98 : 1, y: this.isPressed ? 0.98 : 1 })
    .width('100%')
    .padding({
      left: 12,
      right: 12,
      top: 10,
      bottom: 10
    })
    .hoverEffect(HoverEffect.Highlight)
    .accessibilityDescription(`提交 ${this.commit.shortMessage}，作者 ${this.commit.author}，时间 ${this.commit.timestamp.toString()}`)
    .onTouch((event) => {
      if (event.type === TouchType.Down) {
        this.isPressed = true;
      } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.isPressed = false;
      }
    })
    .onClick(() => {
      this.showCommitDetail = true;
    })
    .bindSheet(this.showCommitDetail, this.showCommitDetailView, {
      showClose: true,
      detents: [SheetSize.LARGE, SheetSize.MEDIUM],
      dragBar: true,
      blurStyle: BlurStyle.COMPONENT_ULTRA_THICK,
      title: {
        title: "Commit详情",
      },
      onDisappear: () => {
        this.showCommitDetail = false;
      },
      backgroundColor: 125831006,
      preferType: SheetType.CENTER,
    })
  }
}