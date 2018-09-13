// ============================================================================
// Internet Code Set Detection: Base Class
// ============================================================================

#include "private.h"
#include "detcbase.h"

/******************************************************************************
**********************   D E T E C T   S T R I N G   A   **********************
******************************************************************************/

int CINetCodeDetector::DetectStringA(LPCSTR lpSrcStr, int cchSrc)
{
	BOOL fDetected = FALSE;

	while (cchSrc-- > 0) {
		if (fDetected = DetectChar(*lpSrcStr++))
			break;
	}

	if (!fDetected)
		(void)CleanUp();

	return GetDetectedCodeSet();
}
