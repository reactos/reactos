/*
 * @(#)PrfData.cxx 
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. * 
 */

#include "core.hxx"
#pragma hdrstop

#ifdef PRFDATA
#ifndef _WIN64

/********************************************************************
Module name: PrfData.cpp
Notices: Written 1998 by Jeffrey Richter
Description: C++ template classes for Performance Object data types.
********************************************************************/


#include <windows.h>
#include "prfdata.hxx"

/////////////////////////////////////////////////////////////////////


// Call this function to force a breakpoint in PerfMon.exe
#ifdef _DEBUG
void ForceDebugBreak() {
   __try { 
      DebugBreak(); 
   }
   __except(UnhandledExceptionFilter(GetExceptionInformation())) {
   }
}
#else
#define ForceDebugBreak()
#endif


/////////////////////////////////////////////////////////////////////


// Helper function that finds a set of bytes in a memory block
PBYTE FindMemory(PBYTE pbBuf, DWORD cbBuf, 
   PBYTE pbSearchBytes, DWORD cbSearchBytes) {

   for (DWORD n = 0; n < (cbBuf - cbSearchBytes); n++) {
      if (pbBuf[n] == pbSearchBytes[0]) {
         for (DWORD x = 1; x < cbSearchBytes; x++) {
            if (pbBuf[n + x] != pbSearchBytes[x]) 
               break; // Not a match
         }
         if (x == cbSearchBytes) return(&pbBuf[n]); // Match!
      }
   }
   return(NULL);  // Not found at all
}


/////////////////////////////////////////////////////////////////////


// Address of the ONE instance of this class (See Collect)
CPrfData* CPrfData::sm_pPrfData = NULL;


/////////////////////////////////////////////////////////////////////


// Constructor: initializes member variables 
CPrfData::CPrfData(LPCWSTR pszAppName, PPRFITM pPrfItms) :
   m_pszAppName(pszAppName), m_pPrfItms(pPrfItms), 
   m_nNumPrfItems(0),        m_nNumObjects(0),         
   m_pPrfData(NULL),         m_hfm(NULL),
   m_dwFirstCounter(0),      m_dwLastCounter(0),
   m_dwFirstHelp(1),         m_dwLastHelp(1)
//   , m_Optex(pszAppName, 4000) 
{

   Assert(sm_pPrfData == NULL);  // Only one instance can exist
   sm_pPrfData = this;
	// Sanity check performance data object/counter map
   VerifyPrfItemTable();
}


/////////////////////////////////////////////////////////////////////


   // Destructor
CPrfData::~CPrfData() {

   if (m_pPrfData != NULL) 
      UnmapViewOfFile(m_pPrfData);

   if (m_hfm != NULL) 
      CloseHandle(m_hfm);
}


/////////////////////////////////////////////////////////////////////


