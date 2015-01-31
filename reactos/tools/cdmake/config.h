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

#define PUBLISHER_ID    "ReactOS Foundation"
#define DATA_PREP_ID    "ReactOS Foundation"
#define APP_ID          "CDMAKE CD-ROM Premastering Utility"
