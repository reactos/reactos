/*****************************************************************/
/**				  Microsoft Windows for Workgroups				**/
/**		      Copyright (C) Microsoft Corp., 1991-1992			**/
/*****************************************************************/ 

/* NPGENERR.C -- Implementation of the DisplayGenericError subroutine.
 *
 * History:
 *	10/07/93	gregj	Created
 */

#include "npcommon.h"
#include "npmsg.h"
#include "npstring.h"

/*******************************************************************

	NAME:		DisplayGenericError

	SYNOPSIS:	Displays an error message, substituting in a
				description of an error code.

	ENTRY:		hwnd - parent window handle
				msg  - string ID for message template
				err  - error code to substitute in
				psz1 - main substitution string
				psz2 - error code substitution string
				wFlags - flags to MessageBox
				nMsgBase - bias for error code

	EXIT:		Returns control ID of user's choice

	NOTES:		nMsgBase is so that the error code need not
				be the same as the string ID.  It is added to
				the error code before loading the description,
				but the error code alone is the number inserted
				into the template.

				The text for "msg" should be of the form:

				The following error occurred while trying to fiddle with %1:

				Error %2: %3

				Do you want to continue fiddling?

				The text for "err" (+nMsgBase if appropriate) should
				be of the form:

				%1 cannot be fiddled with.

				In the primary message, %1 is replaced with psz1, %2
				is replaced with atoi(err), and %3 is replaced with
				a LoadString of "err".  In the specific error text,
				%1 is replaced with psz2.

	HISTORY:
		gregj	09/30/93	Created for Chicago

********************************************************************/

UINT DisplayGenericError(HWND hwnd, UINT msg, UINT err, LPCSTR psz1, LPCSTR psz2,
						 WORD wFlags, UINT nMsgBase)
{
	/*
	 * setup the object name
	 */
	NLS_STR nlsObjectName(STR_OWNERALLOC, (LPSTR)psz1);

	/*
	 * now the error number
	 */
	CHAR szErrorCode[16];
	wsprintf(szErrorCode,"%u",err);
	NLS_STR nlsErrorCode(STR_OWNERALLOC, szErrorCode);

	/*
	 * fetch the error string. If cannot get, use "".
	 */
	NLS_STR nlsSub1(STR_OWNERALLOC, (LPSTR)psz2);

	NLS_STR *apnlsParamStrings[4];
	apnlsParamStrings[0] = &nlsSub1;
	apnlsParamStrings[1] = NULL;

	NLS_STR nlsErrorString(NULL) ;
	nlsErrorString.LoadString(err + nMsgBase, (const NLS_STR **)apnlsParamStrings);
	err = nlsErrorString.QueryError() ;
	if (err)
		nlsErrorString = (const CHAR *)NULL;

	/*
	 * then create the insert strings table
	 */
	apnlsParamStrings[0] = &nlsObjectName;
	apnlsParamStrings[1] = &nlsErrorCode;
	apnlsParamStrings[2] = &nlsErrorString;
	apnlsParamStrings[3] = NULL;

    return MsgBox(hwnd, msg, wFlags, (const NLS_STR **)apnlsParamStrings);
}

