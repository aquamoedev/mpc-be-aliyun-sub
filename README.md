# 🏛️ mpc-be-aliyun-sub

> MPC-BE 实时语音识别 + AI 翻译字幕插件  
> Built with ✨ **divine power** by Goddess Aqua 💙

---

## ✨ 功能

- 🎙️ **实时语音识别 (STT)** — 通过 DirectShow 滤镜拦截 MPC-BE 音频流，发送到 ASR API 转文字
- 🌐 **AI 翻译** — 将识别结果发送到 LLM（兼容 OpenAI Chat Completions 接口），翻译成目标语言
- 🖥️ **屏幕字幕叠加** — 翻译结果以半透明叠加层显示在屏幕底部
- ⚙️ **可配置** — 支持自定义 API 地址、模型、目标语言、字幕偏移

---

## 📥 安装

### 1. 下载

从 [Releases](https://github.com/aquamoedev/mpc-be-aliyun-sub/releases) 下载最新 `mpc-be-aliyun-sub.dll`。

### 2. 注册 DLL

以**管理员身份**打开命令提示符，执行：

```cmd
regsvr32 mpc-be-aliyun-sub.dll
```

> 卸载：`regsvr32 /u mpc-be-aliyun-sub.dll`

### 3. 在 MPC-BE 中启用

| 步骤 | 操作 |
|------|------|
| 1 | 打开 MPC-BE → **选项** → **外部滤镜** |
| 2 | 点击 **添加滤镜...** |
| 3 | 找到 **「Aqua Audio Sub Filter」** 并添加 |
| 4 | 选中它，选择 **「首选」** |
| 5 | 点击确定，**重启播放** |

---

## ⚙️ 配置

### 打开配置面板

在 MPC-BE 的 **外部滤镜** 列表中，双击 **「Aqua Audio Sub Filter」**，或在播放时右键 → 滤镜 → 属性。

### 配置项说明

| 区域 | 字段 | 说明 |
|------|------|------|
| **STT / ASR** | API Base URL | 语音识别 API 地址，如 `https://dashscope.aliyuncs.com/compatible-mode/v1` |
| | API Key | 你的 API Key |
| | Model | 模型名，如 `qwen-audio-turbo`、`whisper-1` |
| **Translation** | API Base URL | 翻译 LLM API 地址（OpenAI 兼容接口） |
| | API Key | 你的 API Key |
| | Model | 模型名，如 `qwen-plus`、`gpt-3.5-turbo` |
| | System Prompt | 翻译系统提示词，`${TARGET_LANG}` 会被替换为目标语言 |
| **General** | Target Language | 目标语言，如 `Chinese`、`Japanese`、`English` |
| | Offset (ms) | 字幕时间偏移（毫秒），正数=延迟，负数=提前 |
| | Enable | 勾选启用字幕处理 |

### 推荐 API 配置

#### 阿里云百炼 (DashScope)

| 字段 | 值 |
|------|-----|
| STT API Base | `https://dashscope.aliyuncs.com/compatible-mode/v1` |
| STT Model | `qwen-audio-turbo` |
| Translation API Base | `https://dashscope.aliyuncs.com/compatible-mode/v1` |
| Translation Model | `qwen-plus` |

#### OpenAI

| 字段 | 值 |
|------|-----|
| STT API Base | `https://api.openai.com/v1` |
| STT Model | `whisper-1` |
| Translation API Base | `https://api.openai.com/v1` |
| Translation Model | `gpt-3.5-turbo` |

#### 本地部署 (vLLM / Ollama)

| 字段 | 值 |
|------|-----|
| STT API Base | `http://localhost:8000/v1` |
| Translation API Base | `http://localhost:8000/v1` |

---

## 🎬 使用流程

```
1. 安装 DLL → 注册 → MPC-BE 添加滤镜
2. 双击滤镜打开配置 → 填入 API 信息
3. 播放视频 → 音频自动发送到 ASR → 翻译 → 字幕显示在屏幕底部
```

> ⚠️ **注意**：字幕是独立的全屏叠加窗口，不是 MPC-BE 内置字幕。如果视频是全屏独占模式，字幕可能被遮挡，建议使用窗口化全屏。

---

## 🛠️ 开发

### 依赖

- **Visual Studio 2022** (MSVC)
- **CMake** ≥ 3.20
- **vcpkg** — 安装 `curl` 和 `nlohmann-json`

```bash
vcpkg install curl[core,schannel]:x64-windows-static-md nlohmann-json:x64-windows-static-md
```

### 构建

```bash
git clone https://github.com/aquamoedev/mpc-be-aliyun-sub.git
cd mpc-be-aliyun-sub

cmake -B build -S . \
  -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake \
  -A x64

cmake --build build --config Release
```

产物：`build/Release/mpc-be-aliyun-sub.dll`

### 项目结构

```
src/
├── MainFilter.cpp/h      # DirectShow 滤镜 + 属性页
├── AliyunClient.cpp/h    # ASR + 翻译 API 客户端
├── SubtitleOverlay.cpp/h # 屏幕字幕叠加层
├── ConfigDialog.cpp/h    # 配置对话框
├── PluginConfig.cpp/h    # 配置持久化（注册表）
├── resource.rc/h         # Win32 资源（对话框模板）
└── exports.def           # DLL 导出
baseclasses/              # DirectShow 基类（来自 Windows SDK）
```

### 调试

注册 Debug 版本后，在 Visual Studio 中附加到 MPC-BE 进程 `mpc-be64.exe` 即可断点调试。

---

## ❓ 常见问题

### 字幕不显示？

1. 检查配置中 **Enable** 是否勾选
2. 确认 API Key 和 Base URL 正确（可以先用 curl 测试）
3. 检查 API 余额是否充足
4. 如果视频是全屏独占模式，切换为窗口化全屏

### 弹窗关掉后还有个小框？

这是 DirectShow 属性页的宿主窗口，重启播放后消失。v1.0.8+ 已修复该问题。

### 如何卸载？

```cmd
regsvr32 /u mpc-be-aliyun-sub.dll
```

然后删除 DLL 文件即可。

---

## 📄 License

MIT — Aqua 女神保佑你的字幕 🏛️💙