/*
    playmidi.c

    MIDI playback tester
*/

#include <stdio.h>
#include <windows.h>
// typedef UINT *LPUINT;
#include <mmsystem.h>
#include <malloc.h>


struct GENERIC_CHUNK
{
    char    Header[4];
    ULONG   Length;
} GENERIC_CHUNK;


struct MTHD_CHUNK
{
    char    Header[4];
    ULONG   Length;
    USHORT  Format;
    USHORT  NumTracks;
    USHORT  Division;
} MTHD_CHUNK;


struct MidiMessage
{
    UINT    Time;
    UCHAR   Status;
    UCHAR   Data1;
    UCHAR   Data2;
    UCHAR   Data3;
};


struct MessageQueue
{
    UINT MessageCount;
    UINT MessageCapacity;
    UINT QueuePos;
    struct MidiMessage* Messages;  // MQ[trk][entry]
};


struct Song
{
    USHORT Format;
    USHORT NumTracks;
    USHORT Division; // correct?
    struct MessageQueue* Tracks;
};


struct Song Song;


USHORT SwapShortBytes(USHORT *Value)
{
    return *Value = ((*Value % 256) * 256) + (*Value / 256);
}


ULONG SwapLongBytes(ULONG *Value)
{
    BYTE b[4];
    INT i;

    for (i = 0; i < 4; i ++)
    {
        b[i] = *Value % 256;
        *Value /= 256;
    }

    printf("SLB : %u %u %u %u\n", b[0], b[1], b[2], b[3]);

    return *Value = (b[0] * 256 * 256 *256) +
           (b[1] * 256 * 256) +
           (b[2] * 256) +
           b[3];
}


ULONG ReadVarLen(FILE* f, ULONG *b)
{
    char c;
    ULONG v;

    *b ++;

    if ((v = getc(f)) & 0x80)
    {
        v &= 0x7f;
        do
        {
            v = (v << 7) + ((c = getc(f)) & 0x7f);
            *b ++;
        }
        while (c & 0x80);
    }

    return v;
}


BOOLEAN QueueMessage(UINT Track, struct MidiMessage* Msg)
{
    PVOID OldPtr = NULL;

//    printf("QueueMessage called\n");

    if (Song.Tracks[Track].MessageCount + 1 > Song.Tracks[Track].MessageCapacity)
    {
        Song.Tracks[Track].MessageCapacity += 1;
        OldPtr = Song.Tracks[Track].Messages;

//        printf("Allocating %d bytes - ", Song.Tracks[Track].MessageCapacity * sizeof(struct MidiMessage));

        Song.Tracks[Track].Messages =
            realloc(Song.Tracks[Track].Messages,
            Song.Tracks[Track].MessageCapacity * sizeof(struct MidiMessage));

//        printf("0x%x\n", Song.Tracks[Track].Messages);

        if (! Song.Tracks[Track].Messages)
        {
            Song.Tracks[Track].Messages = OldPtr;
            return FALSE;
        }
    }

    // Copy the message
    Song.Tracks[Track].Messages[Song.Tracks[Track].MessageCount] = *Msg;

//    printf("Message 0x%x %d %d %d\n", Msg->Status, Msg->Data1, Msg->Data2, Msg->Data3);

    Song.Tracks[Track].MessageCount ++;
//    printf("Msg count %u\n", Song.Tracks[Track].MessageCount);

    return TRUE;
}


BOOLEAN DequeueMessage(UINT Track, struct MidiMessage* Msg)
{
    printf("Dequeueing message from %u < %u\n", Song.Tracks[Track].QueuePos, Song.Tracks[Track].MessageCount);

    if (Song.Tracks[Track].QueuePos >= Song.Tracks[Track].MessageCount)
    {
        printf("No more messages in queue\n");
        return FALSE;
    }

    *Msg = Song.Tracks[Track].Messages[Song.Tracks[Track].QueuePos];

    Song.Tracks[Track].QueuePos ++;

    return TRUE;
}


UINT GetMessageLength(BYTE b)
{
    switch(b)
    {
        case 0xf0 : // case 0xf4 : case 0xf5 : case 0xf6 : case 0xf7 :
            return 0;   // ignore
        case 0xf1 : case 0xf3 :
            return 2;
        case 0xf2 :
            return 3;
        case 0xff : // meta event
            return 0;
    }

    if (b > 0xf8)   return 1;

    switch(b & 0xf0)
    {
        case 0x80 : case 0x90 : case 0xa0 : case 0xb0 : case 0xe0 :
            return 2;
        case 0xc0 : case 0xd0 :
            return 1;
    }

    return 0;   // must be a status byte
}





