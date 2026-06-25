#pragma once
#include <string>
#include <atomic>

class PluginConfig {
public:
    static PluginConfig& Instance() {
        static PluginConfig instance;
        return instance;
    }

    // ---- STT / ASR (OpenAI-compatible) ----
    std::string sttApiBase   = "https://dashscope.aliyuncs.com/compatible-mode/v1";
    std::string sttApiKey    = "";
    std::string sttModel     = "qwen-asr-v1";

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

private:
    PluginConfig() = default;
};
