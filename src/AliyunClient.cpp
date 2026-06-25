#include "AliyunClient.h"
#include "PluginConfig.h"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <string>
#include <cstring>
#include <sstream>

using json = nlohmann::json;

// ============================================================
// cURL helpers
// ============================================================

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// ============================================================
// BuildWavBlob  —  prepend a standard 44-byte WAV header
// ============================================================

std::vector<char> AliyunClient::BuildWavBlob(
    const std::vector<char>& rawPcm,
    const WAVEFORMATEX& wfex,
    bool wfexValid)
{
    // Extract format parameters
    WORD  channels   = wfexValid ? wfex.nChannels      : 2;
    DWORD sampleRate = wfexValid ? wfex.nSamplesPerSec  : 44100;
    WORD  bits       = wfexValid ? wfex.wBitsPerSample  : 16;
    DWORD byteRate   = sampleRate * channels * (bits / 8);
    WORD  blockAlign = channels * (bits / 8);

    DWORD dataSize = static_cast<DWORD>(rawPcm.size());
    DWORD riffSize = 36 + dataSize;

    std::vector<char> wav(44 + dataSize);
    char* p = wav.data();

    // RIFF header
    memcpy(p,     "RIFF", 4);  p += 4;
    memcpy(p, &riffSize, 4);  p += 4;
    memcpy(p,     "WAVE", 4);  p += 4;

    // fmt  subchunk
    memcpy(p,     "fmt ", 4);  p += 4;
    DWORD fmtSize = 16;
    memcpy(p, &fmtSize,    4);  p += 4;
    WORD fmtTag = 1; // PCM
    memcpy(p, &fmtTag,     2);  p += 2;
    memcpy(p, &channels,   2);  p += 2;
    memcpy(p, &sampleRate, 4);  p += 4;
    memcpy(p, &byteRate,   4);  p += 4;
    memcpy(p, &blockAlign, 2);  p += 2;
    memcpy(p, &bits,       2);  p += 2;

    // data subchunk
    memcpy(p, "data", 4);  p += 4;
    memcpy(p, &dataSize, 4);  p += 4;

    // PCM payload
    memcpy(p, rawPcm.data(), dataSize);

    return wav;
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
// SpeechToText  –  multipart/form-data  POST with WAV blob
// ============================================================

std::string AliyunClient::SpeechToText(const std::vector<char>& wavData) {
    auto& cfg = PluginConfig::Instance();

    std::string url = cfg.sttApiBase;
    if (!url.empty() && url.back() != '/') url += '/';
    url += "audio/transcriptions";

    CURL* curl = curl_easy_init();
    if (!curl) return "[ERR: curl init failed]";

    std::string response;

    curl_mime* form = curl_mime_init(curl);

    // --- field: file (WAV) ---
    curl_mimepart* filePart = curl_mime_addpart(form);
    curl_mime_name(filePart, "file");
    curl_mime_filename(filePart, "audio.wav");
    curl_mime_data(filePart, wavData.data(), wavData.size());
    curl_mime_type(filePart, "audio/wav");

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

    if (res != CURLE_OK)
        return "[ERR: STT network error]";
    if (response.empty())
        return "[ERR: STT empty response]";

    try {
        json j = json::parse(response);
        if (j.contains("error")) {
            std::string msg = j["error"].is_object()
                ? j["error"]["message"].get<std::string>()
                : j["error"].get<std::string>();
            return "[ERR: STT API] " + msg;
        }
        if (j.contains("text"))
            return j["text"].get<std::string>();
    } catch (...) {}

    return "[ERR: STT parse failed] " + response.substr(0, 120);
}

// ============================================================
// TranslateText  –  OpenAI-compatible Chat Completions
// ============================================================

std::string AliyunClient::TranslateText(const std::string& text) {
    auto& cfg = PluginConfig::Instance();

    std::string url = cfg.translateApiBase;
    if (!url.empty() && url.back() != '/') url += '/';
    url += "chat/completions";

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
    if (raw.empty()) return "[ERR: Translate network error]";

    try {
        json j = json::parse(raw);
        if (j.contains("error")) {
            std::string msg = j["error"].is_object()
                ? j["error"]["message"].get<std::string>()
                : j["error"].get<std::string>();
            return "[ERR: Translate API] " + msg;
        }
        return j["choices"][0]["message"]["content"].get<std::string>();
    } catch (...) {}

    return "[ERR: Translate parse failed]";
}

// ============================================================
// ProcessAudioChunk  –  pipeline entry point
// ============================================================

std::string AliyunClient::ProcessAudioChunk(
    const std::vector<char>& rawPcm,
    const WAVEFORMATEX& wfex,
    bool wfexValid)
{
    auto& cfg = PluginConfig::Instance();
    if (!cfg.isEnabled) return "";

    // Build WAV blob from raw PCM
    std::vector<char> wav = BuildWavBlob(rawPcm, wfex, wfexValid);

    std::string text = SpeechToText(wav);
    if (text.empty()) return "";
    if (text.find("[ERR:") == 0) return text; // pass through error

    return TranslateText(text);
}