/*
    BeepMidi :: beep.sys MIDI player

    (c) Andrew Greenwood, 2007.

    Released as open-source software. You may copy, re-distribute and modify
    this software, provided this copyright notice remains intact.

    Please see the included README.TXT for more information

    HISTORY :
        16th January 2007   Started
        17th January 2007   Polyphony support and threading added
        18th January 2007   Made threading optional, added comments
*/

/* The timeslice to allocate for all playing notes (in milliseconds) */
#define TIMESLICE_SIZE  60

/*
    If this is defined, notes are added to the playing list, even if
    they already exist. As a result, the note will sound twice during
    each timeslice. Also each note on will require a corresponding note
    off event.
*/
#define ALLOW_DUPLICATE_NOTES

/*
    The maximum number of notes that may be playing at any one time.
    Higher values result in a messier sound as all the frequencies get
    mashed together. Do not set this below 2. Recommended = 4
*/
#define POLYPHONY   3

/*
    Define CONTINUOUS_NOTES to perform note playback in a separate thread.
    This was originally the intended behaviour, but after experimentation
    doesn't sound as good for MIDI files which have a lot going on. If not
    defined, all playing notes are output in sequence as a new note starts.
*/
#define CONTINUOUS_NOTES

#define WIN32_NO_STATUS
#define NTOS_MODE_USER
#include <windows.h>
#include <ndk/ntndk.h>
#include <stdio.h>
#include <ntddbeep.h>
#include <math.h>

#include <mmddk.h>
#include <mmsystem.h>

/*#define DPRINT printf*/
#define DPRINT FakePrintf

/* A few MIDI command categories */
#define MIDI_NOTE_OFF       0x80
#define MIDI_NOTE_ON        0x90
#define MIDI_CONTROL_CHANGE 0xB0
#define MIDI_PROGRAM        0xC0
#define MIDI_PITCH_BEND     0xE0
#define MIDI_SYSTEM         0xFF

/* Specific commands */
#define MIDI_RESET          0xFF


typedef struct _NoteNode
{
    struct _NoteNode* next;
    struct _NoteNode* previous;

    UCHAR note;
    UCHAR velocity; /* 0 is note-off */
} NoteNode;

typedef struct _DeviceInfo
{
    HDRVR mme_handle;
    HANDLE kernel_device;

    DWORD callback;
    DWORD instance;
    DWORD flags;

    UCHAR running_status;

    DWORD playing_notes_count;
    NoteNode* note_list;
    BOOL refresh_notes;

    HANDLE thread_handle;
    BOOL terminate_thread;
    HANDLE thread_termination_complete;
} DeviceInfo;

DeviceInfo* the_device;
CRITICAL_SECTION device_lock;

void
FakePrintf(char* str, ...)
{
    /* Just to shut the compiler up */
}


/*
    This is designed to be treated as a thread, however it behaves as a
    normal function if CONTINUOUS_NOTES is not defined.
*/

DWORD WINAPI
ProcessPlayingNotes(
    LPVOID parameter)
{
    DeviceInfo* device_info = (DeviceInfo*) parameter;
    NTSTATUS status;
    IO_STATUS_BLOCK io_status_block;
    DWORD arp_notes;

    DPRINT("Note processing started\n");

    /* We lock the note list only while accessing it */

#ifdef CONTINUOUS_NOTES
    while ( ! device_info->terminate_thread )
#endif
    {
        NoteNode* node;

        /* Number of notes being arpeggiated */
        arp_notes = 1;

        EnterCriticalSection(&device_lock);

        /* Calculate how much time to allocate to each playing note */

        DPRINT("%d notes active\n", (int) device_info->playing_notes_count);

        node = device_info->note_list;

        while ( ( node != NULL ) && ( arp_notes <= POLYPHONY ) )
        {
            DPRINT("playing..\n");
            BEEP_SET_PARAMETERS beep_data;
            DWORD actually_playing = 0;

            double frequency = node->note;
            frequency = frequency / 12;
            frequency = pow(2, frequency);
            frequency = 8.1758 * frequency;

            if (device_info->playing_notes_count > POLYPHONY)
                actually_playing = POLYPHONY;
            else
                actually_playing = device_info->playing_notes_count;

            DPRINT("Frequency %f\n", frequency);

            // TODO
            beep_data.Frequency = (DWORD) frequency;
            beep_data.Duration = TIMESLICE_SIZE / actually_playing; /* device_info->playing_notes_count; */

            status = NtDeviceIoControlFile(device_info->kernel_device,
                                           NULL,
                                           NULL,
                                           NULL,
                                           &io_status_block,
                                           IOCTL_BEEP_SET,
                                           &beep_data,
                                           sizeof(BEEP_SET_PARAMETERS),
                                           NULL,
                                           0);

            if ( ! NT_SUCCESS(status) )
            {
                DPRINT("ERROR %d\n", (int) GetLastError());
            }

            SleepEx(beep_data.Duration, TRUE);

            if ( device_info->refresh_notes )
            {
                device_info->refresh_notes = FALSE;
                break;
            }

            arp_notes ++;
            node = node->next;
        }

        LeaveCriticalSection(&device_lock);
    }

#ifdef CONTINUOUS_NOTES
    SetEvent(device_info->thread_termination_complete);
#endif

    return 0;
}


