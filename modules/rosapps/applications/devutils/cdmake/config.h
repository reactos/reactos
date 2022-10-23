#ifndef _WIN32
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define DIR_SEPARATOR_CHAR '/'
#define DIR_SEPARATOR_STRING "/"
#else
#define DIR_SEPARATOR_CHAR '\\'
#define DIR_SEPARATOR_STRING "\\"
#endif

#define MANUFACTURER_ID "ReactOS Project"
#define PUBLISHER_ID    "ReactOS Project"
#define DATA_PREP_ID    "ReactOS Project"
#define APP_ID          "CDMAKE CD-ROM Premastering Utility"
