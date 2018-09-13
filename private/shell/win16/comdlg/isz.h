/*++

Copyright (c) 1990-1995,  Microsoft Corporation  All rights reserved.

Module Name:

    isz.h

Abstract:

    This module contains the string resource IDs for the common dialogs.

Revision History:

--*/



//
//  0x0000 - 0x00ff Error codes.
//


//
//  MESSAGES:  0x0100 to 0x01ff.
//
#define iszOverwriteCaption       0x0100
#define iszOverwriteQuestion      0x0101
#define iszDefExitCaption         0x0102
#define iszDefExitText            0x0103

#define iszDefaultPitch           0x0104
#define iszFixedPitch             0x0105
#define iszVariablePitch          0x0106
#define iszAnsiCharset            0x0107
#define iszOemCharset             0x0108
#define iszSymbolCharset          0x0109
#define iszDecorativeFamily       0x010a
#define iszUnknownFamily          0x010b
#define iszModernFamily           0x010c
#define iszRomanFamily            0x010d
#define iszScriptFamily           0x010e
#define iszSwissFamily            0x010f

#define iszSystemFont             0x0110
#define iszHelvFont               0x0111
#define iszCourierFont            0x0112
#define iszTmsRmnFont             0x0113
#define iszSymbolFont             0x0114
#define iszRomanFont              0x0115
#define iszScriptFont             0x0116
#define iszModernFont             0x0117
#define iszLastFont               iszModernFont

#define iszFileSaveIn             0x0170
#define iszFileSaveButton         0x0171
#define iszFileOpenButton         0x0172

#define iszFileOpenTitle          0x0180
#define iszFileSaveTitle          0x0181
#define iszSaveFileAsType         0x0182
#define iszDriveDoesNotExist      0x0183
#define iszNoDiskInDrive          0x0184
#define iszWrongDiskInDrive       0x0185
#define iszUnformatedDisk         0x0186
#define iszFileNotFound           0x0187
#define iszPathNotFound           0x0188
#define iszInvalidFileName        0x0189
#define iszSharingViolation       0x018A
#define iszNetworkAccessDenied    0x018B
#define iszReadOnly               0x018C
#define iszInt24Error             0x018D
#define iszPortName               0x018E
#define iszWriteProtection        0x018F
#define iszDiskFull               0x0190
#define iszNoFileHandles          0x0191
#define iszCreatePrompt           0x0192
#define iszCreateNoModify         0x0193
#define iszSelectDriveTrouble     0x0194
#define iszNoNetButtonResponse    0x0195
#define iszFileAccessDenied       0x0196
#define iszDirAccessDenied        0x0197
#define iszNoDiskInCDRom          0x0198
#define iszNetworkButtonText      0x0199
#define iszNetworkButtonTextAccel 0x019A
#define iszTooManyFiles           0x019B
#define iszSaveAsType             0x019C


//
//  RESOURCES:  0x0200 to 0x020f.
//
//     Menus:           0x0200 to 0x020f
//     Icons:           0x0210 to 0x021f
//     Cursors:         0x0220 to 0x022f
//     Accelerators:    0x0230 to 0x023f
//     Bitmaps:         0x0240 to 0x024f
//     Private:         0x0250 to 0x025f
//
#define ICO_PORTRAIT              0x0210
#define ICO_LANDSCAPE             0x0211
#define ICO_P_NONE                0x0212
#define ICO_L_NONE                0x0213
#define ICO_P_HORIZ               0x0214
#define ICO_L_HORIZ               0x0215
#define ICO_P_VERT                0x0216
#define ICO_L_VERT                0x0217
#define ICO_COLLATE               0x0218
#define ICO_NO_COLLATE            0x0219
#define ICO_P_PSSTAMP             0x021a
#define ICO_L_PSSTAMP             0x021b

#define bmpDirDrive               576


