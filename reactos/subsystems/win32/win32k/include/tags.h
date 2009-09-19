#ifndef _WIN32K_TAGS_H
#define _WIN32K_TAGS_H

#define TAG_STRING      ' RTS' /* string */
#define TAG_RTLREGISTRY 'vrqR' /* RTL registry */

/* ntuser */
#define TAG_MOUSE       'SUOM' /* mouse */
#define TAG_KEYBOARD    ' DBK' /* keyboard */
#define TAG_ACCEL       'LCCA' /* accelerator */
#define TAG_HOOK        'KHNW' /* hook */
#define TAG_HOTKEY      'KTOH' /* hotkey */
#define TAG_MENUITEM    'INEM' /* menu item */
#define TAG_MSG         'GSEM' /* message */
#define TAG_MSGQ        'QGSM' /* message queue */
#define TAG_USRMSG      'GSMU' /* user message */
#define TAG_WNDPROP     'PRPW' /* window property */
#define TAG_WNAM        'MANW' /* window name */
#define TAG_WINLIST     'SLNW' /* window handle list */
#define TAG_WININTLIST  'PINW' /* window internal pos */
#define TAG_WINPROCLST  'LPNW' /* window proc list */
#define TAG_SBARINFO    'NIBS' /* scrollbar info */
#define TAG_TIMER       'RMIT' /* timer entry */
#define TAG_TIMERTD     'TMIT' /* timer thread dereference list */
#define TAG_TIMERBMP    'BMIT' /* timers bitmap */
#define TAG_CALLBACK    'KCBC' /* callback memory */
#define TAG_WINSTA      'ATSW' /* window station */
#define TAG_PDCE        'cdsU' /* dce */
#define TAG_INPUT       'yssU' /* Input */

/* gdi objects from the handle table */
#define TAG_DC          '1alG' /* dc */
#define TAG_REGION      '4alG' /* region */
#define TAG_SURFACE     '5alG' /* bitmap */
#define TAG_CLIENTOBJ   '60hG'
#define TAG_PATH        '70hG'
#define TAG_PALETTE     '8alG'
#define TAG_ICMLCS      '90hG'
#define TAG_LFONT       ':alG'
#define TAG_RFONT       ';0gG' /* correct? */
#define TAG_PFE         '<0hG'
#define TAG_PFT         '=0hG' /* correct? */
#define TAG_ICMCXF      '>0hG' /* correct? */
#define TAG_SPRITE      '?0hG' /* correct? */
#define TAG_BRUSH       '@alG'
#define TAG_UMPD        'A0hG' /* correct? */
#define TAG_SPACE       'c0hG' /* correct? */
#define TAG_META        'E0hG' /* correct? */
#define TAG_EFSTATE     'F0hG' /* correct? */
#define TAG_BMFD        'G0hG' /* correct? */
#define TAG_VTFD        'H0hG' /* correct? */
#define TAG_TTFD        'I0hG' /* correct? */
#define TAG_RC          'J0hG' /* correct? */
#define TAG_TEMP        'K0hG' /* correct? */
#define TAG_DRVOBJ      'L0hG' /* correct? */
#define TAG_DCIOBJ      'M0hG' /* correct? */
#define TAG_SPOOL       'N0hG' /* correct? */

/* other gdi objects */
#define TAG_BEZIER      'RZEB' /* bezier */
#define TAG_BITMAP      'PMTB' /* bitmap */
#define TAG_PATBLT      'TLBP' /* patblt */
#define TAG_CLIP        'PILC' /* clipping */
#define TAG_COORD       'DROC' /* coords */
#define TAG_GDIDEV      'vedG' /* gdi dev support*/
#define TAG_GDIPDEV     'veDG' /* gdi PDev */
#define TAG_GDIHNDTBLE  'HIDG' /* gdi handle table */
#define TAG_GDIICM      'mciG' /* gdi Icm */
#define TAG_DIB         ' BID' /* dib */
#define TAG_COLORMAP    'MLOC' /* color map */
#define TAG_SHAPE       'PAHS' /* shape */
#define TAG_PALETTEMAP  'MLAP' /* palette mapping */
#define TAG_PRINT       'TNRP' /* print */
#define TAG_GDITEXT     'OTXT' /* text */
#define TAG_PENSTYLES   'ytsG' /* pen styles */

/* Eng objects */
#define TAG_CLIPOBJ     'OPLC' /* clip object */
#define TAG_DRIVEROBJ   'OVRD' /* driver object */
#define TAG_DFSM        'msfD' /* Eng event allocation */
#define TAG_EPATH       'tapG' /* path object */
#define TAG_FONT        'ETNF' /* font entry */
#define TAG_FONTOBJ     'tnfG' /* font object */
#define TAG_WNDOBJ      'ODNW' /* window object */
#define TAG_XLATEOBJ    'OALX' /* xlate object */
#define TAG_GSEM        'mesG' /* Gdi Semaphore */

/* misc */
#define TAG_DRIVER      'VRDG' /* video drivers */
#define TAG_FNTFILE     'FTNF' /* font file */
#define TAG_SSECTPOOL   'PCSS' /* shared section pool */
#define TAG_PFF         'ffpG' /* physical font file */

/* Dx internal tags rember I do not known if it right namees */
#define TAG_DXPVMLIST   'LPXD' /* pmvlist for the driver */
#define TAG_DXFOURCC    'OFXD' /* pdwFourCC for the driver */
#define TAG_DDRAW       '1 hD' 
#define TAG_DDSURF      '2 hD'
#define TAG_EDDGBL      'GDDE' /* ? edd_directdraw_global ??*/


#endif /* _WIN32K_TAGS_H */