// Installs performance object/counter map info into registry
void CPrfData::InstallPrfData(LPCWSTR pszDllPathname) 
{
   DWORD dwLastCounter = 0, dwLastHelp = 1;

   // Read the last counter/help global values from the registry
   CRegSettings regPerfLib, regApp;
   if (regPerfLib.OpenSubkey(FALSE, HKEY_LOCAL_MACHINE, 
      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib") != 0)
      return;

   regPerfLib.GetDWORD(L"Last Counter", &dwLastCounter); 
   regPerfLib.GetDWORD(L"Last Help",    &dwLastHelp); 

   // Read the first/last counter/help app values from the registry
   WCHAR szSubkey[100];
   wsprintfW(szSubkey, 
      L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", 
      m_pszAppName);
   LONG rc = regApp.OpenSubkey(FALSE, HKEY_LOCAL_MACHINE, szSubkey);
   Assert(ERROR_SUCCESS == rc);

	// Advance to the next counter / help
   m_dwFirstCounter = m_dwLastCounter = dwLastCounter + 2;
   m_dwFirstHelp    = m_dwLastHelp    = dwLastHelp + 2;

   // Install our counters into the registry
   AppendRegStrings(regPerfLib, TRUE,  &m_dwLastCounter);
   AppendRegStrings(regPerfLib, FALSE, &m_dwLastHelp);

   // Tell the registry where the next set of counter can go
   regPerfLib.SetDWORD(L"Last Counter", m_dwLastCounter);

   regPerfLib.SetDWORD(L"Last Help", m_dwLastHelp); 

   // Save the installation results for our app
   regApp.SetString(L"Library",       pszDllPathname); 
   regApp.SetString(L"Open",          L"PrfData_Open"); 
   regApp.SetString(L"Close",         L"PrfData_Close"); 
   regApp.SetString(L"Collect",       L"PrfData_Collect"); 
   regApp.SetDWORD (L"First Counter", m_dwFirstCounter); 
   regApp.SetDWORD (L"First Help",    m_dwFirstHelp); 
   regApp.SetDWORD (L"Last Counter",  m_dwLastCounter); 
   regApp.SetDWORD (L"Last Help",     m_dwLastHelp); 

}


/////////////////////////////////////////////////////////////////////


// Takes this app's performance info out of the registry
void CPrfData::UninstallPrfData() {

   // Read the last counter/help global values from the registry
   CRegSettings regPerfLib, regApp;
   if (regPerfLib.OpenSubkey(FALSE, HKEY_LOCAL_MACHINE, 
      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Perflib") != 0)
      return;

   DWORD dwLastCounter = 0, dwLastHelp = 1;
   regPerfLib.GetDWORD(L"Last Counter", &dwLastCounter); 
   regPerfLib.GetDWORD(L"Last Help",    &dwLastHelp); 

   // Read the first/last counter/help app values from the registry
   WCHAR szSubkey[100];
   wsprintfW(szSubkey, 
      L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", 
      m_pszAppName);

   // See if MSXML key exists first.
   if (regApp.OpenSubkey(TRUE, HKEY_LOCAL_MACHINE, szSubkey) != 0)
       return ;
   regApp.CloseKey();

   // now open it read/write.
   if (regApp.OpenSubkey(FALSE, HKEY_LOCAL_MACHINE, szSubkey) != 0)
       return ;

   regApp.GetDWORD(L"First Counter", &m_dwFirstCounter); 
   regApp.GetDWORD(L"First Help",    &m_dwFirstHelp); 
   regApp.GetDWORD(L"Last Counter",  &m_dwLastCounter); 
   regApp.GetDWORD(L"Last Help",     &m_dwLastHelp); 

   // Our counters are in the registry, do sanity checks
   Assert((DWORD) m_nNumPrfItems == 
      (m_dwLastCounter - m_dwFirstCounter) / 2 + 1);
   Assert((DWORD) m_nNumPrfItems == 
      (m_dwLastHelp - m_dwFirstHelp) / 2 + 1);
   Assert((m_dwFirstCounter <= m_dwLastCounter) && 
      (m_dwFirstHelp <= m_dwLastHelp));
   Assert((m_dwLastCounter <= dwLastCounter) && 
      (m_dwLastHelp <= dwLastHelp));

   // Remove the strings from the registry
   RemoveRegStrings(regPerfLib, TRUE,  
      m_dwFirstCounter, m_dwLastCounter);
   RemoveRegStrings(regPerfLib, FALSE, 
      m_dwFirstHelp, m_dwLastHelp);

   // If these counters are the last in, truncate the end
   // otherwise we leave a gap in the performance counter numbers
   if (m_dwLastCounter == dwLastCounter) {
      dwLastCounter -= (int) m_nNumPrfItems * 2;
      dwLastHelp    -= (int) m_nNumPrfItems * 2;
      regPerfLib.SetDWORD(L"Last Counter", dwLastCounter); 
      regPerfLib.SetDWORD(L"Last Help", dwLastHelp); 
   }

   // Delete the app's registry key
   regApp.CloseKey(); 
   ::RegDeleteKeyW(HKEY_LOCAL_MACHINE, szSubkey);
}


/////////////////////////////////////////////////////////////////////


// Appends our performance object/counter text/help in the registry
void CPrfData::AppendRegStrings(CRegSettings& regPerfLib, 
   BOOL fCounter, PDWORD pdwIndex) {

   // Calculate the number of required for the stuff we want to add
   DWORD cbInc = 0;
   for (PIINDEX pii = 0; m_pPrfItms[pii].pit != PIT_END; pii++) {
      cbInc += (6 + 1) * sizeof(WCHAR); // 6 digit index plus 0-char
      cbInc += (lstrlenW(fCounter ? m_pPrfItms[pii].pszName 
         : m_pPrfItms[pii].pszHelp) + 1) * sizeof(WCHAR);
   }

   // Allocate buffer big enough for original + new data & read it in
   CRegSettings regLang;
   if (regLang.OpenSubkey(FALSE, regPerfLib, L"009") != 0)
       return;

   LPCWSTR pszValue = fCounter ? L"Counter" : L"Help";
   DWORD cbOrig = 0;
   regLang.GetSize(pszValue, &cbOrig);
   LPWSTR psz = (LPWSTR) 
      HeapAlloc(GetProcessHeap(), 0, cbOrig + cbInc);
   regLang.GetMultiString(pszValue, psz, cbOrig); 

   // Append our new stuff to the end of the buffer
   // Subtract 1 for extra terminating 0 char
   LPWSTR pszInc = (LPWSTR) ((PBYTE) psz + cbOrig) - 1;  

   // Append our strings
   for (pii = 0; m_pPrfItms[pii].pit != PIT_END; pii++) {
      LPCTSTR psz = fCounter ? m_pPrfItms[pii].pszName 
         : m_pPrfItms[pii].pszHelp;
      lstrcpyW(pszInc += 
         wsprintfW(pszInc, L"%d", *pdwIndex) + 1, psz);
      pszInc += lstrlenW(psz) + 1;
      *pdwIndex += 2;   // Always increment the number by 2
   }
   *pdwIndex -= 2;

   *pszInc++ = 0;   // Append extra terminating 0-char
   regLang.SetMultiString(pszValue, psz); 
   HeapFree(GetProcessHeap(), 0, psz);
}


/////////////////////////////////////////////////////////////////////


// Remove the object/counter strings from the registry
void CPrfData::RemoveRegStrings(CRegSettings& regPerfLib, 
   BOOL fCounter, DWORD dwIndexLo, DWORD dwIndexHi) {

   // Allocate buffer big enough for original data & read it in
   CRegSettings regLang;
   if (regLang.OpenSubkey(FALSE, regPerfLib, L"009") != 0)
       return;

   LPCWSTR pszValue = fCounter ? L"Counter" : L"Help";
   DWORD cbOrig = 0;
   regLang.GetSize(pszValue, &cbOrig);
   LPWSTR psz = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, cbOrig);
   regLang.GetMultiString(pszValue, psz, cbOrig); 

   // Find the bounds of what we want to remove
   WCHAR szNum[10] = { 0 };   // Look for \0#\0
   wsprintfW(&szNum[1], L"%d", dwIndexLo);
   PBYTE pbLo = FindMemory((PBYTE) psz, cbOrig, 
      (PBYTE) szNum, sizeof(WCHAR) * (lstrlenW(&szNum[1]) + 2));
   pbLo += sizeof(WCHAR);  // 1st byte of stuff to remove
   wsprintfW(&szNum[1], L"%d", dwIndexHi);
   PBYTE pbHi = FindMemory((PBYTE) psz, cbOrig, 
      (PBYTE) szNum, sizeof(WCHAR) * (lstrlenW(&szNum[1]) + 2));
   pbHi += sizeof(WCHAR);  // 1st byte of last counter to remove

   // Skip over number and text
   pbHi += (lstrlenW((LPCWSTR) pbHi) + 1) * sizeof(WCHAR); 
   pbHi += (lstrlenW((LPCWSTR) pbHi) + 1) * sizeof(WCHAR);

   // Shift the strings to keep down over the stuff to delete
   Assert(pbLo <= pbHi);
   MoveMemory(pbLo, pbHi, ((PBYTE) psz + cbOrig) - pbHi);

   // Save the updated string information
   regLang.SetMultiString(pszValue, psz); 
   HeapFree(GetProcessHeap(), 0, psz);
}


/////////////////////////////////////////////////////////////////////


// Creates the shared memory region for the performance information
DWORD CPrfData::ActivatePrfData() {

   if (m_pPrfData != NULL)    // This only needs to be done once
       return 0;

   // Read the first/last object/counter text/help values
   WCHAR szSubkey[100];
   wsprintfW(szSubkey, 
      L"SYSTEM\\CurrentControlSet\\Services\\%s\\Performance", 
      m_pszAppName);

   CRegSettings regApp;
   DWORD rc = regApp.OpenSubkey(TRUE, HKEY_LOCAL_MACHINE, szSubkey);
   if (rc != 0) return rc;

   regApp.GetDWORD(L"First Counter", &m_dwFirstCounter); 
   regApp.GetDWORD(L"First Help",    &m_dwFirstHelp); 
   regApp.GetDWORD(L"Last Counter",  &m_dwLastCounter); 
   regApp.GetDWORD(L"Last Help",     &m_dwLastHelp); 

   // Do sanity checks
   Assert((DWORD) m_nNumPrfItems == 
      (m_dwLastCounter - m_dwFirstCounter) / 2 + 1);
   Assert((DWORD) m_nNumPrfItems == 
      (m_dwLastHelp - m_dwFirstHelp) / 2 + 1);


   // Calculate how many bytes are needed for the shared memory
   DWORD cbBytesNeededForAllObjs = 0;
   for (OBJORD ObjOrd = 0; ObjOrd < (OBJORD) m_nNumObjects; ObjOrd++) {
      PRFMETRICS pm;
      PPRFITM pPrfObj;
      CalcPrfMetrics(ObjOrd, 0, &pm, &pPrfObj);
      // No instances                   Instances
      // ---------------------------    ---------------------------
      // 1 PERF_OBJECT_TABLE            1 PERF_OBJECT_TABLE
      // 1 PERF_COUNTER_DEFINITION      1 PERF_COUNTER_DEFINITION 
      // 0 PERF_INSTANCE_DEFINITION     x PERF_INSTANCE_DEFINITIONs
      // 1 PERF_COUNTER_BLOCK           x PERF_COUNTER_BLOCKs
      // 0 instance names               x instance names
      pPrfObj->cbOffsetToNextObj = pm.cbPOT + pm.cbPCD + 
         pm.cbPID * (pm.fSupportsInstances ? pm.MaxInstances : 0) + 
         pm.cbPCB * (pm.fSupportsInstances ? pm.MaxInstances : 1) + 
         pm.cbPIN * (pm.fSupportsInstances ? pm.MaxInstances : 0);
      cbBytesNeededForAllObjs += pPrfObj->cbOffsetToNextObj;
   }

   // Attempt to allocate a MMF big enough for the data
   m_hfm = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
      0, cbBytesNeededForAllObjs, m_pszAppName);
   // If dwErr = ERROR_ALREADY_EXISTS, another app has created the 
   // shared data area. This instance doesn't need to initialize it
   DWORD dwErr = GetLastError();
   if (m_hfm == NULL) return(dwErr);

   m_pPrfData = (PBYTE) MapViewOfFile(m_hfm, FILE_MAP_WRITE, 0, 0, 0);
   if (dwErr == ERROR_ALREADY_EXISTS) return(dwErr);

   // This instance actually allocated the shared data, initialize it
   DWORD dwCounter = m_dwFirstCounter, dwHelp = m_dwFirstHelp;

   // Set the PERF_OBJECT_TYPEs for each object
   for (ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
      PRFMETRICS pm;
      PPRFITM pPrfObj;
      CalcPrfMetrics(ObjOrd, 0, &pm, &pPrfObj);
      // Set the PERF_OBJECT_TYPE members
      pm.pPOT->TotalByteLength      = 0;   // Set in Collect function
      pm.pPOT->DefinitionLength     = sizeof(PERF_OBJECT_TYPE) + 
         sizeof(PERF_COUNTER_DEFINITION) * 
         (INT)PtrToInt(pPrfObj->NumCounters); 
      pm.pPOT->HeaderLength         = sizeof(PERF_OBJECT_TYPE); 
      pm.pPOT->ObjectNameTitleIndex = dwCounter;
      pm.pPOT->ObjectNameTitle      = NULL; // Set by PerfMon
      pm.pPOT->ObjectHelpTitleIndex = dwHelp;
      pm.pPOT->ObjectHelpTitle      = NULL; // Set by PerfMon
      pm.pPOT->DetailLevel          = pPrfObj->dwDetailLevel; 
      pm.pPOT->NumCounters          = (DWORD)PtrToUint(pPrfObj->NumCounters);

      if (pPrfObj->DefCounter == (CTRID) -1) {
         pm.pPOT->DefaultCounter = -1;
      } else {
         // If a default CTRID specified, convert it to index
         CvtCtrIdToPrfItmIndex(pPrfObj->DefCounter, 
            (int*) &pm.pPOT->DefaultCounter);
      }

      pm.pPOT->NumInstances         = (int)PtrToInt(pPrfObj->MaxInstances);
      pm.pPOT->CodePage             = NULL; 
      pm.pPOT->PerfTime.QuadPart    = 0; 
      pm.pPOT->PerfFreq.QuadPart    = 0; 

      dwCounter += 2;
      dwHelp += 2;

      // Set the PERF_COUNTER_DEFINITIONs for each counter
      for (CTRORD CtrOrd = 0; CtrOrd < pPrfObj->NumCounters; CtrOrd++) {
         PPRFITM pPrfCtr = CvtCtrOrdToPrfItm(ObjOrd, CtrOrd);
         PPERF_COUNTER_DEFINITION pPCD = &pm.pPCD[(int) PtrToInt(CtrOrd)];
         pPCD->ByteLength            = sizeof(PERF_COUNTER_DEFINITION); 
         pPCD->CounterNameTitleIndex = dwCounter;
         pPCD->CounterNameTitle      = NULL; 
         pPCD->CounterHelpTitleIndex = dwHelp;
         pPCD->CounterHelpTitle      = NULL;
         pPCD->DefaultScale          = pPrfCtr->dwDefaultScale;
         pPCD->DetailLevel           = pPrfCtr->dwDetailLevel;
         pPCD->CounterType           = pPrfCtr->dwCounterType;
         pPCD->CounterSize           = kMaxCounterSize; 
         pPCD->CounterOffset         = sizeof(PERF_COUNTER_BLOCK) + 
            kMaxCounterSize * (int) PtrToInt(CtrOrd); 
         dwCounter += 2;
         dwHelp += 2;
      }

      // Set the PERF_COUNTER_BLOCKs for the 1 (or each) instance
      if (pPrfObj->MaxInstances == (INSTID) PERF_NO_INSTANCES) {
         pm.pPCB->ByteLength = pm.cbPCB;
      } else {
         for (INSTID InstId = 0; InstId < pPrfObj->MaxInstances; InstId++) {
            CalcPrfMetrics(ObjOrd, InstId, &pm);
            pm.pPCB->ByteLength = pm.cbPCB;
         }
      }
   }
   return(dwErr);
}


/////////////////////////////////////////////////////////////////////


// Finds an empty instance entry
CPrfData::INSTID CPrfData::FindFreeInstance(OBJORD ObjOrd) const {
   // Sanity check for valid object and that it support instances
   Assert(IsValidObjOrd(ObjOrd));
   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, 0, &pm);
   Assert(pm.fSupportsInstances);

   LockCtrs();
   // Find an unused instance entry
   INSTID InstId = 0;
   for (; InstId < (INSTID) pm.MaxInstances; InstId++) {
      CalcPrfMetrics(ObjOrd, InstId, &pm);
      if (pm.pPIN[0] == 0) break;
   }
   UnlockCtrs();

   // -1 means all instances are in use
   return((InstId == (INSTID) pm.MaxInstances) 
      ? (INSTID) -1 : InstId);
}


/////////////////////////////////////////////////////////////////////


// Adds an instance to an object
CPrfData::INSTID CPrfData::AddInstance(OBJID ObjId, 
   LPCWSTR pszInstName, LONG lUniqueID, OBJID ObjIdParent, 
   INSTID InstIdParent) {

   // Make sure that instance has a valid identity
   Assert(((pszInstName == NULL) && 
      (lUniqueID != PERF_NO_UNIQUE_ID)) || ((pszInstName != NULL) &&
      (lUniqueID == PERF_NO_UNIQUE_ID)));

   // Make sure the object is valid and supports instances
   OBJORD ObjOrd = CvtObjIdToObjOrd(ObjId);
   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, 0, &pm);
   Assert(pm.fSupportsInstances);

   // Find a place to put this instance
   INSTID InstId = FindFreeInstance(ObjOrd);
   if (InstId != (INSTID) -1) {
      CalcPrfMetrics(ObjOrd, InstId, &pm);
      // Store the Parent Object's ID/Instance ID and Unique ID.
      // The Collect function converts IDs to the appropriate values.
      pm.pPID->ParentObjectTitleIndex = (DWORD) PtrToUint(ObjIdParent);
      pm.pPID->ParentObjectInstance = (DWORD) PtrToUint(InstIdParent);
      pm.pPID->UniqueID = lUniqueID; 

      if (pszInstName == NULL) { // Instance has a string name
         pm.pPIN[0] = 1;                  // Mark instance as in use
         pm.pPID->NameOffset = 0;
         pm.pPID->NameLength = 0;
      } else {                   // Instance has no string name
         Assert(lstrlenW(pszInstName) < 
            (int) CvtObjOrdToPrfItm(ObjOrd)->cchMaxInstName);
         lstrcpyW(pm.pPIN, pszInstName);  // Mark instance as in use
         pm.pPID->NameOffset = pm.cbPID;
         pm.pPID->NameLength = 
            (lstrlenW(pszInstName) + 1) * sizeof(WCHAR);
      }
   }
   return(InstId);
}


