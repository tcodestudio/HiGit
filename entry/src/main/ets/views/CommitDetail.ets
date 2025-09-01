import { CommitItem } from '../data/Commit'
import { formatTimestamp } from "../utils/Utils";

@Component
export struct CommitDetail {
  @Require @Prop commit: CommitItem;

  private getAvatarChar(): string {
    const author: string = this.commit.author ?? '';
    return author.length > 0 ? author.substring(0, 1).toUpperCase() : 'C';
  }

  build() {
    Column() {
      // 顶部标题与元信息
      Column() {
        Text(this.commit.shortMessage)
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .fontColor('#0F172A')
          .alignSelf(ItemAlign.Start)
          .width('100%')
          .maxLines(1)
          .textOverflow({ overflow: TextOverflow.Ellipsis })

        // 作者 / 时间 / 哈希胶囊
        Row({ space: 10 }) {
          Row() {
            Text(this.getAvatarChar())
              .fontSize(12)
              .fontWeight(FontWeight.Medium)
              .fontColor('#0B1220')
          }
          .width(28)
          .height(28)
          .borderRadius(14)
          .backgroundColor('#93C5FD')
          .justifyContent(FlexAlign.Center)
          .alignItems(VerticalAlign.Center)

          Column() {
            Text(this.commit.author ?? '—')
              .fontSize(12)
              .fontColor('#475569')
              .alignSelf(ItemAlign.Start)

            Text(formatTimestamp(this.commit.timestamp))
              .fontSize(12)
              .fontColor('#64748B')
              .alignSelf(ItemAlign.Start)
          }
          .alignItems(HorizontalAlign.Start)
          .justifyContent(FlexAlign.Center)

          Blank().layoutWeight(1)

          // 哈希徽章
          Row() {
            Text((this.commit.shortId ?? '').toUpperCase())
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
        }
        .margin({ top: 12 })
        .width('100%')

        // 分隔线
        Row() {
        }
        .height(1)
        .backgroundColor('#0000000A')
        .width('100%')
      }
      .padding(16)
      .backgroundColor($r('app.color.white'))
      .border({ width: 1, color: '#0000000D' })
      .borderRadius(16)
      .shadow({ radius: 2, color: $r('sys.color.gray_02') })
      .width('100%')

      // 提交说明
      Column() {
        Text('提交说明')
          .fontSize(12)
          .fontColor('#64748B')
          .alignSelf(ItemAlign.Start)
          .width('100%')

        Text(this.commit.message)
          .fontSize(14)
          .fontColor('#0F172A')
          .margin({ top: 8 })
          .alignSelf(ItemAlign.Start)
          .width('100%')
          .textAlign(TextAlign.Start)
      }
      .padding(16)
      .backgroundColor($r('app.color.white'))
      .border({ width: 1, color: '#0000000D' })
      .borderRadius(16)
      .shadow({ radius: 2, color: $r('sys.color.gray_02') })
      .margin({ top: 12 })
      .width('100%')
    }
    .width('100%')
    .padding({
      left: 12,
      right: 12,
      top: 12,
      bottom: 18
    })
    .backgroundColor('#F8FAFC')
    .alignItems(HorizontalAlign.Start)
  }
}