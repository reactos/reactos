#ifndef _WIN32K_TAGS_H
#define _WIN32K_TAGS_H

#define TAG_STRING      TAG('S', 'T', 'R', ' ') /* string */
#define TAG_RTLREGISTRY TAG('R', 'q', 'r', 'v') /* RTL registry */

/* ntuser */
#define TAG_MOUSE       TAG('M', 'O', 'U', 'S') /* mouse */
#define TAG_KEYBOARD    TAG('K', 'B', 'D', ' ') /* keyboard */
#define TAG_ACCEL       TAG('A', 'C', 'C', 'L') /* accelerator */
#define TAG_HOOK        TAG('W', 'N', 'H', 'K') /* hook */
#define TAG_HOTKEY      TAG('H', 'O', 'T', 'K') /* hotkey */
#define TAG_MENUITEM    TAG('M', 'E', 'N', 'I') /* menu item */
#define TAG_MSG         TAG('M', 'E', 'S', 'G') /* message */
#define TAG_MSGQ        TAG('M', 'S', 'G', 'Q') /* message queue */
#define TAG_USRMSG      TAG('U', 'M', 'S', 'G') /* user message */
#define TAG_WNDPROP     TAG('W', 'P', 'R', 'P') /* window property */
#define TAG_WNAM        TAG('W', 'N', 'A', 'M') /* window name */
#define TAG_WINLIST     TAG('W', 'N', 'L', 'S') /* window handle list */
#define TAG_WININTLIST  TAG('W', 'N', 'I', 'P') /* window internal pos */
#define TAG_WINPROCLST  TAG('W', 'N', 'P', 'L') /* window proc list */
#define TAG_SBARINFO    TAG('S', 'B', 'I', 'N') /* scrollbar info */
#define TAG_TIMER       TAG('T', 'I', 'M', 'R') /* timer entry */
#define TAG_TIMERTD     TAG('T', 'I', 'M', 'T') /* timer thread dereference list */
#define TAG_TIMERBMP    TAG('T', 'I', 'M', 'B') /* timers bitmap */
#define TAG_CALLBACK    TAG('C', 'B', 'C', 'K') /* callback memory */
#define TAG_WINSTA      TAG('W', 'S', 'T', 'A') /* window station */
#define TAG_PDCE        TAG('U', 's', 'd', 'c') /* dce */

/* gdi objects from the handle table */
#define TAG_DC          TAG('G', 'l', 'a', '1') /* dc */
#define TAG_REGION      TAG('G', 'l', 'a', '4') /* region */
#define TAG_SURFACE     TAG('G', 'l', 'a', '5') /* bitmap */
#define TAG_CLIENTOBJ   TAG('G', 'h', '0', '6')
#define TAG_PATH        TAG('G', 'h', '0', '7')
#define TAG_PALETTE     TAG('G', 'l', 'a', '8')
#define TAG_ICMLCS      TAG('G', 'h', '0', '9')
#define TAG_LFONT       TAG('G', 'l', 'a', ':')
#define TAG_RFONT       TAG('G', 'h', '0', ';') /* correct? */
#define TAG_PFE         TAG('G', 'h', '0', '<')
#define TAG_PFT         TAG('G', 'h', '0', '=') /* correct? */
#define TAG_ICMCXF      TAG('G', 'h', '0', '>') /* correct? */
#define TAG_SPRITE      TAG('G', 'h', '0', '?') /* correct? */
#define TAG_BRUSH       TAG('G', 'l', 'a', '@')
#define TAG_UMPD        TAG('G', 'h', '0', 'A') /* correct? */
#define TAG_SPACE       TAG('G', 'h', '0', 'C') /* correct? */
#define TAG_META        TAG('G', 'h', '0', 'E') /* correct? */
#define TAG_EFSTATE     TAG('G', 'h', '0', 'F') /* correct? */
#define TAG_BMFD        TAG('G', 'h', '0', 'G') /* correct? */
#define TAG_VTFD        TAG('G', 'h', '0', 'H') /* correct? */
#define TAG_TTFD        TAG('G', 'h', '0', 'I') /* correct? */
#define TAG_RC          TAG('G', 'h', '0', 'J') /* correct? */
#define TAG_TEMP        TAG('G', 'h', '0', 'K') /* correct? */
#define TAG_DRVOBJ      TAG('G', 'h', '0', 'L') /* correct? */
#define TAG_DCIOBJ      TAG('G', 'h', '0', 'M') /* correct? */
#define TAG_SPOOL       TAG('G', 'h', '0', 'N') /* correct? */

