// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    control.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

LARGE_INTEGER CPerformanceCounter::s_qpcFrequency;
BOOL CPerformanceCounter::s_qpcSupported;

//+-----------------------------------------------------------------------------
// CPerformanceCounter ctor
//
// Description: 
//     Initializes the performance counter with the specified minimal sampling
//     interval. Note that the sampling interval will at least be 1000ms.
//-----------------------------------------------------------------------------

CPerformanceCounter::CPerformanceCounter(UINT minIntervalMilliseconds)
{
    
    if (minIntervalMilliseconds < 1000)
    {
        m_samplingIntervalInMilliseconds = 1000;
    }
    else
    {
        m_samplingIntervalInMilliseconds = minIntervalMilliseconds;
    }      

    if (s_qpcSupported)
    {
        QueryPerformanceCounter(&m_startTime);
    }

    m_currentRate = 0;
    m_counter = 0;
}

//+----------------------------------------------------------------------------
// CPerformanceCounter::Initialize   
//-----------------------------------------------------------------------------
/*static*/
void
CPerformanceCounter::Initialize()
{
    s_qpcSupported = QueryPerformanceFrequency(&s_qpcFrequency);  
}

//+----------------------------------------------------------------------------
// CPerformanceCounter::GetCurrentRate   
//-----------------------------------------------------------------------------

UINT
CPerformanceCounter::GetCurrentRate()
{
    if (s_qpcSupported)
    {
        LARGE_INTEGER currentTime;
        QueryPerformanceCounter(&currentTime);

        LARGE_INTEGER passedTime;
        passedTime.QuadPart = (currentTime.QuadPart - m_startTime.QuadPart) * 1000 / s_qpcFrequency.QuadPart;

        if (passedTime.QuadPart > m_samplingIntervalInMilliseconds)
        {
            m_currentRate = static_cast<UINT>((m_counter * 1000) / passedTime.QuadPart);
            m_counter = 0;
            m_startTime = currentTime;
        }        
    }
    return m_currentRate;
}



//---------------------------------------------------------------------------------
// CMediaControl::ctor
//---------------------------------------------------------------------------------

CMediaControl::CMediaControl()
{
    _hMemoryMappedFile = NULL;
    _pFile = NULL;
}

//---------------------------------------------------------------------------------
// CMediaControl::Initialize
//---------------------------------------------------------------------------------

HRESULT
CMediaControl::Initialize(__in PCWSTR lpName)
{
    HRESULT hr = S_OK;

    if (lpName == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }
     
    IFCW32((NULL != (_hMemoryMappedFile = CreateFileMappingW(
        INVALID_HANDLE_VALUE, // Create file mapping in page file.
        NULL,
        PAGE_READWRITE,
        0, 
        sizeof(MemoryMappedFile),
        lpName))));

    //
    // If the file exists already another user might have created the file and still have it in use. If we opened such 
    // a file the other user could manipulate our process. Note that it is not enough to check the security descriptor 
    // of the memory mapped file because a malicious application could first create the file, open it, give access to our 
    // current user and then remove permissions for its own user. Even after removing access for its own user it can still 
    // manipulate the file because permissions are only checked when the file is opened. 
    //
    // By insisting that our process created the file we ensure that only the current user
    // and local system can access the memory mapped file thus avoiding the exploit described above. 


    if (GetLastError() == ERROR_ALREADY_EXISTS)
    { 
        CloseHandle(_hMemoryMappedFile); // Ignoring return value. 
        _hMemoryMappedFile = NULL; 

        IFC(E_FAIL);
    } 

    //
    // File mapping was created successfully. Now get a pointer to the memory
    // mapped file structure.
    //
    
    IFCW32((NULL != (_pFile = (MemoryMappedFile*)MapViewOfFile(
        _hMemoryMappedFile,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        0))));

    //
    // Initialize the memory mapped file.
    //
    ZeroMemory(_pFile, sizeof(MemoryMappedFile));


    //
    // Write the file header
    //
    _pFile->_dwVersion = DEBUGCONTROL_VERSION;

     
Cleanup:
    // This is called from the Create method which will cleanup the object if it fails.
    
    return hr;
}

