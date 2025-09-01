## Loading 组件 - OrbitLoading

特点：
- 三点环绕轨道旋转，中心放置应用图标，科幻感强，且不刺眼。
- 支持尺寸、颜色、文案与全屏遮罩模式。

使用：
```ts
import { OrbitLoading } from '../../entry/src/main/ets/views/OrbitLoading';

@Component
struct Demo {
  build() {
    Column() {
      // 卡片内使用
      OrbitLoading({ size: 56, color: '#6366F1', text: '加载中...' })

      // 全屏遮罩使用
      // OrbitLoading({ fullScreen: true, text: '同步仓库中...' })
    }
  }
}
```

属性：
- `size: number` 组件尺寸，默认 64。
- `color: string` 主题色，默认 `#6366F1`。
- `text: string` 说明文案，可选。
- `fullScreen: boolean` 是否全屏遮罩，默认 false。

无外部依赖，直接放置于 `views` 目录即可复用。

## Loading 组件 - PopupLoading（弹窗灰块 + 高级动画）

特点：
- 灰色半透明弹窗，圆角与投影，弱打扰、信息密度低。
- 无中心 icon，改为条形波浪起伏，循环播放，更现代。
- 带全屏遮罩与弹出过渡（淡入 + 轻缩放），仅 1 个 `visible` 开关即可控制。

使用：
```ts
import { PopupLoading } from '../../entry/src/main/ets/views/PopupLoading';

@Component
struct Demo {
  @State show: boolean = true
  build() {
    Stack() {
      // 页面内容...

      PopupLoading({ visible: this.show, modalSize: 160, message: '正在同步…' })
    }
    .width('100%').height('100%')
  }
}
```

属性：
- `visible: boolean` 是否可见。
- `modalSize: number` 弹窗宽度（像素），默认 140。
- `message: string` 文案，可选。

说明：
- 波浪条形使用 `animation` 无限交替（PlayMode.Alternate），增强呼吸感。
- 弹窗采用短促的 EaseOut 过渡，进入更灵动，退出更干净。
## ArkTS UI 开发速查与实践指南（HarmonyOS）

本文为 HarmonyOS ArkTS/ArkUI 的系统化速查与最佳实践手册，覆盖从概念到落地的全链路：工程/语法、布局与组件、渲染控制、状态与数据流、列表与导航、手势与动画、绘制与样式、资源与多语言、性能与可访问性、版本兼容与常见坑位。适合移动端优先的实际项目快速落地与团队协作。

---

### 目录
- 目的与范围
- 名词与版本适配
- 快速开始（最小可运行示例）
- 核心语法与装饰器
- 布局与度量体系
- 组件与列表
- 渲染控制与可见性
- 状态管理与数据流
- 导航与路由（新旧范式）
- 手势与交互
- 动画
- 绘制（Canvas）
- 样式、主题与资源
- 多语言与区域
- 数据与存储
- 性能优化指南
- 可访问性（a11y）
- 工程规范与实践清单
- 编译器与常见坑（含错误号）
- 版本兼容与迁移建议
- 参考链接

---

### 目的与范围
- 面向手机/平板的移动端优先设计与实现，兼顾高性能与可维护性。
- 为团队提供统一的 ArkTS/ArkUI 编码规范与落地范式，降低踩坑成本。

### 名词与版本适配
- **ArkTS**：TypeScript 的语言扩展，配套 ArkUI 声明式 UI 能力。
- **ArkUI**：HarmonyOS 官方 UI 框架，提供组件、布局、动画、手势、主题与资源管理。
- **版本差异**：不同 SDK 对装饰器、组件构造、样式 API 有差异。本文所有示例以更通用/兼容的写法给出；遇到不匹配以官方当前 SDK 文档为准。

### 快速开始（最小可运行示例）
```ts
@Entry
@Component
struct HelloPage {
  @State message: string = 'Hello ArkTS'

  build() {
    Column() {
      Text(this.message).fontSize(20).padding(12)
      Button() { Text('点我') }.onClick(() => this.message = 'Updated!')
    }
    .width('100%').height('100%')
    .justifyContent(FlexAlign.Center)
    .alignItems(HorizontalAlign.Center)
  }
}
```

### 核心语法与装饰器
- **@Entry / @Component**：入口页面与通用组件；`@Entry.build()` 必须且只能返回一个根容器。
- **@State**：本地可变状态；变更触发 UI 重组。
- **@Prop / @Link**：父传子只读与双向绑定参数。
- **@Provide / @Consume**：跨层级依赖注入式共享。
- **@Observed / @ObjectLink**：类实例响应式；适合复杂领域模型。
- **@StorageProp / @StorageLink / AppStorage / PersistentStorage**：应用级状态与持久化键值对驱动 UI。
- **@Watch('key')**：状态变更副作用统一处理。
- 建议：避免在字段初始器访问上下文/路由；初始化放入 `aboutToAppear()` 或导航钩子。