/*
    Fills a MIDIOUTCAPS structure with information about our device.
*/

MMRESULT
GetDeviceCapabilities(
    MIDIOUTCAPS* caps)
{
    /* These are ignored for now */
    caps->wMid = 0;
    caps->wPid = 0;

    caps->vDriverVersion = 0x0100;

    memset(caps->szPname, 0, sizeof(caps->szPname));
    memcpy(caps->szPname, L"PC speaker\0", strlen("PC speaker\0") * 2);

    caps->wTechnology = MOD_SQSYNTH;

    caps->wVoices = 1;              /* We only have one voice */
    caps->wNotes = POLYPHONY;
    caps->wChannelMask = 0xFFBF;    /* Ignore channel 10 */

    caps->dwSupport = 0;

    return MMSYSERR_NOERROR;
}


/*
    Helper function that just simplifies calling the application making use
    of us.
*/

BOOL
CallClient(
    DeviceInfo* device_info,
    DWORD message,
    DWORD parameter1,
    DWORD parameter2)
{
    DPRINT("Calling client - callback 0x%x mmhandle 0x%x\n", (int) device_info->callback, (int) device_info->mme_handle);
    return DriverCallback(device_info->callback,
                          HIWORD(device_info->flags),
                          device_info->mme_handle,
                          message,
                          device_info->instance,
                          parameter1,
                          parameter2);

}


/*
    Open the kernel-mode device and allocate resources. This opens the
    BEEP.SYS kernel device.
*/

MMRESULT
OpenDevice(
    DeviceInfo** private_data,
    MIDIOPENDESC* open_desc,
    DWORD flags)
{
    NTSTATUS status;
    HANDLE heap;
    HANDLE kernel_device;
    UNICODE_STRING beep_device_name;
    OBJECT_ATTRIBUTES attribs;
    IO_STATUS_BLOCK status_block;

    /* One at a time.. */
    if ( the_device )
    {
        DPRINT("Already allocated\n");
        return MMSYSERR_ALLOCATED;
    }

    /* Make the device name into a unicode string and open it */

    RtlInitUnicodeString(&beep_device_name,
                            L"\\Device\\Beep");

    InitializeObjectAttributes(&attribs,
                                &beep_device_name,
                                0,
                                NULL,
                                NULL);

    status = NtCreateFile(&kernel_device,
                            FILE_READ_DATA | FILE_WRITE_DATA,
                            &attribs,
                            &status_block,
                            NULL,
                            0,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN_IF,
                            0,
                            NULL,
                            0);

    if ( ! NT_SUCCESS(status) )
    {
        DPRINT("Could not connect to BEEP device - %d\n", (int) GetLastError());
        return MMSYSERR_ERROR;
    }

    DPRINT("Opened!\n");

    /* Allocate and initialize the device info */

    heap = GetProcessHeap();

    the_device = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(DeviceInfo));

    if ( ! the_device )
    {
        DPRINT("Out of memory\n");
        return MMSYSERR_NOMEM;
    }

    /* Initialize */
    the_device->kernel_device = kernel_device;
    the_device->playing_notes_count = 0;
    the_device->note_list = NULL;
    the_device->thread_handle = 0;
    the_device->terminate_thread = FALSE;
    the_device->running_status = 0;

    // TODO
    the_device->mme_handle = (HDRVR) open_desc->hMidi;
    the_device->callback = open_desc->dwCallback;
    the_device->instance = open_desc->dwInstance;
    the_device->flags = flags;

    /* Store the pointer in the user data */
    *private_data = the_device;

    /* This is threading-related code */
#ifdef CONTINUOUS_NOTES
    the_device->thread_termination_complete = CreateEvent(NULL, FALSE, FALSE, NULL);

    if ( ! the_device->thread_termination_complete )
    {
        DPRINT("CreateEvent failed\n");
        HeapFree(heap, 0, the_device);
        return MMSYSERR_NOMEM;
    }

    the_device->thread_handle = CreateThread(NULL,
                                             0,
                                             ProcessPlayingNotes,
                                             (PVOID) the_device,
                                             0,
                                             NULL);

    if ( ! the_device->thread_handle )
    {
        DPRINT("CreateThread failed\n");
        CloseHandle(the_device->thread_termination_complete);
        HeapFree(heap, 0, the_device);
        return MMSYSERR_NOMEM;
    }
