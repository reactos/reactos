/*
 * DirectPlay Voice Client Interface
 *
 * Copyright (C) 2014 Alistair Leslie-Hughes
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <assert.h>

#include <initguid.h>
#include <dplay8.h>
#include <dvoice.h>
#include "wine/test.h"

static IDirectPlayVoiceClient *vclient = NULL;
static IDirectPlayVoiceServer *vserver = NULL;
static IDirectPlay8Server *dpserver = NULL;
static IDirectPlay8Client *dpclient = NULL;
static BOOL HasConnected = FALSE;
static DWORD port = 8888;
static HANDLE connected = NULL;

static const GUID appguid = { 0xcd0c3d4b, 0xe15e, 0x4cf2, { 0x9e, 0xa8, 0x6e, 0x1d, 0x65, 0x48, 0xc5, 0xa6 } };
static WCHAR sessionname[] = L"winegamesserver";


static HRESULT WINAPI DirectPlayMessageHandler(void *lpvUserContext, DWORD dwMessageId, void *lpMessage)
{
    trace("msgid: 0x%08lx\n", dwMessageId);
    return S_OK;
}

static HRESULT WINAPI DirectPlayClientMessageHandler(void *lpvUserContext, DWORD dwMessageId, void *lpMessage)
{
    trace("cmsgid: 0x%08lx\n", dwMessageId);
    switch(dwMessageId)
    {
        case DPN_MSGID_CONNECT_COMPLETE:
        {
            PDPNMSG_CONNECT_COMPLETE completemsg;
            completemsg = (PDPNMSG_CONNECT_COMPLETE)lpMessage;

            trace("DPN_MSGID_CONNECT_COMPLETE code: 0x%08lx\n", completemsg->hResultCode);
            SetEvent(connected);
            break;
        }
    }

    return S_OK;
}

static HRESULT CALLBACK DirectPlayVoiceServerMessageHandler(void *lpvUserContext, DWORD dwMessageId, void *lpMessage)
{
    trace("vserver: 0x%08lx\n", dwMessageId);
    return S_OK;
}

static HRESULT CALLBACK DirectPlayVoiceClientMessageHandler(void *lpvUserContext, DWORD dwMessageId, void *lpMessage)
{
    trace("cserver: 0x%08lx\n", dwMessageId);

    return S_OK;
}

static BOOL test_init_dpvoice_server(void)
{
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_DirectPlayVoiceServer, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayVoiceServer, (void **)&vserver);
    if(hr == S_OK)
    {
        DVSESSIONDESC dvSessionDesc;
        IDirectPlay8Address *localaddr = NULL;
        DPN_APPLICATION_DESC dpnAppDesc;

        hr = CoCreateInstance(&CLSID_DirectPlay8Server, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Server, (void **)&dpserver);
        ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);

        hr = IDirectPlay8Server_Initialize(dpserver, NULL, DirectPlayMessageHandler, 0);
        ok(hr == S_OK, "Initialize failed with 0x%08lx\n", hr);

        hr = CoCreateInstance(&CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void **)&localaddr);
        ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "SetSP with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_HOSTNAME, L"localhost", sizeof(L"localhost"),
                                            DPNA_DATATYPE_STRING );
        ok(hr == S_OK, "AddComponent(addr) with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(localaddr, DPNA_KEY_PORT, &port, sizeof(port), DPNA_DATATYPE_DWORD);
        ok(hr == S_OK, "AddComponent(port)) with 0x%08lx\n", hr);

        memset(&dpnAppDesc, 0, sizeof(DPN_APPLICATION_DESC) );
        dpnAppDesc.dwSize           = sizeof( DPN_APPLICATION_DESC );
        dpnAppDesc.dwFlags          = DPNSESSION_CLIENT_SERVER;
        dpnAppDesc.guidApplication  = appguid;
        dpnAppDesc.pwszSessionName  = sessionname;

        hr = IDirectPlay8Server_Host(dpserver, &dpnAppDesc, &localaddr, 1, NULL, NULL, NULL, 0  );
        todo_wine ok(hr == S_OK, "Host failed with 0x%08lx\n", hr);

        hr = IDirectPlayVoiceServer_Initialize(vserver, NULL, &DirectPlayVoiceServerMessageHandler, NULL, 0, 0);
        todo_wine ok(hr == DVERR_NOTRANSPORT, "Initialize failed with 0x%08lx\n", hr);

        hr = IDirectPlayVoiceServer_Initialize(vserver, (IUnknown*)dpserver, &DirectPlayVoiceServerMessageHandler, NULL, 0, 0);
        todo_wine ok(hr == S_OK, "Initialize failed with 0x%08lx\n", hr);

        memset( &dvSessionDesc, 0, sizeof(DVSESSIONDESC) );
        dvSessionDesc.dwSize                 = sizeof( DVSESSIONDESC );
        dvSessionDesc.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;
        dvSessionDesc.dwBufferQuality        = DVBUFFERQUALITY_DEFAULT;
        dvSessionDesc.dwFlags                = 0;
        dvSessionDesc.dwSessionType          = DVSESSIONTYPE_MIXING;
        dvSessionDesc.guidCT                 = DPVCTGUID_DEFAULT;

        hr = IDirectPlayVoiceServer_StartSession(vserver, &dvSessionDesc, 0);
        todo_wine ok(hr == S_OK, "StartSession failed with 0x%08lx\n", hr);

        if(localaddr)
            IDirectPlay8Address_Release(localaddr);
    }
    else
    {
        /* Everything after Windows XP doesn't have dpvoice installed. */
        win_skip("IDirectPlayVoiceServer not supported on this platform\n");
    }

    return vserver != NULL;
}