/////////////////////////////////////////////////////////////////////


// Removes an instance from an object
void CPrfData::RemoveInstance(OBJID ObjId, INSTID InstId) {
   PRFMETRICS pm;
   CalcPrfMetrics(CvtObjIdToObjOrd(ObjId), InstId, &pm);
   Assert(pm.fSupportsInstances);
   Assert(pm.pPIN[0] != 0); // Can't remove an unassigned instance
   LockCtrs();
   pm.pPIN[0] = 0;   // Mark instance as NOT in use
   UnlockCtrs();
}


/////////////////////////////////////////////////////////////////////

 
// Sanity checks the performance object/counter map  
void CPrfData::VerifyPrfItemTable() {
   PIINDEX piiCrntObj = (PIINDEX) -1;  // Object being processed

   // Loop through all entries in the object/counter map
   for (PIINDEX pii = 0; m_pPrfItms[pii].pit != PIT_END; pii++) {
      Assert(m_pPrfItms[pii].dwId != 0); // 0 is an invalid ID
      m_nNumPrfItems++;

      switch (m_pPrfItms[pii].pit) {
      case PIT_END:
         // Make sure the current object is in a good state
         // i.e., object has at least one counter in it
         Assert(!((piiCrntObj != -1) && 
            (m_pPrfItms[piiCrntObj].NumCounters < (CTRORD) 1)));
         break;

      case PIT_OBJECT:     // We found a new object
         // Every Object ID in the table must be unique
         Assert(CvtObjIdToPrfItmIndex((OBJID) m_pPrfItms[pii].dwId) == pii); 

         // Sanity check its parameters
         Assert((m_pPrfItms[pii].DefCounter == (CTRID) -1) ||
            (CvtCtrIdToPrfItmIndex(m_pPrfItms[pii].DefCounter) != -1));
         Assert((m_pPrfItms[pii].MaxInstances == 
            (INSTID) PERF_NO_INSTANCES) || 
            (m_pPrfItms[pii].MaxInstances > 0));
         Assert(
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_NOVICE)   || 
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_ADVANCED) || 
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_EXPERT)   ||
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_WIZARD));

         m_nNumObjects++;

         // Finish up the object that we were just working on
         if (piiCrntObj != -1) {
            // Make sure the current object is in a good state
            // i.e., object has at least one counter in it
            Assert(m_pPrfItms[piiCrntObj].NumCounters > 0);

            // The previous object must point to the current object
            m_pPrfItms[piiCrntObj].IndexNextObj = pii; 
         }

         piiCrntObj = pii; // Save new current object
         m_pPrfItms[piiCrntObj].NumCounters = 0;

         // Assume that this is the last object in the list
         m_pPrfItms[piiCrntObj].IndexNextObj = -1; 
         break;

      case PIT_COUNTER:
         Assert(pii != 0); // First entry in map must be PIT_OBJECT

         // Every Counter ID in the table must be unique
         Assert(CvtCtrIdToPrfItmIndex(
            (CTRID) m_pPrfItms[pii].dwId) == pii); 

         // Sanity check its parameters
         Assert(
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_NOVICE)   || 
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_ADVANCED) || 
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_EXPERT)   ||
            (m_pPrfItms[pii].dwDetailLevel == PERF_DETAIL_WIZARD));

         m_pPrfItms[piiCrntObj].NumCounters++;
         break;
      }
   }
}


