#include <initguid.h>
#include "MainFilter.h"
#include "PluginConfig.h"
#include "ConfigDialog.h"
#include <mmreg.h>

// DLL's own module handle — saved during DllMain so we can load
// resources (dialog template, etc.) from the correct module.
static HINSTANCE g_hDllInst = nullptr;

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
// Minimal IPropertyPage —  shows our config dialog on Activate
// ============================================================

class AudioSubPropPage : public IPropertyPage {
    volatile LONG m_cRef = 1;
    IUnknown*     m_pFilter = nullptr;

public:
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN pUnk, HRESULT* phr) {
        auto* p = new AudioSubPropPage();
        if (!p) { if (phr) *phr = E_OUTOFMEMORY; return nullptr; }
        (void)pUnk; (void)phr;
        return reinterpret_cast<CUnknown*>(p);
    }

    // ---- IUnknown ----
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IPropertyPage)
            *ppv = static_cast<IPropertyPage*>(this);
        else { *ppv = nullptr; return E_NOINTERFACE; }
        AddRef();
        return S_OK;
    }
    STDMETHODIMP_(ULONG) AddRef() override {
        return InterlockedIncrement(&m_cRef);
    }
    STDMETHODIMP_(ULONG) Release() override {
        ULONG c = InterlockedDecrement(&m_cRef);
        if (c == 0) delete this;
        return c;
    }

    // ---- IPropertyPage ----
    STDMETHODIMP SetPageSite(IPropertyPageSite*) override { return S_OK; }
    STDMETHODIMP SetObjects(ULONG c, IUnknown** pp) override {
        if (c >= 1 && pp[0]) m_pFilter = pp[0];
        return S_OK;
    }
    STDMETHODIMP Activate(HWND hParent, LPCRECT, BOOL) override {
        ShowConfigDialog(g_hDllInst, hParent);
        return S_OK;
    }
    STDMETHODIMP Deactivate() override { return S_OK; }
    STDMETHODIMP Show(UINT) override { return S_OK; }
    STDMETHODIMP Move(LPCRECT) override { return S_OK; }
    STDMETHODIMP GetPageInfo(LPPROPPAGEINFO pInfo) override {
        if (!pInfo) return E_POINTER;
        static const WCHAR title[] = L"Aqua Audio Sub Settings";
        pInfo->size.cx = 520;
        pInfo->size.cy = 480;
        pInfo->pszTitle = static_cast<LPOLESTR>(CoTaskMemAlloc(sizeof(title)));
        if (pInfo->pszTitle) memcpy(pInfo->pszTitle, title, sizeof(title));
        pInfo->pszDocString = nullptr;
        pInfo->pszHelpFile  = nullptr;
        pInfo->dwHelpContext = 0;
        return S_OK;
    }
    STDMETHODIMP Apply() override { return S_OK; }
    STDMETHODIMP Help(LPCWSTR) override { return E_NOTIMPL; }
    STDMETHODIMP TranslateAccelerator(LPMSG) override { return E_NOTIMPL; }
    STDMETHODIMP IsPageDirty() override { return S_FALSE; }
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
    if (dwReason == DLL_PROCESS_ATTACH)
        g_hDllInst = hDll;
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
         mtIn->formattype == FORMAT_WaveFormatEx)) {
        // Capture audio format for WAV header generation
        if (mtIn->pbFormat && mtIn->cbFormat >= sizeof(WAVEFORMATEX)) {
            std::lock_guard<std::mutex> lock(m_wfexMtx);
            memcpy(&m_wfex, mtIn->pbFormat, sizeof(WAVEFORMATEX));
            m_wfexValid = true;
        }
        return S_OK;
    }
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

HRESULT AudioSubFilter::StartStreaming() {
    // Re-capture WAVEFORMATEX from the now-connected input pin
    if (m_pInput && m_pInput->IsConnected()) {
        CMediaType mt;
        if (SUCCEEDED(m_pInput->ConnectionMediaType(&mt)) &&
            mt.pbFormat && mt.cbFormat >= sizeof(WAVEFORMATEX)) {
            std::lock_guard<std::mutex> lock(m_wfexMtx);
            memcpy(&m_wfex, mt.pbFormat, sizeof(WAVEFORMATEX));
            m_wfexValid = true;
        }
    }
    return CTransformFilter::StartStreaming();
}

void AudioSubFilter::ProcessingThread() {
    // Accumulate ~3 seconds of audio before sending
    DWORD bytesPerSec = 0;
    {
        std::lock_guard<std::mutex> lock(m_wfexMtx);
        if (m_wfexValid) bytesPerSec = m_wfex.nAvgBytesPerSec;
    }
    if (bytesPerSec == 0) bytesPerSec = 176400; // fallback: 44100×16×2

    const DWORD targetBytes = bytesPerSec * 3; // 3 seconds
    std::vector<char> accumulator;
    accumulator.reserve(targetBytes);

    while (m_running) {
        // Drain the queue into accumulator
        {
            std::lock_guard<std::mutex> lock(m_queueMtx);
            while (!m_audioQueue.empty()) {
                auto& chunk = m_audioQueue.front();
                accumulator.insert(accumulator.end(), chunk.begin(), chunk.end());
                m_audioQueue.pop();
            }
        }

        // If we have enough audio, send it
        if (accumulator.size() >= targetBytes) {
            std::string result = m_client.ProcessAudioChunk(accumulator, m_wfex, m_wfexValid);
            m_overlay.UpdateText(result);
            accumulator.clear();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void AudioSubFilter::ShowConfig(HINSTANCE hInst, HWND hParent) {
    ShowConfigDialog(hInst, hParent);
}
