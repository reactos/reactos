// These were missed out of mmsystem.h so I define them here until I get
// round to submitting a patch for mmsystem.h :)

#define DRVM_INIT       100
#define DRVM_EXIT       101
#define DRVM_DISABLE        102
#define DRVM_ENABLE     103

#define MODM_INIT       DRVM_INIT
#define MODM_GETNUMDEVS     1
#define MODM_GETDEVCAPS     2
#define MODM_OPEN       3
#define MODM_CLOSE      4
#define MODM_PREPARE        5
#define MODM_UNPREPARE      6
#define MODM_DATA       7
#define MODM_LONGDATA       8
#define MODM_RESET              9
#define MODM_GETVOLUME      10
#define MODM_SETVOLUME      11
#define MODM_CACHEPATCHES   12
#define MODM_CACHEDRUMPATCHES   13

#define MIDM_INIT       DRVM_INIT
#define MIDM_GETNUMDEVS     53
#define MIDM_GETDEVCAPS     54
#define MIDM_OPEN           55
#define MIDM_CLOSE          56
#define MIDM_PREPARE        57
#define MIDM_UNPREPARE      58
#define MIDM_ADDBUFFER      59
#define MIDM_START          60
#define MIDM_STOP           61
#define MIDM_RESET          62
