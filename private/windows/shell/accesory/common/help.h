/* help.h
 *
 *	definitions for help facility
 *	most of which comes from FloydR's memo
 */

#define WM_HELP 0x3f8

#define HELP_INDEX   0xffff
#define HELP_FIND    0xfffe
#define HELP_QUIT    0xfffd
#define HELP_HELP    0xfffc
#define HELP_FOCUS   0xfffb
#define HELP_LAST    0xfffa
#define HELP_KEY2STR 0xfff9
#define HELP_STR2KEY 0xfff8
#define HELP_XLATE   0xfff7

BOOL FAR APIENTRY FRequestHelp(HANDLE, HWND, WORD);