### 布局与度量体系
- **容器**：`Row`/`Column`（线性）、`Stack`（层叠）、`Flex`（弹性）、`RelativeContainer`（相对）。
- **网格**：`GridRow`/`GridCol`（12 栅格）、`Grid`/`GridItem`（模板网格）。
- **滚动与分页**：`Scroll`、`List`、`Swiper`。
- **单位**：推荐 `vp`（视口像素）、`fp`（字体像素）；支持 `%` 百分比。
- **常用属性**：`width/height/margin/padding/justifyContent/alignItems/visibility/position` 等。

```ts
Column({ space: 8 }) {
  Row().height(40).backgroundColor('#f5f5f5')
  Row().height(40).backgroundColor('#eaeaea')
}
.padding({ left: 16, right: 16 })
```

#### Row / Column 关键示例
```ts
Row({ space: 8 }) {
  Text('A').padding(8).backgroundColor('#EEE')
  Text('B').padding(8).backgroundColor('#DDD').alignSelf(Alignment.Start)
  Text('C').padding(8).backgroundColor('#CCC')
}
.justifyContent(FlexAlign.SpaceBetween)
.alignItems(VerticalAlign.Center)
```

#### Flex（弹性布局）
- `flexGrow/flexShrink/flexBasis` 控制剩余空间分配与压缩。
```ts
Flex() {
  Text('1').flexGrow(1).backgroundColor('#FDE68A')
  Text('2').flexGrow(2).backgroundColor('#86EFAC')
  Text('1').flexGrow(1).backgroundColor('#93C5FD')
}
.height(48)
```

#### Stack（层叠布局）
```ts
Stack() {
  Image($r('app.media.ic_launcher_foreground')).width('100%').height(160)
  Text('Overlay').align(Alignment.Center).fontSize(18).fontWeight(FontWeight.Medium)
}
```

#### RelativeContainer（相对布局）
```ts
RelativeContainer() {
  Text('左上').alignRules({ top: true, start: true })
  Text('右下').alignRules({ bottom: true, end: true })
}
```

#### Grid 系列
- 12 栅格：
```ts
GridRow({ columns: 12, gutter: 12 }) {
  GridCol({ span: 4 }) { Text('1/3') }
  GridCol({ span: 8 }) { Text('2/3') }
}
```
- 模板网格：
```ts
Grid()
  .columnsTemplate('1fr 1fr 1fr')
  .rowsTemplate('auto')
  .columnsGap(8)
  .rowsGap(8) {
  GridItem() { Text('A') }
  GridItem() { Text('B') }
  GridItem() { Text('C') }
}
```

#### Scroll / List / Swiper 要点
- `Scroll` 用于自定义滚动区域。
```ts
Scroll() {
  Column({ space: 8 }) {
    // ...
  }
}
```
- `List` 长列表与 `LazyForEach` 配合，`.cachedCount(n)` 控制缓存窗口。
- `Swiper` 轮播：`index/loop/autoPlay/interval/indicator`。

### 组件与列表
- **基础组件**：`Text`、`Image`、`Button`、`TextInput`、`Divider`、`Toggle`、`Slider`、`Tabs`。
- **列表渲染**：小数据 `ForEach`，大数据 `LazyForEach`。
```ts
@Entry
@Component
struct ListDemo {
  private items: string[] = Array.from({ length: 1000 }, (_, i) => `Item ${i}`)

  build() {
    List() {
      LazyForEach(this.items, (text: string) => {
        ListItem() { Text(text).padding(12) }
      })
    }
    .cachedCount(10)
  }
}
```

### 渲染控制与可见性
- **条件渲染**：常规 TS 条件选择 UI 分支。
- **循环**：`ForEach` 与 `LazyForEach`；提供稳定 key 函数（如 `item.id`）。
- **可见性**：`visibility(Visibility.Visible|Hidden|None)` 控制占位行为。
- **空/加载/错误**：抽象成复用组件，统一交互与视觉。

#### UI 用法建议（通用兼容写法）
- **Button**：优先内容槽：
```ts
Button() { Text('操作') }.onClick(this.onSubmit)
```
- **Text**：文字样式直接链式到 `Text(...)`。
- **TextInput**：`@State` + `.onChange(...)` 双向联动；复杂表单拆分子组件。
- **居中标题**：用 `Blank().layoutWeight(1)` 两侧占位，避免 `ItemAlign/Alignment` 不匹配。

