#pragma once
#include <vector>
#include <string>

class AliyunClient {
public:
    std::string ProcessAudioChunk(const std::vector<char>& audioData);

private:
    std::string SpeechToText(const std::vector<char>& data);
    std::string TranslateText(const std::string& text);
};