/* other gdi objects */
#define TAG_BEZIER      TAG('B', 'E', 'Z', 'R') /* bezier */
#define TAG_BITMAP      TAG('B', 'T', 'M', 'P') /* bitmap */
#define TAG_PATBLT      TAG('P', 'B', 'L', 'T') /* patblt */
#define TAG_CLIP        TAG('C', 'L', 'I', 'P') /* clipping */
#define TAG_COORD       TAG('C', 'O', 'R', 'D') /* coords */
#define TAG_GDIDEV      TAG('G', 'd', 'e', 'v') /* gdi dev support*/
#define TAG_GDIPDEV     TAG('G', 'D', 'e', 'v') /* gdi PDev */
#define TAG_GDIHNDTBLE  TAG('G', 'D', 'I', 'H') /* gdi handle table */
#define TAG_GDIICM      TAG('G', 'i', 'c', 'm') /* gdi Icm */
#define TAG_DIB         TAG('D', 'I', 'B', ' ') /* dib */
#define TAG_COLORMAP    TAG('C', 'O', 'L', 'M') /* color map */
#define TAG_SHAPE       TAG('S', 'H', 'A', 'P') /* shape */
#define TAG_PALETTEMAP  TAG('P', 'A', 'L', 'M') /* palette mapping */
#define TAG_PRINT       TAG('P', 'R', 'N', 'T') /* print */
#define TAG_GDITEXT     TAG('T', 'X', 'T', 'O') /* text */
#define TAG_PENSTYLES   TAG('G', 's', 't', 'y') /* pen styles */

/* Eng objects */
#define TAG_CLIPOBJ     TAG('C', 'L', 'P', 'O') /* clip object */
#define TAG_DRIVEROBJ   TAG('D', 'R', 'V', 'O') /* driver object */
#define TAG_DFSM        TAG('D', 'f', 's', 'm') /* Eng event allocation */
#define TAG_EPATH       TAG('G', 'p', 'a', 't') /* path object */
#define TAG_FONT        TAG('F', 'N', 'T', 'E') /* font entry */
#define TAG_FONTOBJ     TAG('G', 'f', 'n', 't') /* font object */
#define TAG_WNDOBJ      TAG('W', 'N', 'D', 'O') /* window object */
#define TAG_XLATEOBJ    TAG('X', 'L', 'A', 'O') /* xlate object */
#define TAG_BITMAPOBJ   TAG('B', 'M', 'P', 'O') /* bitmap object */
#define TAG_GSEM        TAG('G', 's', 'e', 'm') /* Gdi Semaphore */

/* misc */
#define TAG_DRIVER      TAG('G', 'D', 'R', 'V') /* video drivers */
#define TAG_FNTFILE     TAG('F', 'N', 'T', 'F') /* font file */
#define TAG_SSECTPOOL   TAG('S', 'S', 'C', 'P') /* shared section pool */
#define TAG_PFF         TAG('G', 'p', 'f', 'f') /* physical font file */

/* Dx internal tags rember I do not known if it right namees */
#define TAG_DXPVMLIST   TAG('D', 'X', 'P', 'L') /* pmvlist for the driver */
#define TAG_DXFOURCC    TAG('D', 'X', 'F', 'O') /* pdwFourCC for the driver */
#define TAG_DDRAW       TAG('D', 'h', ' ', '1') 
#define TAG_DDSURF      TAG('D', 'h', ' ', '2')
#define TAG_EDDGBL      TAG('E', 'D', 'D', 'G') /* ? edd_directdraw_global ??*/


#endif /* _WIN32K_TAGS_H */
