/****************************************************************************\
 *
 *   PICSUSER.C -- Structure for holding user information
 *
 *	 Created:  	02/29/96 gregj
 *				from original sources by t-jasont
 *	 
\****************************************************************************/

/*Includes------------------------------------------------------------------*/
#include "msrating.h"
#include "mslubase.h"


BOOL GetRegBool(HKEY hKey, LPCSTR pszValueName, BOOL fDefault)
{
	BOOL fRet = fDefault;
    DWORD dwSize, dwValue, dwType;
    UINT uErr;

    dwSize = sizeof(dwValue);

    uErr = RegQueryValueEx(hKey, pszValueName, NULL, &dwType, 
    						(LPBYTE)&dwValue, &dwSize);

    if (uErr == ERROR_SUCCESS)
    {
		if ((dwType == REG_DWORD) || (dwType == REG_BINARY && dwSize >= sizeof(fRet)))
			fRet = dwValue;
    }

    return fRet;
}


void SetRegBool(HKEY hkey, LPCSTR pszValueName, BOOL fValue)
{
	RegSetValueEx(hkey, pszValueName, 0, REG_DWORD, (LPBYTE)&fValue, sizeof(fValue));
}


PicsRatingSystem *FindInstalledRatingSystem(LPCSTR pszRatingService)
{
	UINT cServices = gPRSI->arrpPRS.Length();

	for (UINT i=0; i<cServices; i++) {
		PicsRatingSystem *pPRS = gPRSI->arrpPRS[i];
		if (!(pPRS->dwFlags & PRS_ISVALID) || !pPRS->etstrRatingService.fIsInit())
			continue;
		if (!::strcmpf(pPRS->etstrRatingService.Get(), pszRatingService))
			return pPRS;
	}
	return NULL;
}


PicsCategory *FindInstalledCategory(array<PicsCategory *>&arrpPC, LPCSTR pszName)
{
	UINT cCategories = arrpPC.Length();

	for (UINT i=0; i<cCategories; i++) {
		LPSTR pszThisName = arrpPC[i]->etstrTransmitAs.Get();
		if (!::strcmpf(pszThisName, pszName))
			return arrpPC[i];
		if (!::strncmpf(pszThisName, pszName, strlenf(pszThisName)) &&
			arrpPC[i]->arrpPC.Length() > 0) {
			PicsCategory *pCategory = FindInstalledCategory(arrpPC[i]->arrpPC, pszName);
			if (pCategory != NULL)
				return pCategory;
		}
	}
	return NULL;
}


UserRating::UserRating()
	: NLS_STR(NULL),
	  m_nValue(0),
	  m_pNext(NULL),
	  m_pPC(NULL)
{
}


UserRating::UserRating(UserRating *pCopyFrom)
	: NLS_STR(*pCopyFrom),
	  m_nValue(pCopyFrom->m_nValue),
	  m_pNext(NULL),
	  m_pPC(pCopyFrom->m_pPC)
{
}


UserRating::~UserRating()
{
	// needed to destruct name string
}


UserRating *UserRating::Duplicate(void)
{
	UserRating *pNew = new UserRating(this);
	return pNew;
}


UserRatingSystem::UserRatingSystem()
	: NLS_STR(NULL),
	  m_pRatingList(NULL),
	  m_pNext(NULL),
	  m_pPRS(NULL)
{

}


UserRatingSystem::UserRatingSystem(UserRatingSystem *pCopyFrom)
	: NLS_STR(*pCopyFrom),
	  m_pRatingList(NULL),
	  m_pNext(NULL),
	  m_pPRS(pCopyFrom->m_pPRS)
{

}


UserRatingSystem *UserRatingSystem::Duplicate(void)
{
	UserRatingSystem *pNew = new UserRatingSystem(this);
	if (pNew != NULL) {
		UserRating *pRating;

		for (pRating = m_pRatingList; pRating != NULL; pRating = pRating->m_pNext) {
			UserRating *pNewRating = pRating->Duplicate();
			if (pNewRating != NULL) {
				if (pNew->AddRating(pNewRating) != ERROR_SUCCESS) {
					delete pNewRating;
					pNewRating = NULL;
				}
			}

			if (pNewRating == NULL)
				break;
		}
	}

	return pNew;
}


UserRatingSystem *DuplicateRatingSystemList(UserRatingSystem *pOld)
{
	UserRatingSystem *pNewList = NULL;

	while (pOld != NULL) {
		UserRatingSystem *pNewEntry = pOld->Duplicate();
		if (pNewEntry == NULL)
			break;

		pNewEntry->m_pNext = pNewList;
		pNewList = pNewEntry;

		pOld = pOld->m_pNext;
	}

	return pNewList;
}