### 状态管理与数据流
- 层级：`@State`（组件内） → `@Prop/@Link`（父子） → `@Provide/@Consume`（跨层） → `@Observed/@ObjectLink`（模型） → `AppStorage/PersistentStorage`（应用级）。
- 原则：状态就近、单向数据流、精细化重组边界；副作用集中处理。

#### 生命周期与导航钩子
- 组件：`aboutToAppear()` 做初始化与参数读取；`aboutToDisappear()` 释放资源。
- 导航目的地：`NavDestination().onReady((ctx) => { ... })` 读取参数/初始化。
- 避免在字段初始器引用上下文或路由对象。

```ts
@Observed
class CounterModel { count: number = 0 }

@Component
struct Counter {
  @ObjectLink model: CounterModel
  build() {
    Row({ space: 8 }) {
      Button('-').onClick(() => this.model.count--)
      Text(`${this.model.count}`).padding(8)
      Button('+').onClick(() => this.model.count++)
    }
  }
}
```

### 导航与路由（新旧范式）
- 新范式：`Navigation`/`NavPathStack`/`NavDestination`。
- 旧范式：`router.pushUrl/pop`（存量兼容，建议封装后渐进迁移）。

```ts
@Entry
@Component
struct AppNav {
  private navStack: NavPathStack = new NavPathStack()
  build() {
    Navigation(this.navStack) {
      NavDestination('Home', Home())
      NavDestination('Detail', Detail({ id: 0 }))
    }
  }
}
// this.navStack.pushPath({ name: 'Detail', param: { id: 123 } })
```

#### 使用规范与建议
- `@Entry.build()` 保持单根容器，将 `Navigation(...)` 包裹于其中。
- `NavDestination.name` 唯一，集中管理常量。
- 参数轻量化，尽量 `pushPath({ name, param })` + 目标页 `@Prop`。
- 复杂回传：通过上层状态或回调 `@Prop`。
- 兼容性不确定时，先退化到 `pushPath/pop` 与 `@Prop`。

#### 示例（列表→详情）
```ts
@Entry
@Component
struct AppRoot {
  private stack: NavPathStack = new NavPathStack()
  build() {
    Column() {
      Navigation(this.stack) {
        NavDestination('List', RepoList({
          onSelect: (repoId: string) => this.stack.pushPath({ name: 'Detail', param: { id: repoId } })
        }))
        NavDestination('Detail', RepoDetail({ id: '' }))
      }
    }
  }
}
```

### 手势与交互
- 基础事件：`onClick/onLongPress/onTouch/onKeyEvent/onScroll`。
- 组合手势：`TapGesture/PanGesture/PinchGesture/SwipeGesture/DragGesture`。
- 冲突处理：限定方向与优先级，避免与 `Scroll` 冲突。
```ts
Text('Drag me')
  .gesture(
    PanGesture({ direction: PanDirection.All })
      .onActionUpdate(e => {/* 更新坐标状态 */})
  )
```

### 动画
- 隐式动画 `animateTo({ duration, curve }, () => { ... })`。
- 转场/进入/退出：`transition()`、`animation()`；海量列表慎用。
- 曲线：`Curve.Linear/Ease/EaseInOut`、物理 `SpringMotion`。
```ts
animateTo({ duration: 250, curve: Curve.EaseOut }, () => {
  this.expanded = !this.expanded
})
```

#### 交互样式约定：卡片按压与进场
- **进场淡入**：组件在 `aboutToAppear()` 中触发 `opacity` 从 0 → 1（200–250ms，EaseOut）。
- **按压反馈**：`onTouch` 捕获 Down/Up，使用 `animateTo` 做轻微缩放（0.98）与次要图标位移（2–4vp）。
- **手势冲突**：在长列表中尽量用轻量动画，避免持续高频属性变更。
```ts
@Component
struct PressableCard {
  @State private isPressed: boolean = false
  @State private appearOpacity: number = 0
  @State private iconOffsetX: number = 0

  aboutToAppear() {
    animateTo({ duration: 220, curve: Curve.EaseOut }, () => {
      this.appearOpacity = 1
    })
  }

  build() {
    Column() {
      // 内容 ...
    }
    .padding(16)
    .borderRadius(16)
    .opacity(this.appearOpacity)
    .scale({ x: this.isPressed ? 0.98 : 1, y: this.isPressed ? 0.98 : 1 })
    .onTouch(e => {
      if (e.type === TouchType.Down) {
        animateTo({ duration: 120, curve: Curve.EaseOut }, () => {
          this.isPressed = true
          this.iconOffsetX = 2
        })
      } else if (e.type === TouchType.Up || e.type === TouchType.Cancel) {
        animateTo({ duration: 160, curve: Curve.EaseOut }, () => {
          this.isPressed = false
          this.iconOffsetX = 0
        })
      }
    })
  }
}
```

