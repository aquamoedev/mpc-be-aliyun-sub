#include "MainFilter.h"
#include "PluginConfig.h"
#include "ConfigDialog.h"
#include <dvdmedia.h>

// ============================================================
// COM / DirectShow plumbing
// ============================================================

CFactoryTemplate g_Templates[] = {
    { L"Aqua Audio Sub Filter", &CLSID_AudioSubFilter,
      AudioSubFilter::CreateInstance, nullptr, nullptr }
};
int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);

STDAPI DllRegisterServer()   { return AMovieDllRegisterServer2(TRUE); }
STDAPI DllUnregisterServer() { return AMovieDllRegisterServer2(FALSE); }

extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);
BOOL WINAPI DllMain(HINSTANCE hDll, DWORD dwReason, LPVOID lpReserved) {
    return DllEntryPoint(reinterpret_cast<HINSTANCE>(hDll), dwReason, lpReserved);
}

// ============================================================
// AudioSubFilter implementation
// ============================================================

CUnknown* WINAPI AudioSubFilter::CreateInstance(LPUNKNOWN pUnk, HRESULT* phr) {
    AudioSubFilter* p = new AudioSubFilter(pUnk, phr);
    if (p == nullptr && phr) *phr = E_OUTOFMEMORY;
    return p;
}

AudioSubFilter::AudioSubFilter(LPUNKNOWN pUnk, HRESULT* phr)
    : CTransformFilter(NAME("Aqua Audio Sub"), pUnk, CLSID_AudioSubFilter)
{
    (void)phr;
    m_overlay.Init();
    m_worker = std::thread(&AudioSubFilter::ProcessingThread, this);
}

AudioSubFilter::~AudioSubFilter() {
    m_running = false;
    if (m_worker.joinable()) m_worker.join();
}

// ------------ CheckInputType: accept any standard audio ------------
HRESULT AudioSubFilter::CheckInputType(const CMediaType* mtIn) {
    if (!mtIn) return E_POINTER;
    if (mtIn->majortype == MEDIATYPE_Audio &&
        (mtIn->subtype   == MEDIASUBTYPE_PCM ||
         mtIn->formattype == FORMAT_WaveFormatEx))
        return S_OK;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

// ------------ CheckTransform: audio in → audio out (passthrough) ------------
HRESULT AudioSubFilter::CheckTransform(const CMediaType* mtIn,
                                       const CMediaType* mtOut) {
    if (!mtIn || !mtOut) return E_POINTER;
    if (mtIn->majortype == MEDIATYPE_Audio &&
        mtOut->majortype == MEDIATYPE_Audio)
        return S_OK;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

// ------------ DecideBufferSize: minimal 1-second buffer ------------
HRESULT AudioSubFilter::DecideBufferSize(IMemAllocator* pAlloc,
                                         ALLOCATOR_PROPERTIES* pprops) {
    if (!pAlloc || !pprops || !m_pInput || !m_pInput->IsConnected())
        return E_POINTER;

    CMediaType mt;
    m_pInput->ConnectionMediaType(&mt);
    WAVEFORMATEX* pwf = (WAVEFORMATEX*)mt.Format();
    long cb = pwf ? pwf->nAvgBytesPerSec : 176400;
    pprops->cBuffers = 4;
    pprops->cbBuffer = cb;
    ALLOCATOR_PROPERTIES actual;
    return pAlloc->SetProperties(pprops, &actual);
}

// ------------ GetMediaType: pass through the input type ------------
HRESULT AudioSubFilter::GetMediaType(int iPosition, CMediaType* pMediaType) {
    if (iPosition != 0) return VFW_S_NO_MORE_ITEMS;
    if (!m_pInput || !m_pInput->IsConnected()) return E_UNEXPECTED;
    return m_pInput->ConnectionMediaType(pMediaType);
}

// ------------ Transform: snapshot audio → queue → return immediately ------------
HRESULT AudioSubFilter::Transform(IMediaSample* pSample) {
    if (!pSample) return E_POINTER;

    BYTE* pData = nullptr;
    LONG  cbData = pSample->GetActualDataLength();
    if (FAILED(pSample->GetPointer(&pData)) || !pData || cbData <= 0)
        return S_FALSE;

    std::lock_guard<std::mutex> lock(m_queueMtx);
    if (m_audioQueue.size() < 50) {
        m_audioQueue.push(std::vector<char>(pData, pData + cbData));
    }
    return S_OK;   // passthrough — never block playback
}

// ------------ Background worker ------------
void AudioSubFilter::ProcessingThread() {
    while (m_running) {
        std::vector<char> chunk;
        {
            std::lock_guard<std::mutex> lock(m_queueMtx);
            if (!m_audioQueue.empty()) {
                chunk = std::move(m_audioQueue.front());
                m_audioQueue.pop();
            }
        }
        if (!chunk.empty()) {
            std::string result = m_client.ProcessAudioChunk(chunk);
            m_overlay.UpdateText(result);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(80));
    }
}

void AudioSubFilter::ShowConfig(HINSTANCE hInst, HWND hParent) {
    ShowConfigDialog(hInst, hParent);
}