//---------------------------------------------------------------------------------
// CMediaControl::InitializeAttach
//---------------------------------------------------------------------------------

HRESULT
CMediaControl::InitializeAttach(
    __in PCWSTR lpName)
{
    HRESULT hr = S_OK;

    //
    // Try to open the memory mapped file.
    //

    IFCW32((NULL != (_hMemoryMappedFile = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        lpName))));  

    //
    // Now map the whole memory mapped file.
    //

    IFCW32((NULL != (_pFile = (MemoryMappedFile*)MapViewOfFile(
        _hMemoryMappedFile,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        0))));

    //
    // Check if this version is compatible.
    //

    if (_pFile->_dwVersion != DEBUGCONTROL_VERSION)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // It would be good to get the memory mapped file size here and verify that
    // it matches our expectations. However, GetFileSizeEx doesn't seem to be compatible
    // with memory mapped files.

    
Cleanup:
    // This is called from the Attach method which will cleanup the object if it fails.
    
    return hr;
}

//---------------------------------------------------------------------------------
// CMediaControl::Create
//---------------------------------------------------------------------------------

/* static */
__checkReturn HRESULT 
CMediaControl::Create(
    __in PCWSTR lpName,
    __deref_out_ecount(1) CMediaControl** ppMediaControl)
{
    HRESULT hr = S_OK;
    CMediaControl* pCC = NULL;
    
    if ((ppMediaControl == NULL) ||
        (lpName == NULL))
    {
        IFC(E_INVALIDARG);
    }

    pCC = new CMediaControl();
    if (pCC == NULL)
    {
        IFC(E_OUTOFMEMORY);
    }
    
    IFC(pCC->Initialize(lpName));

    *ppMediaControl = pCC;
    pCC = NULL;

Cleanup:
    delete pCC; 
    return hr;
}

//---------------------------------------------------------------------------------
// CMediaControl::Attach
//---------------------------------------------------------------------------------

/* static */
__checkReturn HRESULT 
CMediaControl::Attach(
    __in PCWSTR lpName,
    __deref_out_ecount(1) CMediaControl** ppMediaControl)
{
    HRESULT hr = S_OK;
    CMediaControl* pCC = NULL;
    
    if ((ppMediaControl == NULL) ||
        (lpName == NULL))
    {
        IFC(E_INVALIDARG);
    }

    pCC = new CMediaControl();

    if (pCC == NULL)
    {
        IFC(E_OUTOFMEMORY);
    }
    
    IFC(pCC->InitializeAttach(lpName));

    *ppMediaControl = pCC;
    pCC = NULL;

Cleanup:
    delete pCC; 
    return hr;

}
//---------------------------------------------------------------------------------
// CMediaControl::~CMediaControl
//---------------------------------------------------------------------------------

/* virtual */
CMediaControl::~CMediaControl()
{
    UnmapViewOfFile(_pFile);
    _pFile = NULL;

    CloseHandle(_hMemoryMappedFile); // Ignoring return value. 
    _hMemoryMappedFile = NULL;
}

//---------------------------------------------------------------------------------
// CMediaControl::GetDataPtr
//---------------------------------------------------------------------------------

__out_ecount(1) CMediaControlFile* 
CMediaControl::GetDataPtr()
{
    return &(_pFile->_data);
}

//---------------------------------------------------------------------------------
// CMediaControl::UpdateMaxValue
//---------------------------------------------------------------------------------

void
CMediaControl::UpdateMaxValuePair(
    __inout_ecount(1) DWORD* pdwMaxValue,
    __inout_ecount(1) DWORD* pdwCurrentValue
    )
{
    // Check if we need to update our maximum value
    if (*pdwCurrentValue > *pdwMaxValue)
    {
        InterlockedExchange(reinterpret_cast<volatile LONG *>(pdwMaxValue),
                            static_cast<LONG>(*pdwCurrentValue));
    }

    // Now that we've updated the maximum value, reset the current counter to 0
    InterlockedExchange(reinterpret_cast<volatile LONG *>(pdwCurrentValue), 0);
}