UINT ReadEvent(FILE* f, struct MidiMessage* Msg)
{
    ULONG Bytes = 0;
    ULONG MsgLen = 0;
    UCHAR Status;
    PVOID NextByte = NULL;

    Msg->Data1 = 0;
    Msg->Data2 = 0;
    Msg->Data3 = 0;

    Msg->Time = ReadVarLen(f, &Bytes); // Increment ?
//    Bytes = 4;  // WRONG!
//    printf("Message at %u :  ", Msg->Time);
    Status = getc(f);
    Bytes ++;

    // Figure out if we're using running status or not
    if (Status & 0x80)
    {
        Msg->Status = Status;
        NextByte = &Msg->Data1;
//            printf("NRS\n");
    }
    else
    {
        Bytes --;
        MsgLen --;    // ok?
        Msg->Data1 = Status;
        NextByte = &Msg->Data2;
    }

    Bytes += MsgLen = GetMessageLength(Msg->Status);
//    printf("Len(%u) ", Bytes);


//    if (Bytes <= 4) // System messages are ignored
//    {
        if (Msg->Status == 0xf0)
        {
            ReadVarLen(f, &Bytes);  // we don't care!
            while ((! feof(f)) && (Status = fgetc(f) != 0x7f))
            {
                Bytes ++;
            }
//            printf("Ignored %u bytes of SysEx\n", Bytes);
            return Bytes;
        }

        else if (Msg->Status == 0xff)    // meta event
        {
            UCHAR METype, MELen;
            // process the message somehow... (pass song pointer?)
            METype = getc(f); // What type of event it is
            MELen = getc(f);    // How long it is
            fseek(f, MELen, SEEK_CUR);
            if (METype == 0x2f) // track end
                return FALSE;
//            printf("Found a meta-event of type %u, length %u (ignored it)\n", METype, MELen);
            return Bytes + MELen;
        }

        else if (Msg->Status > 0xf0)
            return Bytes + 1;
//    }
//    else if (Bytes > 4 + 3)
//    {
//        printf("MIDI bytecount > 3. THIS SHOULD NOT HAPPEN!\n");
//    }

//    printf("Reading %u bytes .. ", MsgLen);
    fread(NextByte, 1, MsgLen, f);

//    printf("Message 0x%x %d %d %d\n", Msg->Status, Msg->Data1, Msg->Data2, Msg->Data3);

    if (feof(f))
        return FALSE;

    return Bytes;
}


UINT Trk = 0;

BOOLEAN ReadChunk(FILE* f)
{
    struct GENERIC_CHUNK Chunk;

    fread(&Chunk, sizeof(GENERIC_CHUNK), 1, f);
    SwapLongBytes(&Chunk.Length);

    if (feof(f))
        return FALSE;

    if (! strncmp(Chunk.Header, "MThd", 4))
    {
        int i;

        struct MTHD_CHUNK HeaderChunk;
        memcpy(&HeaderChunk, &Chunk, sizeof(GENERIC_CHUNK));
        printf("Found MThd chunk\n");

        fread(&HeaderChunk.Format, 1, Chunk.Length, f);
        SwapShortBytes(&HeaderChunk.Format);
        SwapShortBytes(&HeaderChunk.NumTracks);
        SwapShortBytes(&HeaderChunk.Division);

        Song.Format = HeaderChunk.Format;
        Song.NumTracks = HeaderChunk.NumTracks;
        Song.Division = HeaderChunk.Division;

        Song.Tracks = malloc(Song.NumTracks * sizeof(struct MessageQueue));

        for (i = 0; i < Song.NumTracks; i ++)
        {
            Song.Tracks[i].MessageCount = 0;
            Song.Tracks[i].MessageCapacity = 0;
            Song.Tracks[i].QueuePos = 0;
            Song.Tracks[i].Messages = NULL;
        }

        switch(HeaderChunk.Format)
        {
            case 0 :
                printf("Single track format\n");
                if (HeaderChunk.NumTracks > 1)
                {
                    printf("Found more than one track!\n");
                    return FALSE;
                }
                printf("1 track\n");
                break;

            case 1 :
                printf("Multi-track format\n");
                if (HeaderChunk.NumTracks < 1)
                {
                    printf("MIDI file contains no tracks!\n");
                    return FALSE;
                }
                printf("%u track(s)\n", HeaderChunk.NumTracks);
                break;

            default :
                printf("Unsupported format!\n");
                return FALSE;
        }
    }

    else if (! strncmp(Chunk.Header, "MTrk", 4))
    {
        struct MidiMessage Msg;
        UINT ByteCount = 0;

//        Song.Tracks[Trk].MessageCapacity = 0;
//        Song.Tracks[Trk].MessageCount = 0;
//        Song.Tracks[Trk].Messages = NULL;

        printf("Found an MTrk chunk - length %u\n", Chunk.Length);

        Msg.Time = 0;   // this gets incremented

        while (ReadEvent(f, &Msg))
        {
            if (Msg.Status >= 0xf0)
                continue;

            if ((Msg.Data1 & 0x80) ||
                (Msg.Data2 & 0x80) ||
                (Msg.Data3 & 0x80))
            {
                printf("WARNING: Malformed MIDI message: 0x%x %u %u %u\n", Msg.Status, Msg.Data1, Msg.Data2, Msg.Data3);
            }
//            else
//                printf("0x%x %u %u %u\n", Msg.Status, Msg.Data1, Msg.Data2, Msg.Data3);

            if (! QueueMessage(Trk, &Msg))
            {
                UINT i;

                printf("Out of memory\n");

                for (i = 0; i < Trk; i ++)
                {
                    free(Song.Tracks[Trk].Messages);
                }
                free(Song.Tracks);
                abort();
            }
        }

        printf("%u messages were processed\n", Song.Tracks[Trk].MessageCount);

        Trk ++;


//        while (ByteCount < Chunk.Length)
//        {
//            printf("%u < %u ?\n", ByteCount, Chunk.Length);
//            ByteCount += ReadEvent(f, &Msg);
            // ...
//        }

//        fseek(f, Chunk.Length, SEEK_CUR);
    }

    else
    {
        // skip it...
        printf("Haven't got a clue what this chunk is: %s\n", Chunk.Header);
        fseek(f, Chunk.Length, SEEK_CUR);
    }

    return TRUE;
}


