// Copyright (c) 1996-1999 Microsoft Corporation

// ==========================================================================
// File: A P I . C P P
// 
// Copyright 1995-1996 Microsoft Corporation.  All Rights Reserved.
// Microsoft Confidential.
// ==========================================================================

// Includes --------------------------------------------------------------
#include "oleacc_p.h"
#include "default.h"
#include "verdefs.h"



#define CCH_ROLESTATEMAX     128

#ifndef WMOBJ_SAMETHREAD
#define WMOBJ_SAMETHREAD  0x8000
#endif

#ifndef WMOBJ_ID
#define WMOBJ_ID   0x0
#endif

// Globals: ----------------------------------------------------------
extern HANDLE   g_hMutexUnique; // mutex used for access to uniqueness value (declared in oleacc.cpp)

// Structs and defines ------------------------------------------------------
#define BUFFERSINCACHE     (5)
#define CBBUFFERSIZE       (1024)
#define SZFILEMAP          __TEXT("SMD.MSAA.FileMap.%08X")


typedef struct {
   BOOL  m_fInUse;
   DWORD m_dwSize;
   BYTE  m_bBuffer[CBBUFFERSIZE];
} BUFFER, *PBUFFER;

typedef struct {
   DWORD  m_dwProcessId;
   HANDLE m_hfm;  
   DWORD  m_cbSize;
} XFERDATA, *PXFERDATA;


// --------------------------------------------------------------------------
// These variables are in the shared data segment - shared between all 
// instances of the oleacc dll
// --------------------------------------------------------------------------

#pragma data_seg("Shared")

DWORD g_dwBufferCount = BUFFERSINCACHE; // Strictly incrementing counter for uniqueness
BUFFER BufferCache[BUFFERSINCACHE] = {0};

#pragma data_seg()
#pragma comment(linker, "/Section:Shared,RWS")


// --------------------------------------------------------------------------
// Following static functions are local to this module
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
//  CloseRemoteHandle
//
//  used to close the remote file handle used in LResultFromObject and
//  ObjectFromLResult
// --------------------------------------------------------------------------

static BOOL CloseRemoteHandle(HANDLE hProcess, HANDLE hObject) 
{
   HANDLE hObjectRemoteInLocal;
   // DUPLICATE_CLOSE_SOURCE is how we get the server to close its file-mapping
   BOOL fOk = DuplicateHandle(hProcess, hObject, 
      GetCurrentProcess(), &hObjectRemoteInLocal,
      0, FALSE, DUPLICATE_CLOSE_SOURCE  | DUPLICATE_SAME_ACCESS);
   fOk = fOk && CloseHandle(hObjectRemoteInLocal);
   return(fOk);
}


// --------------------------------------------------------------------------
//  CloseRemoteHandle
//
//  This is an overloaded function, same as above.
// --------------------------------------------------------------------------

static BOOL CloseRemoteHandle(DWORD dwProcessId, HANDLE hObject) 
{
   HANDLE hProcess = OpenProcess(PROCESS_DUP_HANDLE, FALSE, dwProcessId);
   BOOL fOk = (hProcess != NULL);
   if (fOk) 
   {
      fOk = CloseRemoteHandle(hProcess, hObject);
      CloseHandle(hProcess);
   }
   return(fOk);
}


// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

static HRESULT GetStreamSize(LPSTREAM pstm, PDWORD pcbSize) 
{
   HRESULT hr;
   *pcbSize = 0;  // Note: If anything fails, 0 is returned
   STATSTG statstg;

   // This function also resets the stream pointer to the beginning
   LARGE_INTEGER li = { 0, 0 };
   hr = pstm->Seek(li, STREAM_SEEK_SET, NULL);
   if (!FAILED(hr)) 
   {
      // Get the number of bytes in the stream
      hr = pstm->Stat(&statstg, STATFLAG_NONAME);
      if (!FAILED(hr)) 
          *pcbSize = statstg.cbSize.LowPart;
   }
   return(hr);
}


// --------------------------------------------------------------------------
// This is used to create some shared storage to use to marshall the 
// interface. In the old versionof OLEACC, this code was inside 
// LresultFromObject. Now it has been pulled out so that Win95 and Win NT can
// have different methods. Win95 uses shared memory, while NT uses Memory 
// mapped files. 
// LresultFromObject creates a stream, then marshalls the interface. 
// The marshalled interface is then stored in the shared storage area (by
// the function SharedBuffer_Allocate..) and some id for that shared storage 
// area is returned. For Win95, the id is a pointer to the shared memory
// with the high bit cut off so it looks sorta like a successful HRESULT.
// For NT, the id is a unique number referring to a memory mapped file.
//
// SharedBuffer_Allocate (LPSTREAM pstm)
//
// Parameters:
//      pstm    pointer to a stream that has the marshalled interface to store.
//
// Returns:
//      An HRESULT that indicates success or failure. If the high bit is set,
//      the function failed. If the high bit is not set, the return value is 
//      the id of the shared storage area.
//
// --------------------------------------------------------------------------

