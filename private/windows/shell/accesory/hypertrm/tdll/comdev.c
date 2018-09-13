/* comdev.c --	Communication device manipulation routines. These routines
 *				can be called by installation routines, etc. without
 *				including the rest of the com package. Use comdev.h for
 *				declarations.
 *
 *	Copyright 1992 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 1 $
 *	$Date: 10/05/98 12:38p $
 */
#include <windows.h>
#include <stdlib.h>
#include <tdll\stdtyp.h>
#include "com.h"
#include "comdev.h"
#include "com.hh"


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComFindDevices
 *
 * DESCRIPTION:
 *	May be called repeatedly to enumerate all com drivers available on the
 *	system. pstFind->uAction should be set to COM_FIND_FIRST before the
 *	first call. If the function returns COM_OK, then the fields of pstFind
 *	will have been filled in with information about the driver found. The
 *	uAction member will have automatically been changed to COM_FIND_NEXT.
 *	As long as subsequent calls return COM_OK, the structure will contain
 *	info on the next driver.
 *	Note: between consecutive calls to this function, no changes should be
 *		  made to the COM_FIND_DEVICE structure.
 *
 * ARGUMENTS:
 *	pstFind -- Pointer to a structure of type COM_FIND_DEVICE. The usAction
 *				member of this structure must be set to COM_FIND_FIRST prior
 *				to the first call.
 *
 * RETURNS:
 *	COM_OK		  if another valid driver was found. The pstFind structure
 *				  will be updated with information about the driver. To
 *				  retrieve all drivers, this function should be called again
 *				  if this value is returned.
 *	COM_NOT_FOUND if no more drivers are found. One more call to the function
 *				  should be made with pstFind->usAction set to COM_FIND_DONE
 *	COM_FAILED	  if arguments are invalid: NULL passed as argument; uAction
 *				  member invalid; or calls made out of sequence (COM_FIND_NEXT
 *				  called before COM_FIND_FIRST, COM_FIND_DONE called without
 *				  COM_FIND_FIRST)
 */
