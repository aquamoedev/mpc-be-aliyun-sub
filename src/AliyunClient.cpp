#include "AliyunClient.h"
#include "PluginConfig.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstring>

using json = nlohmann::json;

// ============================================================
// cURL helpers
// ============================================================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

static size_t ReadCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* src = static_cast<std::string*>(userdata);
    if (src->empty()) return 0;
    size_t toCopy = (std::min)(size * nmemb, src->size());
    std::memcpy(ptr, src->data(), toCopy);
    src->erase(0, toCopy);
    return toCopy;
}

// ============================================================
// HttpPostJson  –  send JSON, get JSON
// ============================================================

std::string AliyunClient::HttpPostJson(
    const std::string& url,
    const std::string& apiKey,
    const std::string& body)
{
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + apiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK) ? response : "";
}

// ============================================================
// SpeechToText  –  multipart/form-data  POST
// ============================================================

std::string AliyunClient::SpeechToText(const std::vector<char>& data) {
    auto& cfg = PluginConfig::Instance();

    std::string url = cfg.sttApiBase + "/audio/transcriptions";
    if (cfg.sttApiBase.back() == '/')
        url = cfg.sttApiBase + "audio/transcriptions";

    CURL* curl = curl_easy_init();
    if (!curl) return "";

    std::string response;

    // Build multipart form
    curl_mime* form = curl_mime_init(curl);

    // --- field: file (the raw PCM audio as a blob) ---
    curl_mimepart* filePart = curl_mime_addpart(form);
    curl_mime_name(filePart, "file");
    curl_mime_filename(filePart, "audio.pcm");
    curl_mime_data(filePart, data.data(), data.size());
    curl_mime_type(filePart, "application/octet-stream");

    // --- field: model ---
    curl_mimepart* modelPart = curl_mime_addpart(form);
    curl_mime_name(modelPart, "model");
    curl_mime_data(modelPart, cfg.sttModel.c_str(), CURL_ZERO_TERMINATED);

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers,
        ("Authorization: Bearer " + cfg.sttApiKey).c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_mime_free(form);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || response.empty()) return "";

    // Parse OpenAI / Qwen ASR response: { "text": "..." }
    try {
        json j = json::parse(response);
        if (j.contains("text"))
            return j["text"].get<std::string>();
    } catch (...) {}

    return "";
}

// ============================================================
// TranslateText  –  OpenAI-compatible Chat Completions
// ============================================================

std::string AliyunClient::TranslateText(const std::string& text) {
    auto& cfg = PluginConfig::Instance();

    std::string url = cfg.translateApiBase + "/chat/completions";
    if (cfg.translateApiBase.back() == '/')
        url = cfg.translateApiBase + "chat/completions";

    // Build system prompt (replace ${TARGET_LANG} placeholder)
    std::string sysPrompt = cfg.translateSystemPrompt;
    auto pos = sysPrompt.find("${TARGET_LANG}");
    if (pos != std::string::npos)
        sysPrompt.replace(pos, 16, cfg.targetLang);

    json body;
    body["model"] = cfg.translateModel;
    body["messages"] = json::array({
        {{"role", "system"}, {"content", sysPrompt}},
        {{"role", "user"},   {"content", text}}
    });
    body["temperature"] = 0.3;
    body["max_tokens"]  = 512;

    std::string raw = HttpPostJson(url, cfg.translateApiKey, body.dump());
    if (raw.empty()) return "";

    // Parse: { "choices": [ { "message": { "content": "..." } } ] }
    try {
        json j = json::parse(raw);
        return j["choices"][0]["message"]["content"].get<std::string>();
    } catch (...) {}

    return "";
}

// ============================================================
// ProcessAudioChunk  –  pipeline
// ============================================================

std::string AliyunClient::ProcessAudioChunk(const std::vector<char>& audioData) {
    std::string text = SpeechToText(audioData);
    if (text.empty()) return "";
    return TranslateText(text);
}