### 绘制（Canvas）
- 用途：图表、装饰、自定义控件底层绘制。
- 性能：避免主线程高频重绘；必要时用离屏。
```ts
Canvas((ctx) => {
  ctx.fillRect(10, 10, 100, 60)
  ctx.strokeText('ArkTS', 20, 50)
}).width(200).height(100)
```

### 样式、主题与资源
- 链式样式：尺寸、圆角、阴影、字体、颜色等。
- 样式封装：`@Styles`/`@Extend`（兼容性因版本而异，必要时直接链式写）。
- 主题：通过资源键 `$r('app.color.xxx')`、`$r('app.float.xxx')` 管理；暗色在 `resources/dark/element/color.json` 覆盖同名键。
```ts
Text('多行省略与阴影示例')
  .fontSize(16)
  .lineHeight(22)
  .maxLines(2)
  .textOverflow({ overflow: TextOverflow.Ellipsis })
  .padding({ left: 12, right: 12 })
  .shadow({ color: '#00000033', offsetX: 0, offsetY: 2, blurRadius: 8 })
```

```ts
@Styles
function CardContainer() {
  .backgroundColor('#FFFFFF')
  .borderRadius(12)
  .shadow({ color: '#0000001A', offsetX: 0, offsetY: 4, blurRadius: 16 })
  .padding(16)
}

@Component
struct CardExample {
  build() {
    Column() {
      Text('Title').fontSize(18).fontWeight(FontWeight.Medium)
      Text('Content...').fontSize(14).opacity(0.9)
    }.CardContainer()
  }
}
```

```ts
@Styles
function TitleText() {
  .fontSize(20)
  .fontWeight(FontWeight.Medium)
  .lineHeight(24)
  .fontColor($r('app.color.text_primary'))
}
```

#### 资源与键名
- 资源位于 `resources/base/element` 与 `resources/base/media`；暗色、分辨率有对应目录。
- 键名建议前缀化与分层：如 `app.color.text_primary`、`app.float.padding_md`。

### 多语言与区域
- 在语言目录维护 `string.json`；使用 `$r('app.string.xxx')` 引用。
- 跟随系统语言/区域动态切换；避免硬编码文案。

### 数据与存储
- 轻量：`AppStorage`、`@StorageProp/@StorageLink`、`PersistentStorage`。
- 持久化：偏好 KV 或 RDB；注意线程切换与 UI 同步更新。
- 权限与网络：避免 UI 线程重 IO；所需权限需在配置声明并运行时请求。

### 性能优化指南
- 降低重组：
  - 细粒度拆分组件，控制重组边界；静态 UI 提取为 `@Builder`。
  - 批量状态更新合并一次提交（如一次事件或一次 `animateTo`）。
- 列表优化：
  - 海量数据使用 `LazyForEach` + 合理 `cachedCount`；分段加载与分页。
  - 图片懒加载、缩放裁剪与缓存。
- 状态最小化：
  - 状态就近、按需提升，避免全局可变。
  - 跨层共享首选 `@Provide/@Consume`。
- 动画/手势：
  - 避免长时高频动画；必要时节流/降采样。
  - 明确手势方向与优先级，减少与滚动冲突。
- 绘制：
  - 复用离屏缓冲，简化路径与填充，减少复杂渐变/阴影。

### 可访问性（a11y）
- 图片/图标提供 `accessibilityText/Description`。
- 语义分组、焦点顺序合理；支持触控与键盘可达。
- 遵循系统字体缩放、对比度与暗色模式。

### 工程规范与实践清单
- 结构：`@Entry` 单一根容器；页面按功能/模块分层；资源键集中管理。
- 命名：组件/状态/资源键使用有意义、可读的命名；避免缩写。
- 代码风格：事件尽量方法引用；避免在 `build()` 创建大对象/大闭包。
- 可测性：页面组件以“纯 UI + 明确输入（@Prop）”为主；逻辑可测试化。
- 设计一致性：空/加载/错误态抽象为复用组件。

