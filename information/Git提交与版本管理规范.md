# Git 提交与版本管理规范

<style>
@media print {
  h1, h2, h3, h4 { page-break-after: avoid; break-after: avoid; }
  table { page-break-inside: auto !important; break-inside: auto !important; }
  thead { display: table-header-group; }
  tr, td, th { page-break-inside: auto !important; break-inside: auto !important; }
  img { page-break-inside: avoid; break-inside: avoid; max-width: 100%; }
}
</style>

固件量产工程用。

分支名与图一致：`develop`（日常集成线）为日常合入总线；`main` / `master`（已发布、可量产线）为已发布稳定线。本规范统一使用上述名称（若口头称 `dev`，均指 `develop`）。`origin`（远程仓库地址的别名）不是分支。

Git Flow 中各分支职责不同。`feature`（功能分支）、`release`（发布准备分支）、`hotfix`（线上紧急修复分支）做完一轮合入目标分支后**保留分支，不删除**。

### 目录

- [1. 分支示意图](#1-分支示意图)
  - [1.1 图例与打点约定](#11-图例与打点约定)
  - [1.2 三种工程情形](#12-三种工程情形)
- [2. 分支总览](#2-分支总览)
- [3. 分支命名、版本号与进位规则](#3-分支命名版本号与进位规则)
  - [3.1 分支命名](#31-分支命名)
  - [3.2 版本号各位含义](#32-版本号各位含义)
  - [3.3 官方号 `x.y` 进位](#33-官方号-xy-进位)
  - [3.4 自身版本号进位（`m` / `n`）](#34-自身版本号进位m--n)
  - [3.5 分支相关常用命令](#35-分支相关常用命令)
- [4. Commit 怎么写](#4-commit-怎么写)
  - [4.1 提交前自检](#41-提交前自检)
  - [4.2 Commit 相关常用命令](#42-commit-相关常用命令)
- [5. Tag 命名与打点](#5-tag-命名与打点)
  - [5.1 命名](#51-命名)
  - [5.2 打点时机（钉死）](#52-打点时机钉死)
  - [5.3 Tag 相关常用命令](#53-tag-相关常用命令)
- [6. 快速入门](#6-快速入门)

---

## 1. 分支示意图

![Git Flow 分支示意图](./assets/git-flow-original.png)

​																**图 1　Git Flow 分支示意图**

![分支与版本号演进](./assets/git-branch-version-flow.jpg)

​													**图 2　分支与版本号演进（含情况一 / 二 / 三）**

### 1.1 图例与打点约定

| 图示 | 含义 |
| :--- | :--- |
| 圆圈 ○ | 代码合并节点（临时分支合入主干，或主干间同步） |
| 单三角 △ | 普通 tag（对应 `vx.y.m.n-f`：测试通过后、合入前打在 `feature`） |
| 双三角 | 发行版本 tag（对应 `*-r-RVL` / `*-h-RVL`：测试通过后、合入前打在 `release` / `hotfix`） |

### 1.2 三种工程情形

| 情形 | 适用规模 | 流程说明 |
| :--- | :--- | :--- |
| **情况一** | 小型工程 | 自起始功能开发起连续推进，全部功能合入 `develop` 后统一进入 `release` 做整体测试；测试通过并打 `*-r-RVL` 后，再合入 `main` 与 `develop`。 |
| **情况二** | 大型工程（分阶段） | 完成第一阶段开发后进入全面测试并发布；再开展第二阶段开发，完成后再次全面测试并发布。某一阶段发行版若发现问题，从对应已发布点拉出 `hotfix`，修复并测试通过后打 `*-h-RVL`，再合入 `main` 与 `develop`。 |
| **情况三** | 大型工程（阶段交叉紧急修复） | 第二阶段开发过程中（尚未进入二阶段全面测试），若发现**第一阶段已发行版本**存在必须立即处理的缺陷：先将当前开发工作妥善暂存，自第一阶段发布点拉出 `hotfix` 修复并完成测试、打 `*-h-RVL` 后合入 `main` 与 `develop`；再回到原开发线继续第二阶段功能开发，**同时须将前述修复以补丁形式合入当前开发线，并做全面测试**。若第二阶段发布后出现问题，则在第二阶段发布点之后拉 `hotfix` 修复并测试。 |

**示例（情况三）：** 工程已完成一阶段发布（如 `v2.1.1.3-r-RVL`），正在进行二阶段功能开发。此时一阶段量产包暴露紧急缺陷 → 暂存二阶段改动 → 自 `main` 拉 `hotfix/v2.1.1.4-bugfix` → 修复、测试、打 `v2.1.1.4-h-RVL` → 合入 `main` + `develop` → 恢复二阶段开发，并将该修复补丁合入当前开发线后做全面测试 → 再按 `n` 递增继续开新 `feature`。

命名规则见下文第 3 节。

---

## 2. 分支总览

`feature` / `release` / `hotfix` 合入目标分支后**保留分支，不删除**（便于追溯与对照 tag）。

| 分支 | 创建源与时机 | 合入目标与时机 | 存放内容 | 典型场景 |
| :--- | :--- | :--- | :--- | :--- |
| `main` / `master` | —（仓库初始化时建立） | —（只接收合入，不向外合） | 正式发布 / 量产稳定代码；只收 `release`、`hotfix` 的合入结果；可交付内容以合入前临时分支上已推送的发行 tag 为准 | 生产发布 / 量产出货 |
| `develop` | `main`（仓库初始化时） | —（日常集成总线，不作为发布出口） | 下一版本已集成、但未必已可发布的代码；合入前应能编译、能自测；高风险改动（OTA、Bootloader、Flash、WiFi 重连、屏驱、电机）须真机验证通过后再合入 | 日常集成 |
| `feature/*` | `develop`（开始开发某一功能模块时） | `develop`（该模块开发完成并测试通过时） | 单个功能模块的开发代码（**1 个功能模块 = 1 个 feature 分支**）；开发中的小 bug 仍在本分支用 `[bugfix]` 提交；测试通过后、合入前打 `*-f` | 新功能 |
| `release/*` | `develop`（功能已齐、准备提测时） | **`main` + `develop`**（发布测试通过、可交付时） | 发布候选代码：允许修测出问题、改版本相关内容与文档；测试通过后、合入前打 `*-r-RVL` | 版本发布准备 |
| `hotfix/*` | `main`（已发布版本出现必须立刻修复的缺陷时） | **`main` + `develop`**（补丁验证通过、可交付时） | 已发布版本上的紧急补丁代码；测试通过后、合入前打 `*-h-RVL` | 线上紧急故障 |

禁止下列行为（按分支顺序）：

1. **`main`**：直接开发或直接 `commit`；在本分支打本规范规定的版本 tag
2. **`develop`**：直接开发或直接 `commit`；在本分支打本规范规定的版本 tag
3. **`feature`**：同一分支塞入多个无关功能模块；测试未通过或未打 `*-f` 就合入 `develop`；合入后删除本分支
4. **`release`**：增加新功能；仅合入 `main` 与 `develop` 之一（例如只合 `main`、不回灌 `develop`）；测试未通过或未打 `*-r-RVL` 就合入；合入后删除本分支
5. **`hotfix`**：仅合入 `main` 与 `develop` 之一（例如只合 `main`、不回灌 `develop`）；测试未通过或未打 `*-h-RVL` 就合入；合入后删除本分支

---

## 3. 分支命名、版本号与进位规则

### 3.1 分支命名

`main` / `master`、`develop` 名称固定不变。临时分支命名如下。

| 分支 | 命名格式 | 说明 |
| :--- | :--- | :--- |
| `feature/*` | `feature/vx.y.m.n-xxx` | `xxx` = 本分支主要功能模块的简短标识（见下方示例） |
| `release/*` | `release/vx.y.m.n-test` | 提测 / 发布准备；后缀固定为 `test` |
| `hotfix/*` | `hotfix/vx.y.m.n-bugfix` | 线上紧急修复；后缀固定为 `bugfix` |

`xxx` 填写约定：用英文小写或拼音，单词间用 `_` 连接，概括**本分支唯一主做的功能模块**，勿写版本号、勿写多人无关事项。

| `xxx` 示例 | 含义 |
| :--- | :--- |
| `offline` / `offline_iat` | 离线听写 / 离线词条匹配 |
| `wifi_reconnect` | WiFi 重连优化 |
| `ota_rollback` | OTA 回滚相关 |
| `alarm_timer` | 闹钟 / 定时话术 |
| `screen_driver` | 屏驱适配 |

完整示例：`feature/v2.1.1.0-offline`、`feature/v2.1.1.1-wifi_reconnect`、`feature/v2.1.2.0-ota_rollback`。

### 3.2 版本号各位含义

| 位 | 含义 |
| :--- | :--- |
| `x.y` | 官方固件号，按工程实际对齐的官方版本调整 |
| `m.n` | 项目自身版本号（`m`、`n` 进位见 3.4） |

### 3.3 官方号 `x.y` 进位

| 情形 | 规则 |
| :--- | :--- |
| 某个 `feature` **只合并了新官方版本的部分代码** | **仅该 feature 分支命名用新官方号**；下一个分支仍用合并前的官方号 |
| **将另一个官方号整体合并进工程** | **此后所有分支**一律改用该新官方号 |

示例（与图 2 中官方号演进标注一致）：

- `feature/v2.1.1.0-offline` → 单独合入官方 2.2 部分功能 → 该分支可记为 `v2.2.1.1-xxx`
- 下一分支仍回到 `feature/v2.1.1.2-xxx`（官方号回到 2.1）
- 之后若 **整体合并官方 2.3 全部功能**，则后续全部为 `v2.3.…`（如 `feature/v2.3.3.0-xxx`）

### 3.4 自身版本号进位（`m` / `n`）

| 位 | 进位规则 |
| :--- | :--- |
| `m` | **某个 `release` 出现后，其后第 1 个 `feature` 的 `m` 进 1；此时 `n` 重置为 0** |
| `n` | **每新起一个临时分支，`n` 进 1**（不区分 feature / release / hotfix） |

- 示例：`feature/v2.1.1.0-offline` → `…1.1` → `…1.2` → `release/v2.1.1.3-test` → 下一轮首个 `feature/v2.1.2.0-xxx`（`m`：1→2，`n`：重置为 0）。

### 3.5 分支相关常用命令

**规范命令**

```text
git branch -vv                          // 查看本地分支及跟踪关系
git branch -a                           // 查看本地 + 远程全部分支
git switch <分支名>                     // 切换当前分支（旧命令 git checkout）
git switch -c <分支名>                  // 新建并切换到该分支
git pull origin <分支名>                // 拉取远程更新并合到当前分支
git push -u origin <分支名>             // 首次推送并建立上下游跟踪
git push origin <分支名>                // 推送当前分支到远程
git merge <分支名>                      // 把指定分支合进当前分支
git merge --no-ff <分支名>              // 合入并强制保留合并节点（便于追溯功能合入）
```

**具体示例**

```bash
# 查看分支
git branch -vv
git branch -a

# 从最新 develop 拉 feature（版本号按第 3 节规则递增）
git switch develop
git pull origin develop
git switch -c feature/v2.1.1.0-offline
git push -u origin feature/v2.1.1.0-offline

# 合入 develop（在目标分支上执行；合入后保留 feature，不删除）
git switch develop
git merge --no-ff feature/v2.1.1.0-offline
git push origin develop

# 开 release
git switch develop
git pull origin develop
git switch -c release/v2.1.1.3-test
git push -u origin release/v2.1.1.3-test

# 开 hotfix（从 main）
git switch main
git pull origin main
git switch -c hotfix/v2.3.3.2-bugfix
git push -u origin hotfix/v2.3.3.2-bugfix
```

---

## 4. Commit 怎么写

`git add`（把改动放进暂存区），`git commit`（把暂存区存成一次提交）。一次提交只做一件说得清的事。

标题只能用这四个类型，格式为 **`[类型]简述`**：

| 类型 | 含义 | 说明（正文）写什么 |
| :--- | :--- | :--- |
| `feat` | 新功能 | 新功能介绍 |
| `bugfix` | 修复 bug | **修复方法（必须写）**；问题边界（尽量写）；风险点（尽量写） |
| `perf` | 性能优化（未增加新功能并且未修复 bug） | 优化点与预期收益 |
| `docs` | 只改文档 | 测什么问题的日志，或某某功能文档的修改等 |

固定格式示例：

```text
[bugfix]修复OTA升级回滚失败

1. 修复方法：ota_rollback() 校验失败时恢复备用分区，并做写后读校验
2. 问题边界：仅升级中断后无法回到旧固件；正常升级路径未改
3. 风险点：动了 OTA 和 Flash，必须真机走完升级、断电、回滚
```

```text
[feat]联网定时改为听写完全匹配

1. 新功能介绍：在线走离线时要求听写与词条全等，表外时长走在线
```

```text
[perf]缩短离线词条匹配遍历路径

1. 优化点：热点路径改为二分查找
2. 预期收益：匹配耗时下降，主循环更稳
```

```text
[docs]补充OTA回滚失败用例的测试日志说明

1. 说明：对应测试记录「升级中断回滚」用例的日志字段与判定标准
```

不要写「更新代码」「优化一下」。没编译过、没测完、临时打印，不要提交。

### 4.1 提交前自检

| 检查项 | 要求 |
| :--- | :--- |
| 编译 | 零 error；禁止带着 warning 推共享分支 |
| 普通逻辑 | 开发者自测通过 |
| 高风险模块 | OTA、Bootloader、Flash、WiFi 重连、屏驱、电机等：真机验证通过后才能提交 / 合入 |

### 4.2 Commit 相关常用命令

**规范命令**

```text
git status                              // 查看工作区 / 暂存区状态
git diff                                // 查看未暂存改动
git diff --cached                       // 查看已暂存、尚未提交的改动
git add <文件或目录>                    // 把改动放入暂存区
git commit -m "<说明>"                  // 单行说明提交
git commit -m "$(cat <<'EOF'            // 多行说明提交（标题 + 正文）
[类型]简述

1. …
2. …
EOF
)"
git log --oneline -10                   // 查看最近 10 条提交摘要
```

**具体示例**

```bash
git status
git diff
git add src/ota_rollback.c
git commit -m "$(cat <<'EOF'
[bugfix]修复OTA升级回滚失败

1. 修复方法：ota_rollback() 校验失败时恢复备用分区，并做写后读校验
2. 问题边界：仅升级中断后无法回到旧固件；正常升级路径未改
3. 风险点：动了 OTA 和 Flash，必须真机走完升级、断电、回滚
EOF
)"
git log --oneline -10
```

---

## 5. Tag 命名与打点

Tag（`git tag`：钉在某一次提交上的版本名）用来标明「这包固件是由该 tag 所指向的提交（及其历史）构建出的源码版本」。

### 5.1 命名

| Tag | 打在哪 | 含义 |
| :--- | :--- | :--- |
| `vx.y.m.n-f` | **feature 分支**（测试通过后、合入前） | 在 feature 上打的 tag（`f` = feature） |
| `vx.y.m.n-r-RVL` | **release 分支**（测试通过后、合入前） | 在 release 上打的可发行版本 tag（`r` = release；`RVL` = 可发行版本标识） |
| `vx.y.m.n-h-RVL` | **hotfix 分支**（测试通过后、合入前） | 在 hotfix 上打的可发行版本 tag（`h` = hotfix；`RVL` = 可发行版本标识） |

### 5.2 打点时机（钉死）

1. **测试前不打 tag；合入主干后不再补打同名tag；本地调试的半成品不打 tag。**
2. **测试通过后、合入 `main` / `develop` 之前**，在对应的 `feature` / `release` / `hotfix` 分支上打 tag（`*-f` / `*-r-RVL` / `*-h-RVL`），不在 `main` / `develop` 上打。

### 5.3 Tag 相关常用命令

**规范命令**

```text
git tag -a <标签名> -m "<说明>"         // 创建带说明的 annotated tag（钉在当前提交）
git push origin <标签名>                // 把该 tag 推到远程（普通 git push 不会带上 tag）
git tag -l '<匹配模式>'                 // 按模式列出本地 tag
git show <标签名>                       // 查看该 tag 指向的提交与说明
```

**具体示例**

```bash
# feature：测试通过后、合入前
git tag -a v2.1.1.0-f -m "feature 自测通过，对应 feature/v2.1.1.0-offline"
git push origin v2.1.1.0-f

# release：测试通过后、合入前
git tag -a v2.1.1.3-r-RVL -m "release 可发行版本，对应 release/v2.1.1.3-test"
git push origin v2.1.1.3-r-RVL

# hotfix：测试通过后、合入前
git tag -a v2.3.3.2-h-RVL -m "hotfix 可发行版本，对应 hotfix/v2.3.3.2-bugfix"
git push origin v2.3.3.2-h-RVL

# 查看
git tag -l 'v2.1.*'
git show v2.1.1.3-r-RVL
```

---

## 6. 快速入门

以 `v2.1.1.x` 一轮为例。远程别名默认 `origin`。命令细节见 §3.5 / §4.2 / §5.3。

**1. 初始化 `develop`（仓库只做一次）**

```bash
git switch main
git pull origin main
git switch -c develop
git push -u origin develop
```

**2. 开发功能 → 提交 → 测完打 tag → 合入 `develop`**

```bash
git switch develop && git pull origin develop
git switch -c feature/v2.1.1.0-offline
# 编码 → 编译 → 自测 → 按第 4 节 commit
git push -u origin feature/v2.1.1.0-offline

# 测试通过后、合入前打 tag
git tag -a v2.1.1.0-f -m "feature 自测通过，对应 feature/v2.1.1.0-offline"
git push origin v2.1.1.0-f

# 合入 develop（保留 feature，不删除）
git switch develop && git pull origin develop
git merge --no-ff feature/v2.1.1.0-offline
git push origin develop
```

下一模块再从最新 `develop` 拉 `feature/vx.y.m.n-xxx`（`n` 进 1）。

**3. 开 `release` → 测完打 `*-r-RVL` → 合入 `main` + `develop`**

```bash
git switch develop && git pull origin develop
git switch -c release/v2.1.1.3-test
# 只修测出问题；禁止加新功能；测试通过后、合入前：
git tag -a v2.1.1.3-r-RVL -m "release 可发行版本，对应 release/v2.1.1.3-test"
git push -u origin release/v2.1.1.3-test
git push origin v2.1.1.3-r-RVL

git switch main && git pull origin main
git merge --no-ff release/v2.1.1.3-test
git push origin main

git switch develop && git pull origin develop
git merge --no-ff release/v2.1.1.3-test
git push origin develop
```

此后填《产品测试记录表》：固件版本填 `v2.1.1.3-r-RVL`，分支填 `release/v2.1.1.3-test`。保留 `release`，不删除。

**4. 紧急 `hotfix` → 测完打 `*-h-RVL` → 合入 `main` + `develop`**

```bash
git switch main && git pull origin main
git switch -c hotfix/v2.3.3.2-bugfix
# 真机验证；测试通过后、合入前：
git tag -a v2.3.3.2-h-RVL -m "hotfix 可发行版本，对应 hotfix/v2.3.3.2-bugfix"
git push -u origin hotfix/v2.3.3.2-bugfix
git push origin v2.3.3.2-h-RVL

git switch main && git merge --no-ff hotfix/v2.3.3.2-bugfix && git push origin main
git switch develop && git merge --no-ff hotfix/v2.3.3.2-bugfix && git push origin develop
```

复测记录写 `v2.3.3.2-h-RVL`。两侧都合入；保留 `hotfix`，不删除。