//---------------------------------------------------------------------------------
// CMediaControl::ResetPerFrameCounters
//---------------------------------------------------------------------------------

void
CMediaControl::UpdatePerFrameCounters()
{
    CMediaControlFile* pFile = &_pFile->_data;

    UpdateMaxValuePair(
        &pFile->NumHardwareIntermediateRenderTargetsMax,
        &pFile->NumHardwareIntermediateRenderTargets
        );
    
    UpdateMaxValuePair(
        &pFile->NumSoftwareIntermediateRenderTargetsMax,
        &pFile->NumSoftwareIntermediateRenderTargets
        );
}

//---------------------------------------------------------------------------------
// CMediaControl::TintARGBBitmap
//---------------------------------------------------------------------------------

void
CMediaControl::TintARGBBitmap(
    __inout_bcount(cbStride*(uHeight-1)+uWidth*sizeof(ARGB)) ARGB * pBitmap,
    UINT uWidth,
    UINT uHeight,
    UINT cbStride
    )
{

    // For now we always tint purple, but this may change later
    
    if (   pBitmap != NULL
        && cbStride % sizeof(ARGB) == 0
        && uWidth * sizeof(ARGB) <= cbStride
       )
    {
        GpCC *pScanline = reinterpret_cast<GpCC *>(pBitmap);

        UINT cPixelsStride = cbStride / sizeof(ARGB);

        for (UINT y = 0;
             y < uHeight;
             y++,
             pScanline += cPixelsStride)
        {
            GpCC *pColors = pScanline;

            for (UINT x = 0;
                 x < uWidth;
                 x++,
                 pColors++)
            {
                // Set up references to our ARGB pixel, for easy readability
                BYTE & b = pColors->b;
                BYTE & g = pColors->g;
                BYTE & r = pColors->r;
                BYTE & a = pColors->a;

                // Only recolor if the tint isn't already purple
                if (   g != 0
                    || r != b
                    || r < 102
                    || a < 85
                   )
                {
                    // Always between 102 and 255.
                    r = static_cast<BYTE>((static_cast<UINT>(r) + g + b)/5 + 102);

                    b = r;
                    g = 0;

                    // Always between 85 and 255. Doesn't change fully opaque.
                    a = static_cast<BYTE>(static_cast<UINT>(a) * 2 / 3 + 85);
                }
            }
        }
    }
}


//---------------------------------------------------------------------------------
// CMediaControl::CanAttach
//---------------------------------------------------------------------------------

/* static */
BOOL 
CMediaControl::CanAttach(__in PCWSTR lpName)
{
    HRESULT hr = S_OK;
    HANDLE hFileMapping = NULL;
    const MemoryMappedFile* pFile = NULL;

    if (lpName == NULL)
    {
        IFC(E_INVALIDARG);
    }

    //
    // Try to open the file.
    //
    
    hFileMapping = OpenFileMapping(
        FILE_MAP_ALL_ACCESS,   // read/write access
        FALSE,                 // do not inherit the name
        lpName); 

    if (hFileMapping == NULL)
    {
        IFC(E_FAIL);
    }

    //
    // Now map the memory mapped file.
    //
    
    pFile = (MemoryMappedFile*)MapViewOfFile(
        hFileMapping,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        0);

    if (pFile == NULL)
    {
        IFC(E_FAIL);
    }

    //
    // Check version.
    //
    
    if (pFile->_dwVersion != DEBUGCONTROL_VERSION)
    {
        IFC(E_FAIL);
    }

Cleanup:
    UnmapViewOfFile(pFile);
    CloseHandle(hFileMapping);

    return SUCCEEDED(hr);
}
 


