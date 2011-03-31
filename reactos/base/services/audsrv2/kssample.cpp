#include "kssample.h"
#include "resource.h"
#include "RedirectConsole.h"
// - macros -------------------------------------------------------------
#define ThrowOnFail(hr, strErr)     if (!SUCCEEDED((hr))) throw (strErr) 
#define ThrowOnNull(p, strErr)      if (NULL == p) throw (strErr) 

#define Zero(s)                     ZeroMemory(&s, sizeof(s))

// this defines how many data packets we will use to stream data to the pin
#define cPackets    4

// - enums, structs, etc. -----------------------------------------------
enum {eAbort, eStop, ePause, eRun };

struct DATA_PACKET
{
    KSSTREAM_HEADER Header;
    OVERLAPPED      Signal;
};


// - forward declarations -----------------------------------------------
HANDLE              CreateRenderThread(HANDLE* hControlEvents);


int _tWinMainmhs()
{
char command =0;
HANDLE audioslave;
HANDLE pControls[4];
pControls[0]=CreateEvent(NULL,FALSE,FALSE,NULL);
pControls[1]=CreateEvent(NULL,FALSE,FALSE,NULL);
pControls[2]=CreateEvent(NULL,FALSE,FALSE,NULL);
pControls[3]=CreateEvent(NULL,FALSE,FALSE,NULL);
RedirectIOToConsole();
audioslave=CreateRenderThread(pControls);
printf("ReactOS Audio Mixer Project --Pankaj Yadav \n press R-Run P-Pause S-Stop A-Abort \n");
while(command!='a')
	{
		scanf("%c",&command);
		switch(command)
		{
		case 'a':
			SetEvent(pControls[0]);
			break;
		case 's':
			SetEvent(pControls[1]);
			break;
		case 'p':
			SetEvent(pControls[2]);
			break;
		case 'r':
			SetEvent(pControls[3]);
			break;
		}
	}
WaitForSingleObject(audioslave,INFINITE);
	// Perform application initialization:
	/*if (!InitInstance(hInstance, nCmdShow)) 
	{
		return FALSE;
	}*/

	// Main message loop:
	/*while (GetMessage(&msg, NULL, 0, 0)) 
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int) msg.wParam;*/
return 0;
}


