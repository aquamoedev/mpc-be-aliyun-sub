#pragma once
#include <string>
#include <atomic>

struct PluginConfig {
    std::string aliyunApiKey = "YOUR_DEFAULT_KEY";
    std::string aliyunSecret = "YOUR_DEFAULT_SECRET";
    std::atomic<int> offsetMs{0}; 
    bool isEnabled = true;
    std::string targetLang = "en-US";
};

extern PluginConfig g_Config;