static HRESULT WINAPI SharedBuffer_Allocate(LPSTREAM pstm) 
{
   BOOL fFound = FALSE;
   HRESULT hr;

   DWORD cbSize;
   hr = GetStreamSize(pstm, &cbSize);
   if (FAILED(hr)) 
       return (HRESULT_FROM_WIN32(ERROR_INVALID_ADDRESS));
   
   // NB - have to start at 1 - not 0 - since 0 is a reserved LRESULT meaning
   // 'no IAccessible here'. 
   int nBufNum = 1;
   while (!fFound && (nBufNum < BUFFERSINCACHE) && (cbSize <= CBBUFFERSIZE)) 
   {
       // Atomic test to check whether this cache buffer is currently in use?
       fFound = (MyInterlockedCompareExchange(
           (PVOID*) &BufferCache[nBufNum].m_fInUse, (PVOID) TRUE, FALSE) == FALSE);

       if (fFound) 
       {
           // We can use a buffer from the cache
           BufferCache[nBufNum].m_dwSize = cbSize;
           hr = pstm->Read(&BufferCache[nBufNum].m_bBuffer[0], cbSize, NULL);
           if (SUCCEEDED(hr)) 
           {
               // A successful HRESULT contains the cache buffer number
               hr = nBufNum; 
           } 
           else // Read failed
           {
               // We're not using this cache buffer after all, free it up
               InterlockedExchange((PLONG) &BufferCache[nBufNum].m_fInUse, FALSE);
               fFound = FALSE;
               nBufNum = BUFFERSINCACHE;  // Attempt to use file-mapping buffer instead
           }
       } 
       else // not found
       {
           nBufNum++; // Try the next cache buffer
       }
   }
   
   if (!fFound) 
   {
       // All buffers in the cache are in use, dynamically create a buffer
       HANDLE hfm = NULL;
       PXFERDATA pxd = NULL;

       hr = E_FAIL; // if things don't work out...
       
       // Only allow mutual-exclusive access to the uniqueness counter
       WaitForSingleObject(g_hMutexUnique, INFINITE);//s_MutexBufferCount.Wait();
       
       // Get the Uniqueness value
       DWORD dwUniqueVal = g_dwBufferCount;
       
       // Increment the uniqueness value
       // NOTE: We can only return a value where the high-bit is 0.
       //       This limits us to a value between 0 and 0x7fffffff.  However,
       //       a value from 0 to (BUFFERSINCACHE - 1) indicates a buffer from the 
       //       cache. So, the Uniqueness number must be in the range of 
       //       BUFFERSINCACHE to 0x7fffffff inclusive.
       if (++g_dwBufferCount == 0x80000000) 
           g_dwBufferCount = BUFFERSINCACHE;
       
       ReleaseMutex(g_hMutexUnique);//s_MutexBufferCount.Release();
       
       TCHAR szName[_MAX_PATH];
       wsprintf(szName, SZFILEMAP, dwUniqueVal);
       
       // NOTE: The hfm handle is not closed here.  
       // It will be closed when the client calls SharedBuffer_Free
       hfm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 
           0, cbSize + sizeof(XFERDATA), szName);
       
       if (hfm == NULL) 
           return (hr);
       
       pxd = (PXFERDATA) MapViewOfFile(hfm, FILE_MAP_WRITE, 0, 0, 0);
       if (pxd == NULL) 
           return (hr);
       
       // Save the server (source) information so that the client can
       // free the server's object after the client gets it
       pxd->m_dwProcessId = GetCurrentProcessId();
       pxd->m_hfm = hfm;  
       
       pxd->m_cbSize = cbSize;
       hr = pstm->Read((PVOID) (pxd + 1), cbSize, NULL);
       if (FAILED(hr))
       {
           if (pxd != NULL) 
               UnmapViewOfFile((PVOID) pxd);
           return (hr);
       }

       // Everything was successful, set hr to the uniqueness value
       hr = (HRESULT) dwUniqueVal;
   } // end if !fFound

    return(hr);
}


// --------------------------------------------------------------------------
//
// SharedBuffer_Free(DWORD dwID, LPSTREAM *ppstm,HGLOBAL *phGlobal) 
//
// This takes the id created by SharedBuffer_Allocate and a pointer to a 
// stream and unmarshalls the interface pointer that is stored in the shared
// storage area that the id refers to. This is the NT version, so the id is a
// unique number that identifies a memory mapped file.
//
// Parameters:
//      dwID    this is the lresult passed to ObjectFromLresult - the id of 
//              the shared storage area
//      ppstm   a pointer to a pointer to a stream used to unmarshall the
//              interface. When the function returns successfully, this
//              value will be filled in with a pointer to a stream that 
//              needs to be Released by the caller.
//      phGlobal     pointer to a global memory handle. When the function returns
//              successfully, this value will be filled in with a pointer to
//              a global memory handle that must be GlobalFree()'d by the
//              caller.
//
// returns:
//      An HRESULT indicating success or failure.
//
// --------------------------------------------------------------------------

static HRESULT WINAPI SharedBuffer_Free(DWORD dwID, LPSTREAM *ppstm,HGLOBAL *phGlobal) 
{
BOOL        fOk = FALSE;
HANDLE      hfm = NULL;
PXFERDATA   pxd = NULL;
HGLOBAL     hGlobal;
HRESULT     hr = E_FAIL;
BOOL        fAbnormalTermination = FALSE;

   //__try 
   {
      *ppstm = NULL;  // Set to NULL in case this function fails
      DWORD cbSize;
      PVOID pv = NULL;

      // Small sanity check on dwID parameter
      if (FAILED(dwID)) 
      { 
          hr = E_INVALIDARG; 
          fAbnormalTermination = TRUE;
          goto SHBF_CLEANUP;    //__leave; 
      }

      // We know that dwID must be >= 0
      if (dwID < BUFFERSINCACHE) 
      {
          // Extract data from a cached buffer
          Assert(BufferCache[dwID].m_fInUse);
          cbSize = BufferCache[dwID].m_dwSize;  // Size of data
          pv = &BufferCache[dwID].m_bBuffer[0]; // Address of data

          // Sanity check on cbSize...
          if (cbSize < 1 || cbSize > CBBUFFERSIZE) 
          {
              fAbnormalTermination = TRUE;
              goto SHBF_CLEANUP;    //__leave;
          }
      } 
      else 
      {
         // Extract data from a dynamically allocated buffer

         TCHAR szName[_MAX_PATH];
         wsprintf(szName, SZFILEMAP, dwID);

         hfm = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, szName);
         if (hfm == NULL) 
         { 
             hr = HRESULT_FROM_WIN32(GetLastError()); 
             fAbnormalTermination = TRUE;
             goto SHBF_CLEANUP;    //__leave; 
         }
         pxd = (PXFERDATA) MapViewOfFile(hfm, FILE_MAP_ALL_ACCESS, 0, 0, 0);
         if (pxd == NULL) 
         { 
             hr = HRESULT_FROM_WIN32(GetLastError()); 
             fAbnormalTermination = TRUE;
             goto SHBF_CLEANUP;     //__leave; 
         }

         cbSize = pxd->m_cbSize;    // Size of data
         pv = (PVOID) (pxd + 1);    // Address of data
      }

      // We have the size of the data and the address of the data,
      // make a stream out of it.

      // Allocate memory for data
      hGlobal = GlobalAlloc(GPTR, cbSize);
      if (hGlobal == NULL) 
      {
          fAbnormalTermination = TRUE;
          goto SHBF_CLEANUP;    //__leave;
      }

      *phGlobal = hGlobal;
      // Copy data into buffer
      CopyMemory(hGlobal, pv, cbSize);

      // Create a stream out of the data buffer
      hr = CreateStreamOnHGlobal(hGlobal, FALSE, ppstm);
   }
   
SHBF_CLEANUP:   //   __finally 
   {    // All cleanup goes in here

      if (dwID < BUFFERSINCACHE) 
      {
         // If the data was in a cached buffer, we got it and we
         // can make the cached buffer available again.
         InterlockedExchange((PLONG) &BufferCache[dwID].m_fInUse, FALSE);
      } 
      else 
      {
         // The data was in a dynamically-created file-mapping, free it up.

         // Once we (the client) has the data, the server no longer needs to 
         // hold to it. Close the Server's file-mapping object
         if ((pxd != NULL) && !CloseRemoteHandle(pxd->m_dwProcessId, pxd->m_hfm)) 
            hr = HRESULT_FROM_WIN32(GetLastError());

         if (pxd != NULL) 
             UnmapViewOfFile((PVOID) pxd);
         if (hfm != NULL) 
             CloseHandle(hfm);
      }
      if (fAbnormalTermination) 
         hr = HRESULT_FROM_WIN32(ERROR_INVALID_ADDRESS);
   }
   return(hr);
}



