# RTFrame — Zephyr RTOS 嵌入式项目

## 技术栈
- OS: Zephyr RTOS (上游 `../zephyrproject/zephyr/`)
- 构建: CMake + Kconfig，入口 `./build.sh`
- 语言: C (Zephyr 内核) + C++ (应用模块)
- 目标: NXP i.MX RT1170 (ARM Cortex-M7)
- 参考: PX4 Autopilot (`../fc/px4/PX4-Autopilot/`)

## 项目结构
- `src/lib/` — 可复用库 (crc, adsb, airspeed, dataman 等)
- `src/modules/` — 应用模块 (logger 等)
- `src/drivers/` — 设备驱动
- `boards/` — 板级配置

---

## Agent skills

### Issue tracker

GitHub Issues at `mx3g-lab/RT-FRAME` (via `gh` CLI). PRs are not a triage surface. See `docs/agents/issue-tracker.md`.

### Triage labels

Default labels: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. See `docs/agents/triage-labels.md`.

### Domain docs

Single-context layout: `CONTEXT.md` + `docs/adr/` at repo root. See `docs/agents/domain.md`.

---

## 项目强约束（来源 docs/rules/，违反即为 bug）

以下规则在 `docs/rules/` 下有完整版本，此处为强制要点摘要。

### 规则 0 — 计划管理
跨文件改动、架构变更、新依赖引入，必须先：在 `docs/rules/plans/` 创建计划文件 → 在 `plans/SUMMARY.md` 登记 → 用户确认后再动手。无计划不改代码。

### 规则 1 — Git 提交格式
`[TAG] 简洁描述`（TAG: `FEAT`/`FIX`/`DOCS`/`STYLE`/`REFACTOR`/`TEST`/`CHORE`/`PERF`/`BREAKING`）
- 提交前展示消息给用户确认，禁止自动提交
- 禁止追加 `Co-authored-by` 等自动签名

### 规则 2 — 分层架构
```
modules/（应用层）→ middleware/（通信中间件）
modules/（应用层）→ lib/（通用库）       → drivers/（驱动抽象）→ Zephyr API
```
- `lib/`：不依赖硬件、不依赖中间件，可在任何平台复用
- `drivers/`：只通过 Zephyr 驱动 API 访问硬件，禁止直接操作寄存器、禁止 include `fsl_*.h`
- 禁止跨层污染（lib/drivers 不能依赖 modules）

### 规则 3 — CMake 规范
`src/` 下只用 Zephyr 原生 API，库名以 `rtframe_` 为前缀：
```cmake
zephyr_library_named(rtframe_<name>)
zephyr_library_sources(foo.cpp)
zephyr_library_include_directories(.)
zephyr_library_link_libraries(rtframe_<dep>)  # 显式声明依赖
```
- **禁止**在 `src/` 下使用 `target_sources(app PRIVATE ...)`（仅 targets/ 入口允许）
- 条件编译用 `zephyr_library_sources_ifdef(KCONFIG ...)` + `add_subdirectory_ifdef()`
- 源文件逐一列出，不用 glob

### 规则 4 — Kconfig 命名空间
- 所有符号必须 `RTFRAME_` 前缀
- 每个 `src/` 子目录必须有 `Kconfig`，父目录通过 `rsource` 引入
- Kconfig 新增项同时需要：`rsource`（父 Kconfig）+ `add_subdirectory_ifdef()`（父 CMakeLists.txt）

### 规则 5 — 通信中间件
- uORB (`CONFIG_RTFRAME_UORB`) 和 zbus (`CONFIG_RTFRAME_ZBUS`) 默认启用
- MAVLink 在 `src/modules/mavlink/`，不属于 middleware 层

### 规则 6 — Kconfig 层级化 Master Switch
| Master Switch | 控制范围 | CM7 默认 | CM4 默认 |
|---|---|---|---|
| `RTFRAME_CORE` | src/core/ | y | y |
| `RTFRAME_LIB` | src/lib/ | y | y |
| `RTFRAME_DRIVERS` | src/drivers/ | y | y |
| `RTFRAME_MODULES` | src/modules/ | y | n |
| `RTFRAME_MIDDLEWARE` | src/middleware/ | y | n |

### 规则 7 — 自包含构建
`git clone` + `bash tools/setup_env.sh` 后可直接构建，不依赖任何外部环境变量或系统全局安装。验证：`bash tools/setup_env.sh --check`

### 规则 8 — 新 Board 添加流程
1. 从 `middlewares/zephyr/boards/` 复制原型 → `boards/<vendor>/<board>/`
2. 删除 `middlewares/zephyr/boards/` 下的原型（BOARD_ROOT 同名冲突）
3. 修改 DTS / defconfig / yaml
4. 添加 `targets/` CMakeLists.txt + 根 `Makefile` 目标
详见 [`docs/rules/board-rules.md`](docs/rules/board-rules.md)

### 规则 9 — AI Coding 约束（对所有 AI 工具强制）
- **禁止自作主张**：未确认不得改动架构、CMakeLists.txt 顶层、.gitmodules
- **禁止跨层污染**：lib/drivers 不依赖 modules；不绕过 Zephyr API 访问硬件
- **≥3 文件改动**：必须先在 `docs/rules/plans/` 写计划
- **注释**：只写"为什么"，不写"做了什么"
- **语言**：`src/` 下新建文件用 C++（`.cpp`），Zephyr C 宏定义放 `.c`
- **单文件单类**：不在一文件定义多个不相关类
- **模块组织**：`.h` + `.cpp` 分离，头文件不实现非 inline 函数

