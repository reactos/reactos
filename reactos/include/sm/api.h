/* $Id$ */
#ifndef __SM_API_H
#define __SM_API_H

#include <sm/ns.h>

#pragma pack(push,4)

/*** DATA TYPES ******************************************************/

#define SM_SB_NAME_MAX_LENGTH 120

#pragma pack(push,4)

/* SmConnectApiPort */
typedef struct _SM_CONNECT_DATA
{
  ULONG  Subsystem;
  WCHAR  SbName [SM_SB_NAME_MAX_LENGTH];

} SM_CONNECT_DATA, *PSM_CONNECT_DATA;

/* SmpConnectSbApiPort */
typedef struct _SB_CONNECT_DATA
{
  ULONG SmApiMax;
} SB_CONNECT_DATA, *PSB_CONNECT_DATA;


/*** SM API ***/

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

#define SM_CONNECT_DATA_SIZE(m)  ((m).Header.DataSize-sizeof(ULONG))
#define SM_PORT_DATA_SIZE(c)     (sizeof(DWORD)+sizeof(NTSTATUS)+sizeof(c))
#define SM_PORT_MESSAGE_SIZE     (sizeof(SM_PORT_MESSAGE))


#endif /* !def __SM_API_H */