#endif

    /* Now we call the client application to say the device is open */
    DPRINT("Sending MOM_OPEN\n");
    DPRINT("Success? %d\n", (int) CallClient(the_device, MOM_OPEN, 0, 0));

    return MMSYSERR_NOERROR;
}


/*
    Close the kernel-mode device.
*/

MMRESULT
CloseDevice(DeviceInfo* device_info)
{
    HANDLE heap = GetProcessHeap();

    /* If we're working in threaded mode we need to wait for thread to die */
#ifdef CONTINUOUS_NOTES
    the_device->terminate_thread = TRUE;

    WaitForSingleObject(the_device->thread_termination_complete, INFINITE);

    CloseHandle(the_device->thread_termination_complete);
#endif

    /* Let the client application know the device is closing */
    DPRINT("Sending MOM_CLOSE\n");
    CallClient(device_info, MOM_CLOSE, 0, 0);

    NtClose(device_info->kernel_device);

    /* Free resources */
    HeapFree(heap, 0, device_info);

    the_device = NULL;

    return MMSYSERR_NOERROR;
}


/*
    Removes a note from the playing notes list. If the note is not playing,
    we just pretend nothing happened.
*/

MMRESULT
StopNote(
    DeviceInfo* device_info,
    UCHAR note)
{
    HANDLE heap = GetProcessHeap();
    NoteNode* node;
    NoteNode* prev_node = NULL;

    DPRINT("StopNote\n");

    EnterCriticalSection(&device_lock);

    node = device_info->note_list;

    while ( node != NULL )
    {
        if ( node->note == note )
        {
            /* Found the note - just remove the node from the list */

            DPRINT("Stopping note %d\n", (int) node->note);

            if ( prev_node != NULL )
                prev_node->next = node->next;
            else
                device_info->note_list = node->next;

            HeapFree(heap, 0, node);

            device_info->playing_notes_count --;

            DPRINT("Note stopped - now playing %d notes\n", (int) device_info->playing_notes_count);

            LeaveCriticalSection(&device_lock);
            device_info->refresh_notes = TRUE;

            return MMSYSERR_NOERROR;
        }

        prev_node = node;
        node = node->next;
    }

    LeaveCriticalSection(&device_lock);

    /* Hmm, a good idea? */
#ifndef CONTINUOUS_NOTES
    ProcessPlayingNotes((PVOID) device_info);
#endif

    return MMSYSERR_NOERROR;
}


/*
    Adds a note to the playing notes list. If the note is already playing,
    the definition of ALLOW_DUPLICATE_NOTES determines if an existing note
    may be duplicated. Otherwise, duplicate notes are ignored.
*/

MMRESULT
PlayNote(
    DeviceInfo* device_info,
    UCHAR note,
    UCHAR velocity)
{
    HANDLE heap = GetProcessHeap();

    DPRINT("PlayNote\n");

    NoteNode* node;

    if ( velocity == 0 )
    {
        DPRINT("Zero velocity\n");

        /* Velocity zero is effectively a "note off" */
        StopNote(device_info, note);
    }
    else
    {
        /* Start playing the note */
        NoteNode* new_node;
        NoteNode* tail_node = NULL;

        EnterCriticalSection(&device_lock);
    
        node = device_info->note_list;
    
        while ( node != NULL )
        {
#ifndef ALLOW_DUPLICATE_NOTES
            if ( ( node->note == note ) && ( velocity > 0 ) )
            {
                /* The note is already playing - do nothing */
                DPRINT("Duplicate note playback request ignored\n");
                LeaveCriticalSection(&device_lock);
                return MMSYSERR_NOERROR;
            }
#endif

            tail_node = node;
            node = node->next;
        }

        new_node = HeapAlloc(heap, HEAP_ZERO_MEMORY, sizeof(NoteNode));

        if ( ! new_node )
        {
            LeaveCriticalSection(&device_lock);
            return MMSYSERR_NOMEM;
        }

        new_node->note = note;
        new_node->velocity = velocity;

        /*
            Prepend to the playing notes list. If exceeding polyphony,
            remove the oldest note (which will be at the tail.)
        */

        if ( device_info->note_list )
            device_info->note_list->previous = new_node;

        new_node->next = device_info->note_list;
        new_node->previous = NULL;

        device_info->note_list = new_node;
        device_info->playing_notes_count ++;

/*
        if ( device_info->playing_notes_count > POLYPHONY )
        {
            ASSERT(tail_node);

            DPRINT("Polyphony exceeded\n");

            tail_node->previous->next = NULL;

            HeapFree(heap, 0, tail_node);

            device_info->playing_notes_count --;
        }
*/

        LeaveCriticalSection(&device_lock);

        DPRINT("Note started - now playing %d notes\n", (int) device_info->playing_notes_count);
        device_info->refresh_notes = TRUE;
    }

#ifndef CONTINUOUS_NOTES
    ProcessPlayingNotes((PVOID) device_info);
#endif

    return MMSYSERR_NOERROR;
}