UserRatingSystem::~UserRatingSystem()
{
	UserRating *pRating, *pNext;

	for (pRating = m_pRatingList; pRating != NULL; )
	{
		pNext = pRating->m_pNext;
		delete pRating;
		pRating = pNext;
	}

#ifdef DEBUG
	m_pRatingList = NULL;
#endif
}


UserRating *UserRatingSystem::FindRating(LPCSTR pszTransmitName)
{
	UserRating *p;

	for (p = m_pRatingList; p != NULL; p = p->m_pNext)
	{
		if (!::stricmpf(p->QueryPch(), pszTransmitName))
			break;
	}

	return p;
}


UINT UserRatingSystem::AddRating(UserRating *pRating)
{
	pRating->m_pNext = m_pRatingList;
	m_pRatingList = pRating;
	return ERROR_SUCCESS;
}


UINT UserRatingSystem::ReadFromRegistry(HKEY hkeyProvider)
{
	UINT err;
	DWORD iValue = 0;
	char szValueName[MAXPATHLEN];
	DWORD cchValue;
	DWORD dwValue;
	DWORD cbData;

	do {
		cchValue = sizeof(szValueName);
		cbData = sizeof(dwValue);
		err = RegEnumValue(hkeyProvider, iValue, szValueName, &cchValue,
						   NULL, NULL, (LPBYTE)&dwValue, &cbData);
		if (err == ERROR_SUCCESS && cbData >= sizeof(dwValue)) {
			UserRating *pRating = new UserRating;
			if (pRating != NULL) {
				if (pRating->QueryError()) {
					err = pRating->QueryError();
				}
				else {
					pRating->SetName(szValueName);
					pRating->m_nValue = (INT)dwValue;
					if (m_pPRS != NULL)
						pRating->m_pPC = FindInstalledCategory(m_pPRS->arrpPC, szValueName);
					err = AddRating(pRating);
				}
				if (err != ERROR_SUCCESS)
					delete pRating;
			}
			else
				err = ERROR_NOT_ENOUGH_MEMORY;
		}
		iValue++;
	} while (err == ERROR_SUCCESS);

	if (err == ERROR_NO_MORE_ITEMS)
		err = ERROR_SUCCESS;

	return err;
}


UINT UserRatingSystem::WriteToRegistry(HKEY hkeyRatings)
{
	UserRating *pRating;
	UINT err = ERROR_SUCCESS;
	HKEY hkey;

	err = RegCreateKey(hkeyRatings, QueryPch(), &hkey);
	if (err != ERROR_SUCCESS)
		return err;

	for (pRating = m_pRatingList; pRating != NULL; pRating = pRating->m_pNext)
	{
		err = RegSetValueEx(hkey, pRating->QueryPch(), 0, REG_DWORD,
							(LPBYTE)&pRating->m_nValue, sizeof(pRating->m_nValue));
		if (err != ERROR_SUCCESS)
			break;
	}

	RegCloseKey(hkey);
	return err;
}


PicsUser::PicsUser()
	: nlsUsername(NULL),
	  fAllowUnknowns(FALSE),
	  fPleaseMom(TRUE),
	  fEnabled(TRUE),
	  m_pRatingSystems(NULL)
{
}


PicsRatingSystemInfo::~PicsRatingSystemInfo()
{
	arrpPRS.DeleteAll();
#ifdef NASH
	arrpPU.DeleteAll();
#else
	delete pUserObject;
#endif
}


void DestroyRatingSystemList(UserRatingSystem *pList)
{
	UserRatingSystem *pSystem, *pNext;

	for (pSystem = pList; pSystem != NULL; )
	{
		pNext = pSystem->m_pNext;
		delete pSystem;
		pSystem = pNext;
	}
}


PicsUser::~PicsUser()
{
	DestroyRatingSystemList(m_pRatingSystems);
#ifdef DEBUG
	m_pRatingSystems = NULL;
#endif
}


UserRatingSystem *FindRatingSystem(UserRatingSystem *pList, LPCSTR pszSystemName)
{
	UserRatingSystem *p;

	for (p = pList; p != NULL; p = p->m_pNext)
	{
		if (!::strcmpf(p->QueryPch(), pszSystemName))
			break;
	}

	return p;
}


UINT PicsUser::AddRatingSystem(UserRatingSystem *pRatingSystem)
{
	pRatingSystem->m_pNext = m_pRatingSystems;
	m_pRatingSystems = pRatingSystem;
	return ERROR_SUCCESS;
}