### 编译器与常见坑（含错误号）
- **@Entry 根节点唯一（10905210）**：入口 `build()` 只能一个根容器。修复：统一包裹。
- **严格类型（10605043 / 10605038 / 10605008）**：数组/对象字面量需可推断/显式接口；避免 `any/unknown`。修复：先声明 `interface`，数组注解 `Type[]`，回调与 key 标注类型。
- **对象字面量不可作类型（10605040）**：先定义 `interface` 再引用，如 `CommitItem[]`。
- **装饰器/语法兼容**：部分版本 `@Styles` 或高级装饰器不一致；报 “Unexpected token ... need plugins...”。修复：用链式样式代替。
- **颜色 API 差异**：`Color.rgb(...)` 可能不可用，用 `'#RRGGBB'` 资源色字符串。
- **阴影参数差异（10505001）**：`ShadowOptions.blurRadius` 可能不存在；修复：去除或简化阴影。
- **对齐枚举不匹配**：`alignSelf(Alignment.Center)` 可能要求 `ItemAlign`。修复：`Blank().layoutWeight(1)` 或容器对齐属性。
- **Button 构造差异**：部分版本不支持 `Button('文本')`；使用 `Button() { Text('文本') }`。
- **存储装饰器**：`@StorageProp` 需默认值且非可选；部分 `AppStorage.*` API 可能 deprecated。修复：父组件 `@State` + `@Prop`。
- **导航能力差异**：`Navigation/NavPathStack/NavDestination` 各版本差异大；必要时单组件 `@State` 退化实现。
- **router 迁移建议**：`router.pushUrl/back/getParams` 在部分版本告警。优先使用 `UIContext.getRouter()` 的 `pushUrl/replaceUrl/back/getParams`（API 18+）；存量如仍用 `@kit.ArkUI` 的 `router`，需关注 deprecated 提示。中长期应迁移到 `Navigation/NavPathStack`，通过 `pushPath({ name, param })` + 目标页 `@Prop` 传参，彻底去除旧路由依赖。
- **网格组件可用性**：`GridRow/GridCol` 可能缺失，用 `Row/Column` 或 `Grid/GridItem` 代替。
- **样式生效位置**：文字样式需链到 `Text(...)` 本身。
- **列表 key**：为 `ForEach/LazyForEach` 提供稳定 key（如 `item.id`）。

### 示例工程（sample_in_harmonyos）实践要点
- **模块化导航**：
  - 使用 `PageContext` 封装 `openPage/replacePage/popPage`，内部基于 `NavPathStack`。
  - 各功能域在根处以 `Navigation(pageContext.navPathStack)` 包裹页面树，避免全局路由耦合。
  - 返回策略优先 `Navigation.pop/replacePath`；需系统返回键时配合 `onBackPressed()`。

- **状态与数据流**：
  - 定义 `BaseState/BaseVM/BaseVMEvent`，页面以 `viewModel.getState()` → `@State` 绑定，事件统一 `sendEvent(...)`。
  - 轻量跨域用 `AppStorage + @StorageProp`；复杂跨域用 `GlobalContext` 注册对象。
  - 偏好存储封装 `PreferenceManager`（`@kit.ArkData.preferences`），JSON 序列化读写并 `flush()`。

- **列表与滚动**：
  - `List + Repeat().key().template()` 实现分组与多模板；`Scroller` + `onScrollFrameBegin/onDidScroll` 精准控制吸顶、Banner 高度与回弹效果。
  - 多页签用 `TabsController.preloadItems([...])` 预加载，降低切换抖动。

- **响应式适配**：
  - 以 `BreakpointType/BreakpointTypeEnum` 与全局 `GlobalInfoModel` 控制方向、尺寸、间距与显隐；超大屏结合 `SideBarContainer` 与侧栏导航。
  - 对非活动页使用 `@Component({ freezeWhenInactive: true })` 降低重组；`.backgroundBlurStyle().renderGroup()` 在 `onShown/onHidden` 间切换渲染成本。

- **交互与性能细节**：
  - 双击返回退出：`promptAction.showToast` + `ProcessUtil.moveAbilityToBackground` 节流误触。
  - 冷启动过渡：`animateTo({ delay, duration, onFinish })` 组织预加载与跳转顺序。
  - 滚动帧优化：在 `onScrollFrameBegin` 返回 `{ offsetRemain: 0 }` 拦截多余位移，避免过绘和抖动。

### 版本兼容与迁移建议
- 新项目优先使用 `Navigation` 栈式导航；存量保留 `router` 并抽象封装，分阶段替换。
- 对装饰器/样式 API 不确定时，采用更朴素、链式和参数显式的兼容写法。
- 复杂页面使用 `@Component({ freezeWhenInactive: true })` 冻结非活动页，降低重组成本。

### 参考链接
- 官方总览文档：[ArkTS UI Development](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/arkts-ui-development)
- 导航指南：[HarmonyOS Navigation 指南](https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/arkts-navigation-navigation)