BOOLEAN Load(char* Filename)
{
    FILE* f = fopen(Filename, "rb");

    if (! f)
    {
        printf("File not found!\n");
        return FALSE;
    }

//    fread(&Header, 1, 4, f);
//    printf("Header = %s\n", Header);

//    if (strncmp(Header, "MThd", 4))
//    {
//        printf("Not a MIDI file!\n");
//        return FALSE;
//    }

    while (! feof(f))
        if (! ReadChunk(f)) break;

//    fread(&Length, 4, 1, f);
//    Length = SwapLongBytes(Length);

//    if (Length != 6)
//    {
//        printf("Abnormal MThd chunk length : %u\n", Length);
//        return FALSE;
//    }

//    fread(&Format, 2, 1, f);
//    Format = SwapShortBytes(Format);
//    fread(&NumTracks, 2, 1, f);
//    NumTracks = SwapShortBytes(NumTracks);


    // Ignore this for now:
//    fread(&Division, 2, 1, f);

    fclose(f);
    return TRUE;
}


#define PACK_MIDI(s, d1, d2, d3) \
    (d3 * 256 * 256 * 256) + \
    (d2 * 256 * 256) + \
    (d1 * 256) + s


BOOLEAN Play()
{
    printf("Now playing - press any key to stop...\n");
    UINT Trk;
    struct MidiMessage Msg;
    HANDLE Timer;
    LARGE_INTEGER ns;
    HMIDIOUT Device;

    if (midiOutOpen(&Device, -1, NULL, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
    {
        printf("midiOutOpen() failed!\n");
        return FALSE;
    }

    Timer = CreateWaitableTimer(NULL, TRUE, "PlayMidiTimer");

    while(DequeueMessage(5, &Msg))  // fix this
    {
        if (Msg.Time > 0)
        {
            ns.QuadPart = 20000 * Msg.Time;
            ns.QuadPart = - ns.QuadPart;
            SetWaitableTimer(Timer, &ns, 0, NULL, NULL, FALSE);
            WaitForSingleObject(Timer, INFINITE);
        }

        printf("%u :  ", Msg.Time);
        printf("0x%x %d %d %d\n", Msg.Status, Msg.Data1, Msg.Data2, Msg.Data3);
        midiOutShortMsg(Device, PACK_MIDI(Msg.Status, Msg.Data1, Msg.Data2, Msg.Data3));
    }

    return TRUE;
}


int main(int argc, char* argv[])
{
    printf("MIDI Playback Test Applet\n");
    printf("by Andrew Greenwood\n\n");

    if (argc == 1)
    {
        printf("Need a filename\n");
        return -1;
    }

    printf("Playing %s\n", argv[1]);

    if (Load(argv[1]))
        if (! Play())
            return -1;

    return 0;
}