### 渐进迁移流程
引入新框架/重构已有模块必须按四阶段执行，禁止大爆炸式重写：
1. **新建**（并行编译，新模块编译但不激活）
2. **并行**（Kconfig 开关控制新旧共存，逐步切换）
3. **验证**（书面验收，不修改代码；发现 bug 退回阶段 2）
4. **清除**（删除旧代码和开关）\
详见 [`docs/rules/progressive-migration.md`](docs/rules/progressive-migration.md)

---

## 📋 已注册 Skill 清单（共 25 个，按使用场景分组）

**重要：以下所有 skill 均已在 `.claude/skills/` 安装。当用户消息匹配触发条件时，必须调用 `Skill(skill="xxx")` 加载 skill。DeepSeek 模型尤其注意——不要跳过。**

### 🔧 核心开发（必调）

| Skill | 触发条件（中文） | 触发条件（English） | 用途 |
|-------|-----------------|---------------------|------|
| `diagnosing-bugs` | debug、调试、报错、崩了、不工作、修 bug | diagnose, debug this, broken, crash, error | 系统化五阶段诊断法，禁止直接猜 |
| `codebase-design` | 设计、架构、接口、模块拆分、抽象、解耦 | design module, interface, deep module, seam | 深度模块设计词汇和方法论 |
| `implement` | 实现、写代码、加功能、按需求做 | implement feature, build this, add function | 按 spec/issue 结构化实现 |
| `tdd` | TDD、先写测试、测试驱动、red-green-refactor | test first, red-green-refactor, write test | 测试驱动开发 |
| `prototype` | 原型、demo、验证想法、试试能不能行 | prototype, proof of concept, spike | 快速一次性原型验证 |

### 🔍 代码审查

| Skill | 触发条件（中文） | 触发条件（English） | 用途 |
|-------|-----------------|---------------------|------|
| `review` | review 分支、审查 PR、检查改动、review since | review branch, review PR, review since | 多维度 review：Standards + Spec |
| `code-review` | 代码审查、CR、diff review | code review, review diff | 针对当前 diff 的正确性+简化 review |
| `simplify` | 重构、简化、优化结构、精简代码 | simplify, refactor, clean up | 仅质量优化（不找 bug） |
| `security-review` | 安全审查、安全检查、漏洞 | security review, audit security | 安全审查当前改动 |
| `verify` | 验证、确认修改、测试改动 | verify change, check fix, test manually | 运行 app 验证修改生效 |

### 🏗️ 设计与决策

| Skill | 触发条件（中文） | 触发条件（English） | 用途 |
|-------|-----------------|---------------------|------|
| `architecture` | 架构扫描、代码结构优化、深化机会 | codebase architecture, deepening | 扫描代码库，生成 HTML 架构报告 |
| `domain-modeling` | 领域建模、术语统一、ADR、架构决策 | domain model, ubiquitous language, ADR | 沉淀领域知识和架构决策 |
| `decision-mapping` | 梳理思路、方案太多、怎么选、调研 | map decisions, investigate, which approach | 模糊想法→有序调查 ticket |
| `grill` | 审查设计、挑战方案、尖锐审查 | grill design, challenge plan, sharpen | 尖锐面试式设计审查 |

### 🔀 Git 协作

| Skill | 触发条件（中文） | 触发条件（English） | 用途 |
|-------|-----------------|---------------------|------|
| `merge-conflicts` | merge conflict、合并冲突、冲突解决 | merge conflict, rebase conflict | 解决 in-progress 合并冲突 |
| `git-guardrails` | git push 保护、防止误操作、安全 hooks | guard git, block push, git safety | 设置 git 危险命令拦截 hooks |
| `handoff` | 会话交接、转交任务、给别人接 | handoff, pass to agent, hand over | 压缩当前会话为交接文档 |

### 📄 文档处理

| Skill | 触发条件 | 用途 |
|-------|---------|------|
| `pdf` | PDF、.pdf 文件 | 读/写/合并/拆分/OCR PDF |
| `docx` | Word、.docx、文档、报告 | 读/写/编辑 Word 文档 |
| `xlsx` | Excel、.xlsx、表格、CSV | 读/写/编辑/分析表格 |
| `pptx` | PPT、幻灯片、演示、deck | 读/写/编辑演示文稿 |

### 🚀 工作流工具

| Skill | 触发条件（中文） | 触发条件（English） | 用途 |
|-------|-----------------|---------------------|------|
| `pua` | 摆烂、放弃、太被动、再来 | try harder, stop giving up | 自我驱动，穷尽一切方案 |
| `skill-creator` | 创建 skill、写 skill、优化触发 | create skill, write skill, improve trigger | 创建/优化项目自定义 skill |
| `to-prd` | 生成 PRD、写需求文档 | write PRD, spec document | 对话内容合成 PRD |
| `to-issues` | 生成 issue、拆分任务 | create issues, break down tasks | 方案拆分为独立 issue |

---

## ⛔ 以下技能已安装但与本项目无关，不要调用

GCP 全套（30个）、firebase、frontend-design、webapp-testing、mcp-builder、claude-api——这些是嵌入式 C/C++ 项目用不到的，忽略。

---

## 构建命令
```bash
# 完整构建
./build.sh -p targets/nxp/vmu_rt1170/cm7 -b

# 只编译（不 clean）
./build.sh -b

# clean 后构建
./build.sh -c -b
```

## 代码风格
- 参考 PX4 风格：C++ 类用小写下划线命名（`logger.cpp`）
- Zephyr 风格：C 用 snake_case，宏 UPPER_SNAKE_CASE
- 模块目录结构参考 `src/lib/crc/` 模式

## 参考项目路径
- PX4: `/home/mx3g/work_space/fc/px4/PX4-Autopilot/`
- Zephyr 上游: `/home/mx3g/work_space/rtos/zephyr/zephyrproject/zephyr/`