/////////////////////////////////////////////////////////////////////


// Calculates an object's performance info (addresses and sizes)
void CPrfData::CalcPrfMetrics(OBJORD ObjOrd, INSTID InstId, 
   PPRFMETRICS pPM, PPRFITM* ppPrfItm) const {

   Assert(IsValidInstId(ObjOrd, InstId));

   PPRFITM pPrfItm = CvtObjOrdToPrfItm(ObjOrd);
   if (ppPrfItm != NULL) *ppPrfItm = pPrfItm;

   ZeroMemory(pPM, sizeof(*pPM));
   pPM->fSupportsInstances = 
      (pPrfItm->MaxInstances != (INSTID) PERF_NO_INSTANCES);
   pPM->MaxInstances = (long) PtrToInt(pPrfItm->MaxInstances);

   // Find the start of this object's performance data
   DWORD cb = 0;
   PIINDEX pii = 0;
   for (; 0 != ObjOrd--; pii = m_pPrfItms[pii].IndexNextObj) 
      cb += m_pPrfItms[pii].cbOffsetToNextObj;

   // This object's PERF_OBJECT_TYPE structure & size
   pPM->pPOT = (PPERF_OBJECT_TYPE) (m_pPrfData + cb);
   pPM->cbPOT = sizeof(PERF_OBJECT_TYPE);

   // PERF_COUNTER_DEFINITIONs follow PERF_OBJECT_TYPE
   pPM->pPCD = (PPERF_COUNTER_DEFINITION) (&pPM->pPOT[1]);
   pPM->cbPCD = (int) PtrToInt(pPrfItm->NumCounters) * 
      sizeof(PERF_COUNTER_DEFINITION);

   if (pPrfItm->MaxInstances != (INSTID) PERF_NO_INSTANCES) {
      long MaxInstances = (long) PtrToUint(pPrfItm->MaxInstances);
      PBYTE pbEndOfCtrDefs = (PBYTE) pPM->pPCD + pPM->cbPCD;

      // PERF_INSTANCE_DEFINITIONs follow PERF_COUNTER_DEFINITIONs
      pPM->pPID = (PPERF_INSTANCE_DEFINITION) pbEndOfCtrDefs;
      pPM->pPID = &pPM->pPID[(int) PtrToInt(InstId)]; 
      pPM->cbPID = sizeof(PERF_INSTANCE_DEFINITION);

      // PERF_COUNTER_BLOCKs follow PERF_INSTANCE_DEFINITIONs
      pPM->pPCB = (PPERF_COUNTER_BLOCK) (pbEndOfCtrDefs + 
         sizeof(PERF_INSTANCE_DEFINITION) * MaxInstances);
      pPM->cbPCB = sizeof(PERF_COUNTER_BLOCK) + 
         (long) PtrToInt(pPrfItm->NumCounters) * kMaxCounterSize;
      pPM->pPCB = (PPERF_COUNTER_BLOCK) ((PBYTE) pPM->pPCB + 
         (int) PtrToInt(InstId) * pPM->cbPCB);

      // Instance names follow PERF_COUNTER_BLOCKs
      pPM->pPIN = (LPWSTR) (pbEndOfCtrDefs + 
         (sizeof(PERF_INSTANCE_DEFINITION) + pPM->cbPCB) * 
         MaxInstances);
      pPM->cbPIN = pPrfItm->cchMaxInstName * sizeof(WCHAR);
      pPM->pPIN += pPrfItm->cchMaxInstName * (int) PtrToInt(InstId);

   } else {
      // PERF_COUNTER_BLOCK follows PERF_COUNTER_DEFINITION
      pPM->pPCB = (PPERF_COUNTER_BLOCK) ((PBYTE) pPM->pPCD + pPM->cbPCD);
      pPM->cbPCB = sizeof(PERF_COUNTER_BLOCK) + (int) PtrToInt(pPrfItm->NumCounters) * kMaxCounterSize;
   }
}