static BOOL test_init_dpvoice_client(void)
{
    HRESULT hr;
    WCHAR player[] = L"wineuser";

    hr = CoCreateInstance(&CLSID_DirectPlayVoiceClient, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayVoiceClient, (void **)&vclient);
    if(hr == S_OK)
    {
        DVSOUNDDEVICECONFIG soundDeviceConfig;
        DVCLIENTCONFIG      clientConfig;
        IDirectPlay8Address *localaddr = NULL;
        IDirectPlay8Address *hostaddr  = NULL;
        DPN_PLAYER_INFO playerinfo;
        DPN_APPLICATION_DESC appdesc;
        DPNHANDLE asyncop;

        connected = CreateEventA(NULL, FALSE, FALSE, NULL);

        hr = CoCreateInstance(&CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlay8Client, (void **)&dpclient);
        ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);

        hr = IDirectPlay8Client_Initialize(dpclient, NULL, DirectPlayClientMessageHandler, 0);
        ok(hr == S_OK, "Initialize failed with 0x%08lx\n", hr);

        hr = CoCreateInstance(&CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void **)&hostaddr);
        ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_SetSP(hostaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "SetSP with 0x%08lx\n", hr);

        hr = CoCreateInstance(&CLSID_DirectPlay8Address, NULL, CLSCTX_ALL, &IID_IDirectPlay8Address, (void **)&localaddr);
        ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_SetSP(localaddr, &CLSID_DP8SP_TCPIP);
        ok(hr == S_OK, "SetSP with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(hostaddr, DPNA_KEY_HOSTNAME, L"localhost", sizeof(L"localhost"),
                                            DPNA_DATATYPE_STRING );
        ok(hr == S_OK, "AddComponent(addr) with 0x%08lx\n", hr);

        hr = IDirectPlay8Address_AddComponent(hostaddr, DPNA_KEY_PORT, &port, sizeof(port), DPNA_DATATYPE_DWORD);
        ok(hr == S_OK, "AddComponent(port)) with 0x%08lx\n", hr);

        memset( &playerinfo, 0, sizeof(DPN_PLAYER_INFO) );
        playerinfo.dwSize = sizeof(DPN_PLAYER_INFO);
        playerinfo.dwInfoFlags = DPNINFO_NAME;
        playerinfo.pwszName = player;

        hr = IDirectPlay8Client_SetClientInfo(dpclient, &playerinfo, NULL, NULL, DPNOP_SYNC);
        ok(hr == S_OK, "SetClientInfo with 0x%08lx\n", hr);

        memset( &appdesc, 0, sizeof(DPN_APPLICATION_DESC));
        appdesc.dwSize = sizeof( DPN_APPLICATION_DESC );
        appdesc.guidApplication = appguid;

        hr = IDirectPlay8Client_Connect(dpclient, &appdesc, hostaddr, localaddr, NULL, NULL, NULL, 0,
                           NULL, &asyncop, DPNCONNECT_OKTOQUERYFORADDRESSING);
        ok(hr == S_OK || hr == DPNSUCCESS_PENDING, "Connect with 0x%08lx\n", hr);

        WaitForSingleObject(connected, 5000);

        hr = IDirectPlayVoiceClient_Initialize(vclient, (IUnknown*)dpclient, DirectPlayVoiceClientMessageHandler, NULL, 0, 0 );
        todo_wine ok(hr == S_OK, "Connect failed with 0x%08lx\n", hr);

        soundDeviceConfig.dwSize                    = sizeof(soundDeviceConfig);
        soundDeviceConfig.dwFlags                   = 0;
        soundDeviceConfig.guidPlaybackDevice        = DSDEVID_DefaultVoicePlayback;
        soundDeviceConfig.lpdsPlaybackDevice        = NULL;
        soundDeviceConfig.guidCaptureDevice         = DSDEVID_DefaultVoiceCapture;
        soundDeviceConfig.lpdsCaptureDevice         = NULL;
        soundDeviceConfig.hwndAppWindow             = GetDesktopWindow();
        soundDeviceConfig.lpdsMainBuffer            = NULL;
        soundDeviceConfig.dwMainBufferFlags         = 0;
        soundDeviceConfig.dwMainBufferPriority      = 0;

        memset(&clientConfig, 0, sizeof(clientConfig));
        clientConfig.dwSize                 = sizeof(clientConfig);
        clientConfig.dwFlags                = DVCLIENTCONFIG_AUTOVOICEACTIVATED | DVCLIENTCONFIG_AUTORECORDVOLUME;
        clientConfig.lPlaybackVolume        = DVPLAYBACKVOLUME_DEFAULT;
        clientConfig.dwBufferQuality        = DVBUFFERQUALITY_DEFAULT;
        clientConfig.dwBufferAggressiveness = DVBUFFERAGGRESSIVENESS_DEFAULT;
        clientConfig.dwThreshold            = DVTHRESHOLD_UNUSED;
        clientConfig.lRecordVolume          = DVRECORDVOLUME_LAST;
        clientConfig.dwNotifyPeriod         = 0;

        /* Connect to the voice session */
        hr = IDirectPlayVoiceClient_Connect(vclient, &soundDeviceConfig, &clientConfig, DVFLAGS_SYNC);
        if(hr == DVERR_RUNSETUP)
        {
            IDirectPlayVoiceTest *voicetest;

            /* See if we can get the default values from the registry and try again. */
            hr = CoCreateInstance(&CLSID_DirectPlayVoiceTest, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayVoiceTest, (void **)&voicetest);
            ok(hr == S_OK, "CoCreateInstance failed with 0x%08lx\n", hr);
            if(hr == S_OK)
            {
                hr = IDirectPlayVoiceTest_CheckAudioSetup(voicetest, &DSDEVID_DefaultVoicePlayback, &DSDEVID_DefaultVoiceCapture,
                                                          NULL, DVFLAGS_QUERYONLY);
                todo_wine ok(hr == S_OK || hr == DVERR_RUNSETUP, "CheckAudioSetup failed with 0x%08lx\n", hr);
                if(hr == S_OK)
                {
                    hr = IDirectPlayVoiceClient_Connect(vclient, &soundDeviceConfig, &clientConfig, DVFLAGS_SYNC);
                    todo_wine ok(hr == S_OK, "Voice Connect failed with 0x%08lx\n", hr);
                }
                else
                {
                    win_skip("DirectPlayVoice not setup.\n");
                }

                IDirectPlayVoiceTest_Release(voicetest);
            }
        }

        HasConnected =  (hr == S_OK);
        if(!HasConnected)
        {
            IDirectPlayVoiceClient_Release(vclient);
            vclient = NULL;
        }

        if(hostaddr)
            IDirectPlay8Address_Release(hostaddr);

        if(localaddr)
            IDirectPlay8Address_Release(localaddr);

        CloseHandle(connected);
    }
    else
    {
        /* Everything after Windows XP doesn't have dpvoice installed. */
        win_skip("IDirectPlayVoiceClient not supported on this platform\n");
    }

    return vclient != NULL;
}

