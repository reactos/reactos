//**@@@*@@@****************************************************
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

//
// FileName:    util.cpp
//
// Abstract:    
//      This is the implementation file for general helper functions
//

#include "kssample.h"
#include <mmsystem.h>
#include <math.h>
#include <float.h>


////////////////////////////////////////////////////////////////////////////////
//
//  GetWfxSize()
//
//  Routine Description:
//      Returns the size of the wave format
//  
//  Arguments: 
//      Pointer to the wave format
//
//  Return Value:
//  
//     Size of the wave format
//
DWORD GetWfxSize(const WAVEFORMATEX* pwfxSrc)
{
    assert(pwfxSrc);

    DWORD dwSize;
    if(WAVE_FORMAT_PCM == pwfxSrc->wFormatTag)
    {
        dwSize = sizeof(WAVEFORMATEX);
    }
    else
    {
        dwSize = sizeof(WAVEFORMATEX) + pwfxSrc->cbSize;
    }

    return dwSize;
}


////////////////////////////////////////////////////////////////////////////////
//
//  DebugPrintf()
//
//  Routine Description:
//      printf style function for logging to the debugger
//  
//  Arguments: 
//      debug level
//      printf style format string
//
//  Return Value:
//  
//     None
//
LONG g_lDebugLevel = TRACE_MUTED;

void DebugPrintf(LONG lDebugLevel, LPCTSTR pszFormat, ... )
{
    TCHAR szOutput[MAX_PATH];

    if (lDebugLevel <= g_lDebugLevel)    
    {
        va_list argList;
        va_start(argList, pszFormat);
        _vsntprintf(szOutput, MAX_PATH, pszFormat, argList);
        va_end(argList);
        OutputDebugString(szOutput);
    }
}


#define _8BIT_SILENCE       0x80
#define _8BIT_AMPLITUDE     0x7f

#define _16BIT_SILENCE      0
#define _16BIT_AMPLITUDE    0x7FFF

#define _20BIT_SILENCE      0
#define _20BIT_AMPLITUDE    0x7FFFF

#define _24BIT_SILENCE      0
#define _24BIT_AMPLITUDE    0x7FFFFF

#define _FLOAT_SILENCE      0.0f
#define _FLOAT_AMPLITUDE    0.5f

#define _2pi                6.283185307179586476925286766559



// ------------------------------------------------------------------------------
//
//  GeneratePCMTestTone
//
//  Description:
//      generates sine wave test tone of specified frequency in specified buffer
//
//  Arguments:
//      PWAVEFORMATEX   pwfx            : Format descriptor.
//      LPVOID          pDataBuffer     : Data buffer to fill.
//      DWORD           cbDataBuffer    : Buffer size
//      double          dFreq           : Frequncy of the sine wave.
//
// ------------------------------------------------------------------------------

BOOL 
GeneratePCMTestTone
( 
    LPVOID          pDataBuffer, 
    DWORD           cbDataBuffer,
    UINT            nSamplesPerSec,
    UINT            nChannels,
    WORD            wBitsPerSample,
    double          dFreq,
    double          dAmpFactor
)
{
    BOOL        fRes = TRUE;
    double      dSinVal,
                dAmpVal,
                dK = dFreq * _2pi / (double)nSamplesPerSec;

    UINT i, iend, c;
    UINT j;

    UINT cwf = _clearfp();

    _controlfp(_MCW_RC, _RC_NEAR);
 
    switch(wBitsPerSample)
    {
        case 8:
        {
            PBYTE pbData;

            // Initialize the buffer to silence
            memset( pDataBuffer, _8BIT_SILENCE, cbDataBuffer );

            pbData = (PBYTE)pDataBuffer;

            iend = cbDataBuffer;
            for(j = 0, i = 0; i < iend;)
            {
                dSinVal = cos( (double)j * dK );
                dAmpVal = _8BIT_AMPLITUDE * dSinVal + _8BIT_SILENCE;
                dAmpVal *= dAmpFactor;
                for(c = 0; (c < nChannels) && (i < iend); c++)
                    pbData[i++] = (BYTE)(dAmpVal);
                
                j++;
            }
            break;
        }

        case 16:
        {
            PSHORT psData;

            // Initialize the buffer to silence
            memset( pDataBuffer, _16BIT_SILENCE, cbDataBuffer );

            psData = (PSHORT)pDataBuffer;

            iend = cbDataBuffer / 2;
            for(j = 0, i = 0; i < iend;)
            {
                dSinVal = cos( (double)j * dK );
                dAmpVal = _16BIT_AMPLITUDE * dSinVal;
                dAmpVal *= dAmpFactor;
                
                for(c = 0; (c < nChannels) && (i < iend); c++)
                    psData[i++] = (SHORT)(dAmpVal);

                j++;
            }
            break;
        }
    
        case 20:
        {
            PDWORD  pdwData;

            // Initialize the buffer to silence
            memset( pDataBuffer, _20BIT_SILENCE, cbDataBuffer );

            pdwData = (PDWORD)pDataBuffer;

            iend = cbDataBuffer / 4;
            for(j = 0, i = 0; i < iend; )
            {
                dSinVal = cos( (double)j * dK );
                dAmpVal = _20BIT_AMPLITUDE * dSinVal;
                dAmpVal *= dAmpFactor;

                for(c = 0; (c < nChannels) && (i < iend); c++)
                    pdwData[i++] = ((DWORD)(dAmpVal)<< 12);

                j++;
            }
            break;
        }
    
        case 24:
        {
            PDWORD  pdwData;

            // Initialize the buffer to silence
            memset( pDataBuffer, _24BIT_SILENCE, cbDataBuffer );

            pdwData = (PDWORD)pDataBuffer;

            iend = cbDataBuffer / 4;
            for(j = 0, i = 0; i < iend; )
            {
                dSinVal = cos( (double)j * dK );
                dAmpVal = _24BIT_AMPLITUDE * dSinVal;
                dAmpVal *= dAmpFactor;

                for(c = 0; (c < nChannels) && (i < iend); c++)
                    pdwData[i++] = ((DWORD)(dAmpVal) << 8);

                j++;
            }
            break;
        }

        case 32:
        {
            PFLOAT  pdData;

            // Initialize the buffer to silence
            memset( pDataBuffer, (DWORD)_FLOAT_SILENCE, cbDataBuffer );

            pdData = (PFLOAT)pDataBuffer;

            iend = cbDataBuffer / 4;
            for(j = 0, i = 0; i < iend; )
            {
                dSinVal = cos( (double)j * dK );
                dAmpVal = _FLOAT_AMPLITUDE * dSinVal;
                dAmpVal *= dAmpFactor;

                for(c = 0; (c < nChannels) && (i < iend); c++)
                    pdData[i++] = (FLOAT)dAmpVal;

                j++;
            }
            break;
        }

        default:
            fRes = FALSE;
            break;
    }

    _controlfp(_MCW_RC, (cwf & _MCW_RC));

    return fRes;
}