/*
    Decipher a short MIDI message (which is a MIDI message packed into a DWORD.)
    This will set "running status", but does not take this into account when
    processing messages (is this necessary?)
*/

MMRESULT
ProcessShortMidiMessage(
    DeviceInfo* device_info,
    DWORD message)
{
    DWORD status;

    DWORD category;
    DWORD channel;
    DWORD data1, data2;

    status = message & 0x000000FF;

    /* Deal with running status */

    if ( status < MIDI_NOTE_OFF )
    {
        status = device_info->running_status;
    }

    /* Ensure the status is sane! */

    if ( status < MIDI_NOTE_OFF )
    {
        /* It's garbage, ignore it */
        return MMSYSERR_NOERROR;
    }

    /* Figure out the message category and channel */

    category = status & 0xF0;
    channel = status & 0x0F;    /* we don't use this */

    data1 = (message & 0x0000FF00) >> 8;
    data2 = (message & 0x00FF0000) >> 16;

    DPRINT("0x%x, %d, %d\n", (int) status, (int) data1, (int) data2);

    /* Filter drums (which are *usually* on channel 10) */
    if ( channel == 10 )
    {
        return MMSYSERR_NOERROR;
    }

    /* Pass to the appropriate message handler */

    switch ( category )
    {
        case MIDI_NOTE_ON :
        {
            PlayNote(device_info, data1, data2);
            break;
        }

        case MIDI_NOTE_OFF :
        {
            StopNote(device_info, data1);
            break;
        }
    }

    return MMSYSERR_NOERROR;
}


#define PACK_MIDI(b1, b2, b3) \
    ((b3 * 65536) + (b2 * 256) + b1);


/*
    Processes a "long" MIDI message (ie, a MIDI message contained within a
    buffer.) This is intended for supporting SysEx data, or blocks of MIDI
    events. However in our case we're only interested in short MIDI messages,
    so we scan the buffer, and each time we encounter a valid status byte
    we start recording it as a new event. Once 3 bytes or a new status is
    received, the event is passed to the short message handler.
*/

MMRESULT
ProcessLongMidiMessage(
    DeviceInfo* device_info,
    MIDIHDR* header)
{
    int index = 0;
    UCHAR* midi_bytes = (UCHAR*) header->lpData;

    int msg_index = 0;
    UCHAR msg[3];

    /* Initialize the buffer */
    msg[0] = msg[1] = msg[2] = 0;

    if ( ! ( header->dwFlags & MHDR_PREPARED ) )
    {
        DPRINT("Not prepared!\n");
        return MIDIERR_UNPREPARED;
    }

    DPRINT("Processing %d bytes of MIDI\n", (int) header->dwBufferLength);

    while ( index < header->dwBufferLength )
    {
        /* New status byte? ( = new event) */
        if ( midi_bytes[index] & 0x80 )
        {
            DWORD short_msg;

            /* Deal with the existing event */

            if ( msg[0] & 0x80 )
            {
                short_msg = PACK_MIDI(msg[0], msg[1], msg[2]);

                DPRINT("Complete msg is 0x%x %d %d\n", (int) msg[0], (int) msg[1], (int) msg[2]);
                ProcessShortMidiMessage(device_info, short_msg);
            }

            /* Set new running status and start recording the event */
            DPRINT("Set new running status\n");
            device_info->running_status = midi_bytes[index];
            msg[0] = midi_bytes[index];
            msg_index = 1;
        }

        /* Unexpected data byte? ( = re-use previous status) */
        else if ( msg_index == 0 )
        {
            if ( device_info->running_status & 0x80 )
            {
                DPRINT("Retrieving running status\n");
                msg[0] = device_info->running_status;
                msg[1] = midi_bytes[index];
                msg_index = 2;
            }
            else
                DPRINT("garbage\n");
        }

        /* Expected data ( = append to message until buffer full) */
        else
        {
            DPRINT("Next byte...\n");
            msg[msg_index] = midi_bytes[index];
            msg_index ++;

            if ( msg_index > 2 )
            {
                DWORD short_msg;

                short_msg = PACK_MIDI(msg[0], msg[1], msg[2]);

                DPRINT("Complete msg is 0x%x %d %d\n", (int) msg[0], (int) msg[1], (int) msg[2]);
                ProcessShortMidiMessage(device_info, short_msg);

                /* Reinit */
                msg_index = 0;
                msg[0] = msg[1] = msg[2] = 0;
            }
        }

        index ++;
    }

    /*
        We're meant to clear MHDR_DONE and set MHDR_INQUEUE but since we
        deal with everything here and now we might as well just say so.
    */
    header->dwFlags |= MHDR_DONE;
    header->dwFlags &= ~ MHDR_INQUEUE;

    DPRINT("Success? %d\n", (int) CallClient(the_device, MOM_DONE, (DWORD) header, 0));

    return MMSYSERR_NOERROR;
}


