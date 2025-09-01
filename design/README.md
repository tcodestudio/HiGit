# HiGit 原型（HTML/Tailwind）

- 入口：`design/index.html`（仓库列表页）
- 详情：`design/repo.html`（仓库详情页：统计、Graph、分支选择、简洁提交列表、查看更多提交）
- 提交历史：`design/commits.html`（分支选择器、长列表、加载更多）
- 技术：Tailwind（CDN）、纯 HTML + 少量内联 JS（主题切换、Toast、分支切换演示）。
- 数据：全部为假数据，仅供视觉与交互演示。

## 说明
- 列表页展示多仓概要（最近提交/同步、分支/标签/提交概览、迷你 Graph）。
- 点击列表项进入详情页，查看仓库统计、提交、分支与标签（同页内切换）。