// ----------------------------------------------------------------------
//  FUNCTION: RenderThreadProc
//
//  PURPOSE:  This is the proc for the thread that does all of the streaming.
//  More specifically, it:
//      1) enumerates and instantiates a filter and pin capable of rendering
//         48KHz, 16bit, stereo PCM audio data
//      2) sends audio data to the pin
//      3) responds to packet completion and UI thread events
//
//  The thread exits if anything throws an exception or if the "Abort" event
//  is set to the signaled state by the parent thread (via UI)
// ----------------------------------------------------------------------
DWORD WINAPI RenderThreadProc(LPVOID pParam)
{
    HRESULT hr = S_OK;
    PBYTE   pBuffer = NULL;



    g_lDebugLevel = TRACE_ALWAYS;

    try
    {
        // enumerate audio renderers
        CKsEnumFilters* pEnumerator = new CKsEnumFilters(&hr);
		ThrowOnNull(pEnumerator, "Failed to allocate CKsEnumFilters");
        ThrowOnFail(hr, "Failed to create CKsEnumFilters");

        GUID  aguidEnumCats[] = { STATIC_KSCATEGORY_AUDIO, STATIC_KSCATEGORY_RENDER };

        hr = 
            pEnumerator->EnumFilters
            (
                eAudRen,            // create audio render filters ...
                aguidEnumCats,      // ... of these categories
                2,                  // There are 2 categories
                TRUE,               // While you're at it, enumerate the pins
                FALSE,              // ... but don't bother with nodes
                TRUE                // Instantiate the filters
            );

        ThrowOnFail(hr, "CKsEnumFilters::EnumFilters failed");

        // just use the first audio render filter in the list
        CKsAudRenFilter*    pFilter;
        CKsAudRenPin*       pPin;

        pEnumerator->m_listFilters.GetHead((CKsFilter**)&pFilter);
        ThrowOnNull(pFilter, "No filters available for rendering");

		// instantiate the pin as 16bit, 48KHz, stereo
        // use WAVEFORMATEXTENSIBLE to describe wave format
        WAVEFORMATEXTENSIBLE wfx;
        wfx.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        wfx.Format.nChannels = 2;
        wfx.Format.nSamplesPerSec = 48000;
        wfx.Format.nBlockAlign = 4;
        wfx.Format.nAvgBytesPerSec = wfx.Format.nSamplesPerSec * wfx.Format.nBlockAlign;
        wfx.Format.wBitsPerSample = 16;
        wfx.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE)-sizeof(WAVEFORMATEX);
        wfx.Samples.wValidBitsPerSample = 16;
        wfx.dwChannelMask = KSAUDIO_SPEAKER_STEREO;
        wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

        pPin = pFilter->CreateRenderPin(&wfx.Format, FALSE);

        if (!pPin)
        {
            // driver can't handle WAVEFORMATEXTENSIBLE, so fall back to
            // WAVEFORMATEX format descriptor and try again
            wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
            // set unused members to zero
            wfx.Format.cbSize = 0;
            wfx.Samples.wValidBitsPerSample = 0;
            wfx.dwChannelMask = 0;
            wfx.SubFormat = GUID_NULL;

            pPin = pFilter->CreateRenderPin(&wfx.Format, FALSE);
        }

        ThrowOnNull(pPin, "No pins available for rendering");
/*
        // instantiate the pin as 16bit, 48KHz, stereo
        WAVEFORMATEX    wfx;
        wfx.cbSize = 0;
        wfx.nChannels = 2;
        wfx.nSamplesPerSec = 48000;
        wfx.wBitsPerSample = 16;
        wfx.nBlockAlign = 4;
        wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
        wfx.wFormatTag = WAVE_FORMAT_PCM;

        pPin = pFilter->CreateRenderPin(&wfx, FALSE);
        ThrowOnNull(pPin, "No pins available for rendering");
*/

        // allocate a 1 second buffer
        ULONG cbBuffer = 48000 * 4;
        pBuffer = new BYTE[cbBuffer];

        ThrowOnNull(pBuffer, "Allocation failure");

        // fill the buffer with an 500 Hz sine tone
        GeneratePCMTestTone(pBuffer, cbBuffer, 48000, 2, 16, 500.0, 0.25);

        // set up the data packet.  In this example, we will submit 2 packets and then 
        // wait for their completion.  A real application would probably use a queue or
        // array of a larger number of packets and would stream data from a large pool
        // such as a file source
        HANDLE      hEventPool[cPackets + 4];
        DATA_PACKET Packets[cPackets];
        ULONG       cbPartialBuffer = cbBuffer / cPackets;
        int         i = 0;

        for (i = 0; i < cPackets; i++)
        {
            Zero(Packets[i]);
            hEventPool[i] = CreateEvent(NULL, FALSE, FALSE, NULL);

            Packets[i].Signal.hEvent = hEventPool[i];
            Packets[i].Header.Data = pBuffer + i * cbPartialBuffer;
            Packets[i].Header.FrameExtent = cbPartialBuffer;
            Packets[i].Header.DataUsed = cbPartialBuffer;  // if we were capturing, we would init this to 0
            Packets[i].Header.Size = sizeof(Packets[i].Header);
            Packets[i].Header.PresentationTime.Numerator = 1;
            Packets[i].Header.PresentationTime.Denominator = 1;
        }

        // copy control handles to this local array of events
        HANDLE* pControls = (HANDLE*)pParam;

        hEventPool[cPackets + eAbort] = pControls[0];
        hEventPool[cPackets + eStop]  = pControls[1];
        hEventPool[cPackets + ePause] = pControls[2];
        hEventPool[cPackets + eRun]   = pControls[3];

        // wait for completion
        DWORD   dwWait;
        DWORD   nEventSignaled;
        BOOL    bAbort = FALSE;
        KSSTATE State = KSSTATE_STOP;



        while (!bAbort)
        {
            // wait on control events and "packet complete" events
            dwWait = WaitForMultipleObjects(cPackets + 4, hEventPool, FALSE, INFINITE);
            if ((dwWait == WAIT_FAILED) || (dwWait == WAIT_TIMEOUT))
                break;

            nEventSignaled = dwWait - WAIT_OBJECT_0;

            if (nEventSignaled < cPackets)
            {
                // data submission block.  The event signaled corresponds to one of
                // our data packets.  The device is finished with this packet, so we 
                // can fill with data and submit to the pin.  In this example we are
                // just resubmitting the same data.
                if (State != KSSTATE_STOP)
                {
                    DWORD nPacket = nEventSignaled; // for clarity



                    hr = pPin->WriteData(&Packets[nPacket].Header, &Packets[nPacket].Signal);
					if (!SUCCEEDED(hr)){}

                }
            }
            else
            {
                // state control block.  The event signaled corresponds to one of our
                // menu controls

                DWORD nMenuEvent = nEventSignaled - cPackets; // for clarity

                switch (nMenuEvent)
                {
                    default: 

                        break;

                    case (eAbort):

                        bAbort = TRUE;
                        break;

                    case (eStop):
                        State = KSSTATE_STOP;

                        hr = pPin->SetState(State);

                        break;

                    // if we are transitioning from stop to run or pause, then mark the packets
                    // for reuse.  We have to do this because when we Stopped the pin, 
                    // the device will have completed all of its pending IRPs (thus setting
                    // the associated HEVENTs to "signaled" state).  Since our data pumping
                    // loop is relying on these events to know when we can send more data, 
                    // we need to set them to "signaled" explicitly
                    case (ePause):
                        if (State == KSSTATE_STOP)
                        {
                            for (int i = 0; i < cPackets; i++)
                                SetEvent(hEventPool[i]);
                        }

                        // set new state
                        State = KSSTATE_PAUSE;

                        hr = pPin->SetState(State);
                        break;

                    case (eRun):
                        if (State == KSSTATE_STOP)
                        {
                            for (int i = 0; i < cPackets; i++)
                                SetEvent(hEventPool[i]);
                        }

                        // set new state
                        State = KSSTATE_RUN;

                        hr = pPin->SetState(State);

                        break;
                }
            }
        }


        // stop the pin
        hr = pPin->SetState(KSSTATE_PAUSE);
        hr = pPin->SetState(KSSTATE_STOP);

        // all done for this demo
        pPin->ClosePin();

        delete pFilter;
    }

    catch (LPSTR strErr)
    {
        printf ("Error: %s.  hr = 0x%08x\n\n", strErr, hr);
    }

    if (pBuffer)
        delete []pBuffer;



	return 0;
}

// ----------------------------------------------------------------------
HANDLE CreateRenderThread(HANDLE* hControlEvents)
{
    DWORD   dwID;
    HANDLE  hThread;

    hThread = CreateThread(NULL, 0, RenderThreadProc, hControlEvents, 0, &dwID);

    return hThread;
}
