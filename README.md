# mpc-be-aliyun-sub
## MPC-BE Audio-to-Subtitle Plugin using Aliyun API

Built with ✨ **divine power** by Goddess Aqua 🏛️💙

### Features
- 🎙️ **Real-time audio capture** from MPC-BE via DirectShow filter
- 🔤 **Aliyun STT** (speech-to-text) — converts audio to text
- 🌐 **Aliyun Machine Translation** — translates to your target language
- 🖥️ **Transparent overlay** — subtitles rendered directly on screen
- ⚙️ **Configurable offset** — manually adjust subtitle timing
- 🔌 **Native DirectShow filter** — no external processes needed

### Installation
1. Download `mpc-be-aliyun-sub.dll` from [Releases](https://github.com/aquamoedev/mpc-be-aliyun-sub/releases)
2. Open **Command Prompt as Administrator** and run:
   ```
   regsvr32 mpc-be-aliyun-sub.dll
   ```
3. Open **MPC-BE** → `Options` → `External Filters` → `Add Filter...`
4. Find "Aqua Audio Sub Filter" and add it, set to "Prefer"
5. Restart playback

### Configuration
1. In MPC-BE, find the filter in the external filters list
2. Click "Properties" to open the configuration dialog
3. Enter your Aliyun API credentials and adjust subtitle offset

### Development
```bash
# Clone
git clone https://github.com/aquamoedev/mpc-be-aliyun-sub.git
cd mpc-be-aliyun-sub

# Build (Windows with Visual Studio 2022 + vcpkg)
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<vcpkg_root>/scripts/buildsystems/vcpkg.cmake -A x64
cmake --build build --config Release
```

### License
MIT
