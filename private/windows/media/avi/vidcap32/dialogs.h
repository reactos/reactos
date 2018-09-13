/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 *
 **************************************************************************/
/****************************************************************************
 *
 *   dialogs.h: Dialog box include
 *
 *   Vidcap32 Source code
 *
 ***************************************************************************/

/*
 * IDs for the dialogs themselves (as opposed to the controls within them)
 * are also the HELP_CONTEXT ids for the helpfile that we share with the
 * original 16-bit vidcap, and are thus fixed.
 */



#define IDD_HelpAboutBox            300

#define IDD_CapSetUp                    663
#define IDD_FrameRateData           401
#define IDD_FrameRateArrow          402
#define IDD_TimeLimitFlag           403
#define IDD_SecondsText             404
#define IDD_SecondsData             405
#define IDD_SecondsArrow            406
#define IDD_CapAudioFlag            407
#define IDD_AudioConfig             408
#define IDD_CaptureToDisk           409
#define IDD_CaptureToMemory         410
#define IDD_VideoConfig             411
#define IDD_CompConfig              412
#define IDD_MCIControlFlag          413
#define IDD_MCISetup                414

#define IDD_AudioFormat                 658
#define IDD_SampleIDs               420
#define IDD_Sample8Bit              421
#define IDD_Sample16Bit             422
#define IDD_ChannelIDs              423
#define IDD_ChannelMono             424
#define IDD_ChannelStereo           425
#define IDD_FreqIDs                 426
#define IDD_Freq11kHz               427
#define IDD_Freq22kHz               428
#define IDD_Freq44kHz               429
#define IDD_SetLevel                430

#define IDD_AllocCapFileSpace           652
#define IDD_SetCapFileFree          440
#define IDD_SetCapFileSize          441

#define IDD_MakePalette                 664
#define IDD_MakePalColors           450
#define IDD_MakePalStart            451
#define IDD_MakePalSingleFrame      452
#define IDD_MakePalNumFrames        453
#define IDD_MakePalColorArrow       454

#define IDD_NoCapHardware               657
#define IDD_FailReason              460

#define IDD_Prefs                       656
#define IDD_PrefsMasterAudio        465
#define IDD_PrefsMasterNone         466

#define IDD_PrefsSizeFrame          468

#define IDD_PrefsStatus             470
#define IDD_PrefsToolbar            471
#define IDD_PrefsCentre             472
#define IDD_PrefsDefBackground      473
#define IDD_PrefsLtGrey             474
#define IDD_PrefsDkGrey             475
#define IDD_PrefsBlack              476
#define IDD_PrefsSmallIndex         477
#define IDD_PrefsBigIndex           478

#define IDD_CAPFRAMES                   662
#define IDD_CapMessage              480
#define IDD_CapNumFrames            481

#define IDD_MCISETUP                    665
#define IDD_MCI_SOURCE              490
#define IDD_MCI_PLAY                491
#define IDD_MCI_STEP                492
#define IDD_MCI_AVERAGE_2X          493
#define IDD_MCI_AVERAGE_FR          494
#define IDD_MCI_STARTTIME           495
#define IDD_MCI_STOPTIME            496
#define IDD_MCI_STARTSET            497
#define IDD_MCI_STOPSET             498

#define IDD_RECLVLMONO                  667
#define IDD_RECLVLSTEREO                668
#define IDRL_LEVEL1                 510
#define IDRL_LEVEL2                 511


// help context ids for common dialogs (GetOpenFile etc)
#define IDA_LOADPAL                     650
#define IDA_SETCAPFILE                  651
#define IDA_SAVECAPFILE                 653
#define IDA_SAVEPAL                     654
#define IDA_SAVEDIB                     655

// help contexts for dialogs put up by AVICAP
#define IDA_VIDSOURCE                   659
#define IDA_VIDFORMAT                   660
#define IDA_VIDDISPLAY                  661
#define IDA_MCIFRAMES                   662
#define IDA_COMPRESSION                 669

