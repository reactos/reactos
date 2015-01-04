
#include <user32.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(ddeml);

/*
 * @unimplemented
 */
BOOL WINAPI DdeGetQualityOfService(HWND hWnd, DWORD Reserved, PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  UNIMPLEMENTED;
  return FALSE;
}
