#pragma once
#include <string>
#include <atomic>

// ============================================================
// PluginConfig — singleton with Windows Registry persistence
// ============================================================
// All settings are saved to HKCU\Software\AquaSubFilter and
// loaded automatically on first access.  Changes made through
// the config dialog are persisted immediately on OK.
// ============================================================

class PluginConfig {
public:
    static PluginConfig& Instance() {
        static PluginConfig instance;
        return instance;
    }

    // ---- STT / ASR (OpenAI-compatible) ----
    std::string sttApiBase   = "https://dashscope.aliyuncs.com/compatible-mode/v1";
    std::string sttApiKey    = "";
    std::string sttModel     = "qwen3-asr-flash-realtime";

    // ---- Translation LLM (OpenAI-compatible Chat Completions) ----
    std::string translateApiBase = "https://dashscope.aliyuncs.com/compatible-mode/v1";
    std::string translateApiKey  = "";
    std::string translateModel   = "qwen-plus";
    std::string translateSystemPrompt =
        "You are a professional subtitle translator. "
        "Translate the following text into ${TARGET_LANG}. "
        "Keep the translation concise and natural, suitable for subtitle display. "
        "Do NOT add explanations, notes, or quotation marks. "
        "Only output the translated text.";

    // ---- General ----
    std::string targetLang = "English";
    int         offsetMs   = 0;
    bool        isEnabled  = true;

    // ---- Persistence ----
    void SaveToRegistry();
    void LoadFromRegistry();

private:
    PluginConfig();
    ~PluginConfig() = default;
    PluginConfig(const PluginConfig&) = delete;
    PluginConfig& operator=(const PluginConfig&) = delete;
};