//
//  DIALOGS:  0x0300 to 0x03ff.
//
#define dlgFileOpen               0x0300
#define dlgFileSave               0x0301
#define dlgExitChanges            0x0302
#define dlgChooseColor            0x0303
#define dlgFindText               0x0304
#define dlgReplaceText            0x0305
#define dlgFormatChar             0x0306
#define dlgFontInfo               0x0307
#define dlgPrintDlg               0x0308
#define dlgPrintSetupDlg          0x0309
#define dlgMultiFileOpen          0x030a


//
//  MISC:  0x0400 to 0x06ff.
//
#define BMFONT                    38

#define iszSampleString           0x040c   // sample text for Font picker
#define iszClose                  0x040d   // "Close" text for find/replace


#define iszBlack                  0x0410
#define iszDkRed                  0x0411
#define iszDkGreen                0x0412
#define iszDkYellow               0x0413
#define iszDkBlue                 0x0414
#define iszDkPurple               0x0415
#define iszDkAqua                 0x0416
#define iszDkGrey                 0x0417
#define iszLtGrey                 0x0418
#define iszLtRed                  0x0419
#define iszLtGreen                0x041a
#define iszLtYellow               0x041b
#define iszLtBlue                 0x041c
#define iszLtPurple               0x041d
#define iszLtAqua                 0x041e
#define iszWhite                  0x041f

#define iszAtomData               0x0420

#define iszHighPrnQ               0x0430
#define iszMedPrnQ                0x0431
#define iszLowPrnQ                0x0432
#define iszDraftPrnQ              0x0433

#define iszPrinter                0x0440
#define iszSysPrn                 0x0441
#define iszPrnOnPort              0x0442
#define iszDefCurOn               0x0443

#define iszSizeNumber             0x044A
#define iszSizeRange              0x044B
#define iszSynth                  0x044C
#define iszTrueType               0x044D
#define iszPrinterFont            0x044E
#define iszGDIFont                0x044F

#define iszPageRangeError         0x0450
#define iszFromToError            0x0451
#define iszPageFromError          0x0452
#define iszPageToError            0x0453
#define iszBadMarginError         0x0454
#define iszCopiesZero             0x0455
#define iszTooManyCopies          0x0456
#define iszWarningTitle           0x0457
#define iszNoPrnsInstalled        0x0458
#define iszPrnNotFound            0x0459
#define iszMemoryError            0x045A
#define iszGeneralWarning         0x045B
#define iszUnknownDriver          0x045C

#define iszNoFontsTitle           0x0500
#define iszNoFontsMsg             0x0501
#define iszNoFaceSel              0x0502
#define iszNoStyleSel             0x0503
#define iszRegular                0x0504
#define iszBold                   0x0505
#define iszItalic                 0x0506
#define iszBoldItalic             0x0507
//
//  CCHSTYLE is the max allowed length of iszRegular to iszBoldItalic strings
//
#define CCHSTYLE                  32


#define iszStatusReady            0x0600

#define iszStatusPaused           0x0601
#define iszStatusError            0x0602
#define iszStatusPendingDeletion  0x0603
#define iszStatusPaperJam         0x0604
#define iszStatusPaperOut         0x0605
#define iszStatusManualFeed       0x0606
#define iszStatusPaperProblem     0x0607
#define iszStatusOffline          0x0608
#define iszStatusIOActive         0x0609
#define iszStatusBusy             0x060A
#define iszStatusPrinting         0x060B
#define iszStatusOutputBinFull    0x060C
#define iszStatusNotAvailable     0x060D
#define iszStatusWaiting          0x060E
#define iszStatusProcessing       0x060F
#define iszStatusInitializing     0x0610
#define iszStatusWarmingUp        0x0611
#define iszStatusTonerLow         0x0612
#define iszStatusNoToner          0x0613
#define iszStatusPagePunt         0x0614
#define iszStatusUserIntervention 0x0615
#define iszStatusOutOfMemory      0x0616
#define iszStatusDoorOpen         0x0617

#define iszStatusDocumentsWaiting 0x062F
#define iszStatusDefaultPrinter   0x062E

#define iszPrintRangeAll          0x0630

#define iszMarginsInches          0x0631
#define iszMarginsMillimeters     0x0632
#define iszInches                 0x0633
#define iszMillimeters            0x0634
#define iszDefaultSource          0x0635

