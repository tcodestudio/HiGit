import { RepoItem } from '../data/RepoItem';
import { formatTimestampLocal } from '../utils/Utils';

@Component
export struct RepoItemView {
  @Require @Prop repo: RepoItem;
  @State private isPressed: boolean = false;
  @State private appearOpacity: number = 0;
  @State private arrowOffsetX: number = 0;

  aboutToAppear() {
    // 进场采用属性动画（.animation）配合状态变化
    this.appearOpacity = 1;
  }

  getProviderIcon() {
    switch (this.repo.provider.toLowerCase()) {
      case "gitcode":
        return $r('app.media.gitcode');
      case "github":
        return $r('app.media.github');
      case "gitee":
        return $r('app.media.gitee');
      case "gitlab":
        return $r('app.media.gitlab');
      default:
        return $r('app.media.icon');
    }
  }

  build() {
    ListItem() {
      Column() {
        // 顶部：来源与仓库名同一行
        Row({ space: 10 }) {
          // provider 徽标（占位）
          Row({ space: 8 }) {
            Image(this.getProviderIcon()).width(24).height(24).borderRadius(3)
          }
          .padding({
            left: 10,
            right: 10,
            top: 6,
            bottom: 6
          })
          .borderRadius(999)
          .backgroundColor('#EEF2F6')
          .border({ width: 1, color: '#00000010' })

          // 仓库名
          Text(`${this.repo.name}`)
            .fontSize(16)
            .fontWeight(FontWeight.Medium)
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .layoutWeight(1)
            .fontColor($r('sys.color.font_primary'))

          // 进入箭头
          Image($r('app.media.arrow_right'))
            .width(20)
            .height(20)
            .opacity(0.6)
            .animation({ duration: 160, curve: Curve.EaseOut })
            .translate({ x: this.arrowOffsetX, y: 0 })
        }
        .alignItems(VerticalAlign.Center)

        // 信息：将分支胶囊与次要信息合并为单行，居中对齐并做省略
        Row({ space: 8 }) {
          // 当前分支（无图标，描边胶囊以提升对比）
          Row() {
            Text(this.repo.branch).fontSize(12).fontColor('#334155')
          }
          .padding({
            left: 8,
            right: 8,
            top: 4,
            bottom: 4
          })
          .borderRadius(999)
          .backgroundColor($r('sys.color.comp_background_tertiary'))
          .border({ width: 1, color: '#00000014' })

          // 次要信息行
          Text(`分支 ${this.repo.branches} · 标签 ${this.repo.tags} · 更新 ${this.repo.time > 0 ?
          formatTimestampLocal(this.repo.time) : "-"}`)
            .fontSize(12)
            .fontColor($r('sys.color.font_secondary'))
            .maxLines(1)
            .textOverflow({ overflow: TextOverflow.Ellipsis })
            .layoutWeight(1)
        }
        .alignItems(VerticalAlign.Center)
        .margin({ top: 8 })

      }
      .padding(16)
      .backgroundColor($r('sys.color.background_secondary'))
      .border({ width: 1, color: '#0000000D' })
      .borderRadius(16)
      .animation({ duration: 200, curve: Curve.EaseOut })
      .opacity(this.appearOpacity)
      .shadow({ radius: 2, color: $r('sys.color.gray_02') })
      .scale({ x: this.isPressed ? 0.98 : 1, y: this.isPressed ? 0.98 : 1 })
    }
    .padding({
      left: 12,
      right: 12,
      top: 6,
      bottom: 6
    })
    .hoverEffect(HoverEffect.Highlight)
    .onTouch((event) => {
      if (event.type === TouchType.Down) {
        this.isPressed = true;
        this.arrowOffsetX = 2;
      } else if (event.type === TouchType.Up || event.type === TouchType.Cancel) {
        this.isPressed = false;
        this.arrowOffsetX = 0;
      }
    })
  }
}