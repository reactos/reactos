/* Includes */
#include <stdio.h>

#include <windows.h>
#include <reason.h>
#include <wininet.h>

#include <reactos/buildno.h>

/* Defines */
#define BUFFER_BLOCKSIZE  2048
#define BUILDNO_LENGTH    10
#define PLATFORM_LENGTH   25
#define SERVER_HOSTNAME   L"localhost"
#define SERVER_FILE       L"testman/webservice/"

/* Enums */
typedef enum _TESTTYPES
{
    WineTest
}
TESTTYPES;

/* Structs */
typedef struct _APP_OPTIONS
{
    BOOL Shutdown;
    BOOL Submit;
    PWSTR Module;
    PCHAR Test;
}
APP_OPTIONS, *PAPP_OPTIONS;

typedef struct _WINE_GETSUITEID_DATA
{
    PCHAR Module;
    PCHAR Test;
}
WINE_GETSUITEID_DATA, *PWINE_GETSUITEID_DATA;

typedef struct _GENERAL_SUBMIT_DATA
{
    PCHAR TestID;
    PCHAR SuiteID;
}
GENERAL_SUBMIT_DATA, *PGENERAL_SUBMIT_DATA;

typedef struct _WINE_SUBMIT_DATA
{
    GENERAL_SUBMIT_DATA General;
    PCHAR Log;
}
WINE_SUBMIT_DATA, *PWINE_SUBMIT_DATA;

typedef struct _GENERAL_FINISH_DATA
{
    PCHAR TestID;
}
GENERAL_FINISH_DATA, *PGENERAL_FINISH_DATA;

/* main.c */
extern APP_OPTIONS AppOptions;
extern HANDLE hProcessHeap;
extern PCHAR AuthenticationRequestString;
extern PCHAR SystemInfoRequestString;

/* shutdown.c */
BOOL ShutdownSystem();

/* tools.c */
VOID EscapeString(PCHAR Output, PCHAR Input);
VOID StringOut(PCHAR String);

/* webservice.c */
PCHAR GetTestID(TESTTYPES TestType);
PCHAR GetSuiteID(TESTTYPES TestType, const PVOID TestData);
BOOL Submit(TESTTYPES TestType, const PVOID TestData);
BOOL Finish(TESTTYPES TestType, const PVOID TestData);

/* winetests.c */
BOOL RunWineTests();
