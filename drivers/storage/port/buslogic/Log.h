#include "ntddk.h"

#define DEFAULT_LOG_FILE_NAME	L"\\??\\D:\\Temp\\BL958.log"

BOOLEAN LogMessage(PCHAR szFormat, ...);