#ifdef _X86_

// --------------------------------------------------------------------------
//
// SharedBuffer_Allocate_Win95 (LPSTREAM pstm)
//
// Parameters:
//      pstm    pointer to a stream that has the marshalled interface to store.
//
// Returns:
//      An HRESULT that indicates success or failure. If the high bit is set,
//      the function failed. If the high bit is not set, the return value is 
//      the id of the shared storage area.
//
// --------------------------------------------------------------------------

static HRESULT WINAPI SharedBuffer_Allocate_Win95(LPSTREAM pstm) 
{
    // Get the number of bytes in the stream
    DWORD cbSize;
    HRESULT hr = GetStreamSize(pstm, &cbSize);
    if (SUCCEEDED(hr)) 
    {
        // Since we know we are on Win95, we can specify NULL for
        // the hwnd and hProcess parameters here.
        PVOID pv = SharedAlloc(cbSize,NULL,NULL);
        if (pv == NULL) 
            hr = HRESULT_FROM_WIN32(GetLastError());
        else 
        {
            hr = pstm->Read(pv, cbSize, NULL);
            if (SUCCEEDED(hr)) 
            {
                // Force the high-bit off to indicate a successful return value
                hr = (HRESULT) ((DWORD) pv) & ~HEAP_GLOBAL; 
            }
        }
    }
    return(hr);
}


// --------------------------------------------------------------------------
//
// SharedBuffer_Free_Win95(DWORD dwID, LPSTREAM *ppstm,HGLOBAL *phGlobal) 
//
// This takes the id created by SharedBuffer_Allocate and a pointer to a 
// stream and unmarshalls the interface pointer that is stored in the shared
// storage area that the id refers to. This is the 95 version, so the id is a
// pointer into the shared heap, with the high bit cut off.
//
// Parameters:
//      dwID    this is the lresult passed to ObjectFromLresult - the id of 
//              the shared storage area
//      ppstm   a pointer to a pointer to a stream used to unmarshall the
//              interface. When the function returns successfully, this
//              value will be filled in with a pointer to a stream that 
//              needs to be Released by the caller.
//      phGlobal pointer to a global memory handle. When the function returns
//              successfully, this value will be filled in with a pointer to
//              a global memory handle that must be GlobalFree()'d by the
//              caller.
//
// returns:
//      An HRESULT indicating success or failure.
//
// --------------------------------------------------------------------------

static HRESULT WINAPI SharedBuffer_Free_Win95(DWORD dwID, LPSTREAM *ppstm,HGLOBAL *phGlobal) 
{
HGLOBAL hGlobal;
HRESULT hr;
PVOID   pv;
DWORD   cb; 
BOOL    fAbnormalTermination = FALSE;

    //__try 
    {
        *ppstm = NULL; // Set to NULL in case the function fails

        // Get address of shared memory block
        pv = (PVOID) (dwID | HEAP_GLOBAL);  // Turn the high-bit back on

        // Get the size of the block in the shared heap.
        cb = HeapSize(hheapShared, 0, pv);

        // Allocate a block of memory local to this process for the memory stream
        hGlobal = GlobalAlloc(GPTR, cb);
        if (hGlobal == NULL) 
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            fAbnormalTermination = TRUE;
            goto SHBF95_CLEANUP;    //__leave;
        } 
        else 
        {
            *phGlobal = hGlobal;
            // Copy the data from the shared heap into the local memory block
            CopyMemory(hGlobal, pv, cb);

            // Turn this local memory block into a memory stream
            hr = CreateStreamOnHGlobal(hGlobal, FALSE, ppstm);
        }
    }
SHBF95_CLEANUP:    //__finally 
    {
        if (fAbnormalTermination) 
            hr = HRESULT_FROM_WIN32(ERROR_INVALID_ADDRESS);
    }
    return(hr);
}

#endif // _X86_


// --------------------------------------------------------------------------
//
//  LresultFromObject()
//
//  This function marshals the interface from the server process to the client.
//  The function returns an HRESULT as an LRESULT so, the high bit MUST be 0
//  or the caller will think that an error occurred.
//
//  If the server and the client are the same thread, the LRESULT == IUnknown
//  in order to improve performance because marshalling does not need to be performed.
//  It would be nice if we could optimize if the client and server threads were in 
//  the same process but: OLE objects need to be thread safe and hence must be 
//  marshalled even across threads.  They have state data that needs to be protected.  
//
//  If the client and server are different processes, the LRESULT is a an opaque 
//  32-bit identifier that the client process can use to get to the marshalled 
//  interface data.
//
// --------------------------------------------------------------------------

EXTERN_C LRESULT WINAPI LresultFromObject(REFIID riid, WPARAM wParam, LPUNKNOWN punk) 
{
LPSTREAM  pstm = NULL;
LRESULT   lResult = 0;

    //__try 
    {
      // Optimization for when client and server are the same thread; no need to marshal/unmarshal
      if (wParam & WMOBJ_SAMETHREAD) 
      { 
		  // We addref here to hold onto the object on behalf of the client caller.
		  // This allows the server to safely release() the object after they've used
		  // LresultfromObject to 'convert' it into a LRESULT
		  punk->AddRef();
          lResult = (LRESULT) punk; 
          goto LRFO_CLEANUP;    //__leave; 
      }

      // Create a stream (in memory) for the marshalling data.
      lResult = CreateStreamOnHGlobal(NULL, TRUE, &pstm);
      if (FAILED(lResult)) 
      {
          goto LRFO_CLEANUP;    //__leave;
      }

      // Have COM create the marshalling data and store the data in our memory stream.
      lResult = CoMarshalInterface(pstm, riid, punk, MSHCTX_LOCAL, NULL, MSHLFLAGS_NORMAL);
      if (FAILED(lResult)) 
      {
          goto LRFO_CLEANUP;    //__leave;
      }

#ifdef _X86_
      if (fWindows95) 
      {
         // We're running on Windows 95, use the shared heap for the marshalled data
         lResult = SharedBuffer_Allocate_Win95(pstm);
      }  
      else 
#endif // _X86_
      {
         // We're running on Windows NT, use the cached set of shared buffers in this DLL 
         // or create a memory-mapped file specifically for this interface's marshalled data.
         lResult = SharedBuffer_Allocate(pstm);
      }
   }

LRFO_CLEANUP:    //   __finally 
   {
       if (pstm != NULL) 
           pstm->Release();
       if (FAILED(lResult)) 
           return(lResult);
   }

    return(lResult);
}




