/*
 * Copyright 2017 Alistair Leslie-Hughes
 * Copyright 2019 Vijay Kiran Kamuju
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
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "initguid.h"
#include "wmsdk.h"

#include "wine/test.h"

HRESULT WINAPI WMCreateWriterPriv(IWMWriter **writer);

static void test_wmwriter_interfaces(void)
{
    HRESULT hr;
    IWMWriter          *writer;
    IWMHeaderInfo      *header;
    IWMHeaderInfo2     *header2;
    IWMHeaderInfo3     *header3;

    hr = WMCreateWriter( NULL, &writer );
    ok(hr == S_OK, "WMCreateWriter failed 0x%08x\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMWriter\n");
        return;
    }

    hr = IWMWriter_QueryInterface(writer, &IID_IWMHeaderInfo, (void **)&header);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMWriter_QueryInterface(writer, &IID_IWMHeaderInfo2, (void **)&header2);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMWriter_QueryInterface(writer, &IID_IWMHeaderInfo3, (void **)&header3);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    if(header)
        IWMHeaderInfo_Release(header);
    if(header2)
        IWMHeaderInfo2_Release(header2);
    if(header3)
        IWMHeaderInfo3_Release(header3);
    IWMWriter_Release(writer);
}

static void test_wmreader_interfaces(void)
{
    HRESULT hr;
    IWMReader          *reader;
    IWMReaderAdvanced  *advanced;
    IWMReaderAdvanced2 *advanced2;
    IWMHeaderInfo      *header;
    IWMHeaderInfo2     *header2;
    IWMHeaderInfo3     *header3;
    IWMProfile         *profile;
    IWMProfile2        *profile2;
    IWMProfile3        *profile3;
    IWMPacketSize      *packet;
    IWMPacketSize2     *packet2;
    IWMReaderAccelerator     *accel;
    IWMReaderTimecode        *timecode;
    IWMReaderNetworkConfig   *netconfig;
    IWMReaderNetworkConfig2  *netconfig2;
    IWMReaderStreamClock     *clock;
    IWMReaderTypeNegotiation *negotiation;
    IWMDRMReader       *drmreader;
    IWMDRMReader2      *drmreader2;
    IWMDRMReader3      *drmreader3;
    IWMReaderPlaylistBurn *playlist;
    IWMLanguageList       *langlist;
    IReferenceClock       *refclock;

    hr = WMCreateReader( NULL, 0, &reader );
    ok(hr == S_OK, "WMCreateReader failed 0x%08x\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMReader\n");
        return;
    }

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderAdvanced, (void **)&advanced);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderAdvanced2, (void **)&advanced2);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMHeaderInfo, (void **)&header);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMHeaderInfo2, (void **)&header2);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMHeaderInfo3, (void **)&header3);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMProfile2, (void **)&profile2);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMProfile3, (void **)&profile3);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMPacketSize, (void **)&packet);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMPacketSize2, (void **)&packet2);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderAccelerator, (void **)&accel);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderTimecode, (void **)&timecode);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderNetworkConfig, (void **)&netconfig);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderNetworkConfig2, (void **)&netconfig2);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderStreamClock, (void **)&clock);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderTypeNegotiation, (void **)&negotiation);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMDRMReader, (void **)&drmreader);
    ok(hr == E_NOINTERFACE, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMDRMReader2, (void **)&drmreader2);
    ok(hr == E_NOINTERFACE, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMDRMReader3, (void **)&drmreader3);
    ok(hr == E_NOINTERFACE, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMReaderPlaylistBurn, (void **)&playlist);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IWMLanguageList, (void **)&langlist);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMReader_QueryInterface(reader, &IID_IReferenceClock, (void **)&refclock);
    ok(hr == S_OK, "Failed 0x%08x\n", hr);

    if(packet)
        IWMPacketSize_Release(packet);
    if(packet2)
        IWMPacketSize2_Release(packet2);
    if(advanced)
        IWMReaderAdvanced_Release(advanced);
    if(advanced2)
        IWMReaderAdvanced2_Release(advanced2);
    if(profile)
        IWMProfile_Release(profile);
    if(profile2)
        IWMProfile2_Release(profile2);
    if(profile3)
        IWMProfile3_Release(profile3);
    if(header)
        IWMHeaderInfo_Release(header);
    if(header2)
        IWMHeaderInfo2_Release(header2);
    if(header3)
        IWMHeaderInfo3_Release(header3);
    if(accel)
        IWMReaderAccelerator_Release(accel);
    if(timecode)
        IWMReaderTimecode_Release(timecode);
    if(netconfig)
        IWMReaderNetworkConfig_Release(netconfig);
    if(netconfig2)
        IWMReaderNetworkConfig2_Release(netconfig2);
    if(clock)
        IWMReaderStreamClock_Release(clock);
    if(negotiation)
        IWMReaderTypeNegotiation_Release(negotiation);
    if(playlist)
        IWMReaderPlaylistBurn_Release(playlist);
    if(langlist)
        IWMLanguageList_Release(langlist);
    if(refclock)
        IReferenceClock_Release(refclock);

    IWMReader_Release(reader);
}

static void test_wmsyncreader_interfaces(void)
{
    HRESULT hr;
    IWMSyncReader      *reader;
    IWMHeaderInfo      *header;
    IWMHeaderInfo2     *header2;
    IWMHeaderInfo3     *header3;
    IWMProfile         *profile;
    IWMProfile2        *profile2;
    IWMProfile3        *profile3;
    IWMPacketSize      *packet;
    IWMPacketSize2     *packet2;
    IWMReaderTimecode     *timecode;
    IWMReaderPlaylistBurn *playlist;
    IWMLanguageList       *langlist;

    hr = WMCreateSyncReader( NULL, 0, &reader );
    ok(hr == S_OK, "WMCreateSyncReader failed 0x%08x\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMSyncReader\n");
        return;
    }

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMHeaderInfo, (void **)&header);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMHeaderInfo2, (void **)&header2);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMHeaderInfo3, (void **)&header3);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMProfile, (void **)&profile);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMProfile2, (void **)&profile2);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMProfile3, (void **)&profile3);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMPacketSize, (void **)&packet);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMPacketSize2, (void **)&packet2);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMReaderTimecode, (void **)&timecode);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMReaderPlaylistBurn, (void **)&playlist);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    hr = IWMSyncReader_QueryInterface(reader, &IID_IWMLanguageList, (void **)&langlist);
    todo_wine ok(hr == S_OK, "Failed 0x%08x\n", hr);

    if(packet)
        IWMPacketSize_Release(packet);
    if(packet2)
        IWMPacketSize2_Release(packet2);
    if(profile)
        IWMProfile_Release(profile);
    if(profile2)
        IWMProfile2_Release(profile2);
    if(profile3)
        IWMProfile3_Release(profile3);
    if(header)
        IWMHeaderInfo_Release(header);
    if(header2)
        IWMHeaderInfo2_Release(header2);
    if(header3)
        IWMHeaderInfo3_Release(header3);
    if(timecode)
        IWMReaderTimecode_Release(timecode);
    if(playlist)
        IWMReaderPlaylistBurn_Release(playlist);
    if(langlist)
        IWMLanguageList_Release(langlist);


    IWMSyncReader_Release(reader);
}


static void test_profile_manager_interfaces(void)
{
    HRESULT hr;
    IWMProfileManager  *profile;

    hr = WMCreateProfileManager(&profile);
    ok(hr == S_OK, "WMCreateProfileManager failed 0x%08x\n", hr);
    if(FAILED(hr))
    {
        win_skip("Failed to create IWMProfileManager\n");
        return;
    }

    IWMProfileManager_Release(profile);
}

static void test_WMCreateWriterPriv(void)
{
    IWMWriter *writer, *writer2;
    HRESULT hr;

    hr = WMCreateWriterPriv(&writer);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    hr = IWMWriter_QueryInterface(writer, &IID_IWMWriter, (void**)&writer2);
    ok(hr == S_OK, "got 0x%08x\n", hr);

    IWMWriter_Release(writer);
    IWMWriter_Release(writer2);
}

static void test_urlextension(void)
{
    HRESULT hr;
    const WCHAR mp3file[] = {'t','e','s','t','.','m','p','3',0};
    const WCHAR mkvfile[] = {'t','e','s','t','.','m','k','v',0};
    const WCHAR urlfile[] = {'a','b','c','d',':','/','/','t','e','s','t','/','t','e','s','t','.','w','m','v',0};
    const WCHAR testurl[] = {'h','t','t','p',':','/','/','t','e','s','t','/','t','.','a','s','f','?','a','l','t','=','t','.','m','k','v',0};

    hr = WMCheckURLExtension(NULL);
    ok(hr == E_INVALIDARG, "WMCheckURLExtension failed 0x%08x\n", hr);
    hr = WMCheckURLExtension(mkvfile);
    ok(hr == NS_E_INVALID_NAME, "WMCheckURLExtension failed 0x%08x\n", hr);
    hr = WMCheckURLExtension(mp3file);
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08x\n", hr);
    hr = WMCheckURLExtension(urlfile);
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08x\n", hr);
    hr = WMCheckURLExtension(testurl);
    todo_wine ok(hr == S_OK, "WMCheckURLExtension failed 0x%08x\n", hr);
}

START_TEST(wmvcore)
{
    HRESULT hr;

    hr = CoInitialize(0);
    ok(hr == S_OK, "failed to init com\n");
    if(hr != S_OK)
        return;

    test_wmreader_interfaces();
    test_wmsyncreader_interfaces();
    test_wmwriter_interfaces();
    test_profile_manager_interfaces();
    test_WMCreateWriterPriv();
    test_urlextension();

    CoUninitialize();
}
