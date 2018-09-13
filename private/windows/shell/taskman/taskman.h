/****************************************************************************/
/*                                                                                                                  */
/*  TASKMAN.H -                                                                                             */
/*                                                                                                                  */
/*      Include for TASKMAN program                                                                 */
/*                                                                                                                       */
/****************************************************************************/

#ifndef RC_INVOKED
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#endif

#include <windows.h>
#include <winuserp.h>

/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  Function Templates                                                                                    */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*                                                                                                                  */
/*  Defines                                                                                                        */
/*                                                                                                                  */
/*--------------------------------------------------------------------------*/

#define LBS_MYSTYLE         (LBS_NOTIFY | LBS_OWNERDRAWFIXED | WS_VSCROLL)

#define SHOVEIT(x)          (MAKELONG((x),0))

#define MAXTASKNAMELEN      80
#define MAXMSGBOXLEN        513

#define PWRTASKMANDLG       10
#define WMPTASKMANDLG       11

#define IDD_TEXT            99
#define IDD_TASKLISTBOX     100
#define IDD_TERMINATE       101
#define IDD_CASCADE         102
#define IDD_TILE            103
#define IDD_ARRANGEICONS    104
#define IDD_RUN             105
#define IDD_PATH            106
#define IDD_CLTEXT          107
#define IDD_SWITCH          108

#define IDS_MSGBOXSTR1        201
#define IDS_MSGBOXSTR2        202
#define IDS_EXECERRTITLE      203
#define IDS_NOMEMORYMSG       204
#define IDS_FILENOTFOUNDMSG   205
#define IDS_BADPATHMSG        206
#define IDS_MANYOPENFILESMSG  207
#define IDS_ACCESSDENIED      208
#define IDS_NEWWINDOWSMSG     209
#define IDS_OS2APPMSG         210
#define IDS_MULTIPLEDSMSG     211
#define IDS_PMODEONLYMSG      212
#define IDS_COMPRESSEDEXE     213
#define IDS_INVALIDDLL        214
#define IDS_SHAREERROR        215
#define IDS_ASSOCINCOMPLETE   216
#define IDS_DDEFAIL           217
#define IDS_NOASSOCMSG        218
#define IDS_OOMEXITTITLE      219
#define IDS_OOMEXITMSG        220
#define IDS_UNKNOWNMSG        221


