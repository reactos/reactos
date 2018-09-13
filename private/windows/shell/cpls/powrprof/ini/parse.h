/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       PARSE.H
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        1 Jan, 1997
*
*  DESCRIPTION:
*
*   Declarations and definitions for the parse helper.
*
*******************************************************************************/

#define MAX_LINES           256
#define MAX_STR             128
#define DELIMITERS          ","
#define LINE_DELIMITERS     "\n\r"

// The following defines determine which zero based line of the spread sheet
// a given data will appear on. These are one based line indicies which match
// the spreadsheet line numbers as long as no blank lines are included.
 
#define NAME_LINE                               1
#define DESCRIPTION_LINE                        2

#define PLATFORM_LINE                           3
#define INSTALL_ON_LINE                         4

#define SYSTEM_IDLE_LINE                        7
#define SYSTEM_IDLE_TIMEOUT_LINE                8
#define SYSTEM_IDLE_SLEEP_ACTION_FLAGS_LINE     9
#define SYSTEM_IDLE_SENSITIVITY_LINE            15

#define MIN_SLEEP_LINE                          17
#define MAX_SLEEP_LINE                          18
#define REDUCED_LATENCY_SLEEP_LINE              19
#define DOZE_TIMEOUT_LINE                       20
#define DOZE_S4_TIMEOUT_LINE                    21

#define VIDEO_TIMEOUT_LINE                      23
#define SPINDOWN_TIMEOUT_LINE                   24

#define OPTIMIZE_FOR_POWER_LINE                 26
#define FAN_THROTTLE_TOL_LINE                   27
#define FORCED_THROTTLE_LINE                    28
#define MIN_THROTTLE_LINE                       29
#define OVERTHROTTLED_LINE                      30
#define OVERTHROTTLED_SLEEP_ACTION_FLAGS_LINE   31

// Global
#define ADVANCED_LINE                           38
#define LOCK_ON_SLEEP_LINE                      39
#define WAKE_ON_RING_LINE                       40
#define VIDEO_DIM_DISPLAY_LINE                  41
#define POWER_BUTTON_LINE                       43
#define POWER_BUTTON_SLEEP_ACTION_FLAGS_LINE    44
#define SLEEP_BUTTON_LINE                       50
#define SLEEP_BUTTON_SLEEP_ACTION_FLAGS_LINE    51
#define LID_CLOSE_LINE                          57
#define LID_CLOSE_SLEEP_ACTION_FLAGS_LINE       58
#define LID_OPEN_WAKE_LINE                      64

#define BROADCAST_CAP_RES_LINE                  66
#define BATMETER_ENABLE_SYSTRAY_FLAG_LINE       68
#define BATMETER_ENABLE_MULTI_FLAG_LINE         69
#define DISCHARGE_POLICY_1_LINE                 70
#define DISCHARGE_POLICY_2_LINE                 84

// Handy line offsets, for sleep action flags.
#define QUERY_APPS                      1
#define ALLOW_UI                        2
#define IGNORE_NON_RESP                 3
#define IGNORE_WAKE                     4
#define IGNORE_CRITICAL                 5


// Handy line offsets for DISCHARGE_POLICIES
#define DP_ENABLE                       1
#define DP_BAT_LEVEL                    2
#define DP_POWER_POLICY                 3
#define DP_MIN_SLEEP_STATE              4
#define DP_TEXT_NOTIFY                  6
#define DP_SOUND_NOTIFY                 7
#define DP_SLEEP_ACT_FLAGS              8

// Memphis INF types:

#define TYPICAL         0x01
#define COMPACT         0x02
#define CUSTOM          0x04
#define PORTABLE        0x08
#define SERVER          0x10
#define NUM_INF_TYPES   5

// OS types:
#define WIN_95          1
#define WIN_NT          2

// Function prototypes implemented in MAKEINI.C or MAKEINF.C:
VOID CDECL DefFatalExit(BOOLEAN, char*, ... );

// Function prototypes implemented in PARSE.C:

void StrToUpper(char*, char*);
UINT GetTokens(char*, UINT, char**, UINT, char*);
VOID GetCheckLabelToken(UINT, char*);
UINT GetFlagToken(UINT);
UINT GetPowerStateToken(VOID);
UINT GetIntToken(char*);
VOID GetNAToken(VOID);
POWER_ACTION GetPowerActionToken(VOID);
UINT GetOSTypeToken(VOID);
UINT GetINFTypeToken(VOID);
void StrTrimTrailingBlanks(char *);

