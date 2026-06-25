#pragma once
#include "streams.h"
#include "AliyunClient.h"
#include "SubtitleOverlay.h"
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <mmreg.h>

// Filter CLSID: {B8F4E2D1-7A3C-4F91-A6E5-9D2C0F8B3A71}
DEFINE_GUID(CLSID_AudioSubFilter,
    0xb8f4e2d1, 0x7a3c, 0x4f91, 0xa6, 0xe5, 0x9d, 0x2c, 0x0f, 0x8b, 0x3a, 0x71);

// Property page CLSID: {D5A1C3E7-8F2B-4B6D-9C1A-3E5F7A2D8B4C}
DEFINE_GUID(CLSID_AudioSubPropPage,
    0xd5a1c3e7, 0x8f2b, 0x4b6d, 0x9c, 0x1a, 0x3e, 0x5f, 0x7a, 0x2d, 0x8b, 0x4c);

class AudioSubFilter : public CTransformFilter, public ISpecifyPropertyPages {
public:
    DECLARE_IUNKNOWN;
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr);

    // CTransformFilter overrides
    HRESULT CheckInputType(const CMediaType* mtIn) override;
    HRESULT CheckTransform(const CMediaType* mtIn,
                           const CMediaType* mtOut) override;
    HRESULT Transform(IMediaSample* pIn, IMediaSample* pOut) override;
    HRESULT DecideBufferSize(IMemAllocator* pAlloc,
                             ALLOCATOR_PROPERTIES* pprops) override;
    HRESULT GetMediaType(int iPosition, CMediaType* pMediaType) override;
    HRESULT StartStreaming() override;

    // ISpecifyPropertyPages
    STDMETHODIMP GetPages(CAUUID* pPages) override;

    void ShowConfig(HINSTANCE hInst, HWND hParent);

    // Override NonDelegatingQueryInterface to expose ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void** ppv) override;

private:
    AudioSubFilter(LPUNKNOWN pUnk, HRESULT* phr);
    ~AudioSubFilter();

    void ProcessingThread();

    std::queue<std::vector<char>> m_audioQueue;
    std::mutex                    m_queueMtx;
    std::thread                   m_worker;
    std::atomic<bool>             m_running{true};

    WAVEFORMATEX   m_wfex = {};     // audio format from input pin
    std::mutex     m_wfexMtx;
    bool           m_wfexValid = false;

    AliyunClient    m_client;
    SubtitleOverlay m_overlay;
};