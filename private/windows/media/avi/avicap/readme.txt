*******************************************************************************
NOTE: This tree is no longer used; AVICAP32.DLL is built from the ..\AVICAP.IO
      tree instead.
      --R Jernigan, 6 Dec 1995
*******************************************************************************

Readme.txt 
for AVICap Window Class


Files provided:

\       avicap.dll      Capture Window Class library
        avicap.lib      Import library for avicap.dll
        avicap.h        Include file for the AVICap class
        avicapid.h      Include file for AVICap class status and error IDs

\docs   avicap.doc      Documentation for the window class in WinWord format

\testcap                Directory containing source code for 
                        a test application, Testcap.exe.
                        This application has most of Vidcap's functionality.


Beta Release  August 93 Changes:
================================
1. Added wChunkGranularity, dwIndexSize, fStepCaptureAt2x, and 
wStepCaptureAverageFrames fields to the CAPTUREPARMS structure.  

2. Added WM_CAP_FILE_SAVEDIB message to save the current frame as a
DIB.

3. Added WM_CAP_FILE_SET_INFOCHUNK message.  This allows you to embed
copyrights, keywords, etc. into your AVI file.


Alpha Release 18 May 93 Changes:
================================
1. CAPSTATUS structure has additional fields including:
        hPalCurrent     - The current palette used for capture
        fCapturingNow   - If TRUE, streaming capture in progress
        dwReturn        - Return status value at termination of capture

2. Renamed a few messages.  In particular, note a totally different
   use for capCaptureSingleFrame, vs. capGrabFrame.

3. Capture can now run as a separate task.  See updates to the
   WM_CAP_SEQUENCE and WM_CAP_SEQUENCE_NOFILE messages, or
   capCaptureSequence and capCaptureSequenceNoFile macro functions.

4. WM_CAP_SEQUENCE_STOP and WM_CAP_SEQUENCE_ABORT messages added to 
   terminate streaming capture if either yielding or running as a 
   separate task.

5. WM_CAP_PAL_MANUALCREATE message added.

6. WM_CAP_SINGLE_FRAME_OPEN... messages added to append individual
   frames to the capture file.

7. The WM_CAP_DLG_VIDEOCOMPRESSION message is not yet functional.
