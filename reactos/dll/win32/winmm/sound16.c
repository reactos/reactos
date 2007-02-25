/*
 * 16-bit sound support
 *
 *  Copyright  Robert J. Amstadt, 1993
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include "windef.h"
#include "winbase.h"
#include "wine/windef16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sound);

/***********************************************************************
 *		OpenSound (SOUND.1)
 */
INT16 WINAPI OpenSound16(void)
{
  FIXME("(void): stub\n");
  return -1;
}

/***********************************************************************
 *		CloseSound (SOUND.2)
 */
void WINAPI CloseSound16(void)
{
  FIXME("(void): stub\n");
}

/***********************************************************************
 *		SetVoiceQueueSize (SOUND.3)
 */
INT16 WINAPI SetVoiceQueueSize16(INT16 nVoice, INT16 nBytes)
{
  FIXME("(%d,%d): stub\n",nVoice,nBytes);
  return 0;
}

/***********************************************************************
 *		SetVoiceNote (SOUND.4)
 */
INT16 WINAPI SetVoiceNote16(INT16 nVoice, INT16 nValue, INT16 nLength,
                            INT16 nCdots)
{
  FIXME("(%d,%d,%d,%d): stub\n",nVoice,nValue,nLength,nCdots);
  return 0;
}

/***********************************************************************
 *		SetVoiceAccent (SOUND.5)
 */
INT16 WINAPI SetVoiceAccent16(INT16 nVoice, INT16 nTempo, INT16 nVolume,
                              INT16 nMode, INT16 nPitch)
{
  FIXME("(%d,%d,%d,%d,%d): stub\n", nVoice, nTempo,
	nVolume, nMode, nPitch);
  return 0;
}

/***********************************************************************
 *		SetVoiceEnvelope (SOUND.6)
 */
INT16 WINAPI SetVoiceEnvelope16(INT16 nVoice, INT16 nShape, INT16 nRepeat)
{
  FIXME("(%d,%d,%d): stub\n",nVoice,nShape,nRepeat);
  return 0;
}

/***********************************************************************
 *		SetSoundNoise (SOUND.7)
 */
INT16 WINAPI SetSoundNoise16(INT16 nSource, INT16 nDuration)
{
  FIXME("(%d,%d): stub\n",nSource,nDuration);
  return 0;
}

/***********************************************************************
 *		SetVoiceSound (SOUND.8)
 */
INT16 WINAPI SetVoiceSound16(INT16 nVoice, DWORD lFrequency, INT16 nDuration)
{
  FIXME("(%d, %ld, %d): stub\n",nVoice,lFrequency, nDuration);
  return 0;
}

/***********************************************************************
 *		StartSound (SOUND.9)
 */
INT16 WINAPI StartSound16(void)
{
  return 0;
}

/***********************************************************************
 *		StopSound (SOUND.10)
 */
INT16 WINAPI StopSound16(void)
{
  return 0;
}

/***********************************************************************
 *		WaitSoundState (SOUND.11)
 */
INT16 WINAPI WaitSoundState16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		SyncAllVoices (SOUND.12)
 */
INT16 WINAPI SyncAllVoices16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		CountVoiceNotes (SOUND.13)
 */
INT16 WINAPI CountVoiceNotes16(INT16 x)
{
    FIXME("(%d): stub\n", x);
    return 0;
}

/***********************************************************************
 *		GetThresholdEvent (SOUND.14)
 */
LPINT16 WINAPI GetThresholdEvent16(void)
{
    FIXME("(void): stub\n");
    return NULL;
}

/***********************************************************************
 *		GetThresholdStatus (SOUND.15)
 */
INT16 WINAPI GetThresholdStatus16(void)
{
    FIXME("(void): stub\n");
    return 0;
}

/***********************************************************************
 *		SetVoiceThreshold (SOUND.16)
 */
INT16 WINAPI SetVoiceThreshold16(INT16 a, INT16 b)
{
    FIXME("(%d,%d): stub\n", a, b);
    return 0;
}

/***********************************************************************
 *		DoBeep (SOUND.17)
 */
void WINAPI DoBeep16(void)
{
    FIXME("(void): stub!\n");
}




