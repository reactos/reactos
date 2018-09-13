// jgplay.h -- Public Header for ART 3.5 Player
// --------------------------------------------------------------------
// Copyright (c) 1995 Johnson-Grace Company, all rights reserved
//
// Change History
// --------------
// 950820 DLB Original File
// 960426 Eric Rodriguez-Diaz
//		- For Mac only, added import pragmas.
// 960429 Eric Rodriguez-Diaz
//		- Improved import pragmas for Mac.
// 960605 Eric Rodriguez-Diaz
//		- Now taking into account the fact that for the Mac symbol
//		  GENERATINGCFM is always defined in the universal headers.
// 960606 SED Added UI hook functions.
// 960613 SED Added Art file attributes; removed HasAudio, HasImage,
//			  HasMIDI. Added memory calls for UI DLL.  Added Lossless
//			  Decompression calls for ARTDoc. Added memory hooks for
//			  MAC.  Also added memory function struct arg to
//			  JgPlayStartUp for MAC.  Added ShowIsOver, TitleReady,
//			  JGPLAY_ERR_FULL and JGPLAY_MAX_INPUT for Slide Shows.
//			  Added JgPlayFreeImage.
// 960617 SED Added JGPLAY_PALETTE and modifed JgPlayGetPalette to
//			  use it.
// 960618 SED Added the UI Report hook and modified the UI Input hook.
// 960619 SED Removed JgPlayFreeImage.  Modified JgPlayGetImage to
//			  use JGPLAY_IMAGE_REF and JgPlayGetPalette to use
//			  JGPLAY_PALETTE_REF.  Added the JgPlayStopPlay element
//			  argument and the element constants.  Added a Size member
//			  to the JGPLAY_MEMORY structure.  Changed the JGPLAY_SETUP
//			  DefaultPalette member to a JGPLAY_PALETTE_REF.
// 960621 SED Added Number of colors parameter to JgPlayGetPalette.
// 960621 SED Changed JGPLAY_ERR_OLDERROR to JGPLAY_ERR_PREVIOUS.
//			  Removed JGPLAY_ERR_IMGDECODER.
// 960625 SED Added JgPlayPausePlay, JgPlayResumePlay, JgPlaySetPosition
//			  and JgPlayGetPosition.  Removed element-to-stop arg from
//			  JgPlayStopPlay.  Removed StopTime from JgPlayStartPlay and
//			  StopTime from JgPlayStopPlay. Added UI hooks defines for
//			  JgPlayPausePlay and JgPlayResumePlay.
// 960626 SED Added JGHandle to JGPLAY_SETUP structure to support reuse
// 			  of the JGDW context.
// 960628 SED Added PaletteMode and IndexOverride and associated constants
//			  to JGPLAY_SETUP structure.  
// 960708 SED Changed JGPLAY_XXX to JGP_XXX and JgPlayXXX to JgpXXXX.
// 960709 SED Changed JGPTR to JGFAR * and JGHPTR to JGHUGE *.  Changed
//			  BOOLW to UINTW and removed convenience typedefs.
// 960710 Eric Rodriguez-Diaz
//	- For the Mac, now have pragmas forcing power structure alignment.
//	  Not using native alignment because it isn't supported by
//	  the Symantec compilers. Not using mac68k alignment because 68k
//	  is a dying breed.
// 960805 SED Added Slide Show support.
// --------------------------------------------------------------------

