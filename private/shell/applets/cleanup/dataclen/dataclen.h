/*
**------------------------------------------------------------------------------
** Module:  System data driven cleaner
** File:    dataclen.h
**
** Purpose: Defines the 'System data driven cleaner' for the IEmptyVolumeCache
**          interface
** Notes:
** Mod Log: Created by Jason Cobb (2/97)
**
** Copyright (c)1997 Microsoft Corporation, All Rights Reserved
**------------------------------------------------------------------------------
*/
#ifndef DATACLEN_H
#define DATACLEN_H

#ifndef COMMON_H
    #include "common.h"
#endif

/*
**------------------------------------------------------------------------------
** Project include files 
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Class Declarations
**------------------------------------------------------------------------------
*/

/*
**------------------------------------------------------------------------------
** Class:   CDataDrivenCleanerClassFactory
** Purpose: Manufactures instances of our CDataDrivenCleaner object
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/ 
class CCleanerClassFactory : public IClassFactory
{
private:
protected:
	//
	// Data
	//
	ULONG   _cRef;     // Reference count
    DWORD   _dwID;     // what type of class factory are we?

public:
	//
	// Constructors
	//
	CCleanerClassFactory(DWORD);
	~CCleanerClassFactory();
	
    //
	// IUnknown interface members
	//
    STDMETHODIMP            QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

	//   
	// IClassFactory interface members
    //
    STDMETHODIMP            CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP            LockServer(BOOL);
};

typedef CCleanerClassFactory *LPCCLEANERCLASSFACTORY;


/*
**------------------------------------------------------------------------------
** Class:   CDataDrivenCleaner
** Purpose: This is the actual Data Driven Cleaner Class
** Notes:   
** Mod Log: Created by Jason Cobb (2/97)
**------------------------------------------------------------------------------
*/ 
class CDataDrivenCleaner : public IEmptyVolumeCache
{
private:
protected:
	//
    // Data
	//
	ULONG               _cRef;                 // reference count
	LPDATAOBJECT        _lpdObject;            // Last Data object

    ULARGE_INTEGER      _cbSpaceUsed;
    ULARGE_INTEGER		_cbSpaceFreed;

    FILETIME			_ftMinLastAccessTime;

    TCHAR               _szVolume[MAX_PATH];
	TCHAR               _szFolder[MAX_PATH];
	DWORD               _dwFlags;
	TCHAR               _filelist[MAX_PATH];
	TCHAR				_CleanupString[MAX_PATH];
	BOOL				_bPurged;				// TRUE if Purge() method was run

	PCLEANFILESTRUCT    _head;                   // head of the linked list of files

	BOOL WalkForUsedSpace(PTCHAR lpPath, IEmptyVolumeCacheCallBack *picb);
	BOOL WalkAllFiles(PTCHAR lpPath, IEmptyVolumeCacheCallBack *picb);
	BOOL AddFileToList(PTCHAR lpFile, ULARGE_INTEGER filesize, BOOL bDirectory);
	void PurgeFiles(IEmptyVolumeCacheCallBack *picb, BOOL bDoDirectories);
	void FreeList(PCLEANFILESTRUCT pCleanFile);
	BOOL bLastAccessisOK(FILETIME ftFileLastAccess);
	BOOL bW98ProcessIsRunning(HKEY hRegKey);


public:
	//
    // Constructors
	//
	CDataDrivenCleaner (void);
	~CDataDrivenCleaner (void);

	//
    // IUnknown interface members
	//
	STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)    AddRef (void);
	STDMETHODIMP_(ULONG)    Release (void);
	
    //
    // IEmptyVolumeCache interface members
    //
    STDMETHODIMP    Initialize(
                                HKEY hRegKey,
                                LPCWSTR pszVolume,
                                LPWSTR *ppszDisplayName,
                                LPWSTR *ppszDescription,
                                DWORD *pdwFlags
                                );
                                
    STDMETHODIMP    GetSpaceUsed(
                                DWORDLONG *pdwSpaceUsed,
                                IEmptyVolumeCacheCallBack *picb
                                );
                                
    STDMETHODIMP    Purge(
                                DWORDLONG dwSpaceToFree,
                                IEmptyVolumeCacheCallBack *picb
                                );
                                
    STDMETHODIMP    ShowProperties(
                                HWND hwnd
                                );
                                
    STDMETHODIMP    Deactivate(
                                DWORD *pdwFlags
                                );                                                                                                                                
};

typedef CDataDrivenCleaner *LPCDATADRIVENCLEANER;

/*
**------------------------------------------------------------------------------
** Class:   CDataDrivenPropBag
** Purpose: This is the property bag used to allow string localization for the
**          default data cleaner.  This class implements multiple GUIDs each of
**          which will return different values for the three valid properties.
** Notes:   
** Mod Log: Created by ToddB (9/98)
**------------------------------------------------------------------------------
*/ 
class CDataDrivenPropBag : public IPropertyBag
{
private:
protected:
	//
    // Data
	//
	ULONG               _cRef;                 // reference count

    // We use this object for several different property bags.  Based on the CLSID used
    // to create this object we set the value of _dwFilter to a known value so that we
    // know which property bag we are.
	DWORD               _dwFilter;

public:
	//
    // Constructors
	//
	CDataDrivenPropBag (DWORD);
	~CDataDrivenPropBag (void);

	//
    // IUnknown interface members
	//
	STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)    AddRef (void);
	STDMETHODIMP_(ULONG)    Release (void);
	
    //
    // IPropertyBag interface members
    //
    STDMETHODIMP            Read (LPCOLESTR, VARIANT *, IErrorLog *);
    STDMETHODIMP            Write (LPCOLESTR, VARIANT *);
};

typedef CDataDrivenPropBag *LPCDATADRIVENPROPBAG;

#ifdef WINNT
class CContentIndexCleaner : public IEmptyVolumeCache
{
    private:
        IEmptyVolumeCache * _pDataDriven;
        LONG _cRef;
        
    public:
    	//
        // Constructors
    	//
    	CContentIndexCleaner(void);
    	~CContentIndexCleaner(void);

    	//
        // IUnknown interface members
    	//
    	STDMETHODIMP            QueryInterface (REFIID, LPVOID FAR *);
    	STDMETHODIMP_(ULONG)    AddRef (void);
    	STDMETHODIMP_(ULONG)    Release (void);
    	
        //
        // IEmptyVolumeCache interface members
        //
        STDMETHODIMP    Initialize(
                                    HKEY hRegKey,
                                    LPCWSTR pszVolume,
                                    LPWSTR *ppszDisplayName,
                                    LPWSTR *ppszDescription,
                                    DWORD *pdwFlags
                                    );
                                    
        STDMETHODIMP    GetSpaceUsed(
                                    DWORDLONG *pdwSpaceUsed,
                                    IEmptyVolumeCacheCallBack *picb
                                    );
                                    
        STDMETHODIMP    Purge(
                                    DWORDLONG dwSpaceToFree,
                                    IEmptyVolumeCacheCallBack *picb
                                    );
                                    
        STDMETHODIMP    ShowProperties(
                                    HWND hwnd
                                    );
                                    
        STDMETHODIMP    Deactivate(
                                    DWORD *pdwFlags
                                    );                                                                                                                                
            
};

typedef CContentIndexCleaner *LPCCONTENTINDEXCLEANER;

#endif // WINNT
#endif // DATACLEN_H
/*
**------------------------------------------------------------------------------
** End of File
**------------------------------------------------------------------------------
*/