UINT PicsUser::ReadFromRegistry(HKEY hkey, char *pszUserName)
{
    HKEY hkeyUser, hkeyRatings, hKeyProvider;

#if NASH
	if (!::strcmpf(pszUserName, szDefaultUserName))
		nlsUsername = szBETTERNAME;
	else
#endif        
    	nlsUsername = pszUserName;

	UINT err = (UINT)RegOpenKey(hkey, pszUserName, &hkeyUser);
	if (err != ERROR_SUCCESS)
		return err;

	fAllowUnknowns = GetRegBool(hkeyUser, VAL_UNKNOWNS, FALSE);
	fPleaseMom = GetRegBool(hkeyUser, VAL_PLEASEMOM, TRUE);
	fEnabled = GetRegBool(hkeyUser, VAL_ENABLED, TRUE);

#if 0	
    if (RegOpenKey(hkeyUser, szRATINGS, &hkeyRatings) == ERROR_SUCCESS)
#else
	hkeyRatings = hkeyUser;
#endif
    {
        char szKeyName[MAXPATHLEN];
		int j = 0;
        // enumerate the subkeys, which are rating systems
        while (((err = RegEnumKey(hkeyRatings,j,szKeyName,sizeof(szKeyName)))== ERROR_SUCCESS) &&
            ((err = RegOpenKey(hkeyRatings,szKeyName,&hKeyProvider)) == ERROR_SUCCESS))
        {
			UserRatingSystem *pRatingSystem = new UserRatingSystem;
			if (pRatingSystem == NULL) {
				err = ERROR_NOT_ENOUGH_MEMORY;
				break;
			}

			if (pRatingSystem->QueryError()) {
				err = pRatingSystem->QueryError();
			}
			else {
				pRatingSystem->SetName(szKeyName);
				pRatingSystem->m_pPRS = FindInstalledRatingSystem(szKeyName);
				err = pRatingSystem->ReadFromRegistry(hKeyProvider);
				if (err == ERROR_SUCCESS)
					err = AddRatingSystem(pRatingSystem);
			}
			if (err != ERROR_SUCCESS) {
				delete pRatingSystem;
			}

            RegCloseKey(hKeyProvider);
			j++;
        }

#if 0
		RegCloseKey(hkeyRatings);
#endif
    }
	// end of enum will report ERROR_NO_MORE_ITEMS, don't report this as error
	if (err == ERROR_NO_MORE_ITEMS) err = ERROR_SUCCESS;

	RegCloseKey(hkeyUser);
	return err;
}

BOOL PicsUser::NewInstall()
{
    nlsUsername = szDefaultUserName;
	fAllowUnknowns = FALSE;
    fPleaseMom = TRUE;
	fEnabled = TRUE;
	
    return TRUE;
}


UINT PicsUser::WriteToRegistry(HKEY hkey)
{
    HKEY hkeyUser, hkeyRatings;
	UINT err;

#if NASH
	if (!::strcmpf(nlsUsername.QueryPch(), szBETTERNAME))
		nlsUsername = szDefaultUserName;
#endif        

	//Delete it to clean out registry
    MyRegDeleteKey(hkey, nlsUsername.QueryPch());

	err = RegCreateKeyEx(hkey, nlsUsername.QueryPch(), NULL, NULL,
						 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
						 NULL, &hkeyUser, NULL);
	if (err != ERROR_SUCCESS)
		return err;

	SetRegBool(hkeyUser, VAL_UNKNOWNS, fAllowUnknowns);
	SetRegBool(hkeyUser, VAL_PLEASEMOM, fPleaseMom);
	SetRegBool(hkeyUser, VAL_ENABLED, fEnabled);
#ifdef NASH   
	EtStringRegWrite(etstrLogFileName, hkeyUser, VAL_LOGFILE);
	EtBoolRegWrite(etfControlPanel, hkeyUser, VAL_CONTROLPANEL);
	EtBoolRegWrite(etfNewApps, hkeyUser, VAL_NEWAPPS);
	EtBoolRegWrite(etfMaster, hkeyUser, VAL_MASTERUSER);
#endif    

#if 0
	err = RegCreateKeyEx(hkeyUser, szRATINGS, NULL, NULL,
						 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
						 NULL,&hkeyRatings, NULL);
	if (err == ERROR_SUCCESS)
#else
	hkeyRatings = hkeyUser;
#endif
    {
		UserRatingSystem *pSystem;

		/* Note, if any user settings correspond to invalid or unknown
		 * rating systems, we still save them here.  That way, the user
		 * settings don't get lost if the supervisor later fixes a problem
		 * with a .RAT file.
		 *
		 * We clean up user settings to match the installed rating systems
		 * in the add/remove rating systems dialog code.
		 */
		for (pSystem = m_pRatingSystems; pSystem != NULL; pSystem = pSystem->m_pNext)
		{
			err = pSystem->WriteToRegistry(hkeyRatings);
			if (err != ERROR_SUCCESS)
				break;
		}

#if 0
		RegCloseKey(hkeyRatings);
#endif
	}
	RegCloseKey(hkeyUser);
	return err;
}


