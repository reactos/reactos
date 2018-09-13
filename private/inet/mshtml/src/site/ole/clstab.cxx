//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       clstab.cxx
//
//  Contents:   Class table for CDoc.
//
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CLSTAB_HXX_
#define X_CLSTAB_HXX_
#include "clstab.hxx"
#endif

#ifndef X_TYPENAV_HXX_
#define X_TYPENAV_HXX_
#include <typenav.hxx>
#endif

#ifndef X_OBJEXT_H_
#define X_OBJEXT_H_
#include <objext.h>
#endif

MtDefine(CClassTable, CDoc, "CClassTable")
MtDefine(CClassTable_aryci_pv, CClassTable, "CClassTable::_aryci::_pv")

EXTERN_C const CLSID CLSID_IE4ShellFolderIcon = { 0xE5DF9D10, 0x3B52, 0x11D1, 0x83, 0xE8, 0x00, 0xA0, 0xC9, 0x0D, 0xC8, 0x49 };
EXTERN_C const CLSID CLSID_IE4ShellPieChart =   { 0x1D2B4F40, 0x1F10, 0x11D1, 0x9E, 0x88, 0x00, 0xC0, 0x4F, 0xDC, 0xAB, 0x92 };
EXTERN_C const CLSID CLSID_AppletOCX =          { 0x08B0e5c0, 0x4FCB, 0x11CF, 0xAA, 0xA5, 0x00, 0x40, 0x1C, 0x60, 0x85, 0x01 };
#if DBG == 1
EXTERN_C const CLSID CLSID_WebBrowser;
         const CLSID CLSID_IISForm =            { 0x812AE312, 0x8B8E, 0x11CF, 0x93, 0xC8, 0x00, 0xAA, 0x00, 0xC0, 0x8F, 0xDF };
         const CLSID CLSID_Forms3Optionbutton = { 0x8BD21D50, 0xEC42, 0x11CE, 0x9E, 0x0D, 0x00, 0xAA, 0x00, 0x60, 0x02, 0xF3 };
         const CLSID CLSID_Acrobat =            { 0xCA8A9780, 0x280D, 0x11CF, 0xA2, 0x4D, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
         const CLSID CLSID_SurroundVideo =      { 0x928626A3, 0x6B98, 0x11CF, 0x90, 0xB4, 0x00, 0xAA, 0x00, 0xA4, 0x01, 0x1F };
         const CLSID CLSID_MSInvestor =         { 0xD2F97240, 0xC9F4, 0x11CF, 0xBF, 0xC4, 0x00, 0xA0, 0xC9, 0x0C, 0x2B, 0xDB };
         const CLSID CLSID_PowerPointAnimator = { 0xEFBD14F0, 0x6BFB, 0x11CF, 0x91, 0x77, 0x00, 0x80, 0x5F, 0x88, 0x13, 0xFF };
         const CLSID CLSID_MSInvestorNews =     { 0x025B1052, 0xCB0B, 0x11CF, 0xA0, 0x71, 0x00, 0xA0, 0xC9, 0xA0, 0x6E, 0x05 };
         const CLSID CLSID_MSTreeView =         { 0xB9D029D3, 0xCDE3, 0x11CF, 0x85, 0x5E, 0x00, 0xA0, 0xC9, 0x08, 0xFA, 0xF9 };
         const CLSID CLSID_ActiveMovie =        { 0x05589fa1, 0xc356, 0x11ce, 0xbf, 0x01, 0x00, 0xaa, 0x00, 0x55, 0x59, 0x5a };
         const CLSID CLSID_MediaPlayer =        { 0x22D6F312, 0xB0F6, 0x11D0, 0x94, 0xAB, 0x00, 0x80, 0xC7, 0x4C, 0x7E, 0x95 };
         const CLSID CLSID_MCSITree =           { 0xB3F8F451, 0x788A, 0x11D0, 0x89, 0xD9, 0x00, 0xA0, 0xC9, 0x0C, 0x9B, 0x67 };
         const CLSID CLSID_IEMenu =             { 0x7823A620, 0x9DD9, 0x11CF, 0xA6, 0x62, 0x00, 0xaa, 0x00, 0xC0, 0x66, 0xD2 };
         const CLSID CLSID_CitrixWinframe =     { 0x238f6f83, 0xb8b4, 0x11cf, 0x87, 0x71, 0x00, 0xa0, 0x24, 0x54, 0x1e, 0xe3 };
         const CLSID CLSID_VivoViewer =         { 0x02466323, 0x75ed, 0x11cf, 0xa2, 0x67, 0x00, 0x20, 0xaf, 0x25, 0x46, 0xea };
         const CLSID CLSID_SheridanCommand =    { 0xAAD093B2, 0xF9CA, 0x11CF, 0x9C, 0x85, 0x00, 0x00, 0xC0, 0x93, 0x00, 0xC4 };
         const CLSID CLSID_VActive =            { 0x5A20858B, 0x000D, 0x11D0, 0x8C, 0x01, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
         const CLSID CLSID_SaxCanvas =          { 0x1DF67C43, 0xAEAA, 0x11CF, 0xBA, 0x92, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
         const CLSID CLSID_GregConsDieRoll =    { 0x46646B43, 0xEA16, 0x11CF, 0x87, 0x0C, 0x00, 0x20, 0x18, 0x01, 0xDD, 0xD6 };
         const CLSID CLSID_NCompassBillboard =  { 0x6059B947, 0xEC52, 0x11CF, 0xB5, 0x09, 0x00, 0xA0, 0x24, 0x48, 0x8F, 0x73 };
         const CLSID CLSID_NCompassLightboard = { 0xB2F87B84, 0x26A6, 0x11D0, 0xB5, 0x0A, 0x00, 0xA0, 0x24, 0x48, 0x8F, 0x73 };
         const CLSID CLSID_ProtoviewTreeView =  { 0xB283E214, 0x2CB3, 0x11D0, 0xAD, 0xA6, 0x00, 0x40, 0x05, 0x20, 0x79, 0x9C };
         const CLSID CLSID_ActiveEarthTime =    { 0x9590092D, 0x8811, 0x11CF, 0x80, 0x75, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00 };
         const CLSID CLSID_LeadControl =        { 0x00080000, 0xB1BA, 0x11CE, 0xAB, 0xC6, 0xF5, 0xB2, 0xE7, 0x9D, 0x9E, 0x3F };
         const CLSID CLSID_TextX =              { 0x5B84FC03, 0xE639, 0x11CF, 0xB8, 0xA0, 0x00, 0xA0, 0x24, 0x18, 0x6B, 0xF1 };
         const CLSID CLSID_Plugin =             { 0x06DD38D3, 0xD187, 0x11CF, 0xA8, 0x0D, 0x00, 0xC0, 0x4F, 0xD7, 0x4A, 0xD8 };
         const CLSID CLSID_GreetingsUpload =    { 0x03405265, 0xb4e2, 0x11d0, 0x8a, 0x77, 0x00, 0xaa, 0x00, 0xa4, 0xfb, 0xc5 };
         const CLSID CLSID_GreetingsDownload =  { 0x03405269, 0xb4e2, 0x11d0, 0x8a, 0x77, 0x00, 0xaa, 0x00, 0xa4, 0xfb, 0xc5 };
         const CLSID CLSID_COMCTLTree =         { 0x0713E8A2, 0x850A, 0x101B, 0xAF, 0xC0, 0x42, 0x10, 0x10, 0x2A, 0x8D, 0xA7 };
         const CLSID CLSID_COMCTLProg =         { 0x0713E8D2, 0x850A, 0x101B, 0xAF, 0xC0, 0x42, 0x10, 0x10, 0x2A, 0x8D, 0xA7 };
         const CLSID CLSID_COMCTLImageList =    { 0x58DA8D8F, 0x9D6A, 0x101B, 0xAF, 0xC0, 0x42, 0x10, 0x10, 0x2A, 0x8D, 0xA7 };
         const CLSID CLSID_COMCTLListview =     { 0x58DA8D8A, 0x9D6A, 0x101B, 0xAF, 0xC0, 0x42, 0x10, 0x10, 0x2A, 0x8D, 0xA7 };
         const CLSID CLSID_COMCTLSbar =         { 0x6B7E638F, 0x850A, 0x101B, 0xAF, 0xC0, 0x42, 0x10, 0x10, 0x2A, 0x8D, 0xA7 };
         const CLSID CLSID_MCSIMenu =           { 0x275E2FE0, 0x7486, 0x11D0, 0x89, 0xD6, 0x00, 0xA0, 0xC9, 0x0C, 0x9B, 0x67 };
         const CLSID CLSID_MSNVer =             { 0xA123D693, 0x256A, 0x11d0, 0x9D, 0xFE, 0x00, 0xC0, 0x4F, 0xD7, 0xBF, 0x41 };
         const CLSID CLSID_RichTextCtrl =       { 0x3B7C8860, 0xD78F, 0x101B, 0xB9, 0xB5, 0x04, 0x02, 0x1C, 0x00, 0x94, 0x02 };
         const CLSID CLSID_IETimer =            { 0x59CCB4A0, 0x727D, 0x11CF, 0xAC, 0x36, 0x00, 0xAA, 0x00, 0xA4, 0x7D, 0xD2 };
         const CLSID CLSID_SubScr =             { 0x78A9B22E, 0xE0F4, 0x11D0, 0xB5, 0xDA, 0x00, 0xC0, 0xF0, 0x0A, 0xD7, 0xF8 };
EXTERN_C const CLSID CLSID_Scriptlet;  //AE24FDAE-03C6-11D1-8B76-0080C744F389
         const CLSID CLSID_OldXsl =             { 0x2BD0D2F2, 0x52EC, 0x11D1, 0x8C, 0x69, 0x0E, 0x16, 0xBC, 0x00, 0x00, 0x00 };
         const CLSID CLSID_MMC =                { 0xD306C3B7, 0x2AD5, 0x11D1, 0x9E, 0x9A, 0x00, 0x80, 0x5F, 0x20, 0x00, 0x05 };
         const CLSID CLSID_RealAudio =          { 0xCFCDAA03, 0x8BE4, 0x11CF, 0xB8, 0x4B, 0x00, 0x20, 0xAF, 0xBB, 0xCC, 0xFA };
         const CLSID CLSID_WebCalc =            { 0x0002E510, 0x0000, 0x0000, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 };
         const CLSID CLSID_AnswerList =         { 0x8F2C1D40, 0xC3CD, 0x11D1, 0xA0, 0x8F, 0x00, 0x60, 0x97, 0xBD, 0x99, 0x70 };
         const CLSID CLSID_PreLoader =          { 0x16E349E0, 0x702C, 0x11CF, 0xA3, 0xA0, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0x20 };
         const CLSID CLSID_EyeDog =             { 0x06A7EC63, 0x4E21, 0x11D0, 0xA1, 0x12, 0x00, 0xA0, 0xC9, 0x05, 0x43, 0xAA };
         const CLSID CLSID_ImgAdmin =           { 0x009541A0, 0x3B81, 0x101C, 0x92, 0xF3, 0x04, 0x02, 0x24, 0x00, 0x9C, 0x02 };
         const CLSID CLSID_ImgThumb =           { 0xE1A6B8A0, 0x3603, 0x101C, 0xAC, 0x6E, 0x04, 0x02, 0x24, 0x00, 0x9C, 0x02 };
         const CLSID CLSID_HHOpen =             { 0x130D7743, 0x5F5A, 0x11D1, 0xB6, 0x76, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x33 };
         const CLSID CLSID_RegWiz =             { 0x50E5E3D1, 0xC07E, 0x11D0, 0xB9, 0xFD, 0x00, 0xA0, 0x24, 0x9F, 0x6B, 0x00 };
         const CLSID CLSID_SetupCtl =           { 0xF72A7B0E, 0x0DD8, 0x11D1, 0xBD, 0x6E, 0x00, 0xAA, 0x00, 0xB9, 0x2A, 0xF1 };
         const CLSID CLSID_ImgEdit =            { 0x6D940280, 0x9F11, 0x11CE, 0x83, 0xFD, 0x02, 0x60, 0x8C, 0x3E, 0xC0, 0x8A };
         const CLSID CLSID_ImgEdit2 =           { 0x6D940285, 0x9F11, 0x11CE, 0x83, 0xFD, 0x02, 0x60, 0x8C, 0x3E, 0xC0, 0x8A };
         const CLSID CLSID_ImgScan =            { 0x84926CA0, 0x2941, 0x101C, 0x81, 0x6F, 0x0E, 0x60, 0x13, 0x11, 0x4B, 0x7F };
         const CLSID CLSID_TlntSvr =            { 0xFE9E48A4, 0xA014, 0x11D1, 0x85, 0x5C, 0x00, 0xA0, 0xC9, 0x44, 0x13, 0x8C };

    //
    // This table exists to double check the table in the registry.
    // It is incredibly easy for a typo to corrupt the copy of this table
    // that is stored in f3\rsrc\selfreg.inx and gets propagated to the registry.
    //
struct CompatibilityTableElementAssert
{
    const CLSID *pclsid;
    DWORD        dwCompatFlags;
    DWORD        dwMiscStatusFlags;
} s_aCompatibility[] = 
{
    // 
    // W A R N I N G - Review this list on a regular basis.
    //                 We might find that some of these controls
    //                 fix their problems.
    //
    // **  I M P O R T A N T  **
    //
    // To add to this compatibility table do the following steps:
    //
    // 1. Create a new COMPAT_* flag in clstab.hxx if needed.
    // 2. Implement the COMPAT_* flag check and associated special behavior in
    //    olesite.cxx or whereever.
    // 3. Add the clsid of the particular ActiveX control to the declarations
    //    above if not already there.
    // 4. Add a new entry to the table below for that clsid if not already there.
    //    ** VERY IMPORTANT:  Keep the table sorted in ascending clsid order.
    //    ** EVEN MORE IMPORTANT: Keeping the table sorted is no longer necessary.
    // 5. Add a corresponding clsid string and table entry to the [CompatTable.Reg]
    //    section of msthml\src\f3\rsrc\selfreg.inx.
    // 6. Increment the szOurVersion version number declaration below.
    // 7. Increment the "Version" reg entry value in the [CompatTable.Reg] section
    //    of selfreg.inx.
    //    ** if you fail to do this, your additions will not be installed
    //       into the registry, since ShouldWeRegisterCompatibilityTable()
    //       will think the table is unchanged.
    // 8. Build and do mshtmpad /local to put the new entry in the registry on your
    //    machine.
    // 9. When you check in warn everyone that they must do mshtmpad /local or they
    //    will get asserts.
    //

    { &CLSID_VivoViewer, COMPAT_SEND_SHOW, 0}, 
    { &CLSID_MSInvestorNews, COMPAT_NO_UIACTIVATE | COMPAT_NO_QUICKACTIVATE, 0 },
    { &CLSID_ActiveMovie, COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE, 0 },
    { &CLSID_Plugin,    COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_AppletOCX, COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE, 0 },
    { &CLSID_SaxCanvas, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_MediaPlayer, COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE, 0 },
    { &CLSID_CitrixWinframe, COMPAT_USE_PROPBAG_AND_STREAM, 0 },
    { &CLSID_GregConsDieRoll, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_DISABLEWINDOWLESS, 0 },
    { &CLSID_VActive, COMPAT_SEND_SHOW, 0 },
    { &CLSID_IEMenu, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_WebBrowser, COMPAT_AGGREGATE | COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_Forms3Optionbutton, COMPAT_AGGREGATE, 0 },
    { &CLSID_SurroundVideo, COMPAT_NO_SETEXTENT, 0 },
    { &CLSID_MSTreeView, COMPAT_ALWAYS_INPLACEACTIVATE },
    { &CLSID_Acrobat, COMPAT_SEND_SHOW | COMPAT_PRINTPLUGINSITE, 0 },
    { &CLSID_MSInvestor, COMPAT_SEND_HIDE, 0 },
    { &CLSID_PowerPointAnimator, COMPAT_NO_SETEXTENT | COMPAT_NO_BINDF_OFFLINEOPERATION, 
        OLEMISC_INSIDEOUT | OLEMISC_ACTIVATEWHENVISIBLE },
    { &CLSID_SheridanCommand, COMPAT_DISABLEWINDOWLESS },
    { &CLSID_MCSITree, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_NCompassBillboard,   COMPAT_SETWINDOWRGN, 0 },
    { &CLSID_NCompassLightboard,  COMPAT_SETWINDOWRGN, 0 },
    { &CLSID_ProtoviewTreeView,   COMPAT_SETWINDOWRGN, 0 },
    { &CLSID_ActiveEarthTime,     COMPAT_SETWINDOWRGN, 0 },
    { &CLSID_LeadControl,         COMPAT_SETWINDOWRGN, 0 },
    { &CLSID_TextX, COMPAT_DISABLEWINDOWLESS, 0 },
    { &CLSID_IISForm, COMPAT_NO_OBJECTSAFETY, 0 },
    { &CLSID_GreetingsUpload, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_GreetingsDownload, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_COMCTLTree, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_COMCTLProg, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_COMCTLImageList, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_COMCTLListview, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_COMCTLSbar, COMPAT_ALWAYS_INPLACEACTIVATE | COMPAT_PROGSINK_UNTIL_ACTIVATED, 0 },
    { &CLSID_MCSIMenu, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_MSNVer, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_RichTextCtrl, COMPAT_EVIL_DONT_LOAD, 0 },
    { &CLSID_IETimer, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_SubScr, COMPAT_EVIL_DONT_LOAD, 0 },
    { &CLSID_Scriptlet, COMPAT_ALWAYS_INPLACEACTIVATE, 0 },
    { &CLSID_OldXsl, COMPAT_EVIL_DONT_LOAD, 0 },
    { &CLSID_MMC, COMPAT_EVIL_DONT_LOAD, 0 },
    { &CLSID_RealAudio, COMPAT_INPLACEACTIVATEEVENWHENINVISIBLE, 0 },
    { &CLSID_WebCalc, COMPAT_ALWAYSDEFERSETWINDOWRGN, 0},
    { &CLSID_AnswerList, COMPAT_INPLACEACTIVATESYNCHRONOUSLY, 0 },
    { &CLSID_PreLoader, COMPAT_EVIL_DONT_LOAD, 0 },

    // Hack for IE5 #68793. These controls, used in IE4 shell webview, screw up tabbing once
    // they get focus. Make them unfocussable. The controls will be fixed in NT5 and given
    // new clsids.
    { &CLSID_IE4ShellFolderIcon, COMPAT_NEVERFOCUSSABLE, 0 },
    { &CLSID_IE4ShellPieChart, COMPAT_NEVERFOCUSSABLE, 0 },
    // End hack

    { &CLSID_EyeDog, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_ImgAdmin, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_ImgThumb, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_HHOpen, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_RegWiz, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_SetupCtl, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_ImgEdit, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_ImgEdit2, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_ImgScan, COMPAT_EVIL_DONT_LOAD, 0},
    { &CLSID_TlntSvr, COMPAT_EVIL_DONT_LOAD, 0},
};
#endif

//
// This needs to stay in sync with the Version value in selfreg.inx!
// Note: this version number is also used for urlcomp.cxx
//

TCHAR szOurVersion[] = _T("5.09");

/*
Format of the registry entries
ROOT_LOCATION 
 =HKEY_LOCAL_MACHINE\Software\Microsoft\Internet Explorer\ActiveX Compatibility

That root location key will have one value, "Version" = REG_SZ "5.00". 
This gives the version number of the compatability table. Initially the version 
numberwill be "0.0" allowing increments during development. At RTM it will be "5.00".

Under that will be keys with CLSID names with the same format as HKCR\CLSID.
Each of those CLSID subkeys may have two values.  Either or both values may be absent.
  {SOME-CLSID-VALUE-0000001}\ value name "Compatibility Flags" REG_DWORD 12345
  {SOME-CLSID-VALUE-0000001}\ value name "MiscStatus Flags"    REG_DWORD 12345

The Compat Flags value holds the COMPAT_* type flags.
The OleMiscFlags value holds our OLEMISC_* flags but uses a different name & structure
than in HKCR\CLSID. In that location they have MiscStatus\<version number>.
We don't track the version number of the control we instantiate, so we have
no <version number> subkey.

Self Registration
The compat table will be stored in f3\rsrc\selfreg.inx and written during normal
DLLInstall() operation.  It will be written
if the ROOT_LOCATION key does notexist or if the mshtml.dll 
compatibility table version number is > than thatin the registry. 
Such "reluctant" writing of the compatibility table will
allow users or others to add, modify, or delete entries in the compatibility
table and those changes will not be overwritten easily.
If mshtml.dll is writing the compatibility table to the registry due to a
compat table version number increase it will add entries but not remove
entries that may have been put there by someone else.
*/

static TCHAR szTableRootKey[] = 
   _T("Software\\Microsoft\\Internet Explorer\\ActiveX Compatibility");

static TCHAR szCompatFlags[] =      _T("Compatibility Flags");
static TCHAR szMiscStatusFlags[] =  _T("MiscStatus Flags");

CLASSINFO g_ciNull;

//+---------------------------------------------------------------------------
//
//  Function:   ShouldWeRegisterCompatibilityTable
//
//  Synopsis:   Determine whether would should write our compatibility table,
//              as recorded in selfreg.inx, to the registry.
//  
//              We do if the table is not in the registry at all, or if
//              our version of the table is more recent than that in the
//              registry.
//
//  Returns:    TRUE - yes, write the table please.
//
//----------------------------------------------------------------------------

BOOL 
ShouldWeRegisterCompatibilityTable()
{
    BOOL fOurRet = TRUE;
    LONG lRet;
    HKEY hkeyRoot = NULL;
    DWORD dwSize, dwType;
    TCHAR szVersion[10];

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTableRootKey, 0, KEY_READ, &hkeyRoot);
    if( lRet != ERROR_SUCCESS )
        return( TRUE );

    dwSize = sizeof( szVersion );
    lRet = RegQueryValueEx( hkeyRoot, _T("Version"), NULL, &dwType, 
      (LPBYTE)szVersion, &dwSize );
    if( lRet == ERROR_SUCCESS )
        fOurRet = (_tcscmp( szVersion, szOurVersion ) < 0);

    RegCloseKey( hkeyRoot );

    return( fOurRet );
}

#if DBG == 1
//+---------------------------------------------------------------------------
//
//  Function:   AssertCompatibilityTable
//
//  Synopsis:   Reads the compatibility table from the registry.
//              Confirms that it is identical to our internal compatibility
//              table.
//
//----------------------------------------------------------------------------

HRESULT
CompatFlagsFromClsid(REFCLSID clsid, LPDWORD pdwCompatFlags, LPDWORD pdwMiscStatusFlags);

void
AssertCompatibilityTable()
{
#ifndef WIN16           // we'll deal with ActiveX issue later
    LONG    lRet;       // Dual error code types and variables.
    HRESULT hr = S_OK;  // Notice the two exit paths below.

    HKEY  hkeyRoot = NULL;
    DWORD cKeys = 0, iKey = 0, cbClsid;
    DWORD dwOleMisc, dwSize, dwType;
    DWORD dwCompatFlags;
    CLSID clsid;
    TCHAR szClsid[40];
    TCHAR szVersion[10];
    int   iVersionNumberComparison;

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTableRootKey, 0, KEY_READ, &hkeyRoot);
    if( lRet != ERROR_SUCCESS )
        goto Win32ErrorExit;

    // Check the version number:
    dwSize = sizeof( szVersion );
    lRet = RegQueryValueEx( hkeyRoot, _T("Version"), NULL, &dwType, 
      (LPBYTE)szVersion, &dwSize );
    if( lRet != ERROR_SUCCESS )
        goto Win32ErrorExit;

    iVersionNumberComparison = _tcscmp( szVersion, szOurVersion );

    // The version number should not be less than our internal one:
    Assert( szVersion >= 0 );

    // First count how many entries there are:
    lRet = RegQueryInfoKey( hkeyRoot, NULL, NULL, NULL, &cKeys,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL );
    if( lRet != ERROR_SUCCESS )
        goto Win32ErrorExit;

    // If the version number is the same the count should be the
    // same:
    if( iVersionNumberComparison == 0 )
        Assert( cKeys == ARRAY_SIZE( s_aCompatibility ) );

    // Read each table entry:
    for( ; iKey < cKeys; iKey++ )
    {
        cbClsid = sizeof( szClsid );
        lRet = RegEnumKeyEx( hkeyRoot, iKey, szClsid, &cbClsid, NULL, NULL, NULL, NULL );
        if( lRet != ERROR_SUCCESS )
            goto Win32ErrorExit;

        // Get the clsid value from the string:
        szClsid[cbClsid] = _T('\0');
        hr = CLSIDFromString( szClsid, &clsid );
        if( FAILED(hr) )
            goto Exit;

        Assert( *s_aCompatibility[iKey].pclsid == clsid );

        hr = CompatFlagsFromClsid( clsid, &dwCompatFlags, &dwOleMisc );
        Assert( SUCCEEDED( hr ) );

        Assert( s_aCompatibility[iKey].dwCompatFlags == dwCompatFlags );
        Assert( s_aCompatibility[iKey].dwMiscStatusFlags == dwOleMisc );
    }

  Exit:
    if( hkeyRoot )
        RegCloseKey( hkeyRoot );

    Assert( SUCCEEDED( hr ) );
    return;

  Win32ErrorExit:  // convert WIN32 error code to HRESULT:
    hr = HRESULT_FROM_WIN32( lRet );
    goto Exit;
#endif    
}
#endif

EXTERN_C const IID IID_IRowset;
EXTERN_C const IID IID_OLEDBSimpleProvider;
EXTERN_C const IID IID_IRowCursor;
EXTERN_C const IID IID_DataSource;

DeclareTag(tagShowHideVerb, "OleSite", "DoVerb(SHOW/HIDE) before Inplace (de)activate.");


//+------------------------------------------------------------------------
//
//  Member:     CompatFlagsFromClsid
//
//  Synopsis:   Get compatibility flags for given clsid.
//
//-------------------------------------------------------------------------

HRESULT
CompatFlagsFromClsid(REFCLSID clsid, LPDWORD pdwCompatFlags, LPDWORD pdwMiscStatusFlags)
{
    LONG lRet;      // Dual error code types and variables.
    HRESULT hr;     // Notice the two exit paths below.

    HKEY hkeyRoot = NULL, hkeyClsid = NULL;
    DWORD dwSize, dwType;
    LPOLESTR pszClsid = NULL;
    union Yuck { 
        DWORD dw;
        TCHAR tch[10];
    } yuckValue;

    Assert( pdwCompatFlags != NULL );
    Assert( pdwMiscStatusFlags != NULL );

    lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE, szTableRootKey, 0, KEY_READ, &hkeyRoot);
    if( lRet != ERROR_SUCCESS )
        goto Win32ErrorExit;

    hr = StringFromCLSID( clsid, &pszClsid );
    if( FAILED( hr ) )
        goto Exit;

    // Open the {####-####-####...} Clsid subkey:
    lRet = RegOpenKeyEx(hkeyRoot, pszClsid, 0, KEY_READ, &hkeyClsid);
    if( lRet != ERROR_SUCCESS )
    {
        hr = S_FALSE;  // if there is no entry in the table that is OK.
        goto Exit;
    }

    // Get the flags from the subkeys named values. Either value may
    // be missing from the registry, we'll just use 0 then.
    // The yuckValue mechanism is due to the fact that the .inf tools
    // sometimes put STRINGS rather than DWORDS in the registry on win95.
    *pdwCompatFlags = 0;    // default initial value in case we fail anywhere.
    dwSize = sizeof( yuckValue );
    lRet = RegQueryValueEx( hkeyClsid, szCompatFlags, NULL, &dwType, 
      (LPBYTE)&yuckValue, &dwSize );
    if( lRet == ERROR_SUCCESS )
    {
        if( dwType == REG_DWORD )
            *pdwCompatFlags = yuckValue.dw;
        else if( dwType == REG_SZ && dwSize > 2 )
            *pdwCompatFlags = _tcstol( yuckValue.tch+2, NULL, 16 );
    }
    
    *pdwMiscStatusFlags = 0; // default initial value in case we fail anywhere.
    dwSize = sizeof( yuckValue );
    lRet = RegQueryValueEx( hkeyClsid, szMiscStatusFlags, NULL, &dwType, 
      (LPBYTE)&yuckValue, &dwSize );
    if( lRet == ERROR_SUCCESS )
    {
        if( dwType == REG_DWORD )
            *pdwMiscStatusFlags = yuckValue.dw;   
        else if( dwType == REG_SZ && dwSize > 2)
            *pdwMiscStatusFlags = _tcstol( yuckValue.tch+2, NULL, 16 );
    }

  Exit:
    if( hkeyClsid )
        RegCloseKey( hkeyClsid );

    if( hkeyRoot )
        RegCloseKey( hkeyRoot );

    if( pszClsid )
        CoTaskMemFree( pszClsid );

    return( hr );

  Win32ErrorExit:  // convert WIN32 error code to HRESULT:
    hr = HRESULT_FROM_WIN32( lRet );
    goto Exit;
}


