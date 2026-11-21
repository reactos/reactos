#include <kdebugprint.h>
#include <ntddk.h>

int virtioDebugLevel = 0;
int bDebugPrint = 1;
tDebugPrintFunc VirtioDebugPrintProc = (tDebugPrintFunc)DbgPrint;
