#pragma once
#include <vector>
#include <string>
#include <mmreg.h>

// AliyunClient encapsulates both STT (speech-to-text) and translation
// via OpenAI-compatible API endpoints (works with Qwen ASR, Qwen LLM,
// or any self-hosted OpenAI-compatible service like vLLM / Ollama).

class AliyunClient {
public:
    // Process one audio chunk: prepend WAV header → STT → translate → return translated text.
    // Returns empty string on any failure, or "[ERR: ...]" on API error.
    std::string ProcessAudioChunk(
        const std::vector<char>& rawPcm,
        const WAVEFORMATEX& wfex,
        bool wfexValid);

private:
    // Build a WAV header + PCM data blob
    static std::vector<char> BuildWavBlob(
        const std::vector<char>& rawPcm,
        const WAVEFORMATEX& wfex,
        bool wfexValid);

    // POST /v1/audio/transcriptions  (multipart/form-data: file + model)
    std::string SpeechToText(const std::vector<char>& wavData);

    // POST /v1/chat/completions  (JSON: model + messages)
    std::string TranslateText(const std::string& text);

    // Shared helper
    static std::string HttpPostJson(
        const std::string& url,
        const std::string& apiKey,
        const std::string& body);
};