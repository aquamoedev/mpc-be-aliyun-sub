#include "AliyunClient.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

std::string AliyunClient::ProcessAudioChunk(const std::vector<char>& audioData) {
    std::string text = SpeechToText(audioData);
    if (text.empty()) return "";
    return TranslateText(text);
}

std::string AliyunClient::SpeechToText(const std::vector<char>& data) {
    (void)data;
    return "Detected speech text from Aliyun";
}

std::string AliyunClient::TranslateText(const std::string& text) {
    return "[Translated] " + text;
}