//+------------------------------------------------------------------------
//
//  Member:     CClassTable::Reset
//
//  Synopsis:   Releases the ITypeInfo interface pointes we have stored in the
//              _aryci array.
//
//-------------------------------------------------------------------------

void
CClassTable::Reset()
{
    int i = _aryci.Size();

    for( ; i > 0; --i )
    {
        ClearInterface( &_aryci[i-1]._pTypeInfoEvents );
    }

    _aryci.DeleteAll();

#if DBG == 1
    // Take out temporarily due to table order issues on WIN95.  -Tom
    //AssertCompatibilityTable();
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     InitializeClassInfo
//
//  Synopsis:   Initializes a CLASSINFO structure
//
//  Arguments:  pci         CLASSINFO structure
//              clsid       The GUID identifier.
//
//  Returns:    void
//
//-------------------------------------------------------------------------

void
CLASSINFO::Init(REFCLSID rclsid, BOOL fInitCompatFlags)
{
    // Initialize CLASSINFO members to defaul state

    // 0-fill for speed
    memset(this, 0, sizeof(CLASSINFO));
    
    clsid = rclsid;

    // Assume IDispatch for these two IIDs.  Note that if the object doesn't
    //  actually have a default interface or default event interface, subsequent
    //  code will simply fail when we try to QI or FindConnectionPoint for these
    //  IIDs, so we're no worse off than if we started with IID_NULL here.
    iidDefault = IID_IDispatch;
    iidDispEvent = IID_IDispatch;

    Assert(vtValueType == VT_EMPTY);
    Assert(vtBindType == VT_EMPTY);
    dispIDBind = DISPID_UNKNOWN;
    dispidIDataSource = DISPID_UNKNOWN;
    dispidRowset = DISPID_UNKNOWN;
    dispidCursor = DISPID_UNKNOWN;
    dispidSTD = DISPID_UNKNOWN;

#ifndef WIN16
    if (fInitCompatFlags)
    {
        Verify(SUCCEEDED(CompatFlagsFromClsid(rclsid, &dwCompatFlags, &dwMiscStatusFlags)));
    }
#endif
}

//+------------------------------------------------------------------------
//
//  Member:     CClassTable::AssignWclsid
//
//  Synopsis:   Assign an integer id given a guid id.
//
//  Arguments:  pDoc       The hosting form.
//              clsid       The GUID identifier.
//              pwclsid     The integer identifier.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CClassTable::AssignWclsid(CDoc *pDoc, REFCLSID clsid, WORD *pwclsid)
{
    int                     wclsid;
    CLASSINFO *             pci;
    HRESULT                 hr = S_OK;
    ILicensedClassManager * pLCM = NULL;
    IRequireClasses *       pRC = NULL;

    if (clsid == GUID_NULL)
    {
        wclsid = 0;
        goto Cleanup;
    }

    for (wclsid = 1, pci = _aryci; wclsid <= _aryci.Size(); wclsid++, pci++)
    {
        if (pci->clsid == clsid)
            goto Cleanup;
    }

    hr = THR(_aryci.EnsureSize(_aryci.Size() + 1));
    if (hr)
        goto Cleanup;

    pci = &_aryci[_aryci.Size()];

    pci->Init(clsid, TRUE);

    wclsid = _aryci.Size() + 1;

    _aryci.SetSize(_aryci.Size() + 1);

    // Notify the world that we have added a class.

    if (OK(THR_NOTRACE(pDoc->QueryService(
            SID_SLicensedClassManager,
            IID_ILicensedClassManager,
            (void **)&pLCM))))
    {
        if (OK(THR(pDoc->QueryInterface(IID_IRequireClasses, (void **)&pRC))))
        {
            IGNORE_HR(pLCM->OnChangeInRequiredClasses(pRC));
            pRC->Release();
        }
        pLCM->Release();
    }

 Cleanup:
    *pwclsid = (WORD)wclsid;
    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CClassTable::IsInterfaceProperty, static
//
//  Synopsis:   Determine if the given TypeDesc is for an interface-valued
//              property, and return the IID of that interface if it is.
//
//  Arguments:  pTI             TypeInfo which we found the given TypeDesc
//              pTypeDesc       type descriptor of a property
//              piid            Where to return the discovered IID
//              
//
//  Returns:    TRUE            if the TypeDesc is for an interface-valued
//                              property; fills in *piid.
//              FALSE           if the property is not interface-valued;
//                              leaves *piid unaffected.
//
//-------------------------------------------------------------------------

BOOL
CClassTable::IsInterfaceProperty (ITypeInfo *pTI, TYPEDESC *pTypeDesc, IID *piid)
{
    BOOL        fResult = FALSE;

    Assert(pTI);
    Assert(pTypeDesc);
    Assert(piid);

    // We are looking for a pointer to GUID'd user-defined type

    // first, make sure it's a pointer
    if (!pTypeDesc || pTypeDesc->vt != VT_PTR)
    {
        goto Cleanup;
    }

    // now examine the type of the object pointed to, make sure
    //  it's a user-defined type
    pTypeDesc = pTypeDesc->lptdesc;
    if (pTypeDesc && pTypeDesc->vt == VT_USERDEFINED)
    {
        // Yes, so it could be the interface-valued object.
        HREFTYPE    hreftype;
        ITypeInfo * pUserTI;

        // We have a property which has a undefined type.
        hreftype = pTypeDesc->hreftype;

        if (!pTI->GetRefTypeInfo(hreftype, &pUserTI))
        {
            TYPEATTR *pTypeAttr;

            if (!pUserTI->GetTypeAttr(&pTypeAttr))
            {
                *piid = pTypeAttr->guid;
                fResult = TRUE;

                pUserTI->ReleaseTypeAttr(pTypeAttr);
            }

            ReleaseInterface(pUserTI);
        }
    }

Cleanup:
    return fResult;
}


//+------------------------------------------------------------------------
//
//  Member:     GetDefaultBindInfoForGet, static helper
//
//  Synopsis:   save the info we need about the default bind property's
//              "get" method
//
//  Arguments:  pci             the class info structure
//              pfDesc          function description (from typelib)
//              pClassTable     the class table entry
//              cTINav          typeinfo navigator (for checking interface properties)
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CClassTable::GetDefaultBindInfoForGet(CLASSINFO *pci, FUNCDESC *pfDesc,
                                        CTypeInfoNav& cTINav)
{
    IID iid;
    VARTYPE vtParamType;
    
    pci->uGetBindIndex = pfDesc->oVft;
    vtParamType = pfDesc->elemdescFunc.tdesc.vt;
    pci->dispIDBind = pfDesc->memid;

    // the type should agree with the put method (if we've seen it)
    Assert(pci->vtBindType == VT_EMPTY || (pci->vtBindType & VT_TYPEMASK) == vtParamType);
    pci->vtBindType = vtParamType | (pci->vtBindType & ~VT_TYPEMASK);

    // special check for complex data consumers
    if (IsInterfaceProperty(cTINav.getITypeInfo(),
                                &pfDesc->elemdescFunc.tdesc,
                                &iid))
    {
        if (IsEqualIID(iid, IID_DataSource) ||
            IsEqualIID(iid, IID_OLEDBSimpleProvider) ||
            IsEqualIID(iid, IID_IRowset))
        {
            pci->vtBindType = VT_UNKNOWN;
        }
    }
}


//+------------------------------------------------------------------------
//
//  Member:     GetDefaultBindInfoForPut, static helper
//
//  Synopsis:   save the info we need about the default bind property's
//              "put" method
//
//  Arguments:  pci             the class info structure
//              pfDesc          function description (from typelib)
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CClassTable::GetDefaultBindInfoForPut(CLASSINFO *pci, FUNCDESC *pfDesc)
{
    VARTYPE vtParamType;
    BOOL fParamIsPointer;
    
    pci->uPutBindIndex = pfDesc->oVft;
    pci->fImmediateBind = !!(pfDesc->wFuncFlags & FUNCFLAG_FIMMEDIATEBIND);
    pci->dwFlagsBind = pfDesc->invkind;

    // get the type of the put method's parameter
    Assert(pfDesc->cParams == 1);
    vtParamType = pfDesc->lprgelemdescParam->tdesc.vt;
    fParamIsPointer = (vtParamType == VT_PTR);
    if (fParamIsPointer)
    {
        vtParamType = pfDesc->lprgelemdescParam->tdesc.lptdesc->vt | VT_BYREF;
    }

    // it should agree with the type of the get method (if we've seen it)
    Assert(pci->vtBindType == VT_EMPTY || 
            (pci->vtBindType & VT_TYPEMASK) == (vtParamType & VT_TYPEMASK));
    pci->vtBindType = vtParamType;
}


//+------------------------------------------------------------------------
//
//  Member:     CClassTable::GetDualInfo, static
//
//  Synopsis:   Save the
//
//  Arguments:  pTI             the class typeinfo.
//              pci             the class info structure, may change:
//                                  fDualInterface
//                                  fImmediateBind
//                                  dispIDBind
//                                  uGetBindIndex
//                                  uPutBindIndex
//                                  vtBindType
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CClassTable::GetDualInfo(
        CTypeInfoNav & cTINav,
        FUNCDESC *pfDesc,
        CLASSINFO *pci)
{
    BOOL fValueProp = (pfDesc->memid == DISPID_VALUE);
    BOOL fDefaultBind = ( (pfDesc->wFuncFlags & 
                                (FUNCFLAG_FBINDABLE|FUNCFLAG_FDEFAULTBIND)) ==
                            (FUNCFLAG_FBINDABLE | FUNCFLAG_FDEFAULTBIND));
    
    switch (pfDesc->invkind)
    {
    case INVOKE_PROPERTYGET:
        // Save information about the defaultbind property
        if (fDefaultBind)
        {
            GetDefaultBindInfoForGet(pci, pfDesc, cTINav);
        }
        
        // Save information about the value property information (DISPID_VALUE)
        if (fValueProp)
        {
            pci->uGetValueIndex = pfDesc->oVft;
            pci->vtValueType = pfDesc->elemdescFunc.tdesc.vt;

            // if we haven't seen the defaultbind property, use the value property;
            // this will be overwritten if the defaultbind property shows up later.
            // The effect is to bind to the value property (if any) if no defaultbind
            // property is present.
            if (pci->dispIDBind == DISPID_UNKNOWN || pci->dispIDBind == DISPID_VALUE)
            {
                GetDefaultBindInfoForGet(pci, pfDesc, cTINav);
            }
        }
        break;

    case INVOKE_PROPERTYPUT:
    case INVOKE_PROPERTYPUTREF:
        // Save information about the defaultbind property
        if (fDefaultBind)
        {
            GetDefaultBindInfoForPut(pci, pfDesc);
        }
        
        // Save information about the value property information (DISPID_VALUE)
        if (fValueProp)
        {
            pci->uPutValueIndex = pfDesc->oVft;
            pci->dwFlagsValue = pfDesc->invkind;

            // if we haven't seen the defaultbind property, use the value property;
            // this will be overwritten if the defaultbind property shows up later.
            // The effect is to bind to the value property (if any) if no defaultbind
            // property is present.
            if (pci->dispIDBind == DISPID_UNKNOWN || pci->dispIDBind == DISPID_VALUE)
            {
                GetDefaultBindInfoForPut(pci, pfDesc);
            }
        }
        break;
    }
}



//+------------------------------------------------------------------------
//
//  Member:     CClassTable::FindTypelibInfo, static
//
//  Synopsis:   Look for the IRowset property and value property FuncFlags or
//              ValueFlags for this class, if they are found.
//
//  Arguments:  pTI             the class typeinfo.
//              pci             the class info structure, may change:
//                                  fImmediateBind
//                                  dispidRowset
//                                  dispIDBind
//                                  uGetBindIndex
//                                  uPutBindIndex
//                                  vtBindType
//                                  uSetRowset
//                                  dispidCursor
//                                  dispidSTD
//                                  uSetCursor
//                                  uGetSTD
//
//  Returns:    none
//
//-------------------------------------------------------------------------
void
CClassTable::FindTypelibInfo(ITypeInfo *pTI, CLASSINFO *pci)
{
    HRESULT         hr;
    CTypeInfoNav    cTINav;
    WORD            wVFFlags = 0;

    Assert(pTI);
    Assert(pci);

    // Look for the displaybind and bindable properties.
    hr = THR(cTINav.InitITypeInfo(pTI, 0));
    if (hr)
        goto Cleanup;

    // Keep looping through typeinfo until we find a property match.
    while (!cTINav.Next())
    {
        FUNCDESC    *pfDesc;

        pfDesc = cTINav.getFuncD();

        // Do we have a user defined type?
        if (pfDesc)
        {
            // Check for interesting interface-valued Get properties
            // with no paramter types.
            if (pfDesc->invkind == INVOKE_PROPERTYGET &&
                pfDesc->cParams == 0)
            {
                IID iid;

                // Check out the return type for a known interface.
                if (IsInterfaceProperty(pTI,
                                        &pfDesc->elemdescFunc.tdesc,
                                        &iid))
                {
                    if (IsEqualIID(iid, IID_DataSource))
                    {
                        pci->dispidIDataSource = pfDesc->memid;
                        pci->uGetIDataSource = pfDesc->oVft;
                    }
                    if (IsEqualIID(iid, IID_OLEDBSimpleProvider))
                    {
                        pci->dispidSTD = pfDesc->memid;
                        pci->uGetSTD = pfDesc->oVft;
                    }
                    if (IsEqualIID(iid, IID_IRowset))
                    {
                        pci->dispidRowset = pfDesc->memid;
                        pci->uGetRowset = pfDesc->oVft;
                    }
                }
            }

            // Check for interesting interface-valued put property.
            if (pfDesc->lprgelemdescParam &&
                (pfDesc->invkind == INVOKE_PROPERTYPUT ||
                 pfDesc->invkind == INVOKE_PROPERTYPUTREF))
            {
                IID iid;
                
                if (IsInterfaceProperty(pTI,
                                        &pfDesc->lprgelemdescParam->tdesc,
                                        &iid ) )
                {
                    if (IsEqualIID(iid, IID_DataSource))
                    {
                        pci->dispidIDataSource = pfDesc->memid;
                        pci->uSetIDataSource = pfDesc->oVft;
                        pci->dwFlagsDataSource = pfDesc->invkind;
                    }
                    if (IsEqualIID(iid, IID_IRowset))
                    {
                        pci->dispidRowset = pfDesc->memid;
                        pci->uSetRowset = pfDesc->oVft;
                        pci->dwFlagsRowset = pfDesc->invkind;
                    }
                    else if (IsEqualIID(iid, IID_IRowCursor))
                    {
                        pci->dispidCursor = pfDesc->memid;
                        pci->uSetCursor = pfDesc->oVft;
                    }
                }
            }

            if (cTINav.IsDualInterface())
            {
                GetDualInfo(cTINav, pfDesc, pci);
            }
        }
        else
        {
            VARDESC *pvDesc;

            pvDesc = cTINav.getVarD();
            if (pvDesc)
            {
                IID iid;
                
                wVFFlags = pvDesc->wVarFlags;

                // Check for the interface-valued properties
                if (IsInterfaceProperty(pTI, &pvDesc->elemdescVar.tdesc,
                                        &iid ) )
                {
                    if (IsEqualIID(iid, IID_DataSource))
                    {
                        pci->dispidIDataSource = pvDesc->memid;
                    }
                    else if (IsEqualIID(iid, IID_IRowset))
                    {
                        pci->dispidRowset = pvDesc->memid;
                    }
                    else if(IsEqualIID(iid, IID_OLEDBSimpleProvider))
                    {
                        pci->dispidSTD = pvDesc->memid;
                    }
                    else if(IsEqualIID(iid, IID_IRowCursor))
                    {
                        pci->dispidCursor = pvDesc->memid;
                    }
                }
            }

            // Default bindable property?
            if (((wVFFlags&(FUNCFLAG_FBINDABLE|FUNCFLAG_FDEFAULTBIND)) ==
                 (FUNCFLAG_FBINDABLE | FUNCFLAG_FDEFAULTBIND)) ||
                (pvDesc->memid == 0 && pci->dispIDBind == DISPID_UNKNOWN))
            {
                pci->dispIDBind = pvDesc->memid;
                pci->vtBindType = pvDesc->elemdescVar.tdesc.vt;
                if (pvDesc->wVarFlags & VARFLAG_FIMMEDIATEBIND)
                {
                    pci->fImmediateBind = TRUE;
                }

            }
        }
    }
    
    pci->fDualInterface = cTINav.IsDualInterface() &&
                            pci->uGetBindIndex!=0 && pci->uPutBindIndex!=0;

Cleanup:
    return;
}


//+------------------------------------------------------------------------
//
//  Member:     CClassTable::InitializeIIDsFromTIDefault, static
//
//  Synopsis:   Given the TypeInfo for the default interface of a class,
//              fill in whatever CLASSINFO members can be computed from it.
//              Helper function used no matter what mechanism is used to
//              obtain the TypeInfo.
//
//  Arguments:  pci         Fill in this CLASSINFO ..
//              pTIDefault  ... using this type info
//              ptaDefault  and the caller-fetched type attribute for
//                          this TypeInfo
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CClassTable::InitializeIIDsFromTIDefault(CLASSINFO *pci,
                                         ITypeInfo *pTIDefault,
                                         TYPEATTR *ptaDefault)
{
    HRESULT hr = S_OK;
    ITypeInfo * pTIDual= NULL;
    TYPEATTR *  ptaDual = NULL;
    HREFTYPE    hrt;

    
    //
    // BUGBUG: For now, only handle dual interfaces.
    //

    // Does the control class have an IRowset,Value and/or default
    // bindable property?
    FindTypelibInfo(pTIDefault, pci);

    pci->iidDefault = ptaDefault->guid;

    if (ptaDefault->wTypeFlags & TYPEFLAG_FDUAL)
    {
        hr = THR(pTIDefault->GetRefTypeOfImplType((UINT) -1, &hrt));
        if (hr)
            goto Cleanup;

        hr = THR(pTIDefault->GetRefTypeInfo(hrt, &pTIDual));
        if (hr)
            goto Cleanup;

        hr = THR(pTIDefault->GetTypeAttr(&ptaDual));
        if (hr)
            goto Cleanup;

        pci->cMethodsDefault = (int) (ptaDual->cbSizeVft / sizeof(void (*)()));
// BUGBUG (garybu) Should assert that tearoff table is large enough to thunk to this itf.
    }

Cleanup:
    if (ptaDual)
        pTIDual->ReleaseTypeAttr(ptaDual);
    ReleaseInterface(pTIDual);

    RRETURN1(hr, S_FALSE);
}

//+------------------------------------------------------------------------
//
//  Member:     CClassTable::InitializeIIDs, static
//
//  Synopsis:   Fetch the IIDs for a given CLASSINFO
//
//  Arguments:  pci     Fill in this CLASSINFO ..
//              pUnk    ... using type info from this object.
//
//  Returns:    HRESULT
//
//-------------------------------------------------------------------------

HRESULT
CClassTable::InitializeIIDs(CLASSINFO *pci, IUnknown *pUnk)
{
    HRESULT     hr = S_OK;
    int         i;
    ITypeInfo * pTIClass = NULL;
    TYPEATTR *  ptaClass = NULL;
    IProvideClassInfo *pPCI = NULL;
    IDispatch * pDispatch = NULL;

    Assert(!pci->fAllInitialized);
    
    // Should have been set when instantiated the control
    if (!pUnk)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // Get the ITypeInfo, either via IProvideClassInfo, or IDispatch
    //  BUGBUG: should we consider going to registry if we don't have
    //  IProvideClassInfo?  IDispatch mechanism below won't discover
    //  any eventset.
    if (OK(THR_NOTRACE(pUnk->
               QueryInterface(IID_IProvideClassInfo,
                              (void **) &pPCI))))
    {
        hr = THR(pPCI->GetClassInfo(&pTIClass));
        if (hr)
            goto Cleanup;

        hr = THR(pTIClass->GetTypeAttr(&ptaClass));
        if (hr)
            goto Cleanup;

        if (ptaClass->typekind != TKIND_COCLASS)
        {
            hr = E_FAIL;
            goto Cleanup;
        }

        for (i = 0; i < ptaClass->cImplTypes; i++)
        {
            int         implTypeFlags;
            HREFTYPE    hrt;
            ITypeInfo * pTIDefault = NULL;
            TYPEATTR *  ptaDefault = NULL;

            hr = THR(pTIClass->GetImplTypeFlags(i, &implTypeFlags));
            if (hr)
                goto Cleanup;

            if (!(implTypeFlags & IMPLTYPEFLAG_FDEFAULT))
                continue;

            hr = THR(pTIClass->GetRefTypeOfImplType(i, &hrt));
            if (hr)
                goto Cleanup;

            hr = THR(pTIClass->GetRefTypeInfo(hrt, &pTIDefault));
            if (hr)
                goto Cleanup;

            hr = THR(pTIDefault->GetTypeAttr(&ptaDefault));
            if (hr)
                goto LoopCleanup;

            if (!(ptaDefault->typekind & TKIND_DISPATCH))
                goto LoopCleanup;

            if (implTypeFlags & IMPLTYPEFLAG_FSOURCE)
            {
                pci->_pTypeInfoEvents = pTIDefault;
                pTIDefault->AddRef();

                pci->iidDispEvent = ptaDefault->guid;

            }
            else
            {
                hr = THR(InitializeIIDsFromTIDefault(pci, pTIDefault, ptaDefault));
            }
LoopCleanup:
            if (ptaDefault)
                pTIDefault->ReleaseTypeAttr(ptaDefault);

            ReleaseInterface(pTIDefault);
            if (hr)
                goto Cleanup;
        }

    }
    else if (OK(THR_NOTRACE(pUnk->
               QueryInterface(IID_IDispatch,
                              (void **) &pDispatch))))
    {
        // we cheat a little on variable names here; we are using pTIClass
        //  and ptaClass, even though we have the TypeInfo for the Dispatch
        //  itself.  This lets us leverage the cleanup logic below
        hr = THR(pDispatch->GetTypeInfo(0, 0 /* BUGBUG? */, &pTIClass));
        if (hr)
            goto Cleanup;
        hr = THR(pTIClass->GetTypeAttr(&ptaClass));
        if (hr)
            goto Cleanup;

        if (!(ptaClass->typekind & TKIND_DISPATCH))
        {
            // BUGBUG: frankman revmoved bogus in 819...Assert(!"IDispatch::GetTypeInfo return isn't TKIND_DISPATCH");
            goto Cleanup;
        }

        hr = THR(InitializeIIDsFromTIDefault(pci, pTIClass, ptaClass));
    }

Cleanup:
    if (ptaClass)
        pTIClass->ReleaseTypeAttr(ptaClass);

    ReleaseInterface(pTIClass);
    ReleaseInterface(pPCI);
    ReleaseInterface(pDispatch);

    if (hr != E_OUTOFMEMORY)
    {
        // we only try to initialize once, unless the problem was E_OUTOFMEMORY, which
        //  might get alleviated for subsequent attempts.
        pci->fAllInitialized = TRUE;
    }

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Member:     CClassTable::GetClassInfo
//
//  Synopsis:   Find pointer to class info given word class identifier.
//              Forces load of TypeInfo for class, if not already done.
//
//-------------------------------------------------------------------------

CLASSINFO *
CClassTable::GetClassInfo(int wclsid, IUnknown *pUnk)
{
    CLASSINFO * pci;

    Assert(0 <= wclsid && wclsid <= _aryci.Size());

    if (wclsid == 0)
    {
        return(&g_ciNull);
    }

    pci = &_aryci[wclsid - 1];
    
    if (!pci->fAllInitialized)          // Is it actually initialized?
    {
        // No, so initialize it.  We trust InitializeIIDS to leave us
        //  us something sane on failure.
        IGNORE_HR(InitializeIIDs(pci, pUnk));
    }

#if DBG == 1
    if (IsTagEnabled(tagShowHideVerb))
        pci->dwCompatFlags |= (COMPAT_SEND_SHOW | COMPAT_SEND_HIDE);
#endif

    return pci;
}



//+------------------------------------------------------------------------
//
//  Member:     CClassTable::GetQuickClassInfo
//
//  Synopsis:   Find pointer to "quick" class info given word class
//              identifier.  Tries to avoid loading the TypInfo for
//              the class, but may need to, if the class doesn't
//              support IProvideClassInfo2.
//
//-------------------------------------------------------------------------

QUICKCLASSINFO *
CClassTable::GetQuickClassInfo(int wclsid, IUnknown *pUnk)
{
    Assert(0 <= wclsid && wclsid <= _aryci.Size());

    HRESULT             hr;
    CLASSINFO *         pci;
    IProvideClassInfo2 *pPCI2 = 0;

    if (wclsid == 0)
    {
        pci = &g_ciNull;
        goto Cleanup;
    }

    pci = &_aryci[wclsid - 1];

    // IE5 Bug 66813: Some controls, namely the TAPI control
    // QI us after we have already started shutting down. When
    // they QI us, we have already deleted everything in _aryci
    // in the Reset() method. This method is called with wclsid
    // set to 1. In that case, _aryci returns NULL for position 0,
    // since everything is already deleted. This check is to 
    // save ourselves from controls not doing what they are supposed to do.
    //
    if (!pci)
        goto Cleanup;

    //
    // If the Default Event has not yet been initialized and the entire
    // class info has also not been initialized, QI the pUnk for 
    // IProvideClassInfo2 and cache the info.
    //
    // HACKHACKHACK: All applets have the same clsid but will have 
    // different event iids.  To have event sinking on multiple applets
    // work, never depend on the cached event iid if we're the applet ocx.
    // ASSUMPTION: The VM implements IProvideClassInfo2.
    //
    
    if ((!pci->fAllInitialized && !pci->fiidDEInitialized) ||
        (pci->clsid == CLSID_AppletOCX))
    {
        if (OK(THR(pUnk->
                   QueryInterface(IID_IProvideClassInfo2,
                                  (void **) &pPCI2))))
        {
            hr = pPCI2->GetGUID(GUIDKIND_DEFAULT_SOURCE_DISP_IID,
                                &pci->iidDispEvent);
            
            if (SUCCEEDED(hr))
            {
                pci->fiidDEInitialized = TRUE;
                goto Done;
            }
        }
        
        // If IProvideClassInfo2 is not supported, or the GetGUID
        // method failed, then we must try querying the TypeLibInfo..
        hr = InitializeIIDs(pci, pUnk);
    }

Done:
#if DBG == 1
    if (IsTagEnabled(tagShowHideVerb))
        pci->dwCompatFlags |= (COMPAT_SEND_SHOW | COMPAT_SEND_HIDE);
#endif
    ReleaseInterface(pPCI2);

Cleanup:
    return (QUICKCLASSINFO *) pci;
}



//+------------------------------------------------------------------------
//
//  Member:     CLASSINFO::ClearFDualInterface
//
//  Synopsis:   Change CLASSINFO marked as dual to a consistent state
//              reflecting that the class is IDispatch-only.  Meant
//              to be called if QI for the primary interface fails because
//              the primary interface isn't remoted across thread boundaries,
//              but a QI for IDispatch succeeds, because the system does
//              know how to remote IDispatch.
//
//-------------------------------------------------------------------------

void
CLASSINFO::ClearFDualInterface()
{
    // Mark control as NOT dual interface.
    fDualInterface = FALSE;

    // Set it so in later instances, we know to QI for IDispatch right
    // away, instead of trying the iidDefault marked in the Object's
    // typelib.
    iidDefault = IID_IDispatch;

    // This should really not be necessary, since people should check
    // dwFlags for CLSTABLE_DUALINTERFACE before jumping through the
    // VTable, but for now..
    uGetBindIndex = 0;
    uPutBindIndex = 0;
    uGetValueIndex = 0;
    uPutValueIndex = 0;
    uSetIDataSource = 0;
    uGetIDataSource = 0;
    uSetRowset = 0;
    uGetRowset = 0;
    uSetCursor = 0;
    uGetSTD = 0;
}

void
InitClassTable()
{
    g_ciNull.Init(GUID_NULL, FALSE);
}
