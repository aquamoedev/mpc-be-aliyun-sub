#pragma once
#include <string>
#include <atomic>

class PluginConfig {
public:
    static PluginConfig& Instance() {
        static PluginConfig instance;
        return instance;
    }

    std::string aliyunApiKey   = "YOUR_DEFAULT_KEY";
    std::string aliyunSecret   = "YOUR_DEFAULT_SECRET";
    std::string targetLang     = "en-US";
    int offsetMs               = 0;
    bool isEnabled             = true;

private:
    PluginConfig() = default;
};
