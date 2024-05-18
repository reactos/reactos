/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Network Provider for MPEG2 based networks
 * FILE:            dll/directx/msdvbnp/scanningtuner.cpp
 * PURPOSE:         IScanningTunner interface
 *
 * PROGRAMMERS:     Johannes Anderwald (johannes.anderwald@reactos.org)
 */
#include "precomp.h"

class CScanningTunner : public IScanningTuner
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);
        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }

    //ITuner methods
    HRESULT STDMETHODCALLTYPE get_TuningSpace(ITuningSpace **TuningSpace);
    HRESULT STDMETHODCALLTYPE put_TuningSpace(ITuningSpace *TuningSpace);
    HRESULT STDMETHODCALLTYPE EnumTuningSpaces(IEnumTuningSpaces **ppEnum);
    HRESULT STDMETHODCALLTYPE get_TuneRequest(ITuneRequest **TuneRequest);
    HRESULT STDMETHODCALLTYPE put_TuneRequest(ITuneRequest *TuneRequest);
    HRESULT STDMETHODCALLTYPE Validate(ITuneRequest *TuneRequest);
    HRESULT STDMETHODCALLTYPE get_PreferredComponentTypes(IComponentTypes **ComponentTypes);
    HRESULT STDMETHODCALLTYPE put_PreferredComponentTypes(IComponentTypes *ComponentTypes);
    HRESULT STDMETHODCALLTYPE get_SignalStrength(long *Strength);
    HRESULT STDMETHODCALLTYPE TriggerSignalEvents(long Interval);

    //IScanningTuner methods
    HRESULT STDMETHODCALLTYPE SeekUp();
    HRESULT STDMETHODCALLTYPE SeekDown();
    HRESULT STDMETHODCALLTYPE ScanUp(long MillisecondsPause);
    HRESULT STDMETHODCALLTYPE ScanDown(long MillisecondsPause);
    HRESULT STDMETHODCALLTYPE AutoProgram();

    CScanningTunner(std::vector<IUnknown*> & DeviceFilters) : m_Ref(0), m_TuningSpace(0), m_DeviceFilters(DeviceFilters){};
    virtual ~CScanningTunner() {};
    HRESULT STDMETHODCALLTYPE StartChanges();
    HRESULT STDMETHODCALLTYPE CommitChanges();
    HRESULT STDMETHODCALLTYPE CheckChanges();
    HRESULT STDMETHODCALLTYPE SetLnbInfo(IBDA_LNBInfo * pLnbInfo, ULONG ulLOFLow, ULONG ulLOFHigh, ULONG ulSwitchFrequency);
    HRESULT STDMETHODCALLTYPE SetDigitalDemodulator(IBDA_DigitalDemodulator * pDigitalDemo, ModulationType ModType, FECMethod InnerFEC, BinaryConvolutionCodeRate InnerFECRate, FECMethod OuterFEC, BinaryConvolutionCodeRate OuterFECRate, ULONG SymbolRate);
    HRESULT SetFrequency(IBDA_FrequencyFilter * pFrequency, ULONG FrequencyMultiplier, ULONG Frequency, Polarisation Polarity, ULONG Range, ULONG Bandwidth);
    HRESULT STDMETHODCALLTYPE performDVBTTune(IDVBTuneRequest * pDVBTRequest, IDVBTLocator *pDVBTLocator);
protected:
    LONG m_Ref;
    ITuningSpace * m_TuningSpace;
    std::vector<IUnknown*> & m_DeviceFilters;
};

HRESULT
STDMETHODCALLTYPE
CScanningTunner::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    if (IsEqualGUID(refiid, IID_ITuner))
    {
        *Output = (ITuner*)(this);
        reinterpret_cast<ITuner*>(*Output)->AddRef();
        return NOERROR;
    }

    if (IsEqualGUID(refiid, IID_IScanningTuner))
    {
        *Output = (IScanningTuner*)(this);
        reinterpret_cast<IScanningTuner*>(*Output)->AddRef();
        return NOERROR;
    }

    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(refiid, &lpstr);
    swprintf(Buffer, L"CScanningTunner::QueryInterface: NoInterface for %s\n", lpstr);
    OutputDebugStringW(Buffer);
    CoTaskMemFree(lpstr);


    return E_NOINTERFACE;
}