/////////////////////////////////////////////////////////////////////


// Returns the address of a counter in the shared data block
PBYTE CPrfData::GetCtr(CTRID CtrId, INSTID InstId) const {
   int nCtrNum;
   OBJORD ObjOrd = CvtCtrIdToObjOrd(CtrId, &nCtrNum);
   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, InstId, &pm);
   // NOTE: for convenience, all counters are 64-bit values
   return((PBYTE) pm.pPCB + sizeof(PERF_COUNTER_BLOCK) + 
      nCtrNum * kMaxCounterSize);
}


/////////////////////////////////////////////////////////////////////


// Converts an object/counter ID to its map index
CPrfData::PIINDEX CPrfData::CvtIdToPrfItmIndex(
   BOOL fObjectId, DWORD dwId) const {

   for (PIINDEX pii = 0; m_pPrfItms[pii].pit != PIT_END; pii++) {

      if (fObjectId && (m_pPrfItms[pii].pit == PIT_OBJECT) && 
          (m_pPrfItms[pii].dwId == dwId)) 
         return(pii);

      if (!fObjectId && (m_pPrfItms[pii].pit == PIT_COUNTER) && 
          (m_pPrfItms[pii].dwId == dwId)) 
         return(pii);
   }
   return(-1); // Not found
}


