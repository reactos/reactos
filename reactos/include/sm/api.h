/* $Id$ */
#ifndef __SM_API_H
#define __SM_API_H

#define SM_API_PORT_NAME   L"\\SmApiPort"
#define SM_DBGSS_PORT_NAME L"\\DbgSsApiPort"
#define SM_DBGUI_PORT_NAME L"\\DbgUiApiPort"

#pragma pack(push,4)

/*** 1 ****************************************************************/

#define SM_API_COMPLETE_SESSION	1	/* complete a session initialization */

typedef struct _SM_PORT_MESSAGE_COMPSES
{
	HANDLE  hApiPort;
	HANDLE  hSbApiPort;

} SM_PORT_MESSAGE_COMPSES, *PSM_PORT_MESSAGE_COMPSES;

/*** 2 ****************************************************************/

#define SM_API_2 2

/* obsolete */

/*** 3 ****************************************************************/

#define SM_API_3 3

/* unknown */

/*** 4 ****************************************************************/

#define SM_API_EXECUTE_PROGRAMME	4	/* start a subsystem (server) */

#define SM_EXEXPGM_MAX_LENGTH	32		/* max count of wide string */

typedef struct _SM_PORT_MESSAGE_EXECPGM
{
  ULONG  NameLength;
  WCHAR  Name [SM_EXEXPGM_MAX_LENGTH];

} SM_PORT_MESSAGE_EXECPGM, *PSM_PORT_MESSAGE_EXECPGM;

/*** | ****************************************************************/

typedef struct _SM_PORT_MESSAGE
{
  /*** LPC common header ***/
  LPC_MESSAGE Header;
  /*** SM common header ***/
  DWORD       ApiIndex;
  NTSTATUS    Status;
  /*** SM per API arguments ***/
  union {
    SM_PORT_MESSAGE_COMPSES      CompSes;
    SM_PORT_MESSAGE_EXECPGM      ExecPgm;
  };

} SM_PORT_MESSAGE, * PSM_PORT_MESSAGE;

#pragma pack(pop)

/*** MACRO ***********************************************************/

#define SM_PORT_DATA_SIZE(c)  (sizeof(DWORD)+sizeof(NTSTATUS)+sizeof(c))
#define SM_PORT_MESSAGE_SIZE  (sizeof(SM_PORT_MESSAGE))


#endif /* !def __SM_API_H */
