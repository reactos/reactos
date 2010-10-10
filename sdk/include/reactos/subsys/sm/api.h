#ifndef __SM_API_H
#define __SM_API_H

#include <sm/ns.h>

/*** DATA TYPES ******************************************************/

#define SM_SB_NAME_MAX_LENGTH 120

#include <pshpack4.h>

/* SmConnectApiPort (SS->SM) */
typedef struct _SM_CONNECT_DATA
{
  USHORT SubSystemId;
  WORD   Unused;
  WCHAR  SbName [SM_SB_NAME_MAX_LENGTH];

} SM_CONNECT_DATA, *PSM_CONNECT_DATA;

/* SmpConnectSbApiPort (SM->SS) */
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

/*** 5 ****************************************************************/

#define SM_API_QUERY_INFORMATION	5	/* ask SM to send back some data */
						/* Note: this is not in NT */
#define SM_QRYINFO_MAX_SS_COUNT 8
#define SM_QRYINFO_MAX_ROOT_NODE 30

typedef enum {
	SmBasicInformation     = 0,
	SmSubSystemInformation = 1,
} SM_INFORMATION_CLASS;

typedef struct _SM_BASIC_INFORMATION
{
	USHORT SubSystemCount;
	WORD Unused;
	struct {
		WORD Id;
		WORD Flags;
		DWORD ProcessId;
	} SubSystem [SM_QRYINFO_MAX_SS_COUNT];
} SM_BASIC_INFORMATION, *PSM_BASIC_INFORMATION;

typedef struct _SM_SUBSYSTEM_INFORMATION
{
	WORD  SubSystemId;
	WORD  Flags;
	DWORD ProcessId;
	WCHAR NameSpaceRootNode [SM_QRYINFO_MAX_ROOT_NODE];
} SM_SUBSYSTEM_INFORMATION, *PSM_SUBSYSTEM_INFORMATION;

typedef struct _SM_PORT_MESSAGE_QRYINFO
{
	SM_INFORMATION_CLASS SmInformationClass;
	ULONG DataLength;
	union {
		SM_BASIC_INFORMATION BasicInformation;
		SM_SUBSYSTEM_INFORMATION SubSystemInformation;
	};
} SM_PORT_MESSAGE_QRYINFO, * PSM_PORT_MESSAGE_QRYINFO;

/*** | ****************************************************************/

typedef struct _SM_PORT_MESSAGE
{
    /*** LPC common header ***/
    PORT_MESSAGE Header;
    union
    {
        struct
        {
            /*** SM common header ***/
            struct
            {
                DWORD       ApiIndex;
                NTSTATUS    Status;
            } SmHeader;
            /*** SM per API arguments ***/
            union
            {
                union
                {
                    SM_PORT_MESSAGE_COMPSES      CompSes;
                    SM_PORT_MESSAGE_EXECPGM      ExecPgm;
                    SM_PORT_MESSAGE_QRYINFO      QryInfo;
                } Request;
                union
                {
                    SM_PORT_MESSAGE_COMPSES      CompSes;
                    SM_PORT_MESSAGE_EXECPGM      ExecPgm;
                    SM_PORT_MESSAGE_QRYINFO      QryInfo;
                } Reply;
            };
        };
        SM_CONNECT_DATA ConnectData;
    };
} SM_PORT_MESSAGE, * PSM_PORT_MESSAGE;

#include <poppack.h>

/*** MACRO ***********************************************************/

#define SM_CONNECT_DATA_SIZE(m)  ((m).Header.u1.s1.DataLength-sizeof(USHORT)-sizeof(WORD))
#define SM_PORT_DATA_SIZE(c)     (sizeof(DWORD)+sizeof(NTSTATUS)+sizeof(c))
#define SM_PORT_MESSAGE_SIZE     (sizeof(SM_PORT_MESSAGE))


#endif /* !def __SM_API_H */
