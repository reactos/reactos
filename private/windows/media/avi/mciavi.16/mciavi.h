/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *
**	Copyright (C) Microsoft Corporation 1991-1993. All rights reserved.
**
**	Title: mciavi.h - Multimedia Systems Media Control Interface
**	AVI driver external header file
**
**	Version:	1.00	
**
**	Date:		16-JUL-1992
**
**	Depends on MMSYSTEM.H and WINDOWS.h
*/

/************************************************************************/


/*
** These three flags apply to the 'play' command:
**	play <alias> window		Play in normal window
**	play <alias> fullscreen		Play in 320x240 full-screen mode
**	play <alias> fullscreen by 2	Play fullscreen, zoomed by 2
*/
#define MCI_MCIAVI_PLAY_WINDOW		0x01000000L
#define	MCI_MCIAVI_PLAY_FULLSCREEN	0x02000000L
#define MCI_MCIAVI_PLAY_FULLBY2		0x04000000L
/*
** Debugging constants for AVI diagnostics
*/
/* 
** Returns number of frames not drawn during last play.  If this number
** is more than a small fraction of the number of frames that should have
** been displayed, things aren't looking good.
*/
#define MCI_AVI_STATUS_FRAMES_SKIPPED		0x8001L
/*
** Returns a number representing how well the last AVI play worked.
** A result of 1000 indicates that the AVI sequence took the amount
** of time to play that it should have; a result of 2000, for instance,
** would indicate that a 5-second AVI sequence took 10 seconds to play,
** implying that the audio and video were badly broken up.
*/
#define MCI_AVI_STATUS_LAST_PLAY_SPEED		0x8002L
/*
** Returns the number of times that the audio definitely broke up.
** (We count one for every time we're about to write some audio data
** to the driver, and we notice that it's already played all of the
** data we have.
*/
#define MCI_AVI_STATUS_AUDIO_BREAKS		0x8003L


#define MCI_AVI_SETVIDEO_DRAW_PROCEDURE		0x8000L


/*
** This constant specifies that the "halftone" palette should be
** used, rather than the default palette.
*/
#define MCI_AVI_SETVIDEO_PALETTE_HALFTONE       0x0000FFFFL

/*
**	Custom error return values
*/
#define MCIERR_AVI_OLDAVIFORMAT		(MCIERR_CUSTOM_DRIVER_BASE + 100)
#define MCIERR_AVI_NOTINTERLEAVED	(MCIERR_CUSTOM_DRIVER_BASE + 101)
#define MCIERR_AVI_NODISPDIB		(MCIERR_CUSTOM_DRIVER_BASE + 102)
#define MCIERR_AVI_CANTPLAYFULLSCREEN	(MCIERR_CUSTOM_DRIVER_BASE + 103)
#define MCIERR_AVI_TOOBIGFORVGA		(MCIERR_CUSTOM_DRIVER_BASE + 104)
#define MCIERR_AVI_NOCOMPRESSOR         (MCIERR_CUSTOM_DRIVER_BASE + 105)
#define MCIERR_AVI_DISPLAYERROR         (MCIERR_CUSTOM_DRIVER_BASE + 106)
#define MCIERR_AVI_AUDIOERROR		(MCIERR_CUSTOM_DRIVER_BASE + 107)
#define MCIERR_AVI_BADPALETTE		(MCIERR_CUSTOM_DRIVER_BASE + 108)