static void test_cleanup_dpvoice(void)
{
    HRESULT hr;

    if(vclient)
    {
        if(HasConnected)
        {
            hr = IDirectPlayVoiceClient_Disconnect(vclient, 0);
            todo_wine ok(hr == S_OK || hr == DV_PENDING, "Disconnect failed with 0x%08lx\n", hr);
        }
        IDirectPlayVoiceClient_Release(vclient);
    }

    if(dpclient)
    {
        hr = IDirectPlay8Client_Close(dpclient, 0);
        ok(hr == S_OK, "IDirectPlay8Client_Close failed with 0x%08lx\n", hr);

        IDirectPlay8Client_Release(dpclient);
    }

    if(vserver)
    {
        hr = IDirectPlayVoiceServer_StopSession(vserver, 0);
        todo_wine ok(hr == S_OK, "StopSession failed with 0x%08lx\n", hr);

        IDirectPlayVoiceServer_Release(vserver);
    }

    if(dpserver)
    {
        hr = IDirectPlay8Server_Close(dpserver, 0);
        todo_wine ok(hr == S_OK, "got 0x%08lx\n", hr);

        IDirectPlay8Server_Release(dpserver);
    }
}

static void create_voicetest(void)
{
    HRESULT hr;
    IDirectPlayVoiceTest *voicetest;

    hr = CoCreateInstance(&CLSID_DirectPlayVoiceTest, NULL, CLSCTX_INPROC_SERVER, &IID_IDirectPlayVoiceTest, (void **)&voicetest);
    if(hr == S_OK)
    {
        hr = IDirectPlayVoiceTest_CheckAudioSetup(voicetest, &DSDEVID_DefaultVoicePlayback, &DSDEVID_DefaultVoiceCapture,
                                                          NULL, DVFLAGS_QUERYONLY);
        todo_wine ok(hr == S_OK || hr == DVERR_RUNSETUP, "CheckAudioSetup failed with 0x%08lx\n", hr);

        IDirectPlayVoiceTest_Release(voicetest);
    }
    else
    {
        /* Everything after Windows XP doesn't have dpvoice installed. */
        win_skip("IDirectPlayVoiceClient not supported on this platform\n");
    }
}

