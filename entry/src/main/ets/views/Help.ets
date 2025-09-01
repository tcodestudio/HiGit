import { LengthUnit } from "@kit.ArkUI"

@Component
export struct HelpView {
  build() {
    Scroll() {
      Column() {
        Text("使用指南")
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .alignSelf(ItemAlign.Start)

        Text("1. 应用概览")
          .fontColor($r('sys.color.font_primary'))
          .margin({ top: 16 })
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("· 定位: ").fontWeight(FontWeight.Bold)
          Span("HiGit 是一款本地 Git 客户端，可以添加远程 Git 仓库并在本地查看仓库信息与提交历史。\n")

          Span("· 连接方式: ").fontWeight(FontWeight.Bold)
          Span("支持 HTTP/HTTPS 与 SSH。应用会自动生成并管理 SSH 密钥对；无需手动管理文件或路径。可在设置中心的 SSH 公钥页面复制或重新生成公钥。\n")

          Span("· 数据范围: ").fontWeight(FontWeight.Bold)
          Span("当前版本聚焦“查看”能力：远程分支/标签/提交历史/提交详情；不在本地检出工作区，不执行提交/推送等写操作。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Text("2. 快速开始")
          .fontColor($r('sys.color.font_primary'))
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("在首页，点击右上角")
          Span("“+”").fontWeight(FontWeight.Bold).fontColor(Color.Black)
          Span("添加远程仓库，按提示填写：\n")

          Span("· 仓库地址: ").fontWeight(FontWeight.Bold)
          Span("支持 HTTP/HTTPS(示例: https://example.com/owner/repo.git) 或 SSH(示例: git@example.com:owner/repo.git)。\n")

          Span("· 本地名称: ").fontWeight(FontWeight.Bold)
          Span("应用会在本地以裸仓库形式维护仓库元数据，并将远程设置为 origin；无需手动管理任何本地路径。\n")

          Span("初始化成功后，可获取远程分支/标签，并查看提交历史与详情。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Text("3. 远程连接方式")
          .fontColor($r('sys.color.font_primary'))
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("· HTTP/HTTPS: ").fontWeight(FontWeight.Bold)
          Span("适用于公开仓库访问。应用会自动进行证书校验，一般无需额外配置。对于私有仓库，建议优先使用 SSH 以获得更稳定的认证体验。\n")

          Span("· SSH: ").fontWeight(FontWeight.Bold)
          Span("首次运行后，应用会自动生成 RSA(2048位) 密钥对。使用 SSH 前，请前往设置中心的 SSH 公钥页面复制公钥，并添加到托管平台(如 GitHub、GitCode) 的 SSH Keys；随后使用 SSH 地址即可拉取仓库信息。\n")

          Span("· 权限与安全: ").fontWeight(FontWeight.Bold)
          Span("密钥仅保存在本机并受系统保护；如需更换，可在设置中心的 SSH 公钥页面重新生成，并将新公钥更新到托管平台。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Text("4. 仓库数据与同步")
          .fontColor($r('sys.color.font_primary'))
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("· 裸仓库: ").fontWeight(FontWeight.Bold)
          Span("每个远程仓库会以裸仓库形式在本地维护，仅包含 Git 对象与引用信息，不会在本地检出文件。\n")

          Span("· 获取数据: ").fontWeight(FontWeight.Bold)
          Span("添加成功后，应用会连接到 origin，并按需获取远程分支/标签与提交对象。必要时可在页面触发同步以更新最新提交。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Text("5. 隐私与控制")
          .fontColor($r('sys.color.font_primary'))
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("· SSH 公钥: ").fontWeight(FontWeight.Bold)
          Span("可在设置中心的 SSH 公钥页面复制或重新生成。重新生成后，请到托管平台更新公钥。\n")

          Span("· 隐私: ").fontWeight(FontWeight.Bold)
          Span("密钥与仓库数据仅存储在本机的应用沙箱中，应用不会上传至任何服务器。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)

        Text("常见问题")
          .fontColor($r('sys.color.font_primary'))
          .alignSelf(ItemAlign.Start)

        Text() {
          Span("Q：无法连接远程仓库怎么办？\n").fontWeight(FontWeight.Bold)
          Span("A：请先确认仓库地址正确且网络可达；若为私有仓库，优先使用 SSH，并确保已在“设置中心 → SSH 公钥”复制的公钥已添加到托管平台。\n\n")

          Span("Q：SSH 验证失败怎么办？\n").fontWeight(FontWeight.Bold)
          Span("A：请确认使用 SSH 地址(如 git@github.com:owner/repo.git)；确保已在“设置中心 → SSH 公钥”复制的公钥添加到 GitHub/GitCode 账户；如仍失败，可在该页面重新生成密钥并重新添加公钥。\n\n")

          Span("Q：为什么看不到分支或标签？\n").fontWeight(FontWeight.Bold)
          Span("A：请确认远程已连接并具备访问权限。必要时在页面触发同步(fetch) 后再试。\n\n")

          Span("Q：是否支持提交/推送？\n").fontWeight(FontWeight.Bold)
          Span("A：当前版本专注于浏览与检索(分支、标签、提交历史与详情)，暂不提供提交/推送功能。\n\n")

          Span("Q：HTTPS 证书相关问题？\n").fontWeight(FontWeight.Bold)
          Span("A：应用会自动进行证书校验。若处于公司代理/自签环境导致访问失败，建议改用 SSH。\n")
        }
        .lineSpacing({
          value: 8,
          unit: LengthUnit.VP,
        })
        .fontColor($r('sys.color.font_secondary'))
        .margin({ top: 8 })
        .alignSelf(ItemAlign.Start)
      }
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