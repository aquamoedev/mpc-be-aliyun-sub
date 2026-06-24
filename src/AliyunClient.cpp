#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include "PluginConfig.h"
#include <vector>
#include <string>

using json = nlohmann::json;

class AliyunClient {
public:
    std::string ProcessAudioChunk(const std::vector<char>& audioData) {
        std::string text = SpeechToText(audioData);
        if (text.empty()) return "";
        return TranslateText(text);
    }

private:
    std::string SpeechToText(const std::vector<char>& data) {
        CURL* curl = curl_easy_init();
        if (!curl) return "";
        curl_easy_cleanup(curl);
        return "Detected speech text from Aliyun"; 
    }

    std::string TranslateText(const std::string& text) {
        return "[Translated] " + text;
    }
};
