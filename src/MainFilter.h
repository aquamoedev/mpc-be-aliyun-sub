#pragma once
#include "transfrm.h"
#include "AliyunClient.h"
#include "SubtitleOverlay.h"
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>

// {B8F4E2D1-7A3C-4F91-A6E5-9D2C0F8B3A71}
DEFINE_GUID(CLSID_AudioSubFilter,
    0xb8f4e2d1, 0x7a3c, 0x4f91, 0xa6, 0xe5, 0x9d, 0x2c, 0x0f, 0x8b, 0x3a, 0x71);

class AudioSubFilter : public CTransformFilter {
public:
    // COM
    DECLARE_IUNKNOWN;
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

    // CTransformFilter
    HRESULT CheckInputType(const CMediaType* mtIn)  override;
    HRESULT CheckTransform(const CMediaType* mtIn,
                           const CMediaType* mtOut)  override;
    HRESULT Transform(IMediaSample* pSample)          override;
    HRESULT DecideBufferSize(IMemAllocator* pAlloc,
                             ALLOCATOR_PROPERTIES* pprops) override;
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType) override;

    // Config
    void ShowConfig(HINSTANCE hInst, HWND hParent);

private:
    AudioSubFilter(LPUNKNOWN pUnk, HRESULT* phr);
    ~AudioSubFilter();

    void ProcessingThread();   // background API + overlay loop

    std::queue<std::vector<char>> m_audioQueue;
    std::mutex                    m_queueMtx;
    std::thread                   m_worker;
    std::atomic<bool>             m_running{true};

    AliyunClient    m_client;
    SubtitleOverlay m_overlay;
};
