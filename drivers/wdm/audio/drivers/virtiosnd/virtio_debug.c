#include <ntddk.h>
#include <kdebugprint.h>

int virtioDebugLevel = 0;
int bDebugPrint = 1;
tDebugPrintFunc VirtioDebugPrintProc = (tDebugPrintFunc)DbgPrint;
