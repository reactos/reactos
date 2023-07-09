#include "headers.hxx"

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_MMSYSTEM_H_
#define X_MMSYSTEM_H_
#include <mmsystem.h>
#endif

// Number of bytes read from the file, used for tasting the file in DwValidSoundFile
// Note that the array using this is automatic -- if it gets too large it will have
// to be allocated and freed.
#define DATA_FOR_SOUND 100

#define SUN_MAGIC       0x2e736e64      /* Really '.snd' */
#define SUN_INV_MAGIC   0x646e732e      /* '.snd' upside-down */
#define DEC_MAGIC       0x2e736400      /* Really '\0ds.' (for DEC) */
#define DEC_INV_MAGIC   0x0064732e      /* '\0ds.' upside-down */
#define AIFF_MAGIC      0x464f524d      /* 'FORM' */
#define AIFF_INV_MAGIC  0x4d524f46      /* 'MROF' */

enum HTTasteResult
{
    DefinitelyNot,
    ProbablyNot,
    Uncertain,
    ProbablyIs,
    DefinitelyIs
};

enum HTTasteResult TasteAU(unsigned char *pTaste,unsigned int cbTaste)
{
    unsigned long magic;

    if (cbTaste < 4) return Uncertain;

    magic = ((ULONG)pTaste[0] << 24L) | ((ULONG)pTaste[1] << 16L) | ((ULONG)pTaste[2] << 8L) | pTaste[3];
    if (magic == DEC_INV_MAGIC ||
        magic == SUN_INV_MAGIC ||
        magic == SUN_MAGIC ||
        magic == DEC_MAGIC)
    {
        return ProbablyIs;
    }
    else
    {
        return DefinitelyNot;
    }
}

enum HTTasteResult TasteAIFF(unsigned char *pTaste,unsigned int cbTaste)
{
    unsigned long magic;

    if (cbTaste < 4) return Uncertain;

    magic = ((ULONG)pTaste[0] << 24L) | ((ULONG)pTaste[1] << 16L) | ((ULONG)pTaste[2] << 8L) | pTaste[3];
    if (magic == AIFF_MAGIC ||
        magic == AIFF_INV_MAGIC)
    {
        return ProbablyIs;
    }
    else
    {
        return DefinitelyNot;
    }
}

enum HTTasteResult TasteMIDI(unsigned char *pTaste,unsigned int cbTaste)
{
    if (cbTaste < 4) return Uncertain;

    if (strncmp((char *) pTaste, "MThd", 4) == 0)
    {
        return ProbablyIs;
    }
    else
    if (cbTaste >= 12 &&
        strncmp((char *) pTaste, "RIFF", 4) == 0 && strncmp((char*) pTaste+8, "RMID", 4) == 0)
    {
        return ProbablyIs;
    }
    else
    {
        return DefinitelyNot;
    }
}

enum HTTasteResult TasteWAVE(unsigned char *pTaste,unsigned int cbTaste)
{
    if (cbTaste < 12) return Uncertain;

    if (strncmp((char *) pTaste, "RIFF", 4) == 0 && strncmp((char*) pTaste+8, "WAVE", 4) == 0)
    {
        return ProbablyIs;
    }
    else
    {
        return DefinitelyNot;
    }
}

static enum HTTasteResult TasteAVI(unsigned char *pTaste,unsigned int cbTaste)
{
    if (cbTaste < 12) return Uncertain;

    if (strncmp((char *) pTaste, "RIFF", 4) == 0 && strncmp((char*) pTaste+8, "AVI ", 4) == 0)
    {
        return ProbablyIs;
    }
    else
    {
        return DefinitelyNot;
    }
}

WORD MapFileToDeviceType(LPCTSTR pcszFileName)
{
    if (pcszFileName)
    {
        // OK, let's sample the file to see if we recognize what type it is.
        enum HTTasteResult taste;
        unsigned char ach[DATA_FOR_SOUND];
        FILE *fp = fopen(pcszFileName, "rb");
        int cbRead;
        if (!fp)
        {
            return 0;
        }

        cbRead = fread(ach, 1, DATA_FOR_SOUND, fp);
        fclose(fp);

        // We need to have read some data.
        if (DATA_FOR_SOUND != cbRead)
        {
            return 0;
        }

        taste = TasteAU(ach, DATA_FOR_SOUND);
        if (ProbablyIs == taste)
        {
            return MCI_ALL_DEVICE_ID;
        }
        taste = TasteAIFF(ach, DATA_FOR_SOUND);
        if (ProbablyIs == taste)
        {
            return MCI_ALL_DEVICE_ID;
        }
        taste = TasteMIDI(ach, DATA_FOR_SOUND);
        if (ProbablyIs == taste)
        {
            return MCI_ALL_DEVICE_ID;
        }
        taste = TasteWAVE(ach, DATA_FOR_SOUND);
        if (ProbablyIs == taste)
        {
            return MCI_DEVTYPE_WAVEFORM_AUDIO;
        }

        return 0;
    }

    return 0;
}

