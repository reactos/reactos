/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS WDM Streaming ActiveMovie Proxy
 * FILE:            dll/directx/ksproxy/interface.cpp
 * PURPOSE:         IKsInterfaceHandler interface
 *
 * PROGRAMMERS:     Johannes Anderwald (janderwald@reactos.org)
 */
#include "precomp.h"

const GUID IID_IKsObject           = {0x423c13a2, 0x2070, 0x11d0, {0x9e, 0xf7, 0x00, 0xaa, 0x00, 0xa2, 0x16, 0xa1}};

class CKsInterfaceHandler : public IKsInterfaceHandler
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
    HRESULT STDMETHODCALLTYPE KsSetPin(IKsPin *KsPin);
    HRESULT STDMETHODCALLTYPE KsProcessMediaSamples(IKsDataTypeHandler *KsDataTypeHandler, IMediaSample** SampleList, PLONG SampleCount, KSIOOPERATION IoOperation, PKSSTREAM_SEGMENT *StreamSegment);
    HRESULT STDMETHODCALLTYPE KsCompleteIo(PKSSTREAM_SEGMENT StreamSegment);

    CKsInterfaceHandler() : m_Ref(0), m_Handle(NULL), m_Pin(0){};
    virtual ~CKsInterfaceHandler(){};

protected:
    LONG m_Ref;
    HANDLE m_Handle;
    IKsPinEx * m_Pin;
};

typedef struct
{
    KSSTREAM_SEGMENT StreamSegment;
    IMediaSample * MediaSample[64];

    ULONG SampleCount;
    ULONG ExtendedSize;
    PKSSTREAM_HEADER StreamHeader;
    OVERLAPPED Overlapped;
}KSSTREAM_SEGMENT_EXT, *PKSSTREAM_SEGMENT_EXT;


HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUID(refiid, IID_IUnknown) ||
        IsEqualGUID(refiid, IID_IKsInterfaceHandler))
    {
        *Output = PVOID(this);
        reinterpret_cast<IUnknown*>(*Output)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsSetPin(
    IKsPin *KsPin)
{
    HRESULT hr;
    IKsObject * KsObject;
    IKsPinEx * Pin;

    // get IKsPinEx interface
    hr = KsPin->QueryInterface(IID_IKsPinEx, (void**)&Pin);
    if (SUCCEEDED(hr))
    {
        // check if IKsObject is supported
        hr = KsPin->QueryInterface(IID_IKsObject, (void**)&KsObject);

        if (SUCCEEDED(hr))
        {
            // get pin handle
            m_Handle = KsObject->KsGetObjectHandle();

            // release IKsObject interface
            KsObject->Release();

            if (!m_Handle)
            {
                // expected a file handle
                hr = E_UNEXPECTED;
                Pin->Release();
            }
            else
            {
                if (m_Pin)
                {
                    // release old interface
                    m_Pin->Release();
                }
                m_Pin = Pin;
            }
        }
        else
        {
            //release IKsPinEx interface
            Pin->Release();
        }
    }

    // done
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsProcessMediaSamples(
     IKsDataTypeHandler *KsDataTypeHandler,
     IMediaSample** SampleList,
     PLONG SampleCount,
     KSIOOPERATION IoOperation,
     PKSSTREAM_SEGMENT *OutStreamSegment)
{
    PKSSTREAM_SEGMENT_EXT StreamSegment;
    ULONG ExtendedSize, Index, BytesReturned;
    HRESULT hr = S_OK;

    OutputDebugString("CKsInterfaceHandler::KsProcessMediaSamples\n");

    // sanity check
    assert(*SampleCount);

    if (*SampleCount == 0 || *SampleCount < 0)
        return E_FAIL;

    // zero stream segment
    *OutStreamSegment = NULL;

    // allocate stream segment
    StreamSegment = (PKSSTREAM_SEGMENT_EXT)CoTaskMemAlloc(sizeof(KSSTREAM_SEGMENT_EXT));
    if (!StreamSegment)
        return E_OUTOFMEMORY;

    // zero stream segment
    ZeroMemory(StreamSegment, sizeof(KSSTREAM_SEGMENT_EXT));

    //allocate event
    StreamSegment->StreamSegment.CompletionEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    if (!StreamSegment->StreamSegment.CompletionEvent)
    {
        // failed to create event
        CoTaskMemFree(StreamSegment);
        return MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, GetLastError());
    }

    // increase our own reference count
    AddRef();

    // setup stream segment
    StreamSegment->StreamSegment.KsDataTypeHandler = KsDataTypeHandler;
    StreamSegment->StreamSegment.KsInterfaceHandler = (IKsInterfaceHandler*)this;
    StreamSegment->StreamSegment.IoOperation = IoOperation;
    StreamSegment->Overlapped.hEvent = StreamSegment->StreamSegment.CompletionEvent;


    // ge extension size
    ExtendedSize = 0;
    if (KsDataTypeHandler)
    {
        // query extension size
        KsDataTypeHandler->KsQueryExtendedSize(&ExtendedSize);

        if (ExtendedSize)
        {
            // increment reference count
            KsDataTypeHandler->AddRef();
        }
        else
        {
            // no need for the datatype handler
            StreamSegment->StreamSegment.KsDataTypeHandler = NULL;
        }
    }

    StreamSegment->ExtendedSize = ExtendedSize;
    StreamSegment->SampleCount = (ULONG)*SampleCount;

    // calculate stream header size count
    ULONG StreamHeaderSize = StreamSegment->SampleCount * (sizeof(KSSTREAM_HEADER) + ExtendedSize);

    // allocate stream header
    StreamSegment->StreamHeader = (PKSSTREAM_HEADER)CoTaskMemAlloc(StreamHeaderSize);
    if (!StreamSegment->StreamHeader)
    {
        // not enough memory
        CloseHandle(StreamSegment->StreamSegment.CompletionEvent);

        if (StreamSegment->StreamSegment.KsDataTypeHandler)
            StreamSegment->StreamSegment.KsDataTypeHandler->Release();

        // free stream segment
        CoTaskMemFree(StreamSegment);

        //release our reference count
        Release();
        return E_OUTOFMEMORY;
    }

    // zero stream headers
    ZeroMemory(StreamSegment->StreamHeader, StreamHeaderSize);

    PKSSTREAM_HEADER CurStreamHeader = StreamSegment->StreamHeader;

    // initialize all stream headers
    for(Index = 0; Index < StreamSegment->SampleCount; Index++)
    {
         if (ExtendedSize)
         {
             // initialize extended size
             hr = KsDataTypeHandler->KsPrepareIoOperation(SampleList[Index], (CurStreamHeader + 1), IoOperation);
             // sanity check
             assert(hr == NOERROR);
         }

         // query for IMediaSample2 interface
         IMediaSample2 * MediaSample;
         AM_SAMPLE2_PROPERTIES Properties;

         hr = SampleList[Index]->QueryInterface(IID_IMediaSample2, (void**)&MediaSample);
         if (SUCCEEDED(hr))
         {
             //get properties

             hr = MediaSample->GetProperties(sizeof(AM_SAMPLE2_PROPERTIES), (BYTE*)&Properties);

             //release IMediaSample2 interface
             MediaSample->Release();
             if (FAILED(hr))
				 OutputDebugStringW(L"CKsInterfaceHandler::KsProcessMediaSamples MediaSample::GetProperties failed\n");
         }
         else
         {
				 OutputDebugStringW(L"CKsInterfaceHandler::KsProcessMediaSamples MediaSample:: only IMediaSample supported\n");
             // get properties
             hr = SampleList[Index]->GetPointer((BYTE**)&Properties.pbBuffer);
             assert(hr == NOERROR);
             hr = SampleList[Index]->GetTime(&Properties.tStart, &Properties.tStop);
             assert(hr == NOERROR);

             Properties.dwSampleFlags = 0;

             if (SampleList[Index]->IsDiscontinuity() == S_OK)
                 Properties.dwSampleFlags |= AM_SAMPLE_DATADISCONTINUITY;

             if (SampleList[Index]->IsPreroll() == S_OK)
                 Properties.dwSampleFlags |= AM_SAMPLE_PREROLL;

             if (SampleList[Index]->IsSyncPoint() == S_OK)
                 Properties.dwSampleFlags |= AM_SAMPLE_SPLICEPOINT;
         }

         WCHAR Buffer[100];
		 swprintf(Buffer, L"BufferLength %lu Property Buffer %p ExtendedSize %u lActual %u\n", Properties.cbBuffer, Properties.pbBuffer, ExtendedSize, Properties.lActual);
         OutputDebugStringW(Buffer);

         CurStreamHeader->Size = sizeof(KSSTREAM_HEADER) + ExtendedSize;
         CurStreamHeader->PresentationTime.Denominator = 1;
         CurStreamHeader->PresentationTime.Numerator = 1;
         CurStreamHeader->FrameExtent = Properties.cbBuffer;
         CurStreamHeader->Data = Properties.pbBuffer;

         if (IoOperation == KsIoOperation_Write)
         {
             // set flags
             CurStreamHeader->OptionsFlags = Properties.dwSampleFlags;
             CurStreamHeader->DataUsed = Properties.lActual;
             // increment reference count
             SampleList[Index]->AddRef();
         }

         // store sample in stream segment
         StreamSegment->MediaSample[Index] = SampleList[Index];

         // move to next header
         CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)CurStreamHeader + CurStreamHeader->Size);
    }

    // submit to device
    m_Pin->KsIncrementPendingIoCount();

    if (DeviceIoControl(m_Handle,
                        IoOperation == KsIoOperation_Write ? IOCTL_KS_WRITE_STREAM : IOCTL_KS_READ_STREAM,
                        NULL, 0,
                        StreamSegment->StreamHeader,
                        StreamHeaderSize,
                        &BytesReturned,
                        &StreamSegment->Overlapped))
    {
        // signal completion
        SetEvent(StreamSegment->StreamSegment.CompletionEvent);
        hr = S_OK;
        *OutStreamSegment = (PKSSTREAM_SEGMENT)StreamSegment;
    }
    else
    {
        if (GetLastError() == ERROR_IO_PENDING)
        {
            *OutStreamSegment = (PKSSTREAM_SEGMENT)StreamSegment;
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT
STDMETHODCALLTYPE
CKsInterfaceHandler::KsCompleteIo(
    PKSSTREAM_SEGMENT InStreamSegment)
{
    PKSSTREAM_SEGMENT_EXT StreamSegment;
    PKSSTREAM_HEADER CurStreamHeader;
    DWORD dwError = ERROR_SUCCESS, BytesReturned;
    BOOL bOverlapped;
    ULONG Index;
    HRESULT hr;
    IMediaSample2 * MediaSample;
    AM_SAMPLE2_PROPERTIES Properties;
    REFERENCE_TIME Start, Stop;

    OutputDebugStringW(L"CKsInterfaceHandler::KsCompleteIo\n");

    // get private stream segment
    StreamSegment = (PKSSTREAM_SEGMENT_EXT)InStreamSegment;

    // get result
    bOverlapped = GetOverlappedResult(m_Handle, &StreamSegment->Overlapped, &BytesReturned, FALSE);
    dwError = GetLastError();

    CurStreamHeader = StreamSegment->StreamHeader;

    //iterate through all stream headers
    for(Index = 0; Index < StreamSegment->SampleCount; Index++)
    {
        if (!bOverlapped)
        {
            // operation failed
            m_Pin->KsNotifyError(StreamSegment->MediaSample[Index], MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, dwError));
        }

        // query IMediaSample2 interface
        hr = StreamSegment->MediaSample[Index]->QueryInterface(IID_IMediaSample2, (void**)&MediaSample);
        if (SUCCEEDED(hr))
        {
            // media sample properties
            hr = MediaSample->GetProperties(sizeof(AM_SAMPLE2_PROPERTIES), (BYTE*)&Properties);
            if (SUCCEEDED(hr))
            {
                //update media sample properties
                Properties.dwTypeSpecificFlags = CurStreamHeader->TypeSpecificFlags;
                Properties.dwSampleFlags |= (CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEDISCONTINUITY);

                MediaSample->SetProperties(sizeof(AM_SAMPLE2_PROPERTIES), (BYTE*)&Properties);
            }
            // release IMediaSample2 interface
            MediaSample->Release();
        }

        // was an extended header used
        if (StreamSegment->ExtendedSize)
        {
            // unprepare stream header extension
            StreamSegment->StreamSegment.KsDataTypeHandler->KsCompleteIoOperation(StreamSegment->MediaSample[Index], (CurStreamHeader + 1), StreamSegment->StreamSegment.IoOperation, bOverlapped == FALSE);
        }

        Start = 0;
        Stop = 0;
        if (bOverlapped && StreamSegment->StreamSegment.IoOperation == KsIoOperation_Read)
        {
            // update common media sample details
            StreamSegment->MediaSample[Index]->SetSyncPoint((CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_SPLICEPOINT));
            StreamSegment->MediaSample[Index]->SetPreroll((CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_PREROLL));
            StreamSegment->MediaSample[Index]->SetDiscontinuity((CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY));

            if (CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_TIMEVALID)
            {
                // use valid timestamp
                Start = CurStreamHeader->PresentationTime.Time;

                if (CurStreamHeader->OptionsFlags & KSSTREAM_HEADER_OPTIONSF_DURATIONVALID)
                {
                    Stop = CurStreamHeader->PresentationTime.Time + CurStreamHeader->Duration;
                }
            }
        }

        // now set time
        hr = StreamSegment->MediaSample[Index]->SetTime(&Start, &Stop);
        if (FAILED(hr))
        {
            // use start time
            StreamSegment->MediaSample[Index]->SetTime(&Start, &Start);
        }

        // set valid data length
        StreamSegment->MediaSample[Index]->SetActualDataLength(CurStreamHeader->DataUsed);

        if (StreamSegment->StreamSegment.IoOperation == KsIoOperation_Read)
        {
            if (bOverlapped)
            {
                // deliver sample
                m_Pin->KsDeliver(StreamSegment->MediaSample[Index], CurStreamHeader->OptionsFlags);
            }
        }
        else if (StreamSegment->StreamSegment.IoOperation == KsIoOperation_Write)
        {
            // release media sample reference
            StreamSegment->MediaSample[Index]->Release();
        }

        CurStreamHeader = (PKSSTREAM_HEADER)((ULONG_PTR)CurStreamHeader + CurStreamHeader->Size);
    }

    // delete stream headers
    CoTaskMemFree(StreamSegment->StreamHeader);

    if (StreamSegment->StreamSegment.KsDataTypeHandler)
    {
        // release reference
        StreamSegment->StreamSegment.KsDataTypeHandler->Release();
    }

    // decrement pending i/o count
    m_Pin->KsDecrementPendingIoCount();

    //notify of completion
    m_Pin->KsMediaSamplesCompleted(InStreamSegment);

    //destroy stream segment
    CoTaskMemFree(StreamSegment);

    //release reference to ourselves
    Release();

    // done
    // Event handle is closed by caller
    return S_OK;
}

HRESULT
WINAPI
CKsInterfaceHandler_Constructor(
    IUnknown * pUnkOuter,
    REFIID riid,
    LPVOID * ppv)
{
    OutputDebugStringW(L"CKsInterfaceHandler_Constructor\n");

    CKsInterfaceHandler * handler = new CKsInterfaceHandler();

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