// --------------------------------------------------------------------------
//
//  ObjectFromLresult()
//
//  This function converts the 32-bit opaque value returned from LresultFromObject
//  into a marshalled interface pointer.  
//
// --------------------------------------------------------------------------

EXTERN_C HRESULT WINAPI ObjectFromLresult(LRESULT ref, REFIID riid, WPARAM wParam, void **ppvObject) 
{
HRESULT     hr;
LPSTREAM    pstm = NULL;
HGLOBAL     hGlobal = NULL;
LPVOID      pv = (LPVOID)ref;

    //__try 
    {
        // Do a basic sanity check on parameters
        if (FAILED(ref) || ppvObject == NULL) 
        { 
            hr = E_INVALIDARG; 
            goto OFLR_CLEANUP;  //__leave; 
        }

        // If the client and server are in the same thread, LresultFromObject is
        // optimized to return the original interface pointer since no marshalling 
        // needs to occur.
        if ((wParam & WMOBJ_SAMETHREAD) != 0) 
        {
			// SMD 4-30-98
			// Found a case where some bozo (Lotus Notes Splash Screen Window)
			// was responding to WM_GETOBJECT message with 1. This can cause
			// a problem for folks responding to events in-context - it comes
			// to this point and blows (unless we check that the pointer is
			// valid first)
			// If the WMOBJ_SAMETHREAD is set, then the ref should just be
			// an interface pointer.
			if (!IsBadReadPtr(pv,1))
			{
				hr = ((LPUNKNOWN) pv)->QueryInterface(riid, ppvObject);
				// Release the 'old' LPUNKNOWN interface - pass the QI'd one back to the client,
				// it has the responsibility of releasing that.
				((LPUNKNOWN) pv)->Release();
				return (hr); //__leave; (do not want to free ref, not a shared memory deal anyways!)
			}
			AssertStr( "Some bozo just returned non-zero invalid value when sent WM_GETOBJECT" );
			return (E_INVALIDARG);
        }

#ifdef _X86_        
        if (fWindows95) 
        {
            // We're running on Windows 95, get the marshalled data from the shared heap.
            hr = SharedBuffer_Free_Win95 (ref,&pstm,&hGlobal);
            if (FAILED(hr)) 
                goto OFLR_CLEANUP;  //__leave;
        } 
        else 
#endif // _X86_
        {

            // We're running on Windows NT, get the marshalled data from the cached set 
            // of shared buffers in this DLL or out of a memory-mapped file that was created
            // specifically for this interface's marshalled data.

            // Cast to convert from (64/42-bit) LRESULT to 32-bit buffer index
            hr = SharedBuffer_Free ((DWORD)ref,&pstm,&hGlobal);
            if (FAILED(hr)) 
                goto OFLR_CLEANUP;  //__leave;

        }

        // Marshall the interface into this client process
        hr = CoUnmarshalInterface(pstm, riid, ppvObject);
        if (FAILED(hr)) 
            goto OFLR_CLEANUP;  //__leave;
    }
    
OFLR_CLEANUP:  //__finally 
    {
#ifdef _X86_
        if (fWindows95) 
        {
            // we know we are on Win95, can use NULL hProcess, but we need
			// to convert pv to a real pointer - add the high bit back in.
			pv = (PVOID) (ref | HEAP_GLOBAL);
            SharedFree(pv,NULL);
        }
#endif // _X86_
        if (hGlobal != NULL) 
            GlobalFree(hGlobal);

        if (pstm != NULL) 
            pstm->Release();
    }
    return(hr);
}



// --------------------------------------------------------------------------
//
//  AccessibleObjectFromWindow()
//
//  This gets an interface pointer from the object specified by dwId inside
//  of the window.
//
// --------------------------------------------------------------------------
STDAPI
AccessibleObjectFromWindow(HWND hwnd, DWORD dwId, REFIID riid, void **ppvObject)
{
DWORD_PTR    ref;
WPARAM      wParam = 0;

    if (IsBadWritePtr(ppvObject,sizeof(void*)))
        return (E_INVALIDARG);

    // clear out-param
    *ppvObject = NULL;
    ref = 0;

    //
    // Window can be NULL (cursor, alert, sound)
    // Window can also be bad (trying to talk to window that generated event and
    // client is getting events out of context, and window is gone)
    //
    if (IsWindow(hwnd))
    {
        wParam = WMOBJ_ID;
    
        //
        // If the window is on our thread, optimize the marshal/unmarshal
        //
        if (GetWindowThreadProcessId(hwnd, NULL) == GetCurrentThreadId())
            wParam |= WMOBJ_SAMETHREAD;

        SendMessageTimeout(hwnd, WM_GETOBJECT, wParam, dwId,
            SMTO_ABORTIFHUNG, 10000, &ref);
    }


    if (FAILED((HRESULT)ref))
        return (HRESULT)ref;
    else if (ref)
        return(ObjectFromLresult(ref, riid, wParam, ppvObject));
    else
    {
        //
        // Is this the ID of an object we understand and a REFIID we can
        // handle?  BOGUS!  For now, we always create the object and QI
        // on it, only to fail if the riid isn't one we know.  
        //

        //-----------------------------------------------------------------
        // [v-jaycl, 5/15/97] Handle custom OBJIDs -- TODO: UNTESTED!!!
        //-----------------------------------------------------------------

//        if (fCreateDefObjs && ((LONG)dwId <= 0))
        if (fCreateDefObjs )
        {
            return(CreateStdAccessibleObject(hwnd, dwId, riid, ppvObject));
        }

        return(E_FAIL);
    }
}


