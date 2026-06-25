#include <initguid.h>
#include "MainFilter.h"
#include "PluginConfig.h"
#include "ConfigDialog.h"
#include "cprop.h"
#include <mmreg.h>

// ============================================================
// DirectShow filter registration data
// ============================================================

// Accept any audio type
const AMOVIESETUP_MEDIATYPE sudAudioType = {
    &MEDIATYPE_Audio,
    nullptr             // any audio subtype
};

const AMOVIESETUP_PIN sudPins[] = {
    {
        L"Input",             // pin name
        FALSE,                // rendered
        FALSE,                // output
        FALSE,                // zero instances
        FALSE,                // multiple instances
        nullptr,              // connects to any filter
        nullptr,              // connects to any pin
        1,                    // number of media types
        &sudAudioType         // media types
    },
    {
        L"Output",
        FALSE,                // rendered
        TRUE,                 // IS output
        FALSE,
        FALSE,
        nullptr,
        nullptr,
        1,
        &sudAudioType
    }
};

const AMOVIESETUP_FILTER sudFilter = {
    &CLSID_AudioSubFilter,        // CLSID
    L"Aqua Audio Sub Filter",     // friendly name
    MERIT_DO_NOT_USE + 1,         // merit: won't auto-insert, but visible in list
    2,                            // number of pins
    sudPins                       // pin descriptors
};

// ============================================================
// Property page –  pops up the config dialog when the user
// double-clicks the filter in MPC-BE
// ============================================================

class AudioSubPropPage : public CBasePropertyPage {
public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr) {
        AudioSubPropPage* p = new AudioSubPropPage(pUnk, phr);
        if (p == nullptr && phr) *phr = E_OUTOFMEMORY;
        return p;
    }

    AudioSubPropPage(LPUNKNOWN pUnk, HRESULT* phr)
        : CBasePropertyPage(NAME("Aqua Audio Sub PropPage"), pUnk, 0, 0)
    {
        (void)phr;
    }

    // When the property page is activated, show our config dialog
    HRESULT OnActivate() override {
        HWND hParent = m_hwnd;
        HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
            GetWindowLongPtr(hParent, GWLP_HINSTANCE));
        ShowConfigDialog(hInst, hParent);
        return S_OK;
    }
};

// ============================================================
// COM / DirectShow plumbing
// ============================================================

CFactoryTemplate g_Templates[] = {
    {   // Filter
        L"Aqua Audio Sub Filter",
        &CLSID_AudioSubFilter,
        AudioSubFilter::CreateInstance,
        nullptr,
        &sudFilter
    },
    {   // Property page
        L"Aqua Audio Sub PropPage",
        &CLSID_AudioSubPropPage,
        AudioSubPropPage::CreateInstance,
        nullptr,
        nullptr
    }
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

// ---- ISpecifyPropertyPages ----

STDMETHODIMP AudioSubFilter::GetPages(CAUUID* pPages) {
    if (!pPages) return E_POINTER;
    pPages->cElems = 1;
    pPages->pElems = static_cast<GUID*>(CoTaskMemAlloc(sizeof(GUID)));
    if (!pPages->pElems) return E_OUTOFMEMORY;
    *pPages->pElems = CLSID_AudioSubPropPage;
    return S_OK;
}

// ---- NonDelegatingQueryInterface: expose ISpecifyPropertyPages ----

STDMETHODIMP AudioSubFilter::NonDelegatingQueryInterface(REFIID riid, void** ppv) {
    if (riid == IID_ISpecifyPropertyPages) {
        return GetInterface(static_cast<ISpecifyPropertyPages*>(this), ppv);
    }
    return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

// ---- Filter overrides ----

HRESULT AudioSubFilter::CheckInputType(const CMediaType* mtIn) {
    if (!mtIn) return E_POINTER;
    if (mtIn->majortype == MEDIATYPE_Audio &&
        (mtIn->subtype   == MEDIASUBTYPE_PCM ||
         mtIn->formattype == FORMAT_WaveFormatEx))
        return S_OK;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

HRESULT AudioSubFilter::CheckTransform(const CMediaType* mtIn,
                                       const CMediaType* mtOut) {
    if (!mtIn || !mtOut) return E_POINTER;
    if (mtIn->majortype == MEDIATYPE_Audio &&
        mtOut->majortype == MEDIATYPE_Audio)
        return S_OK;
    return VFW_E_TYPE_NOT_ACCEPTED;
}

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

HRESULT AudioSubFilter::GetMediaType(int iPosition, CMediaType* pMediaType) {
    if (iPosition != 0) return VFW_S_NO_MORE_ITEMS;
    if (!m_pInput || !m_pInput->IsConnected()) return E_UNEXPECTED;
    return m_pInput->ConnectionMediaType(pMediaType);
}

HRESULT AudioSubFilter::Transform(IMediaSample* pIn, IMediaSample* pOut) {
    if (!pIn || !pOut) return E_POINTER;

    BYTE* pSrc = nullptr;
    BYTE* pDst = nullptr;
    LONG  cbData = pIn->GetActualDataLength();
    HRESULT hr = pIn->GetPointer(&pSrc);
    if (FAILED(hr) || !pSrc) return S_FALSE;
    hr = pOut->GetPointer(&pDst);
    if (FAILED(hr) || !pDst) return S_FALSE;
    CopyMemory(pDst, pSrc, cbData);
    pOut->SetActualDataLength(cbData);

    REFERENCE_TIME tStart, tEnd;
    if (SUCCEEDED(pIn->GetTime(&tStart, &tEnd)))
        pOut->SetTime(&tStart, &tEnd);
    pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
    pOut->SetPreroll(pIn->IsPreroll() == S_OK);
    pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);

    {
        std::lock_guard<std::mutex> lock(m_queueMtx);
        if (m_audioQueue.size() < 50) {
            m_audioQueue.push(std::vector<char>(pSrc, pSrc + cbData));
        }
    }
    return S_OK;
}

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