int ComFindDevices(COM_FIND_DEVICE * const pstFind)
	{
	int 	  iRetVal = COM_OK;
	unsigned  uDriverVersion;
	TCHAR	  szPath[MAX_PATH];
	TCHAR	 *pszNamePart;
	int 	  fFound = FALSE;
	int 	  fResult;
	HINSTANCE hModule;
	int 	  (*pfDeviceGetInfo)(unsigned *, TCHAR *, int);

	if (!pstFind)
		return COM_FAILED;

	szPath[0] = (TCHAR)0;
	if (pstFind->iAction == COM_FIND_DONE)
		{
		// Can't do a find done until we've at least done a find first
		if (pstFind->iReserved != 1)
			iRetVal = COM_FAILED;
		else
			{
			pstFind->iReserved = 0;
			if (pstFind->hFind != INVALID_HANDLE_VALUE)
				FindClose(pstFind->hFind);
			}
		}
	else
		{
		if (pstFind->iAction == COM_FIND_FIRST)
			{
			GetModuleFileName((HINSTANCE)0, pstFind->szFileName,
					sizeof(pstFind->szFileName));
			GetFullPathName(pstFind->szFileName, sizeof(szPath), szPath,
					&pszNamePart);

			// GetFullPathName gave us the address of the file name
			//	component of the path. Lop off the name and replace it
			//	with the pattern we want to search for.
			*pszNamePart = (TCHAR)0;
			lstrcat(szPath, TEXT("HACW*.DLL"));

			// set flag to indicate that the find first has been done so
			// we know that find nexts are ok
			pstFind->iReserved = 1;
			}
		else if (pstFind->iAction == COM_FIND_NEXT)
			{
			if (pstFind->iReserved != 1)
				iRetVal = COM_FAILED;
			}
		else
			iRetVal = COM_FAILED;

		fFound = FALSE;
		while (iRetVal == COM_OK && !fFound)
			{
			if (pstFind->iAction == COM_FIND_FIRST)
				{
				pstFind->hFind = FindFirstFile(szPath, &pstFind->stFindData);
				fResult = (pstFind->hFind != INVALID_HANDLE_VALUE);
				pstFind->iAction = COM_FIND_NEXT;
				}
			else
				{
				fResult = FindNextFile(pstFind->hFind, &pstFind->stFindData);
				}

			if (!fResult)
				{
				if (pstFind->hFind != INVALID_HANDLE_VALUE)
					FindClose(pstFind->hFind);
				iRetVal = COM_NOT_FOUND;
				pstFind->iReserved = 0;
				pstFind->iAction = COM_FIND_DONE;
				}
			else
				{
				// Found a file, check to see if it is a valid driver and
				//	 retrieve the device name
				lstrcpy(pstFind->szFileName, pstFind->stFindData.cFileName);

				hModule = LoadLibrary(pstFind->szFileName);
				if (hModule)
					{
					// Note that GetProcAddress does not accept unicode strings
					pfDeviceGetInfo = (int (*)(unsigned *, TCHAR *, int))
							GetProcAddress(hModule, "DeviceGetInfo");
					if (pfDeviceGetInfo &&
							(*pfDeviceGetInfo)(&uDriverVersion,
							pstFind->szDeviceName,
							sizeof(pstFind->szDeviceName)) == COM_OK &&
							uDriverVersion == COM_VERSION)
						fFound = TRUE;

					FreeLibrary(hModule);
					lstrcpy(pstFind->szFileName, pstFind->stFindData.cFileName);
					}
				}
			}
		}
	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComFindPorts
 *
 * DESCRIPTION:
 *	Allows enumeration of all standard port names acceptable to a given
 *	com device driver. Note that it is not always possible to enumerate
 *	all possible port names. This function will provide a list of names
 *	to suggest to the user, but non-enumerated names may also be valid.
 *
 * ARGUMENTS:
 *	pstFind 	 -- A structure of type COM_FIND_PORT that must be supplied
 *					by the caller.
 *	pszFileName  -- The file name of the module to be queried. If the desired
 *					module has already been loaded, then this field can be
 *					passed as NULL and the hModule argument can be
 *					included. If only a device type name is known, the
 *					function ComGetFileNameFromDeviceName() can be used.
 *	hModule 	 -- The module handle of the driver from which port names
 *					should be obtained. If this argument is non-zero, this
 *					module will be used and pszFileName will be ignored.
 *
 * RETURNS:
 *	COM_OK		  If another port name was found. The pstFind structure
 *				  will be updated with information about the Name. To
 *				  retrieve all drivers, this function should be called again
 *				  if this value is returned.
 *	COM_NOT_FOUND if no more names are found.
 *	COM_FAILED	  if arguments are invalid
 */
int ComFindPorts(COM_FIND_PORT * const pstFind, const TCHAR * const pszFileName,
							const HINSTANCE hModule)
	{
	int iRetVal = COM_OK;
	int (*pfFindPorts)(COM_FIND_PORT *);

	if (!pstFind || (!pszFileName && hModule == (HINSTANCE)0))
		return COM_FAILED;

	switch (pstFind->iAction)
		{
	case COM_FIND_FIRST:
		if (hModule)
			pstFind->hModule = hModule;
		else
			pstFind->hModule = LoadLibrary(pszFileName);

		if (!pstFind->hModule)
			iRetVal = COM_DEVICE_INVALID;
		break;

	case COM_FIND_NEXT:
	case COM_FIND_DONE:
		if (!pstFind->hModule)
			iRetVal = COM_FAILED;
		break;

	default:
		iRetVal = COM_FAILED;
		break;
		}

	if (iRetVal == COM_OK)
		{
		pfFindPorts = (int (*)(COM_FIND_PORT *))
				GetProcAddress(pstFind->hModule, "DeviceFindPorts");
		if (!pfFindPorts)
			iRetVal = COM_DEVICE_ERROR;
		else
			iRetVal = (*pfFindPorts)(pstFind);

		switch (pstFind->iAction)
			{
		case COM_FIND_FIRST:
		case COM_FIND_NEXT:
			if (iRetVal == COM_OK)
				{
				pstFind->iAction = COM_FIND_NEXT;
				break;
				}
			/* fall through */

		case COM_FIND_DONE:
			if (pstFind->hModule)
				FreeLibrary(pstFind->hModule);
			pstFind->hModule = (HINSTANCE)0;
			pstFind->iAction = COM_FIND_DONE;
			break;

		default:
			iRetVal = COM_FAILED;
			break;
			}
		}

	return iRetVal;
	}


/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION: ComGetFileNameFromDeviceName
 *
 * DESCRIPTION:
 *	Given the name of a com device, this routine will attempt to find the
 *	.DLL file that implements that device and return its name.
 *
 * ARGUMENTS:
 *	pszDeviceName -- The name of the device whose file should be found
 *	pszFileName   -- A buffer to receive the file name if it is located
 *	usSize		  -- The size of the pszFileName buffer
 *
 * RETURNS:
 *	COM_OK if device driver was found (file name is copied to pszFileName)
 *	COM_NOT_FOUND if no such driver can be found
 *	COM_FAILED if arguments are invalid or if usSize is too small.
 */
int ComGetFileNameFromDeviceName(const TCHAR * const pszDeviceName,
		TCHAR * const pszFileName, const int nSize)
	{
	int iRetVal = COM_NOT_FOUND;
	COM_FIND_DEVICE stComFind;

	// Note: since this routine can be called from non-HA/Win programs (such
	//		 as install routines, it must not call any functions that
	//		 assume HA/Win is running.

	if (!pszDeviceName || !*pszDeviceName || !pszFileName)
		return COM_FAILED;

	*pszFileName = (TCHAR)0;
	stComFind.iAction = COM_FIND_FIRST;
	while (ComFindDevices(&stComFind) == COM_OK)
		{
		if (lstrcmpi(pszDeviceName, stComFind.szDeviceName) == 0)
			{
			if ((lstrlen(stComFind.szFileName) + 1) > nSize)
				iRetVal = COM_FAILED;
			else
				{
				lstrcpy(pszFileName, stComFind.szFileName);
				iRetVal = COM_OK;
				}

			// Since we stop scanning names early, we must shut down search
			stComFind.iAction = COM_FIND_DONE;
			ComFindDevices(&stComFind);
			break;
			}
		}
	return iRetVal;
	}