// --------------------------------------------------------------------------
//
//  GetRoleTextA()
//
//  Loads the string for the specified role.  If the role is bogus, we will
//  get nothing since the role area is at the end of the string table.  We
//  return the number of chars of the string.
//  
//  CWO: 12/3/96, we now return 0 if the string ptr passed in was bogus
//
//  The caller can pass in a NULL buffer, in which case we just return the
//  # of chars so that he can turn around and allocate something the right
//  size.
//
// --------------------------------------------------------------------------
#ifdef UNICODE
STDAPI_(UINT)   GetRoleTextW(DWORD lRole, LPWSTR lpszRole, UINT cchRoleMax)
#else
STDAPI_(UINT)   GetRoleTextA(DWORD lRole, LPSTR lpszRole, UINT cchRoleMax)
#endif
{
    TCHAR    szRoleT[CCH_ROLESTATEMAX];

    // NULL string is valid, use our temporary string and return count
    if (!lpszRole)
    {
        lpszRole = szRoleT;
        cchRoleMax = CCH_ROLESTATEMAX;
    }
    else
    {
        // CWO: Added 12/3/96, Error checking of parameters
        if (IsBadWritePtr(lpszRole,(sizeof(TCHAR) * cchRoleMax)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    
    if( cchRoleMax == 1 )
    {
        // Special case for 1-len string - we expect it to copy nothing, but
        // NUL-terminate (for consistency with other cases) - but LoadString
        // just returns 0 w/o terminating...
        *lpszRole = '\0';
        return 0;
    }
    else
        return LoadString(hinstResDll, STR_ROLEFIRST+lRole, lpszRole, cchRoleMax);
}


// --------------------------------------------------------------------------
//
//  GetStateTextA()
//
//  Loads the string for ONE particular state bit.  We return the number of
//  characters in the string.
//
//  CWO: 12/3/96, we now return 0 if the string ptr passed in was bogus
//  CWO, 12/4/96, Added parameter checking and set last error to 
//                ERROR_INVALID_PARAMETER.
//
//  Like GetRoleTextA(), the caller can pass in a NULL buffer.  We will 
//  simply return the character count necessary in that case.
//
// --------------------------------------------------------------------------
#ifdef UNICODE
STDAPI_(UINT)   GetStateTextW(DWORD lStateBit, LPWSTR lpszState, UINT cchStateMax)
#else
STDAPI_(UINT)   GetStateTextA(DWORD lStateBit, LPSTR lpszState, UINT cchStateMax)
#endif
{
    TCHAR   szStateT[CCH_ROLESTATEMAX];
    int     iStateBit;

    //
    // Figure out what state bit this is.
    //
    iStateBit = 0;
    while (lStateBit > 0)
    {
        lStateBit >>= 1;
        iStateBit++;
    }

    // NULL string is valid, use our temporary string and return count
    if (!lpszState)
    {
        lpszState = szStateT;
        cchStateMax = CCH_ROLESTATEMAX;
    }
    else
    {
        // CWO: Added 12/3/96, Error checking of parameters
        if (IsBadWritePtr(lpszState,(sizeof(TCHAR) * cchStateMax)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return (0);
        }
    }

    if( cchStateMax == 1 )
    {
        // Special case for 1-len string - we expect it to copy nothing, but
        // NUL-terminate (for consistency with other cases) - but LoadString
        // just returns 0 w/o terminating...
        *lpszState = '\0';
        return 0;
    }
    else
        return LoadString(hinstResDll, STR_STATEFIRST+iStateBit, lpszState, cchStateMax);
}




// --------------------------------------------------------------------------
//
//  [INTERNAL]
//  GetRoleStateTextWCommon()
//
//  Calls GetRoleTextA or GetStateTextA (passed in through pfnGetRoleStateANSI
//  parameter), and converts resulting string to UNICODE.
//
//  Ensures that...
//  (1) return value equals number of chars copied, excluding terminating NUL.
//  (2) if buffer is too small, as much of string as possible will be
//      copied (truncation occurs).
//  (2) terminating NUL added, even when trucation occurs.
//
//  Eg. buffer of size 4 used when getting text for 'default'...
//  Buffer will contain  'def\0' (in unicode),
//  return value of 3, since 3 chars (excl. NUL) copied.
//
//  This ensures comsistency with the 'A' versions of GetXText().
//
//  (Note that MultiByteToWideChar is not a particularly boundary-case-
//  friendly API - if the buffer is too short, it doesn't truncate neatly -
//  it *does not* add a terminating NUL, and returns 0! - so it's effectively
//  all-or-nothing, with no way of getting partial strings, for piecemeal
//  conversion, for example. To get around this, we use MBtoWC to translate
//  into a stack allocated buf of CCH_ROLEMAX, and then copy as necessary
//  from that to the output string, terminating/truncating neatly.)
//
// --------------------------------------------------------------------------

typedef UINT (WINAPI FN_GetRoleOrStateTextT)( DWORD lVal, LPTSTR lpszText, UINT cchTextMax );


#ifdef UNICODE

STDAPI_(UINT) GetRoleStateTextACommon( FN_GetRoleOrStateTextT * pfnGetRoleStateThisCS,
                                       DWORD lVal, 
                                       LPSTR lpszTextOtherCS,
                                       UINT cchTextMax)
#else

STDAPI_(UINT) GetRoleStateTextWCommon( FN_GetRoleOrStateTextT * pfnGetRoleStateThisCS,
                                       DWORD lVal, 
                                       LPWSTR lpszTextOtherCS,
                                       UINT cchTextMax)

#endif
{
    TCHAR szTextThisCS[ CCH_ROLESTATEMAX ];
    if( pfnGetRoleStateThisCS( lVal, szTextThisCS, CCH_ROLESTATEMAX ) == 0 )
        return 0;

    // Note - cchPropLen includes the terminating nul...
#ifdef UNICODE
    CHAR szTextOtherCS[ CCH_ROLESTATEMAX ];
    int cchPropLen = WideCharToMultiByte( CP_ACP, 0, szTextThisCS, -1, szTextOtherCS, CCH_ROLESTATEMAX, NULL, NULL );
#else
    WCHAR szTextOtherCS[ CCH_ROLESTATEMAX ];
    int cchPropLen = MultiByteToWideChar( CP_ACP, 0, szTextThisCS, -1, szTextOtherCS, CCH_ROLESTATEMAX );
#endif

    // unexpected error...
    if( cchPropLen == 0 )
        return 0;

    // Ignore terminating NUL in length...
    cchPropLen--;

    // lpszRole == NULL means just return length...
    if( ! lpszTextOtherCS )
        return cchPropLen; // (number of TCHARS, not bytes)
    else
    {
        // string requested...
#ifdef UNICODE
        if( IsBadWritePtr( lpszTextOtherCS, ( sizeof(CHAR) * cchTextMax ) ) )
#else
        if( IsBadWritePtr( lpszTextOtherCS, ( sizeof(WCHAR) * cchTextMax ) ) )
#endif
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }

        // need space for at least terminating NUL...
        if( cchTextMax <= 0 )
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

        // Copy as much string as necessary (cchCopyLen excludes NUL)...
        // (-1 to reserve terminating NUL)
        int cchCopyLen = cchTextMax - 1;
        if( cchCopyLen > cchPropLen )
            cchCopyLen = cchPropLen;

#ifdef UNICODE
		// Copy/truncate the ANSI string...
		// TODO - is strncpy sufficient? Does it slice DBCS correctly?
        // +1 to add back space for terminating NUL, which lstrncpyA adds for us
		lstrcpynA( lpszTextOtherCS, szTextOtherCS, cchCopyLen + 1 );
#else
        // Since we're explicitly copying UNICODE, use of memcpy is safe...
        memcpy( lpszTextOtherCS, szTextOtherCS, cchCopyLen * sizeof( WCHAR ) );
        lpszTextOtherCS[ cchCopyLen ] = '\0';
#endif
        return cchCopyLen;
    }
}





// --------------------------------------------------------------------------
//
//  GetRoleTextW()
//
//  Like GetRoleTextA() but returns a UNICODE string.
//
//  Calls GetRoleStateTextWCommon, which just calls GetStateTextA and
//  converts the result to UNICODE.
//
// --------------------------------------------------------------------------
#ifdef UNICODE

STDAPI_(UINT)   GetRoleTextA(DWORD lRole, LPSTR lpszRole, UINT cchRoleMax)
{
    return GetRoleStateTextACommon( GetRoleTextW, lRole, lpszRole, cchRoleMax );
}

#else

STDAPI_(UINT)   GetRoleTextW(DWORD lRole, LPWSTR lpszRole, UINT cchRoleMax)
{
    return GetRoleStateTextWCommon( GetRoleTextA, lRole, lpszRole, cchRoleMax );
}

#endif

// --------------------------------------------------------------------------
//
//  GetStateTextW()
//
//  Like GetStateTextA() but returns a UNICODE string.
//
//  Calls GetRoleStateTextWCommon, which just calls GetStateTextA and
//  converts the result to UNICODE.
//
//
// --------------------------------------------------------------------------
#ifdef UNICODE

STDAPI_(UINT)   GetStateTextA(DWORD lStateBit, LPSTR lpszState, UINT cchStateMax)
{
    return GetRoleStateTextACommon( GetStateTextW, lStateBit, lpszState, cchStateMax );
}

#else

STDAPI_(UINT)   GetStateTextW(DWORD lStateBit, LPWSTR lpszState, UINT cchStateMax)
{
    return GetRoleStateTextWCommon( GetStateTextA, lStateBit, lpszState, cchStateMax );
}

#endif

// --------------------------------------------------------------------------
//
//  CreateStdAccessibleObject()
//
//  See Also: CreateStdAccessibleProxy() in default.cpp
//
//  This function takes an HWND and an OBJID.  If the OBJID is one of the
//  system reserved IDs (OBJID_WINDOW, OBJID_CURSOR, OBJID_MENU, etc.)
//  we create a default object that implements the interface whose IID we
//  ask for. This is usually IAccessible, but might also be IDispatch, IText,
//  IEnumVARIANT...
//
//  This function is used by both the AccessibleObjectFromWindow API
//  and apps that want to do a little of their own thing but let us
//  handle most of the work.
//
// --------------------------------------------------------------------------
STDAPI
CreateStdAccessibleObject(HWND hwnd, LONG idObject, REFIID riid,
    void **ppvObject)
{
    HRESULT hr;
    TCHAR   szClassName[128];
    BOOL    bFound = FALSE;

    if (IsBadWritePtr(ppvObject,sizeof(void *)))
        return (E_INVALIDARG);

    *ppvObject = NULL;

    if (!hwnd && (idObject != OBJID_CURSOR))
        return(E_FAIL);
    
    switch(idObject)
    {
        case OBJID_SYSMENU:
            hr = CreateSysMenuBarObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_MENU:
            // HACK for IE4/Shell windows
            GetClassName (hwnd, szClassName,ARRAYSIZE(szClassName));
            if ((0 == lstrcmp (szClassName,TEXT("IEFrame"))) ||
                (0 == lstrcmp (szClassName,TEXT("CabinetWClass"))))
            {
                HWND            hwndWorker;
                HWND            hwndRebar;
                HWND            hwndSysPager;
                HWND            hwndToolbar;
                VARIANT         varChild;
                VARIANT         varState;

                hwndWorker = NULL;
                while (!bFound)
                {
                    hwndWorker = FindWindowEx (hwnd,hwndWorker,TEXT("Worker"),NULL);
                    if (!hwndWorker)
                        break;

                    hwndRebar = FindWindowEx (hwndWorker,NULL,TEXT("RebarWindow32"),NULL);
                    if (!hwndRebar)
                        continue;
            
					hwndSysPager = NULL;
                    while (!bFound)
                    {
                        hwndSysPager = FindWindowEx (hwndRebar,hwndSysPager,TEXT("SysPager"),NULL);
                        if (!hwndSysPager)
                            break;
                        hwndToolbar = FindWindowEx (hwndSysPager,NULL,TEXT("ToolbarWindow32"),NULL);
                        hr = AccessibleObjectFromWindow (hwndToolbar,OBJID_MENU,
                                                         IID_IAccessible, ppvObject);
                        if (SUCCEEDED(hr))
                        {
                            varChild.vt=VT_I4;
                            varChild.lVal = CHILDID_SELF;

                            if (SUCCEEDED (((IAccessible*)*ppvObject)->get_accState(varChild,&varState)))
							{
								if (!(varState.lVal & STATE_SYSTEM_INVISIBLE))
									bFound = TRUE;
							}
                        }
						
						// If we got an IAccessible, but it's not needed here (doesn't
						// satisfy the above visibility test), then release it.
						if (!bFound && *ppvObject != NULL)
							((IAccessible*)*ppvObject)->Release ();
                    }
                }
            } // end if we are talking to an IE4/IE4 Shell window

            if (!bFound)
                hr = CreateMenuBarObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_CLIENT:
            hr = CreateClientObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_WINDOW:
            hr = CreateWindowObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_HSCROLL:
        case OBJID_VSCROLL:
            hr = CreateScrollBarObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_SIZEGRIP:
            hr = CreateSizeGripObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_TITLEBAR:
            hr = CreateTitleBarObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_CARET:
            hr = CreateCaretObject(hwnd, idObject, riid, ppvObject);
            break;

        case OBJID_CURSOR:
            hr = CreateCursorObject(hwnd, idObject, riid, ppvObject);
            break;

        default:
            //-----------------------------------------------------------------
            // [v-jaycl, 5/15/97] Handle custom OBJIDs -- 
            //  Second parameter to FindWindowClass() is irrelevant since 
            //  we're looking for a reg.handler, not an intrinsic window or client
            //-----------------------------------------------------------------

            return FindAndCreateWindowClass( hwnd, TRUE, NULL,
                                           riid, idObject, ppvObject );
    }

    //BUGBUG:   Per bug #6497 we do not return ACC_S_SYNTHETIC for a
    //          object that we create.  Value does not exist, refer
    //          to bug for more information.  It must be an SCODE.
    //          CWO, 12/3/96

    return(hr);
}




// --------------------------------------------------------------------------
//
//  AccessibleObjectFromEvent()
//
//  This takes care of getting the container and checking if the child
//  is an object in its own right.  Standard stuff that everyone would have
//  to do. Basically a wrapper that uses AccessibleObjectFromWindow and
//  then get_accChild().
//
// --------------------------------------------------------------------------
STDAPI AccessibleObjectFromEvent(HWND hwnd, DWORD dwId, DWORD dwChildId,
                                 IAccessible** ppacc, VARIANT* pvarChild)
{
HRESULT hr;
IAccessible* pacc;
IDispatch* pdispChild;
VARIANT varT;

    //CWO, 12/4/96, Added check for valid window handle
    //CWO, 12/6/96, Allow a NULL window handle
    if (IsBadWritePtr(ppacc,sizeof(void*)) || IsBadWritePtr (pvarChild,sizeof(VARIANT)) || (!IsWindow(hwnd) && hwnd != NULL))
        return (E_INVALIDARG);

    InitPv(ppacc);
    VariantInit(pvarChild);

    //
    // Try to get the object for the container
    //
    pacc = NULL;
    hr = AccessibleObjectFromWindow(hwnd, dwId, IID_IAccessible, (void**)&pacc);
    if (!SUCCEEDED(hr))
        return(hr);
    if (!pacc)
        return(E_FAIL);

    //
    // Now, is the child an object?
    //
    VariantInit(&varT);
    varT.vt = VT_I4;
    varT.lVal = dwChildId;

    pdispChild = NULL;
    hr = pacc->get_accChild(varT, &pdispChild);
    if (SUCCEEDED(hr) && pdispChild)
    {
        //
        // Yes, it is.
        //

        // Release the parent.
        pacc->Release();

        // Convert the child to an IAccessible*
        pacc = NULL;
        hr = pdispChild->QueryInterface(IID_IAccessible, (void**)&pacc);

        // Release the IDispatch* form of the child
        pdispChild->Release();

        // Did it succeed?
        if (!SUCCEEDED(hr))
            return(hr);
        if (!pacc)
            return(E_FAIL);

        // Yes.  Clear out the lVal (0 is 'container' child id)
        varT.lVal = 0;
    }

    //
    // We have something.  Return it.
    //
    *ppacc = pacc;
    VariantCopy(pvarChild, &varT);

    return(S_OK);
}




// --------------------------------------------------------------------------
//
//  AccessibleObjectFromPoint()
//
//  Walks down the OLEACC hierarchy to get the object/element that is
//  at the current screen point. Starts with AccessibleObjectFromWindow
//  using WindowFromPoint() and then uses acc_HitTest to get to the
//  innermost object.
//
// --------------------------------------------------------------------------
STDAPI AccessibleObjectFromPoint(POINT ptScreen, IAccessible **ppAcc,
                                 VARIANT * pvarChild)
{
    HRESULT hr;
    IAccessible * pAcc;
    VARIANT varChild;
    HWND    hwndPoint;

    if (IsBadWritePtr(ppAcc,sizeof(void*)) || IsBadWritePtr (pvarChild,sizeof(VARIANT)))
        return (E_INVALIDARG);

    *ppAcc = NULL;
    pvarChild->vt = VT_EMPTY;

    //
    // Is this a valid screen point?
    //
    hwndPoint = WindowFromPoint(ptScreen);
    if (!hwndPoint)
        return(E_INVALIDARG);

    //
    // Get the top level window of this one and work our way down.  We have
    // to do this because applications may implement Acc at an intermediate
    // level above the child window.  Our default implementation will let us
    // get there and mesh.
    //
    hwndPoint = MyGetAncestor(hwndPoint, GA_ROOT);
    if (!hwndPoint)
        return(E_FAIL);

    hr = AccessibleObjectFromWindow(hwndPoint, OBJID_WINDOW, IID_IAccessible,
        (void **)&pAcc);

    //
    // OK, now we are cooking.
    //
    while (SUCCEEDED(hr))
    {
        //
        // Get the child at this point in the container object.
        //
        VariantInit(&varChild);
        hr = pAcc->accHitTest(ptScreen.x, ptScreen.y, &varChild);
        if (!SUCCEEDED(hr))
        {
            // Uh oh, error.  This should never happen--something moved.
            pAcc->Release();
            return(hr);
        }

        //
        // Did we get back a VT_DISPATCH?  If so, there is a child object.
        // Otherwise, we have our thing (container object or child element
        // too small for object).
        //
        if (varChild.vt == VT_DISPATCH)
        {
            pAcc->Release();

            if (! varChild.pdispVal)
                return(E_POINTER);

            pAcc = NULL;
            hr = varChild.pdispVal->QueryInterface(IID_IAccessible,
                (void **)&pAcc);

            varChild.pdispVal->Release();
        }
        else if ((varChild.vt == VT_I4) || (varChild.vt == VT_EMPTY))
        {
            //
            // accHitTest should ALWAYS return an object if the child is
            // an object.  Unlike with accNavigate, where you usually
            // have to pick by-index or by_object only and intermixed means
            // get_accChild is needed.
            //
            *ppAcc = pAcc;
            VariantCopy(pvarChild, &varChild);
            return(S_OK);
        }
        else
        {
            //
            // Failure.  Shouldn't have been returned.
            //
            VariantClear(&varChild);
            pAcc->Release();
            hr = E_INVALIDARG;
        }
    }

    return(hr);
}



// --------------------------------------------------------------------------
//
//  WindowFromAccessibleObject()
//
//  This walks UP the ancestor chain until we find something who responds to
//  IOleWindow().  Then we get the HWND from it.
//
// Returns E_INVALIDARG if object cannot be read or if HWND pointer is invalid
// (CWO, 12/4/96)
// --------------------------------------------------------------------------
STDAPI WindowFromAccessibleObject(IAccessible* pacc, HWND* phwnd)
{
IAccessible* paccT;
IOleWindow* polewnd;
IDispatch* pdispParent;
HRESULT     hr;

    //CWO: 12/4/96, Added check for NULL object
    //CWO: 12/13/96, Removed NULL check, replaced with IsBadReadPtr check (#10342)
    if (IsBadWritePtr(phwnd,sizeof(HWND*)) || IsBadReadPtr(pacc, sizeof(void*)))
        return (E_INVALIDARG);

    *phwnd = NULL;
    paccT = pacc;
    hr = S_OK;

    while (paccT && SUCCEEDED(hr))
    {
        polewnd = NULL;
        hr = paccT->QueryInterface(IID_IOleWindow, (void**)&polewnd);
        if (SUCCEEDED(hr) && polewnd)
        {
            hr = polewnd->GetWindow(phwnd);
            polewnd->Release();
            //
            // Release an interface we obtained on our own, but not the one
            // passed in.
            //
            if (paccT != pacc)
            {
                paccT->Release();
                paccT = NULL;
            }
            break;
        }

        //
        // Get our parent.
        //
        pdispParent = NULL;
        hr = paccT->get_accParent(&pdispParent);

        //
        // Release an interface we obtained on our own, but not the one
        // passed in.
        //
        if (paccT != pacc)
        {
            paccT->Release();
        }

        paccT = NULL;

        if (SUCCEEDED(hr) && pdispParent)
        {
            hr = pdispParent->QueryInterface(IID_IAccessible, (void**)&paccT);
            pdispParent->Release();
        }
    }

    return(hr);
}


// --------------------------------------------------------------------------
//
//  AccessibleChildren()
//
//  This function fills in an array of VARIANTs that refer to all the chilren
//  of an IAccessible object. This should simplify many of the test 
//  applications lives, as well as lots of other people as well.
//
//  Parameters:
//      paccContainer   This is a pointer to the IAccessible interface of the
//                      container object - the one you want to get the 
//                      children of.
//      iChildStart     The INDEX (NOT ID!!!) of the first child to get. 
//                      Usually the caller will use 0 to get all the children.
//                      If the caller wants something else, they need to remember
//                      that this expects an INDEX (0 to n-1) and not an ID
//                      (1 to n, or some private ID).
//      cChildren       Count of how many children to get. Usually the
//                      caller will first call IAccessible::get_accChildCount
//                      and use that value.
//      rgvarChildren   The array of VARIANTs that will be filled in by the
//                      function. Each VARIANT can be used to get info 
//                      about the child it references. The caller should be
//                      careful if they didn't use 0 for iChildStart, because
//                      then the index of the array and the index of the 
//                      children won't match up. 
//                      Each VARIANT will be of type either VT_I4 or 
//                      VT_DISPATCH. For a VT_I4, the caller will just ask the 
//                      container for info about the child, using the 
//                      VARIANT.lVal as a child id. For a VT_DISPATCH, the 
//                      caller should do a QueryInterface on VARIANT.pdispVal 
//                      to get an IAccessible interface and then talk to the 
//                      child object directly. 
//                  *** The caller must also do a Release on any IDispatch 
//                      Interfaces, and free this array of variants when done!! ***
//      pcObtained      This value will be filled in by the function and
//                      will indicate the number of VARIANTs in the array 
//                      that were successfully filled in. May not be NULL.
//
//  Returns:
//      S_OK if the number of elements supplied is cChildren; S_FALSE if
//      it succeeded but fewer than the number of children requested was
//      returned, or if you try to skip more children than exist. 
//      Error return values are E_INAVLIDARG if rgvarChildren is not as
//      big as cChildren, or if pcObtained is not a valid pointer.
//
// --------------------------------------------------------------------------
STDAPI AccessibleChildren (IAccessible* paccContainer, LONG iChildStart, 
                           LONG cChildren, VARIANT* rgvarChildren,LONG* pcObtained)
{
HRESULT         hr;
IEnumVARIANT*   penum;
IDispatch*      pdisp;
LONG            ArrayIndex;
LONG            ChildIndex;
LONG            celtTotal;

    Assert(paccContainer);
    if (IsBadWritePtr(rgvarChildren,sizeof(VARIANT)*cChildren) || IsBadWritePtr (pcObtained,sizeof(LONG)))
        return (E_INVALIDARG);

    // start by initializing the VARIANT array
    for (ArrayIndex = 0; ArrayIndex < cChildren; ArrayIndex++)
        VariantInit (&(rgvarChildren[ArrayIndex]));
  
    //
    // Try querying for IEnumVARIANT.  If that fails, use index+1 based IDs.
    //
    penum = NULL;
    hr = paccContainer->QueryInterface(IID_IEnumVARIANT, (void**)&penum);

    if (penum)
    {
        penum->Reset();
		// SMD 4/27/98 - fix 689 regression
		// if we are doing the case of getting everything (skipping 0)
		// then don't bother calling it. Fixes a problem in CClient::Skip
		// where it returned S_FALSE when skipping 0 items. Since others
		// may accidentally do this too, we'll "fix" it here to localize
		// the change
		if (iChildStart > 0)
		{
	        hr = penum->Skip(iChildStart);
			// hr should still be set to S_OK from QI call
		}
        if (hr == S_OK)
            hr = penum->Next(cChildren,rgvarChildren,(ULONG*)pcObtained);
        else
            *pcObtained = 0;

        penum->Release();
        if (FAILED(hr))
            return (hr);
    }
    else
    {
        // okay,so it doesn't support IEnumVARIANT. We'll just have to 
        // create an array of variants with sequential Child Id's.
        celtTotal = 0;
        paccContainer->get_accChildCount((LONG*)&celtTotal);

        if (iChildStart < celtTotal)
            *pcObtained = celtTotal - iChildStart;
        else
            *pcObtained = 0;

        ChildIndex = iChildStart+1;

        for (ArrayIndex = 0;ArrayIndex < *pcObtained;ArrayIndex++)
        {
            rgvarChildren[ArrayIndex].vt = VT_I4;
            rgvarChildren[ArrayIndex].lVal = ChildIndex;
            
            ChildIndex++;
        }
    } // end else - doesn't support IEnumVARIANT


    // Now that we've filled in the array of variants, let's check each
    // item to see if it is a real object or not.
    for (ArrayIndex = 0;ArrayIndex < *pcObtained;ArrayIndex++)
    {
        // check to see if this child is an IAccessible object or not
        if (rgvarChildren[ArrayIndex].vt == VT_I4)
        {
            pdisp = NULL;
            hr = paccContainer->get_accChild(rgvarChildren[ArrayIndex], &pdisp);
            if (SUCCEEDED(hr) && pdisp)
            {
                rgvarChildren[ArrayIndex].vt = VT_DISPATCH;
                rgvarChildren[ArrayIndex].pdispVal = pdisp; 
            } // end if child seems to be an object (has an IDispatch)
        } // end if child is VT_I4
    } // end for loop through 

    if (*pcObtained == cChildren)
        return(S_OK);
    else
        return (S_FALSE);
}



WORD g_VerInfo [ 4 ]= { BUILD_VERSION_INT };

STDAPI_(VOID) GetOleaccVersionInfo(DWORD* pVer, DWORD* pBuild)
{
    *pVer = MAKELONG( g_VerInfo[1], g_VerInfo[0] ); // MAKELONG(lo, hi)
    *pBuild = MAKELONG( g_VerInfo[3], g_VerInfo[2] ); // MAKELONG(lo, hi)
}
