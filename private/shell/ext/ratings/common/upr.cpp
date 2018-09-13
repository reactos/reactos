#include "npcommon.h"

/* In a DBCS build, the Win32 CharUpper API will uppercase double-byte
 * Romanji characters, but many low-level network components such as
 * IFSMGR (and NetWare servers!) do not uppercase any double-byte
 * characters.  So we have to have our own function which avoids them.
 *
 * This could be implemented by just calling CharUpper on each character,
 * but the NLS APIs have a fair amount of overhead to them, so calling
 * into the NLS subsystem as few times as possible is desirable.
 */
LPSTR WINAPI struprf(LPSTR lpString)
{
    if (!::fDBCSEnabled)
        return CharUpper(lpString);

	LPSTR pchStart = lpString;

	while (*pchStart != '\0') {
		// Skip any double-byte characters that may be here.
		// Don't need to check for end of string in the loop because
		// the null terminator is not a DBCS lead byte.
		while (IsDBCSLeadByte(*pchStart))
			pchStart += 2;	/* skip double-byte chars */

		if (*pchStart == '\0')
			break;			/* no more SBCs to uppercase */

		// Find the end of this range of single-byte characters, and
		// uppercase them.
		LPSTR pchEnd = pchStart + 1;
		while (*pchEnd && !IsDBCSLeadByte(*pchEnd))
			pchEnd++;		/* count single-byte chars */

		CharUpperBuff(pchStart, (int)(pchEnd-pchStart));
		pchStart = pchEnd;
	}

	return lpString;
}