PicsUser *GetUserObject(LPCSTR pszUsername /* = NULL */ )
{
#ifdef NASH
	if (pszUsername == NULL)
		pszUsername = ::szCurrentUser;

	UINT cUsers = gPRSI->arrpPU.Length();
	for (UINT iUser=0; iUser < cUsers; iUser++)
	{
		if (strcmpf(gPRSI->arrpPU[iUser]->etstrUserName.Get(), pszUsername) == 0)
			break;
	}

	if (iUser < cUsers)
		return gPRSI->arrpPU[iUser];
	else
		return NULL;
#else
	return gPRSI->pUserObject;
#endif
}


void DeleteUserSettings(PicsRatingSystem *pPRS)
{
	if (!pPRS->etstrRatingService.fIsInit())
		return;		/* can't recognize user settings without this */

	PicsUser *pPU = GetUserObject();

	UserRatingSystem **ppLast = &pPU->m_pRatingSystems;

	while (*ppLast != NULL) {
		if (!stricmpf((*ppLast)->QueryPch(), pPRS->etstrRatingService.Get())) {
			UserRatingSystem *pCurrent = *ppLast;
			*ppLast = pCurrent->m_pNext;	/* remove from list */
			delete pCurrent;
			break;
		}
		else
			ppLast = &((*ppLast)->m_pNext);
	}
}


void CheckUserCategory(UserRatingSystem *pURS, PicsCategory *pPC)
{
	for (UserRating *pRating = pURS->m_pRatingList;
		 pRating != NULL;
		 pRating = pRating->m_pNext)
	{
		if (!::strcmpf(pRating->QueryPch(), pPC->etstrTransmitAs.Get()))
			break;
	}

	if (pRating == NULL) {
		/* User setting not found for this category.  Add one. */

		pRating = new UserRating;
		if (pRating != NULL) {
			pRating->SetName(pPC->etstrTransmitAs.Get());
			pRating->m_pPC = pPC;
			pRating->m_pNext = pURS->m_pRatingList;
			pURS->m_pRatingList = pRating;
			if ((pPC->etfLabelled.fIsInit() && pPC->etfLabelled.Get()) ||
				!pPC->etnMin.fIsInit())
				pRating->m_nValue = 0;
			else
				pRating->m_nValue = pPC->etnMin.Get();
		}
	}

	/* Check all subcategories in this category as well.
	 */
	UINT cCategories = pPC->arrpPC.Length();
	for (UINT i=0; i<cCategories; i++)
		CheckUserCategory(pURS, pPC->arrpPC[i]);
}


void CheckUserSettings(PicsRatingSystem *pPRS)
{
	if (pPRS == NULL || !(pPRS->dwFlags & PRS_ISVALID) ||
		!pPRS->etstrRatingService.fIsInit())
		return;

	PicsUser *pPU = GetUserObject();

	UserRatingSystem **ppLast = &pPU->m_pRatingSystems;

	while (*ppLast != NULL) {
		if (!stricmpf((*ppLast)->QueryPch(), pPRS->etstrRatingService.Get())) {
			break;
		}
		ppLast = &((*ppLast)->m_pNext);
	}

	if (*ppLast == NULL) {
		*ppLast = new UserRatingSystem;
		if (*ppLast == NULL)
			return;
		(*ppLast)->SetName(pPRS->etstrRatingService.Get());
	}

	UserRatingSystem *pCurrent = *ppLast;

	pCurrent->m_pPRS = pPRS;

	/* First go through all the settings for the user and make sure the
	 * categories are valid.  If not, delete them.
	 */
	UserRating **ppRating = &pCurrent->m_pRatingList;
	while (*ppRating != NULL) {
		UserRating *pRating = *ppRating;
		pRating->m_pPC = FindInstalledCategory(pPRS->arrpPC, pRating->QueryPch());
		if (pRating->m_pPC == NULL) {
			*ppRating = pRating->m_pNext;		/* remove from list */
			delete pRating;
		}
		else
			ppRating = &pRating->m_pNext;
	}

	/* Now go through all the categories in the rating system and make
	 * sure the user has settings for them.  If any are missing, add
	 * settings for the default values (minimums).
	 */
	UINT cCategories = pPRS->arrpPC.Length();
	for (UINT i=0; i<cCategories; i++)
		CheckUserCategory(pCurrent, pPRS->arrpPC[i]);
}