/////////////////////////////////////////////////////////////////////


// Converts an object ID to its map ordinal
CPrfData::OBJORD CPrfData::CvtObjIdToObjOrd(OBJID ObjId) const {
   for (OBJORD ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
      if (CvtObjOrdToPrfItm(ObjOrd)->dwId == (DWORD) PtrToInt(ObjId))
         return(ObjOrd);
   }
   Assert("Object ID Not in list");
   return((OBJORD) -1);
}


/////////////////////////////////////////////////////////////////////


// Converts an objeict ordinal to its map index
CPrfData::PIINDEX CPrfData::CvtObjOrdToPrfItmIndex(
   OBJORD ObjOrd) const {

   Assert(IsValidObjOrd(ObjOrd));
   PIINDEX pii = 0;
   for (; 0 != ObjOrd--; pii = m_pPrfItms[pii].IndexNextObj) ;   
   return(pii);
}


/////////////////////////////////////////////////////////////////////


// Converts an object's ordinal to the address of its map info
CPrfData::PPRFITM CPrfData::CvtObjOrdToPrfItm(OBJORD ObjOrd) const {
   return(&m_pPrfItms[CvtObjOrdToPrfItmIndex(ObjOrd)]);
}


/////////////////////////////////////////////////////////////////////


// Converts a counter's ID to its map index
CPrfData::PIINDEX CPrfData::CvtCtrIdToPrfItmIndex(
   CTRID CtrId, int* pnCtrIndexInObj) const {

   int nCtrIndexInObj = 0;
   PIINDEX pii = CvtIdToPrfItmIndex(FALSE, (DWORD) PtrToInt(CtrId));
   if (pii != -1) {
      PIINDEX piiT = pii;
      while (m_pPrfItms[--piiT].pit != PIT_OBJECT) 
         nCtrIndexInObj++;
   }
   if (pnCtrIndexInObj != NULL) 
      *pnCtrIndexInObj = nCtrIndexInObj;
   return(pii); // -1 if not found
}


/////////////////////////////////////////////////////////////////////


// Converts a counter's ordinal to the address of its map info
CPrfData::PPRFITM CPrfData::CvtCtrOrdToPrfItm(
   OBJORD ObjOrd, CTRORD CtrOrd) const {

   PPRFITM pPrfItm = CvtObjOrdToPrfItm(ObjOrd);
   return(&pPrfItm[(int) PtrToInt(CtrOrd) + 1]);
}


/////////////////////////////////////////////////////////////////////


// Converts a counter's ID its owning object's ordinal
CPrfData::OBJORD  CPrfData::CvtCtrIdToObjOrd(
   CTRID CtrId, int* pnCtrIndexInObj) const {

   PIINDEX pii = CvtCtrIdToPrfItmIndex(CtrId, pnCtrIndexInObj);
   OBJORD ObjOrd = 0;
   while (--pii > 0) {
      if (m_pPrfItms[pii].pit == PIT_OBJECT) ObjOrd++;
   }
   return(ObjOrd);
}


/////////////////////////////////////////////////////////////////////


// The following functions are only necessary in the collection DLL
// They are not needed by an app that simply updates the counters

extern "C" {
DWORD __declspec(dllexport) WINAPI PrfData_Open(LPWSTR lpDevNames) {
   // Create/open the shared memory containing the performance info
   CPrfData::Activate();
   return(ERROR_SUCCESS);
}


DWORD __declspec(dllexport) WINAPI PrfData_Close(void) {
   // Nothing to do here
   return(ERROR_SUCCESS);
}


DWORD __declspec(dllexport) WINAPI PrfData_Collect(LPWSTR lpValueName, 
   LPVOID* lppData, LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes) {

   // Call the class's static Collect function to populate the passed
   // memory block with our performance info
   return(CPrfData::Collect(lpValueName, (PBYTE*) lppData, 
      lpcbTotalBytes, lpNumObjectTypes));
}
}  // extern "C"

// Export these 3 functions as the interface to our Performance Data
#pragma comment(linker, "/export:PrfData_Open=_PrfData_Open@4")
#pragma comment(linker, "/export:PrfData_Close=_PrfData_Close@0")
#pragma comment(linker, "/export:PrfData_Collect=_PrfData_Collect@16")


/////////////////////////////////////////////////////////////////////

// This is a help class used to return only the requested objects
class CWhichCtrs {
public:
   CWhichCtrs(LPCWSTR pszObjNums = NULL);
   ~CWhichCtrs();
   BOOL IsNumInList(int nNum);

private:
   LPWSTR m_pszObjNums;
};


CWhichCtrs::CWhichCtrs(LPCWSTR pszObjNums) {
   // Save the list of requested object numbers
   if ((lstrcmpiW(L"Global", pszObjNums) == 0)) {
      m_pszObjNums = NULL;
   } else {
      m_pszObjNums = (LPWSTR) HeapAlloc(GetProcessHeap(), 0, 
         (lstrlenW(pszObjNums) + 3) * sizeof(WCHAR));
      // Put spaces around all the numbers
      wsprintfW(m_pszObjNums, L" %s ", pszObjNums);
   }
}