#ifndef JGPLAY_H
	#define JGPLAY_H 1

	#ifdef _MAC
		#include <Types.h>
		#include <Palettes.h>
		#include <QuickDraw.h>
		#include <QDOffscreen.h>
	#else
		#pragma warning(disable:4201)
		#include <windows.h>
		#pragma warning(default:4201)
	#endif

	#include "jgtypes.h"	// Include this for basic JG Types
	
	#ifdef __cplusplus
		extern "C" {         	// Indicate C declarations if C++
	#endif
	
	#ifdef _MAC
		// make sure GENERATINGCFM is defined
		#include <ConditionalMacros.h>
		#if GENERATINGCFM
			#pragma import on
		#endif
		// select PowerPC structure alignment
		#pragma options align=power
	#endif
	

	// --------------------------------------------------------------------
	// Error Codes

	#define JGP_ERR_BASE    3000  // Get this from the registor. (not yet)
	#define JGP_SUCCESS     0     // Always true at JG.

	#define JGP_ERR_MEM				((JGP_ERR_BASE +  1) | JG_ERR_MEMORY)
	#define JGP_ERR_INIT			((JGP_ERR_BASE +  2) | JG_ERR_CHECK)
	#define JGP_ERR_BADDATA			((JGP_ERR_BASE +  3) | JG_ERR_DATA)
	#define JGP_ERR_BADARG			((JGP_ERR_BASE +  4) | JG_ERR_ARG)
	#define JGP_ERR_CODECHECK		((JGP_ERR_BASE +  5) | JG_ERR_CHECK)
	#define JGP_ERR_PREVIOUS		((JGP_ERR_BASE +  6) | JG_ERR_ARG)
	#define JGP_ERR_NOTREADY		((JGP_ERR_BASE +  7) | JG_ERR_CHECK)
	#define JGP_ERR_RESOURCE		((JGP_ERR_BASE +  8) | JG_ERR_CHECK)
	#define JGP_ERR_BADSTRUCTSIZE	((JGP_ERR_BASE +  9) | JG_ERR_CHECK)
	#define JGP_ERR_AUDIODECODER	((JGP_ERR_BASE + 10) | JG_ERR_CHECK)
	#define JGP_ERR_MIDIDECODER    	((JGP_ERR_BASE + 11) | JG_ERR_CHECK)
	#define JGP_ERR_VOLUME			((JGP_ERR_BASE + 12) | JG_ERR_CHECK)
	#define JGP_ERR_NONOTE			((JGP_ERR_BASE + 13) | JG_ERR_CHECK)
	#define JGP_ERR_UNSUPPORTED		((JGP_ERR_BASE + 14) | JG_ERR_DATA)
	#define JGP_ERR_NOPALETTE		((JGP_ERR_BASE + 15) | JG_ERR_CHECK)
	#define JGP_ERR_FULL			((JGP_ERR_BASE + 16) | JG_ERR_CHECK)
	#define JGP_ERR_OPENHANDLE		((JGP_ERR_BASE + 17) | JG_ERR_CHECK)
	#define JGP_ERR_NOTAVAILABLE    ((JGP_ERR_BASE + 18) | JG_ERR_ARG)
	#define JGP_ERR_BADSTATE        ((JGP_ERR_BASE + 19) | JG_ERR_ARG)
	#define JGP_ERR_UIMODULE        ((JGP_ERR_BASE + 20) | JG_ERR_UNKNOWN)

	// Error Codes dealing with Library Startup/Shutdown
	#define	JGP_ERR_IGNORED_MEMORY_HOOKS	((JGP_ERR_BASE + 21) | JG_ERR_UNKNOWN)
	#define	JGP_ERR_BAD_SHUTDOWN			((JGP_ERR_BASE + 22) | JG_ERR_UNKNOWN)

	// Max number of input bytes accepted at once by JgpInputStream,
	// one-half a mega byte.
	#define JGP_MAX_INPUT 524288

	// Max length of ART Note
	#define JGP_ARTLENGTH 200

	// --------------------------------------------------------------------
	// Options for scaling.

	#define JGP_SCALE_NONE 		  	0	// No Scaling
	#define JGP_SCALE_EXACT        	1	// Scale to ScaleWidth and ScaleHeight
	#define JGP_SCALE_BESTFIT      	2	// Maintain aspect ratio, use best fit

	// --------------------------------------------------------------------
	// Options for gamma correction.

	#define JGP_GAMMA_NONE		0
	#define JGP_GAMMA_UP		1  	
	#define JGP_GAMMA_DOWN 		2

	// --------------------------------------------------------------------
	// File Types

	#define JGP_UNSUPPORTED 0
	#define JGP_ART	JG4C_ART
	#define JGP_BMP	JG4C_BMP
	#define JGP_GIF	JG4C_GIF
	#define JGP_JPG	JG4C_JPEG

	// --------------------------------------------------------------------
	// File Attributes

	#define JGP_HASIMAGE 			1
	#define JGP_HASAUDIO 		    2
	#define JGP_HASMIDI				4
	#define JGP_HASARTNOTE			8
	#define JGP_HASDYNAMICIMAGES	16
	#define JGP_ISTEMPORAL			32
	#define JGP_HASPAUSE			64
	#define JGP_HASTIMELINE			128

	// --------------------------------------------------------------------
	// Audio Modes.  Choose one of these for playback or zero to choose the
	// mode with the lowest processing cost on the CPU (usually 11K, 16bit).

	#define JGP_AUDIO_DEFAULT	0x0000
	#define JGP_AUDIO_11K_8BIT	0x0001
	#define JGP_AUDIO_11K_16BIT	0x0002
	#define JGP_AUDIO_22K_8BIT	0x0004
	#define JGP_AUDIO_22K_16BIT	0x0008

	// --------------------------------------------------------------------
	// Common UI hooks.

	#define JGP_CLOSEUIHOOK		0	// JgPClose HEAD hook
	#define JGP_STARTUIHOOK		1	// JgPStartPlay TAIL hook
	#define JGP_STOPUIHOOK		2	// JgPStopPlay TAIL hook
	#define JGP_PAUSEUIHOOK		3	// JgPPausePlay TAIL hook
	#define JGP_RESUMEUIHOOK	4	// JgPResumePlay TAIL hook
	#define JGP_MAXUIHOOKS		5	// INCREASE if more hooks are allowed


	// --------------------------------------------------------------------
	// PaletteModes.

	#define JGP_PALETTE_AUTO	0	// Use first available: file, input, 332
	#define JGP_PALETTE_INPUT	1	// Use input palette
	#define JGP_PALETTE_332		2	// Use 332

	// --------------------------------------------------------------------
	// Use this constant to turn off IndexOverride option.

	#define JGP_OVERRIDE_NONE	0xFFFF

	// --------------------------------------------------------------------
	// Typedefs for System dependent Image and Palette data types. 

	#ifndef _MAC
		typedef HGLOBAL JGP_IMAGE_REF;
		typedef HGLOBAL JGP_PALETTE_REF;
	#else
		typedef GWorldPtr JGP_IMAGE_REF;
		typedef PaletteHandle JGP_PALETTE_REF;
	#endif

	// These typedefs must match the jgdw lossless typedefs.

	typedef struct {
		UINT16 nSize;                // Size of structure in bytes
		INT16  SearchSize;           // (Compression control)
		UINT32 CompressedSize;       // Total compressed block bytes
		UINT32 CompressedSoFar;      // Compressed processed so far
		UINT32 CompressedLastCall;   // Compressed processed last call
		UINT32 DecompressedSize;     // Total decompressed block bytes
		UINT32 DecompressedSoFar;    // Decompressed processed so far
		UINT32 DecompressedLastCall; // Decompressed processed last call
	} JGP_LOSSLESS;

	// lossless decompression handle type
	typedef void JGFAR * JGP_LOSSLESS_HANDLE; 


	// --------------------------------------------------------------------
	// JGP_IMG -- Defines the exported image format for the ART file Player.
	// This format is designed to be system independent, but it can carry
	// information that is helpful in the system dependent conversion process.
	// It is also  designed to be very close to JG_IMG which is used by JGIPROC.
	// It can also hold the data in the form of a DIB for windows, or a  GWorld
	// for a MAC.

	typedef struct {
		UINTW Rows;				// Rows, in pixels
		UINTW Cols;				// Cols, in pixels
		UINTW Colordepth;		// Colordepth: 4, 8, 16 or 24
		UINT8 JGHUGE *pPixels;	// Pointer to top row of pixels in this image
		INTW  RowDisp;			// Offset between rows in this image
		UINTW nColors;			// Number of colors in the palette.
		JG_BGRX JGFAR *pPalette;// Pointer to list of colors for palette. 
		void JGFAR *pManager;	// Reserved.  (Internally maintained ptr.)
	} JGP_IMG;

	// --------------------------------------------------------------------
	// JGRECT -- A rectangle structure, defined to be equivalent to the
	// Window's RECT.  Used here to avoid machine dependance.  These rects
	// assume that screen coordinates run top (starting at zero) to bottom
	// (positive increase) and from left (starting at zero) to right 
	// (positive increase).  We also assume that the bottom-right border
	// defined by the structure is not part of the rectanglar area.

	typedef struct {
		INTW left;				// Location of left border
		INTW top;				// Location of top border
		INTW right;				// Location of right border, (plus 1)
		INTW bottom;			// Location of bottom border, (plus 1)
	} JGRECT;

	// --------------------------------------------------------------------
	// JGP_REPORT -- This structure is used to obtain information
	// about a specific show which can be idenitified by an instance handle.

	typedef struct {
		UINTW	Size;				// Size of this struct in bytes, set by caller
		UINTW   ImageValid;			// True if the image is valid.
		UINT32  CurrentTime;		// Current Time of play, in ms.
		UINT32  AvailPlayTime;		// Total Play Time in buffer from start to end.	
		UINTW   DoingAudio;			// Non-zero if audio is playing.
		UINTW   DoingMIDI;			// Non-zero if MIDI is playing.
		UINTW   ShowStalled;		// If nothing else to do, ... Waiting for input
		UINTW   GotEOF;				// If this show has received an EOF mark
		UINTW   IsDone;				// The presentation is finished.
		UINTW   UpdateImage;		// Non-zero if the image needs an update.
		UINTW   TransparentIndex;	// Index of transparent color.
		JGRECT	UpdateRect;			// Area in image that has changed.
		UINTW	ShowIsOver;			// The show is finished playing.
		UINTW	TitleReady;			// Title page is ready.
		UINTW	IsPaused;			// The show is paused.
		UINTW   IsPlaying;			// The show is playing
	} JGP_REPORT;

	// --------------------------------------------------------------------
	// JGP_SETUP -- This structure is used to communicate the input parameters
	// for the playing of a show.

	typedef struct {
		UINTW Size;				// Size of this struct in bytes, set by caller
		UINTW ColorDepth;		// Output colordepth  (Allowed: 4, 8, 16 or 24)
		UINTW InhibitImage;		// If true, no image returned.
		UINTW InhibitAudio;		// If true, no audio played.
		UINTW InhibitMIDI;		// If true, no MIDI played.
		UINTW InhibitDither;	// If true, no dithering is done
		UINTW InhibitSplash;	// If true, no splash with image	
		UINTW AudioMode;		// Desired Audio mode, or zero for default.
		UINTW CreateMask;		// If true, create mask imacdge
		UINTW ScaleImage;		// Scaling option
		UINTW ScaleWidth;		// Scaled width
		UINTW ScaleHeight;		// Scaled height
		UINTW  GammaAdjust;		// Gamma adjustment value
	    JG_RGBX BackgroundColor;// Background color under the image.
	    JGP_PALETTE_REF DefaultPalette; // Default Palette, NULL if none
		UINTW PaletteSize; 		// Number of Colors in Default Palette, or 0
		JGHANDLE OldHandle;		// Handle for image context reuse
		UINTW	PaletteMode;	// Specifies palette mode
		UINTW	IndexOverride;	// Transparent override/create index
		UINTW	TempMemory;		// TRUE to use temporary memory 
		UINT32  InputBaud;		// Input Baud if known by app. Zero otherwise.
	} JGP_SETUP;

	// --------------------------------------------------------------------
	// JGP_STREAM -- This structure is used to return information
	// about the ART Stream

	typedef struct {
		UINTW Size;				// Size of this struct in bytes, set by caller
		UINTW MajorVersion;		// Major version of the ART tstream.
		UINTW MinorVersion;		// Subversion of the ART stream.
		UINTW CanDecode;		// TRUE if this module can decode the stream.
		UINT32 Filetype;		// JGPL_xxx returned constant
		UINT32 Attributes;		// File attributes	
		UINT32 PlayTime;		// Playtime if temporal, non-realtime stream.
		JGRECT Dimensions;		// Native dimensions of stream if has image(s).
		UINTW  ColorDepth;		// Color Depth of Stream (4,8,16,24)
		UINT32 UpdateRate;		// Minimum ms between heartbeat calls.
		UINT32 Baud;			// Minimum input baud rate.
		UINT32 BytesToTitle;	// Number of bytes needed to get title image.	
		UINTW  SubType;			// ART subtype ID
	} JGP_STREAM;

	// --------------------------------------------------------------------
	// JGP_TEST -- This structure contains the information about the
	// test to see if the CPU and other environment is up to doing the 
	// show in real-time.

	typedef struct {
		UINTW	Size;				// Size of this struct in bytes, set by caller
		UINTW   CanDoAudio;			// Non-zero if audio can be part of the show.
		UINTW   CanDoMIDI;		    // Non-zero if MIDI	 can be part of the show.
		UINTW   PrefAudioMode;		// Preferred audio mode.
	} JGP_TEST;

	// --------------------------------------------------------------------
	// JGP_NOTE -- This structure contains information about the ART note.

	typedef struct {
		UINTW Size;					// Size of this struct in bytes, set by caller
		UINTW Display;				// If true ART note should always be displayed
		UINTW Copyrighted;			// If true the note is copyrighted
	} JGP_NOTE;


	// --------------------------------------------------------------------
	// Generic palette pointer for MAC and Windows

	typedef void JGFAR * JGP_PALETTE;

	// --------------------------------------------------------------------
	// Memory function typedefs

	typedef void JGFAR * (JGFFUNC * JGMEMALLOC) (
		UINT32 RequestedSize		// In: Number of bytes to allocate
	);

	typedef void JGFAR * (JGFFUNC * JGMEMREALLOC) (
		void JGFAR *pBuffer,		// In: Allocated buffer pointer
		UINT32 RequestedSize		// In: Number of bytes to allocate
	);

	typedef void (JGFFUNC * JGMEMFREE) (
		void  JGFAR *pBuffer		// In: Allocated buffer pointer
	);

	typedef struct {
		UINTW Size;					// Size of this struct in bytes, set by caller
		JGMEMALLOC MemAlloc;		// Allocation function pointer
		JGMEMREALLOC MemReAlloc;    // Re-Allocation function pointer
		JGMEMFREE MemFree;          // Free function pointer
	} JGP_MEMORY;

	// --------------------------------------------------------------------
	// UI hooks typedef and constants.

	typedef JGERR (JGFFUNC * JGP_UIHOOK) (
		JGHANDLE Handle,		// In: Instance handle	
		JGERR	 Err			// In: Calling function status
	);

	typedef JGERR (JGFFUNC * JGP_INPUT_UIHOOK) (
		JGHANDLE Handle,		// In: Instance handle
		UINT8  JGHUGE *pARTStream,// In: Pointer to the ART Stream
		UINT32   nBytes,		// In: Number of bytes being input	
		JGERR	 iErr			// In: Calling function status
	);

	typedef JGERR (JGFFUNC * JGP_REPORT_UIHOOK) (
		JGHANDLE Handle,		// In: Instance handle	
		JGP_REPORT JGFAR *pReport,// In: Report structure
		JGERR	 Err			// In: Calling function status
	);

	// --------------------------------------------------------------------
	// JgpStartUp{} -- This function can be called when the library is
	// started.  Under Windows, the LibMain does this.

	JGERR JGFFUNC JgpStartUp(
		JGP_MEMORY JGFAR *pMemFcns);	// In: Memory function struct pointer

	// --------------------------------------------------------------------
	// JgpShutDown{} -- This function can be called to shut the player 
	// down.  Under Windows, the WEP function does this.

	JGERR JGFFUNC JgpShutDown(void);

	// --------------------------------------------------------------------
	// JgpHeartBeat{} -- This function is used to keep the Show going.
	// This function or JgpInputStream must be called every 100 ms.

	JGERR JGFFUNC JgpHeartBeat(
		JGHANDLE SHandle);		// In: Show handle

	// --------------------------------------------------------------------
	// JgpQueryStream{} -- This is a utility function, which if given the
	// first part of an ART Stream will return useful info about it.  Usually,
	// the data to fill the info structure can be found in the first
	// 100 bytes of a Stream, but not necessarily.

	JGERR JGFFUNC JgpQueryStream(
		UINT8 JGHUGE *pARTStream,		// In: ART Stream
		UINT32 nARTStreamBytes,			// In: Size of ARTStream in Bytes
		JGP_STREAM JGFAR *pInfo); 		// Out: Info structure

	// --------------------------------------------------------------------
	// JgpDoTest{} -- Perform a test to determine ability of CPU to do
	// the show in real-time.

	JGERR JGFFUNC JgpDoTest(
		JGP_TEST JGFAR *pInfo);			// In: Info struct to be filled

	// --------------------------------------------------------------------
	// JgpOpen{} -- This function is used to obtain a handle for a
	// show.

	JGERR JGFFUNC JgpOpen(
		JGHANDLE JGFAR *pSHandle,	// Out: Place to receive handle   
		JGP_SETUP JGFAR *pSetup);	// In: The setup structure

	// --------------------------------------------------------------------
	// JgpClose{} -- This function frees all the resources associated
	// with a show.  If the show (and sound) is playing, it is immediately
	// stopped.

	JGERR JGFFUNC JgpClose(
		JGHANDLE SHandle);			// In: Show handle

	// --------------------------------------------------------------------
	// JgpSetEOFMark{} -- Sets and EOF mark for the ART stream.
	// This tells the player that no more data is expected for this stream,
	// and if, during play, the EOF mark is reached, to shut down play.
	// Otherwise, if an out-of-data condition occures, play goes into a
	// suppended state which continues to hold onto output resources of the
	// computer.

	JGERR JGFFUNC JgpSetEOFMark(
		JGHANDLE SHandle);			// In: Show handle

	// --------------------------------------------------------------------
	// JgpInputStream{} -- Accepts the Show stream. Call in a loop!
	// Must be called every XX ms even if there is no new data.
	//
	// Note, by calling this routine, the show does not automatically
	// start.  Call JgpStartShow to do this.

	JGERR JGFFUNC JgpInputStream(
		JGHANDLE SHandle,			// In: Show Handle
		UINT8  JGHUGE *pARTStream,	// In: Pointer to the ART Stream
		UINT32 nBytes);				// In: Number of bytes being input

	// --------------------------------------------------------------------
	// JgpStartPlay{} -- Starts play of a show at an arbitrary point
	// in the stream.  The arbitrary point in the stream can be specifed
	// using the JgpSetPostion. The call can be made anytime after the
	// stream is opened, even if there is no data.  In a download situation,
	// the show is not started until there is enough data to play at the
	// given starttime plus 5 seconds.

	JGERR JGFFUNC JgpStartPlay(
		JGHANDLE SHandle);			// In: Show Handle

	// --------------------------------------------------------------------
	// JgpResumePlay{} -- Starts play of an ART stream at the point
	// in the stream at which it was paused.

	UINTW JGFFUNC JgpResumePlay(
		JGHANDLE SHandle);			// In: Show Handle

	// --------------------------------------------------------------------
	// JgpPausePlay{} -- Causes play of an ART stream to stop.  The
	// stop time is saved for later resuming.
		
	UINTW JGFFUNC JgpPausePlay(
		JGHANDLE SHandle);			// In: Show Handle

	// --------------------------------------------------------------------
	// JgpStopPlay{} -- Stops the play of the show.   

	JGERR JGFFUNC JgpStopPlay(
		JGHANDLE SHandle);			// In: Show Handle
		
	// --------------------------------------------------------------------
	// JgpReleaseSound{} -- Disconnects the instance from the sound devices.

	JGERR JGFFUNC JgpReleaseSound(
		JGHANDLE SHandle);			// In: Show Handle	

	// --------------------------------------------------------------------
	// JgpResumeSound{} -- Reconnects the instance to the sound devices.

	JGERR JGFFUNC JgpResumeSound(
		JGHANDLE SHandle);			// In: Show Handle	
	
#ifdef _MAC
	// --------------------------------------------------------------------
	// JgpGetVolume{} -- Gets sound volume for a show instance.
	
	JGERR JGFFUNC JgpGetVolume(
		JGHANDLE SHandle,
		UINTW JGFAR *pnOutVolume
	);
	
	// --------------------------------------------------------------------
	// JgpSetVolume{} -- Sets sound volume for a show instance.
	
	JGERR JGFFUNC JgpSetVolume(
		JGHANDLE SHandle,
		UINTW nInVolume
	);
#endif

	// --------------------------------------------------------------------
	// JgpSetPosition{} -- This function sets the stop/pause position
	// time associated with the Show instance.
		
	JGERR JGFFUNC JgpSetPosition(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 nPosition);				// In: Position

	// --------------------------------------------------------------------
	// JgpGetPosition{} -- This function gets the stop/pause position
	// time associated with the Show instance.
		
	JGERR JGFFUNC JgpGetPosition(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 JGFAR *pPosition);		// Out: Position

	// --------------------------------------------------------------------
	// JgpGetImage{} -- Returns a system independent representation of
	// the image.  
	//
	// Depending on the platform phImg is either a GWorldPtr or a huge
	// pointer to a DIB.  The caller must lock the returned HGLOBAL to use
	// the DIB.

	JGERR JGFFUNC JgpGetImage(
		JGHANDLE SHandle,				// In: Show handle            
		JGP_IMAGE_REF JGFAR *phImg);	// Out: Handle to Image memory

	// --------------------------------------------------------------------
	// JgpGetMask{} -- Returns a system independent representation of
	// the mask image.  
	//
	// Depending on the platform phImg is either a BitMapPtr or a huge
	// pointer to a DIB.  The caller must lock the returned HGLOBAL to use
	// the DIB.

	JGERR JGFFUNC JgpGetMask(
		JGHANDLE SHandle,				// In: Show handle
		JGP_IMAGE_REF JGFAR *phImg);	// Out: Handle to Image memory

	// --------------------------------------------------------------------
	// JgpGetPalette{} -- Returns a system independent representation
	// of the image palette.  
	//
	// The caller is responsible for freeing the memory buffer space.
	// Depending on the platform pPal is either a pointer to a PaletteHandle
	// or a pointer to PALETTEENTRY structures.

	JGERR JGFFUNC JgpGetPalette(
		JGHANDLE SHandle,				// In: Show Handle
		JGP_PALETTE_REF JGFAR *phPal, 	// Out: Palette data	
		UINTW JGFAR	*pnColors);			// Out: Number of colors

	// --------------------------------------------------------------------
	// JgpGetARTNote{} -- Returns the ART note.

	JGERR JGFFUNC JgpGetARTNote(
		UINT8 JGHUGE *pARTStream,		// In: ART Stream
		UINT32 nARTStreamBytes,			// In: Size of ARTStream in Bytes
		JGP_NOTE JGFAR *pNote,			// Out: Info structure
		UINT8 JGFAR *pData);				// Out: Note text

	// --------------------------------------------------------------------
	// JgpGetReport{} -- Reports the activity of a show, given it's handle.
	// This is usually called after 'bImageUpdate' becomes true.

	JGERR JGFFUNC JgpGetReport(
		JGHANDLE SHandle,			// In:  Show Handle
		JGP_REPORT JGFAR *pReport);	// Out: Structure to receive the report
		
	// --------------------------------------------------------------------
	// JgpSetUiHook{} -- This function sets a UI function hook.
		
	JGERR JGFFUNC JgpSetUiHook(
		JGP_UIHOOK pHook,				// In: Ui Hook function
		UINTW UiHook);					// In: Hook tag

	// --------------------------------------------------------------------
	// JgpGetUiHook{} -- This function returns a UI function hook.

	JGERR JGFFUNC JgpGetUiHook(
		JGP_UIHOOK JGFAR *hHook,			// Out: Ui Hook function
		UINTW UiHook);					// In: Hook tag	
		
	// --------------------------------------------------------------------
	// JgpSetUiInputHook{} -- This function sets the JgpInputStream
	// UI function hook.
		
	JGERR JGFFUNC JgpSetUiInputHook(
		JGP_INPUT_UIHOOK pHook);		// In: Ui Hook function

	// --------------------------------------------------------------------
	// JgpGetUiInputHook{} -- This function returns the JgpInputStream
	// UI function hook.

	JGERR JGFFUNC JgpGetUiInputHook(
		JGP_INPUT_UIHOOK JGFAR *hHook);	// Out: Ui Hook function

	// --------------------------------------------------------------------
	// JgpSetUiReportHook{} -- This function sets the JgpGetReport
	// UI function hook.
		
	JGERR JGFFUNC JgpSetUiReportHook(
		JGP_REPORT_UIHOOK pHook);		// In: Ui Hook function

	// --------------------------------------------------------------------
	// JgpGetUiReportHook{} -- This function returns the JgpGetReport
	// UI function hook.

	JGERR JGFFUNC JgpGetUiReportHook(
		JGP_REPORT_UIHOOK JGFAR *hHook);	// Out: Ui Hook function

	// --------------------------------------------------------------------
	// JgpSetUiLong{} -- This function sets a UI long value associated
	// with the Show instance.
		
	JGERR JGFFUNC JgpSetUiLong(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 LongVal);				// In: Long value

	// --------------------------------------------------------------------
	// JgpGetUiLong{} -- This function returns the UI long value
	// associated with the Show instance.

	JGERR JGFFUNC JgpGetUiLong(
		JGHANDLE SHandle,				// In: Show Handle
		UINT32 JGFAR *pLongVal);		// Out: Long value
		
	// --------------------------------------------------------------------
	// JgpAlloc{} -- This function allocates memory.  The returned
	// JGFAR * can be cast to HUGE.

	void JGFAR * JGFFUNC JgpAlloc(
		JGHANDLE SHandle,               // In: Show Handle
		UINT32 nBytes);					// In: Bytes to allocate

	// --------------------------------------------------------------------
	// JgpReAlloc{} -- This function re-allocates memory.  The returned
	// JGFAR * can be cast to HUGE.

	void JGFAR * JGFFUNC JgpReAlloc(
		JGHANDLE SHandle,               // In: Show Handle
		void JGFAR *pMem,				// In: Old pointer
		UINT32 nBytes);					// In: Bytes to allocate

	// --------------------------------------------------------------------
	// JgpFree{} -- This function frees memory.

	void JGFFUNC JgpFree(
		JGHANDLE SHandle,               // In: Show Handle
		void JGFAR *pMem);				// In: Pointer to free

	// --------------------------------------------------------------------
	// JgpLosslessQuery{} -- This function interrogates a lossless stream.

	JGERR JGFFUNC JgpLosslessQuery(
	    UINT8 JGHUGE *pInBuffer,   		// In: Beginning of compressed stream
	    UINT32 InBufferSize,         	// In: Bytes in InBuffer (0-n)
	    JGP_LOSSLESS JGFAR *pLosslessInfo); // Out: Stream info returned here

	// --------------------------------------------------------------------
	// JgpLosslessCreate{} -- This function creates a decompression handle.

	JGERR JGFFUNC JgpLosslessCreate(
	    JGP_LOSSLESS_HANDLE JGFAR *pDecHandle); // In: Pointer to new handle

	// --------------------------------------------------------------------
	// JgpLosslessDestroy{} -- This function destroys a decompression handle.

	void JGFFUNC JgpLosslessDestroy(
	    JGP_LOSSLESS_HANDLE DecHandle); // In: Handle from decompress create

	// --------------------------------------------------------------------
	// JgpLosslessReset{} -- This function resets an existing handle.

	JGERR JGFFUNC JgpLosslessReset(
	    JGP_LOSSLESS_HANDLE DecHandle); // In: Handle from decompress create

	// --------------------------------------------------------------------
	// JgpLosslessBlock{} -- This function decompresses a block of data.

	JGERR JGFFUNC JgpLosslessBlock(
	    JGP_LOSSLESS_HANDLE DecHandle,	// In: Handle from decompress create
	    UINT8 JGHUGE *pInBuffer,    	// In: Input (compressed) data
	    UINT32 InBufferSize,          	// In: Bytes at *InBuffer (0-n)
	    UINT8 JGHUGE *pOutBuffer,   	// Out: Output (decompressed result) buff
	    UINT32 OutBufferSize,         	// In: Free bytes at *OutBuffer
	    JGP_LOSSLESS JGFAR *pLosslessInfo);// Out: Updated info returned here

	// --------------------------------------------------------------------
	// JgpLosslessPartitionReset{} -- This function does a new partition reset.

	JGERR JGFFUNC JgpLosslessPartitionReset(
	    JGP_LOSSLESS_HANDLE DecHandle);  // In: Handle from decompress create 
	    

	// ====================================================================
	// UNDOCUMENTED/UNSUPPORTED FUNCTIONS (Below) -- (Use at your own risk...)
	// ====================================================================

	// --------------------------------------------------------------------
	// JGP_PERFORMANCE -- This structure returns performance data.  It can
	// be used to map the CPU and Memory requirements for a Slideshow...

	typedef struct {  
		UINTW Size;				// Size of this structure, in bytes, set by caller
		UINTW IsSS;				// If a slideshow is being played
		UINTW IsValidInfo;		// If info in this structure contains valid info.
		UINTW Mode;				// Show Mode: 0=not Init, 1=Preload, 2=Playable
		UINT32 CPUTime;			// CPU Time (of call)
		UINT32 ShowTime;		// ShowTime for this measurement
		UINTW nAssets;			// Number of known assets.               
		UINT32 nAssetBytes;		// Allocated bytes for storing raw assets
		UINTW nVisPics;			// Number of visible pictures
		UINTW nCashedPics;		// Number of cashed pictures
		UINT32 nPixelBytes;		// Number of allocated bytes for pixels        
		UINT32 tCPUUsed;		// Total ms of CPU useage since last call...
	} JGP_PERFORMANCE;


	// --------------------------------------------------------------------
	// JgpGetPerformance{} -- Reports the performance measurement of a show.

	JGERR JGFFUNC JgpGetPerformance(
		JGHANDLE SHandle,						// In:  Show Handle
		JGP_PERFORMANCE JGFAR *pPerformance);	// Out: Structure to receive the report

	
	#ifdef _MAC
		// restore structure alignment mode
		#pragma options align=reset
		#if GENERATINGCFM
			#pragma import reset
		#endif
	#endif

	#ifdef __cplusplus
		}
	#endif

#endif
