#include <ddk/ntddk.h>
#include <ddk/ntifs.h>
#include <windows.h>

#include "regtests.h"

static int
RunTest(char *Buffer)
{
  UNICODE_STRING Expression, Name;

  RtlInitUnicodeString(&Expression, L"f0_*.*");
  RtlInitUnicodeString(&Name, L"F0_000");
  FAIL_IF_FALSE(FsRtlDoesNameContainWildCards(&Expression),
                "FsRtlDoesNameContainWildCards didn't recognize valid expression");
  FAIL_IF_FALSE(FsRtlIsNameInExpression(&Expression, &Name, TRUE, NULL),
                "FsRtlIsNameInExpression failed to recognize valid match");
  FAIL_IF_TRUE(FsRtlIsNameInExpression(&Expression, &Name, FALSE, NULL),
               "FsRtlIsNameInExpression fails to enforce case sensitivity rules");

  return TS_OK;
}

DISPATCHER(Fs_1Test, "Kernel File System Runtime Library API")