CWhichCtrs::~CWhichCtrs() {
   if (m_pszObjNums != NULL) 
      HeapFree(GetProcessHeap(), 0, m_pszObjNums);
}


BOOL CWhichCtrs::IsNumInList(int nNum) {
   BOOL fIsNumInList = TRUE;
   if (m_pszObjNums != NULL) {
      // Put spaces around this number and see if it's in the list
      WCHAR szNum[10];
      wsprintfW(szNum, L" %d ", nNum);
      fIsNumInList = (wcsstr(m_pszObjNums, szNum) != NULL);
   }
   return(fIsNumInList);
}

/////////////////////////////////////////////////////////////////////


// Determine which parent objects need to be collected
void CPrfData::DetermineObjsToCollect(OBJORD ObjOrd) const {
   PPRFITM pPrfItm = CvtObjOrdToPrfItm(ObjOrd);

   // Assume this object is being collected
   Assert(pPrfItm->fCollectThisObj);
   
   if (pPrfItm->MaxInstances != (INSTID) PERF_NO_INSTANCES) {
      // If this counter supports instances, collect the 
      // counters that any instances refer to (recursively). 
      INSTID InstId = 0;
      for (; InstId < (INSTID) pPrfItm->MaxInstances; InstId++) {
         PRFMETRICS pm;
         CalcPrfMetrics(ObjOrd, InstId, &pm);
         if ((pm.pPIN[0] != 0) && 
             (pm.pPID->ParentObjectTitleIndex != 0)) {
            // Instance is in use and refers to a parent object
            // Collect the parent object
            PPRFITM pPrfItmParent = CvtObjIdToPrfItm((OBJID) 
               pm.pPID->ParentObjectTitleIndex);
            if (pPrfItmParent->fCollectThisObj == FALSE) {
               pPrfItmParent->fCollectThisObj = TRUE;
               DetermineObjsToCollect(CvtObjIdToObjOrd(
                  (OBJID) pm.pPID->ParentObjectTitleIndex));
            }
         }
      }
   }
}


/////////////////////////////////////////////////////////////////////

DWORD CPrfData::Collect(LPWSTR lpValueName, PBYTE* ppbData, 
   LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes) {

   DWORD dwErr = ERROR_SUCCESS;  // Assume success
   PBYTE ppbOriginalStartOfBuffer = *ppbData;

   // Wrap everything inside an SEH frame so that we NEVER bring
   // down an app that is trying to collect our performance info
   __try {
      // While we do all this work, lock out other threads so that
      // our data structures do not become corrupted.
      sm_pPrfData->LockCtrs();

      dwErr = sm_pPrfData->CollectAllObjs(lpValueName, ppbData, 
         lpcbTotalBytes, lpNumObjectTypes);
   }
   __except (EXCEPTION_EXECUTE_HANDLER) {
      *ppbData = ppbOriginalStartOfBuffer;
      *lpcbTotalBytes = 0;
      *lpNumObjectTypes = 0;
   }
   sm_pPrfData->UnlockCtrs();
   return(dwErr);
}


/////////////////////////////////////////////////////////////////////

// Collects all of the requested objects
DWORD CPrfData::CollectAllObjs(LPWSTR lpValueName, PBYTE *ppbData, 
   LPDWORD lpcbTotalBytes, LPDWORD lpNumObjectTypes) const {

   // lpValueName:      [in]  Set of object numbers requested
   // lppData:          [in]  Buffer where object info goes
   //                   [out] Address after our data
   // lpcbTotalBytes:   [in]  Size of buffer
   //                   [out] Bytes we put in buffer
   // lpNumObjectTypes: [in]  Ignore 
   //                   [out] Number of objects we put in the buffer
   // Return Value:     ERROR_MORE_DATA or ERROR_SUCCESS
   DWORD dwErr = ERROR_SUCCESS;
   PBYTE pbWhereOurDataGoes = *ppbData;
   CWhichCtrs CtrList(lpValueName);

   *lpNumObjectTypes = 0;

   // Default to collecting none of our objects
   for (OBJORD ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
      CvtObjOrdToPrfItm(ObjOrd)->fCollectThisObj = FALSE;
   }

   // Collect only the objects explicitly specified
   // and any instances' parent objects
   for (ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
      // Should this object's counters be returned?
      if (CtrList.IsNumInList(
         CvtObjOrdToPrfItmIndex(ObjOrd) * 2 + m_dwFirstCounter)) {
         CvtObjOrdToPrfItm(ObjOrd)->fCollectThisObj = TRUE;
         DetermineObjsToCollect(ObjOrd);
      }
   }

   // Calculcate the bytes required for the desired objects
   DWORD cbBytesForAllObjs = 0;
   for (ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
      if (CvtObjOrdToPrfItm(ObjOrd)->fCollectThisObj)
         cbBytesForAllObjs += CalcBytesForPrfObj(ObjOrd);
   }

   if (*lpcbTotalBytes < cbBytesForAllObjs) {
      // If buffer too small for desired objects, return failure
      *lpcbTotalBytes = 0;
      dwErr = ERROR_MORE_DATA;
   } else {
      // Buffer is big enough, append objects' data to buffer
      *lpcbTotalBytes = 0;

      for (ObjOrd = 0; ObjOrd < m_nNumObjects; ObjOrd++) {
         if (CvtObjOrdToPrfItm(ObjOrd)->fCollectThisObj) {
            CollectAnObj(ObjOrd, ppbData);
            *lpcbTotalBytes += CalcBytesForPrfObj(ObjOrd);
            (*lpNumObjectTypes)++;
         }
      }
   }
   return(dwErr);
}

/////////////////////////////////////////////////////////////////////

