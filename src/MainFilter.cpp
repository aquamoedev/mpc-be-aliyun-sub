#include <dshow.h>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include "PluginConfig.h"
#include "AliyunClient.cpp"
#include "OverlayWindow.cpp"

PluginConfig g_Config;

class AudioSubFilter : public CBaseFilter {
    std::queue<std::vector<char>> audioQueue;
    std::mutex queueMtx;
    
    void ProcessingThread() {
        AliyunClient client;
        SubtitleOverlay overlay;
        overlay.Init();

        while (true) {
            std::vector<char> chunk;
            {
                std::lock_guard<std::mutex> lock(queueMtx);
                if (!audioQueue.empty()) {
                    chunk = audioQueue.front();
                    audioQueue.pop();
                }
            }

            if (!chunk.empty()) {
                std::string result = client.ProcessAudioChunk(chunk);
                overlay.UpdateText(result);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    HRESULT Transform(IMediaSample *pSample) {
        BYTE *pData;
        LONG cbData;
        pSample->GetPointer(&pData);
        pSample->GetActualSampleSize(&cbData);

        std::lock_guard<std::mutex> lock(queueMtx);
        audioQueue.push(std::vector<char>(pData, pData + cbData));

        return S_OK;
    }
};