//-------------------------------------------------------------------
//ITuner
//
HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_TuningSpace(
    ITuningSpace **TuningSpace)
{
    OutputDebugStringW(L"CScanningTunner::get_TuningSpace\n");

    *TuningSpace = m_TuningSpace;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_TuningSpace(
    ITuningSpace *TuningSpace)
{
    OutputDebugStringW(L"CScanningTunner::put_TuningSpace\n");
    m_TuningSpace = TuningSpace;
    return S_OK;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::EnumTuningSpaces(
    IEnumTuningSpaces **ppEnum)
{
    OutputDebugStringW(L"CScanningTunner::EnumTuningSpaces : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_TuneRequest(
    ITuneRequest **TuneRequest)
{
    OutputDebugStringW(L"CScanningTunner::get_TuneRequest : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_TuneRequest(
    ITuneRequest *TuneRequest)
{
    IDVBTuneRequest * pDVBTRequest;
    ILocator *pLocator;
    IDVBTLocator *pDVBTLocator;
    HRESULT hr;


    OutputDebugStringW(L"CScanningTunner::put_TuneRequest\n");

    // query for IDVBTuneRequest interface
    hr = TuneRequest->QueryInterface(IID_IDVBTuneRequest, (void**)&pDVBTRequest);

    // sanity check
    assert(hr == NOERROR);

    // get the IDVBTLocator
    hr = pDVBTRequest->get_Locator((ILocator**)&pLocator);

    // sanity check
    assert(hr == NOERROR);
    assert(pLocator);

    hr = pLocator->QueryInterface(IID_ILocator, (void**)&pDVBTLocator);

    // sanity check
    assert(hr == NOERROR);


    StartChanges();
    CommitChanges();
    StartChanges();

    hr = performDVBTTune(pDVBTRequest, pDVBTLocator);


    pDVBTLocator->Release();
    pDVBTRequest->Release();

    CheckChanges();
    CommitChanges();
    StartChanges();

    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::Validate(
    ITuneRequest *TuneRequest)
{
    OutputDebugStringW(L"CScanningTunner::Validate : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_PreferredComponentTypes(
    IComponentTypes **ComponentTypes)
{
    OutputDebugStringW(L"CScanningTunner::get_PreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::put_PreferredComponentTypes(
    IComponentTypes *ComponentTypes)
{
    OutputDebugStringW(L"CScanningTunner::put_PreferredComponentTypes : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::get_SignalStrength(
    long *Strength)
{
    OutputDebugStringW(L"CScanningTunner::get_SignalStrength : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::TriggerSignalEvents(
    long Interval)
{
    OutputDebugStringW(L"CScanningTunner::TriggerSignalEvents : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
//IScanningTuner
HRESULT
STDMETHODCALLTYPE
CScanningTunner::SeekUp()
{
    OutputDebugStringW(L"CScanningTunner::SeekUp : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::SeekDown()
{
    OutputDebugStringW(L"CScanningTunner::SeekDown : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::ScanUp(
    long MillisecondsPause)
{
    OutputDebugStringW(L"CScanningTunner::ScanUp : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::ScanDown(
    long MillisecondsPause)
{
    OutputDebugStringW(L"CScanningTunner::ScanDown : NotImplemented\n");
    return E_NOTIMPL;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::AutoProgram()
{
    OutputDebugStringW(L"CScanningTunner::AutoProgram : NotImplemented\n");
    return E_NOTIMPL;
}

//-------------------------------------------------------------------
HRESULT
STDMETHODCALLTYPE
CScanningTunner::performDVBTTune(
    IDVBTuneRequest * pDVBTRequest,
    IDVBTLocator *pDVBTLocator)
{
    HRESULT hr = S_OK;
    ULONG Index;
    IBDA_Topology *pTopo;
    IUnknown *pNode;
    IBDA_FrequencyFilter * pFrequency;
    IBDA_LNBInfo * pLnbInfo;
    IBDA_DigitalDemodulator *pDigitalDemo;
    LONG BandWidth;
    LONG Frequency;
    LONG SymbolRate;
    FECMethod InnerFEC, OuterFEC;
    BinaryConvolutionCodeRate InnerFECRate, OuterFECRate;
    ModulationType Modulation;

    pDVBTLocator->get_Bandwidth(&BandWidth);
    pDVBTLocator->get_CarrierFrequency(&Frequency);
    pDVBTLocator->get_InnerFEC(&InnerFEC);
    pDVBTLocator->get_InnerFECRate(&InnerFECRate);
    pDVBTLocator->get_Modulation(&Modulation);
    pDVBTLocator->get_OuterFEC(&OuterFEC);
    pDVBTLocator->get_OuterFECRate(&OuterFECRate);
    pDVBTLocator->get_SymbolRate(&SymbolRate);


    WCHAR Buffer[1000];
    swprintf(Buffer, L"BandWidth %lu Frequency %lu Rate %lu InnerFEC %ld OuterFEC %ld InnerFECRate %ld OuterFECRate %ld Modulation %lu\n",
             BandWidth, Frequency, SymbolRate, InnerFEC, OuterFEC, InnerFECRate, OuterFECRate, Modulation);

    OutputDebugStringW(Buffer);



    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get device filter
        IUnknown * pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        hr = pFilter->QueryInterface(IID_IBDA_Topology, (void**)&pTopo);
        // sanity check
        assert(hr == NOERROR);

        pNode = NULL;
        hr = pTopo->GetControlNode(0, 1, 0, &pNode); //HACK

        WCHAR Buffer[100];
        swprintf(Buffer, L"CScanningTunner::performDVBTTune GetControlNode %lx\n", hr);
        OutputDebugStringW(Buffer);

        if (FAILED(hr))
            continue;

        // sanity check
        assert(hr == NOERROR);
        assert(pNode);

        hr = pNode->QueryInterface(IID_IBDA_FrequencyFilter, (void**)&pFrequency);

        swprintf(Buffer, L"CScanningTunner::performDVBTTune IID_IBDA_FrequencyFilter hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        // sanity check
        assert(hr == NOERROR);

        hr = SetFrequency(pFrequency, 1000 /* FIXME */, Frequency, BDA_POLARISATION_NOT_DEFINED /* FIXME */, BDA_RANGE_NOT_SET /* FIXME */, BandWidth);

        swprintf(Buffer, L"CScanningTunner::performDVBTTune SetFrequency hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        //sanity check
        assert(hr == NOERROR);

        // release interface
        pFrequency->Release();


        hr = pNode->QueryInterface(IID_IBDA_LNBInfo, (void**)&pLnbInfo);

        swprintf(Buffer, L"CScanningTunner::performDVBTTune IID_IBDA_LNBInfo hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        // sanity check
        assert(hr == NOERROR);

        hr = SetLnbInfo(pLnbInfo, ULONG_MAX /* FIXME */, ULONG_MAX /* FIXME*/, ULONG_MAX /*FIXME*/);


        swprintf(Buffer, L"CScanningTunner::performDVBTTune SetLnbInfo hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        // sanity check
        assert(hr == NOERROR);

        // release interface
        pLnbInfo->Release();

        hr = pNode->QueryInterface(IID_IBDA_DigitalDemodulator, (void**)&pDigitalDemo);

        swprintf(Buffer, L"CScanningTunner::performDVBTTune IID_IBDA_DigitalDemodulator hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        // sanity check
        assert(hr == NOERROR);

        hr = SetDigitalDemodulator(pDigitalDemo, Modulation, InnerFEC, InnerFECRate, OuterFEC, OuterFECRate, SymbolRate);

        swprintf(Buffer, L"CScanningTunner::performDVBTTune SetDigitalDemodulator hr %lx\n", hr);
        OutputDebugStringW(Buffer);

        // sanity check
        assert(hr == NOERROR);

        // release interface
        pDigitalDemo->Release();

        // release control node
        pNode->Release();

        // release IBDA_Topology;
        pTopo->Release();

    }
    return hr;
}



HRESULT
STDMETHODCALLTYPE
CScanningTunner::CheckChanges()
{
    ULONG Index;
    HRESULT hResult = NOERROR;
    IBDA_DeviceControl * pDeviceControl;

    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get filter
        IUnknown * pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        // query for IBDA_DeviceControl interface
        hResult = pFilter->QueryInterface(IID_IBDA_DeviceControl, (void**)&pDeviceControl);

        // sanity check
        assert(hResult == NOERROR);

        //start changes
        hResult = pDeviceControl->CheckChanges();

        // fix for unimplemented
        if (hResult == E_NOTIMPL)
            hResult = NOERROR;

        // release interface
        pDeviceControl->Release();

        if (FAILED(hResult))
        {
            //shouldnt happen
            break;
        }
    }
    // done
    return hResult;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::CommitChanges()
{
    ULONG Index;
    HRESULT hResult = NOERROR;
    IBDA_DeviceControl * pDeviceControl;

    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get filter
        IUnknown * pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        // query for IBDA_DeviceControl interface
        HRESULT hr = pFilter->QueryInterface(IID_IBDA_DeviceControl, (void**)&pDeviceControl);

        // sanity check
        assert(hr == NOERROR);

        //start changes
        hr = pDeviceControl->CommitChanges();

        // fix for unimplemented
        if (hr == E_NOTIMPL)
            hr = NOERROR;

        if (FAILED(hr))
        {
            pDeviceControl->StartChanges();
            pDeviceControl->CommitChanges();
            hResult = E_UNEXPECTED;
        }

        // release interface
        pDeviceControl->Release();

    }

    //done
    return hResult;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::StartChanges()
{
    ULONG Index;
    IBDA_DeviceControl * pDeviceControl;

    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get filter
        IUnknown * pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        // query for IBDA_DeviceControl interface
        HRESULT hr = pFilter->QueryInterface(IID_IBDA_DeviceControl, (void**)&pDeviceControl);

        // sanity check
        assert(hr == NOERROR);

        //start changes
        hr = pDeviceControl->StartChanges();

        // release interface
        pDeviceControl->Release();

        // fix for unimplemented
        if (hr == E_NOTIMPL)
            hr = NOERROR;

        if (FAILED(hr))
            return hr;

    }

    // now commit the changes
    for(Index = 0; Index < m_DeviceFilters.size(); Index++)
    {
        // get filter
        IUnknown * pFilter = m_DeviceFilters[Index];

        if (!pFilter)
            continue;

        // query for IBDA_DeviceControl interface
        HRESULT hr = pFilter->QueryInterface(IID_IBDA_DeviceControl, (void**)&pDeviceControl);

        // sanity check
        assert(hr == NOERROR);

        hr = pDeviceControl->CommitChanges();

        // release interface
        pDeviceControl->Release();
    }

    // done
    return NOERROR;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::SetLnbInfo(
    IBDA_LNBInfo * pLnbInfo,
    ULONG ulLOFLow,
    ULONG ulLOFHigh,
    ULONG ulSwitchFrequency)
{
    HRESULT hr;

    hr = pLnbInfo->put_LocalOscillatorFrequencyLowBand(ulLOFLow);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;

    if (FAILED(hr))
        return hr;

    hr = pLnbInfo->put_LocalOscillatorFrequencyHighBand(ulLOFHigh);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;

    if (FAILED(hr))
        return hr;

    hr = pLnbInfo->put_HighLowSwitchFrequency(ulSwitchFrequency);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;

    return hr;
}

HRESULT
CScanningTunner::SetFrequency(
    IBDA_FrequencyFilter * pFrequency,
    ULONG FrequencyMultiplier,
    ULONG Frequency,
    Polarisation Polarity,
    ULONG Range,
    ULONG Bandwidth)
{
    HRESULT hr;

    hr = pFrequency->put_FrequencyMultiplier(FrequencyMultiplier);
    if (FAILED(hr))
        return hr;

    hr = pFrequency->put_Frequency(Frequency);
    if (FAILED(hr))
        return hr;

    hr = pFrequency->put_Polarity(Polarity);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND))
        hr = NOERROR;

    if (FAILED(hr))
        return hr;

    hr = pFrequency->put_Range(Range);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND))
        hr = NOERROR;

    if (FAILED(hr))
        return hr;

    hr = pFrequency->put_Bandwidth(Bandwidth);
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CScanningTunner::SetDigitalDemodulator(
    IBDA_DigitalDemodulator * pDigitalDemo,
    ModulationType ModType,
    FECMethod InnerFEC,
    BinaryConvolutionCodeRate InnerFECRate,
    FECMethod OuterFEC,
    BinaryConvolutionCodeRate OuterFECRate,
    ULONG SymbolRate)
{
    HRESULT hr;

    hr = pDigitalDemo->put_ModulationType(&ModType);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;

    if (FAILED(hr))
        return hr;

    hr = pDigitalDemo->put_InnerFECMethod(&InnerFEC);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;
    if (FAILED(hr))
        return hr;

    hr = pDigitalDemo->put_InnerFECRate(&InnerFECRate);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;
    if (FAILED(hr))
        return hr;

    hr = pDigitalDemo->put_OuterFECMethod(&OuterFEC);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;
    if (FAILED(hr))
        return hr;

    hr = pDigitalDemo->put_OuterFECRate(&OuterFECRate);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;
    if (FAILED(hr))
        return hr;

    hr = pDigitalDemo->put_SymbolRate(&SymbolRate);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;
    if (FAILED(hr))
        return hr;

    SpectralInversion Inversion = BDA_SPECTRAL_INVERSION_NOT_DEFINED;
    hr = pDigitalDemo->put_SpectralInversion(&Inversion);
    if (hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_NOT_FOUND) || hr == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, ERROR_SET_NOT_FOUND))
        hr = NOERROR;

    return hr;
}


HRESULT
WINAPI
CScanningTunner_fnConstructor(
    std::vector<IUnknown*> & DeviceFilter,
    REFIID riid,
    LPVOID * ppv)
{
    CScanningTunner * handler = new CScanningTunner(DeviceFilter);

#ifdef MSDVBNP_TRACE
    WCHAR Buffer[MAX_PATH];
    LPOLESTR lpstr;
    StringFromCLSID(riid, &lpstr);
    swprintf(Buffer, L"CScanningTunner_fnConstructor riid %s\n", lpstr);
    OutputDebugStringW(Buffer);
#endif

    if (!handler)
        return E_OUTOFMEMORY;

    if (FAILED(handler->QueryInterface(riid, ppv)))
    {
        /* not supported */
        delete handler;
        return E_NOINTERFACE;
    }

    return NOERROR;
}