DWORD CPrfData::CollectAnObj(OBJORD ObjOrd, PBYTE *ppbData) const {
   Assert(IsValidObjOrd(ObjOrd));

   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, 0, &pm);

   // Append PERF_OBJECT_TYPE
   CopyMemory(*ppbData, pm.pPOT, pm.cbPOT);
   PPERF_OBJECT_TYPE pPOT = (PPERF_OBJECT_TYPE) *ppbData;
   pPOT->TotalByteLength = CalcBytesForPrfObj(ObjOrd); 
   pPOT->NumInstances = (pm.MaxInstances == PERF_NO_INSTANCES) ? 
      PERF_NO_INSTANCES : HowManyInstancesInUse(ObjOrd);
   *ppbData += pm.cbPOT;
   
   // Append array of PERF_COUNTER_DEFINITIONs
   CopyMemory(*ppbData, pm.pPCD, pm.cbPCD);
   *ppbData += pm.cbPCD;

   if (!pm.fSupportsInstances) {
      // Append 1 PERF_COUNTER_BLOCK
      CopyMemory(*ppbData, pm.pPCB, pm.cbPCB);
      *ppbData += pm.cbPCB;
   } else {
      // Append PERF_INSTANCE_DEFINITION/PERF_COUNTER_BLOCKs
      INSTID InstId = 0;
      for (; InstId < (INSTID) pm.MaxInstances; InstId++) {
         CalcPrfMetrics(ObjOrd, InstId, &pm);

         if (pm.pPIN[0] != 0) {  // This instance is in use

            // Append PERF_INSTANCE_DEFINITIONs
            CopyMemory(*ppbData, pm.pPID, pm.cbPID);
            PPERF_INSTANCE_DEFINITION pPID = 
               (PPERF_INSTANCE_DEFINITION) *ppbData;
            *ppbData += pm.cbPID;

            pPID->ByteLength = sizeof(PERF_INSTANCE_DEFINITION);

            // The ParentObjectTitleIndex contains the parent 
            // object's ID. If this is not 0 (an invalid object ID),
            // convert the ID to the Performance Object number
            if (pPID->ParentObjectTitleIndex != 0) {
               PIINDEX pii = CvtObjIdToPrfItmIndex(
                  (OBJID) pm.pPID->ParentObjectTitleIndex);
               pPID->ParentObjectTitleIndex = 
                  m_dwFirstCounter + 2 * pii;

               // Convert Instance ID to In-Use Instance number.
               pPID->ParentObjectInstance = 
                  CvtInstIdToInUseInstId(CvtObjIdToObjOrd(
                     (OBJID) pm.pPID->ParentObjectTitleIndex), 
                     (INSTID) pm.pPID->ParentObjectInstance);
            }

            // Append instance name after PERF_INSTANCE_DEFINITION
            if (pPID->UniqueID == PERF_NO_UNIQUE_ID) {
               DWORD cchName = lstrlenW(pm.pPIN) + 1;
               DWORD cbName = cchName * sizeof(WCHAR);
               CopyMemory(*ppbData, pm.pPIN, cbName);

               // If an odd number of characters, add 2 bytes so
               // that next structure starts on 32-bit boundary 
               if ((cchName & 1) == 1) cbName += 2;  
               *ppbData += cbName;
               pPID->ByteLength += cbName;
            }

            // Append PERF_COUNTER_BLOCK to the buffer
            CopyMemory(*ppbData, pm.pPCB, pm.cbPCB);
            *ppbData += pm.cbPCB;
         }
      }
   }
   return(ERROR_SUCCESS);
}


/////////////////////////////////////////////////////////////////////

DWORD CPrfData::CalcBytesForPrfObj(OBJORD ObjOrd) const {
   Assert(IsValidObjOrd(ObjOrd));
   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, 0, &pm);
   DWORD cbBytesNeeded = pm.cbPOT + pm.cbPCD;

   if (!pm.fSupportsInstances) {
      cbBytesNeeded += pm.cbPCB;
   } else {
      cbBytesNeeded += (pm.cbPID + pm.cbPCB) * 
         HowManyInstancesInUse(ObjOrd);

      INSTID InstId = 0;
      for (; InstId < (INSTID) pm.MaxInstances; InstId++) {
         CalcPrfMetrics(ObjOrd, InstId, &pm);
         if (pm.pPIN[0] != 0) {
            DWORD cch = lstrlenW(pm.pPIN) + 1;  // For 0 character
            if ((cch & 1) == 1) cch++;  

            // If an odd number of characters, add 2 bytes so
            // that next structure starts on 32-bit boundary 
            cbBytesNeeded += sizeof(WCHAR) * cch;
         }
      }
   }
   return(cbBytesNeeded);
}


/////////////////////////////////////////////////////////////////////

int CPrfData::HowManyInstancesInUse(OBJORD ObjOrd) const {
   Assert(IsValidObjOrd(ObjOrd));
   PRFMETRICS pm;
   CalcPrfMetrics(ObjOrd, 0, &pm);
   Assert(pm.fSupportsInstances);

   int nNumInstancesInUse = 0;
   INSTID InstId = 0;
   for (; InstId < (INSTID) pm.MaxInstances; InstId++) {
      if (pm.pPIN[0] != 0) nNumInstancesInUse++;
      pm.pPIN += pm.cbPIN / sizeof(pm.pPIN[0]);
   }
   return(nNumInstancesInUse);
}


/////////////////////////////////////////////////////////////////////

int CPrfData::CvtInstIdToInUseInstId(
   OBJORD ObjOrd, INSTID InstId) const {

   Assert(IsValidInstId(ObjOrd, InstId));
   int nIndexInstInUse = 0;
   for (; InstId > 0; InstId--) {
      PRFMETRICS pm;
      CalcPrfMetrics(ObjOrd, InstId, &pm);
      if (pm.pPIN[0] != 0) nIndexInstInUse++;
   }
   return(nIndexInstInUse);
}

#endif // _WIN64
#endif

//////////////////////////// End Of File ////////////////////////////