static void test_GetCompressionTypes(IDirectPlayVoiceClient *client_iface, IDirectPlayVoiceServer *server_iface)
{
    const char *name = client_iface ? "client" : "server";
    HRESULT ret;
    DVCOMPRESSIONINFO data[32];
    DWORD data_size = 0, num_elements, i, j;
    WCHAR *string_loc;
    BOOL found_pcm;

    /* some variables are initialized to 99 just to check that they are not overwritten with 0 */
    static struct
    {
        /* inputs */
        DWORD data_size;
        DWORD num_elements;
        DWORD flags;
        /* expected output */
        HRESULT ret;
        /* test flags */
        enum
        {
            NULL_DATA = 1,
            NULL_DATA_SIZE = 2,
            NULL_NUM_ELEMENTS = 4,
            SHARED_VARIABLE = 8
        }
        test_flags;
    }
    tests[] =
    {
        /* tests NULL data with an insufficient data_size */
        { 10, 0, 0, DVERR_BUFFERTOOSMALL, NULL_DATA },

        /* tests NULL data with an ample data_size */
        { sizeof(data) - 1, 0, 0, DVERR_INVALIDPOINTER, NULL_DATA },

        /* tests NULL data_size */
        { 0, 99, 0, DVERR_INVALIDPOINTER, NULL_DATA_SIZE },

        /* tests NULL num_elements */
        { 99, 0, 0, DVERR_INVALIDPOINTER, NULL_NUM_ELEMENTS },

        /* tests NULL everything */
        { 99, 99, 0, DVERR_INVALIDPOINTER, NULL_DATA | NULL_DATA_SIZE | NULL_NUM_ELEMENTS },

        /* tests passing the same pointer for data_size and num_elements */
        { 10, 0, 0, DVERR_BUFFERTOOSMALL, SHARED_VARIABLE },

        /* tests passing the same pointer but with an ample data_size */
        { sizeof(data) - 1, 0, 0, DVERR_BUFFERTOOSMALL, SHARED_VARIABLE },

        /* tests flags!=0 */
        { 99, 99, 1, DVERR_INVALIDFLAGS },

        /* tests data_size=0 */
        { 0, 0, 0, DVERR_BUFFERTOOSMALL },

        /* tests data_size=1 */
        { 1, 0, 0, DVERR_BUFFERTOOSMALL },

        /* tests data_size = sizeof(DVCOMPRESSIONINFO) */
        { sizeof(DVCOMPRESSIONINFO), 0, 0, DVERR_BUFFERTOOSMALL },

        /* tests data_size = returned data_size - 1 */
        { 0 /* initialized later */, 0, 0, DVERR_BUFFERTOOSMALL },

        /* tests data_size = returned data_size */
        { 0 /* initialized later */, 0, 0, DV_OK },

        /* tests data_size = returned data_size + 1 */
        { 0 /* initialized later */, 0, 0, DV_OK }
    };

    /* either client_iface or server_iface must be given, but not both */
    assert(!client_iface ^ !server_iface);

    if (client_iface)
        ret = IDirectPlayVoiceClient_GetCompressionTypes(client_iface, NULL, &data_size, &num_elements, 0);
    else
        ret = IDirectPlayVoiceServer_GetCompressionTypes(server_iface, NULL, &data_size, &num_elements, 0);
    ok(ret == DVERR_BUFFERTOOSMALL,
       "%s: expected ret=%lx got ret=%lx\n", name, DVERR_BUFFERTOOSMALL, ret);
    ok(data_size > sizeof(DVCOMPRESSIONINFO) && data_size < sizeof(data) - 1,
       "%s: got data_size=%lu\n", name, data_size);
    tests[ARRAY_SIZE(tests) - 3].data_size = data_size - 1;
    tests[ARRAY_SIZE(tests) - 2].data_size = data_size;
    tests[ARRAY_SIZE(tests) - 1].data_size = data_size + 1;

    for(i = 0; i < ARRAY_SIZE(tests); i++)
    {
        memset(data, 0x23, sizeof(data));

        data_size = tests[i].data_size;
        num_elements = tests[i].num_elements;

        if(client_iface)
            ret = IDirectPlayVoiceClient_GetCompressionTypes(
                client_iface,
                tests[i].test_flags & NULL_DATA         ? NULL : data,
                tests[i].test_flags & NULL_DATA_SIZE    ? NULL : &data_size,
                tests[i].test_flags & NULL_NUM_ELEMENTS ? NULL :
                    tests[i].test_flags & SHARED_VARIABLE ? &data_size : &num_elements,
                tests[i].flags
            );
        else
            ret = IDirectPlayVoiceServer_GetCompressionTypes(
                server_iface,
                tests[i].test_flags & NULL_DATA         ? NULL : data,
                tests[i].test_flags & NULL_DATA_SIZE    ? NULL : &data_size,
                tests[i].test_flags & NULL_NUM_ELEMENTS ? NULL :
                    tests[i].test_flags & SHARED_VARIABLE ? &data_size : &num_elements,
                tests[i].flags
            );

        ok(ret == tests[i].ret,
           "%s: tests[%lu]: expected ret=%lx got ret=%lx\n", name, i, tests[i].ret, ret);

        if(ret == DV_OK || ret == DVERR_BUFFERTOOSMALL || tests[i].test_flags == NULL_DATA)
        {
            ok(data_size > sizeof(DVCOMPRESSIONINFO) && data_size < sizeof(data) - 1,
               "%s: tests[%lu]: got data_size=%lu\n", name, i, data_size);
            if(!(tests[i].test_flags & SHARED_VARIABLE))
                ok(num_elements > 0 && num_elements < data_size / sizeof(DVCOMPRESSIONINFO) + 1,
                   "%s: tests[%lu]: got num_elements=%lu\n", name, i, num_elements);
        }
        else
        {
            ok(data_size == tests[i].data_size,
               "%s: tests[%lu]: expected data_size=%lu got data_size=%lu\n",
               name, i, tests[i].data_size, data_size);
            ok(num_elements == tests[i].num_elements,
               "%s: tests[%lu]: expected num_elements=%lu got num_elements=%lu\n",
               name, i, tests[i].num_elements, num_elements);
        }

        if(ret == DV_OK)
        {
            string_loc = (WCHAR*)(data + num_elements);
            found_pcm = FALSE;
            for(j = 0; j < num_elements; j++)
            {
                if(memcmp(&data[j].guidType, &DPVCTGUID_NONE, sizeof(GUID)) == 0)
                {
                    ok(data[j].dwMaxBitsPerSecond == 64000,
                       "%s: tests[%lu]: data[%lu]: expected dwMaxBitsPerSecond=64000 got dwMaxBitsPerSecond=%lu\n",
                       name, i, j, data[j].dwMaxBitsPerSecond);
                    found_pcm = TRUE;
                }
                ok(data[j].dwSize == 80,
                   "%s: tests[%lu]: data[%lu]: expected dwSize=80 got dwSize=%lu\n",
                   name, i, j, data[j].dwSize);
                ok(data[j].lpszName == string_loc,
                   "%s: tests[%lu]: data[%lu]: expected lpszName=%p got lpszName=%p\n",
                   name, i, j, string_loc, data[j].lpszName);
                ok(!data[j].lpszDescription,
                   "%s: tests[%lu]: data[%lu]: expected lpszDescription=NULL got lpszDescription=%s\n",
                   name, i, j, wine_dbgstr_w(data[j].lpszDescription));
                ok(!data[j].dwFlags,
                   "%s: tests[%lu]: data[%lu]: expected dwFlags=0 got dwFlags=%lu\n",
                   name, i, j, data[j].dwFlags);
                string_loc += lstrlenW(data[j].lpszName) + 1;
            }
            ok((char*)string_loc == (char*)data + data_size,
               "%s: tests[%lu]: expected string_loc=%p got string_loc=%p\n",
               name, i, (char*)data + data_size, string_loc);
            ok(*(char*)string_loc == 0x23,
               "%s: tests[%lu]: expected *(char*)string_loc=0x23 got *(char*)string_loc=0x%x\n",
               name, i, *(char*)string_loc);
            ok(found_pcm, "%s: tests[%lu]: MS-PCM codec not found\n", name, i);
        }
        else
        {
            ok(*(char*)data == 0x23,
               "%s: tests[%lu]: expected *(char*)data=0x23 got *(char*)data=0x%x\n",
               name, i, *(char*)data);
        }
    }
}

START_TEST(voice)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    create_voicetest();

    if(test_init_dpvoice_server())
    {
        test_GetCompressionTypes(NULL, vserver);

        if(test_init_dpvoice_client())
        {
            test_GetCompressionTypes(vclient, NULL);
        }
        else
        {
            skip("client failed to initialize\n");
        }
    }
    else
    {
        skip("server failed to initialize\n");
    }

    test_cleanup_dpvoice();

    CoUninitialize();
}