/*
    Exported function that receives messages from WINMM (the MME API.)
*/

FAR PASCAL
MMRESULT
modMessage(
    UINT device_id,
    UINT message,
    DWORD private_data,
    DWORD parameter1,
    DWORD parameter2)
{
    switch ( message )
    {
        case MODM_GETNUMDEVS :
        {
            /* Only one internal PC speaker device (and even that's too much) */
            DPRINT("MODM_GETNUMDEVS\n");
            return 1;
        }

        case MODM_GETDEVCAPS :
        {
            DPRINT("MODM_GETDEVCAPS\n");
            return GetDeviceCapabilities((MIDIOUTCAPS*) parameter1);
        }

        case MODM_OPEN :
        {
            DPRINT("MODM_OPEN\n");

            return OpenDevice((DeviceInfo**) private_data,
                              (MIDIOPENDESC*) parameter1,
                              parameter2);
        }

        case MODM_CLOSE :
        {
            DPRINT("MODM_CLOSE\n");
            return CloseDevice((DeviceInfo*) private_data);
        }

        case MODM_DATA :
        {
            return ProcessShortMidiMessage((DeviceInfo*) private_data, parameter1);
        }

        case MODM_PREPARE :
        {
            /* We don't bother with this */
            MIDIHDR* hdr = (MIDIHDR*) parameter1;
            hdr->dwFlags |= MHDR_PREPARED;
            return MMSYSERR_NOERROR;
        }

        case MODM_UNPREPARE :
        {
            MIDIHDR* hdr = (MIDIHDR*) parameter1;
            hdr->dwFlags &= ~MHDR_PREPARED;
            return MMSYSERR_NOERROR;
        }

        case MODM_LONGDATA :
        {
            DPRINT("LONGDATA\n");
            return ProcessLongMidiMessage((DeviceInfo*) private_data, (MIDIHDR*) parameter1);
        }

        case MODM_RESET :
        {
            /* TODO */
            break;
        }
    }

    DPRINT("Not supported %d\n", message);

    return MMSYSERR_NOTSUPPORTED;
}


/*
    Driver entrypoint.
*/

FAR PASCAL LONG
DriverProc(
    DWORD driver_id,
    HDRVR driver_handle,
    UINT message,
    LONG parameter1,
    LONG parameter2)
{
    switch ( message )
    {
        case DRV_LOAD :
            DPRINT("DRV_LOAD\n");
            the_device = NULL;
            return 1L;

        case DRV_FREE :
            DPRINT("DRV_FREE\n");
            return 1L;

        case DRV_OPEN :
            DPRINT("DRV_OPEN\n");
            InitializeCriticalSection(&device_lock);
            return 1L;

        case DRV_CLOSE :
            DPRINT("DRV_CLOSE\n");
            return 1L;

        case DRV_ENABLE :
            DPRINT("DRV_ENABLE\n");
            return 1L;

        case DRV_DISABLE :
            DPRINT("DRV_DISABLE\n");
            return 1L;

        /*
            We don't provide configuration capabilities. This used to be
            for things like I/O port, IRQ, DMA settings, etc.
        */

        case DRV_QUERYCONFIGURE :
            DPRINT("DRV_QUERYCONFIGURE\n");
            return 0L;

        case DRV_CONFIGURE :
            DPRINT("DRV_CONFIGURE\n");
            return 0L;

        case DRV_INSTALL :
            DPRINT("DRV_INSTALL\n");
            return DRVCNF_RESTART;
    };

    DPRINT("???\n");

    return DefDriverProc(driver_id,
                         driver_handle,
                         message,
                         parameter1,
                         parameter2);
}
