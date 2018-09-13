/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
#ifndef _FILESTREAM_HXX
#define _FILESTREAM_HXX

#include <stdio.h>
#include "unknown.hxx"

class FileStream : public _unknown<IStream>
{
public:
	FileStream() 
	{ 
        hFile = NULL;
        read = true;
	}

	~FileStream() 
	{ 
		::CloseHandle(hFile);
	}

    bool open(char* name, bool read = true)
    {
        this->read = read;
        long len;
        for (len = 0; name[len] != 0; len++);
        WCHAR* name2 = new WCHAR[len+1];
        for (len = 0; name[len] != 0; len++)
            name2[len] = name[len];
        name2[len] = '\0';  
        
        if (read)
        {
		    hFile = ::CreateFile( 
                name2,
			    GENERIC_READ,
			    FILE_SHARE_READ,
			    NULL,
			    OPEN_EXISTING,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);
        }
        else
        {
		    hFile = ::CreateFile(
			    name2,
			    GENERIC_WRITE,
			    FILE_SHARE_READ,
			    NULL,
			    CREATE_ALWAYS,
			    FILE_ATTRIBUTE_NORMAL,
			    NULL);
        }
        delete[] name2;
        return (hFile == INVALID_HANDLE_VALUE) ? false : true;
    }

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Read( 
        /* [out] */ void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbRead)
	{	
        if (!read) return E_FAIL;

        DWORD len;
		BOOL rc = ReadFile(
			hFile,	// handle of file to read 
			pv,	// address of buffer that receives data  
			cb,	// number of bytes to read 
			&len,	// address of number of bytes read 
			NULL 	// address of structure for data 
		   );
        if (pcbRead)
            *pcbRead = len;
		return (rc) ? S_OK : E_FAIL;
	}
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Write( 
        /* [size_is][in] */ const void __RPC_FAR *pv,
        /* [in] */ ULONG cb,
        /* [out] */ ULONG __RPC_FAR *pcbWritten)
	{
        if (read) return E_FAIL;

		BOOL rc = WriteFile(
			hFile,	// handle of file to write 
			pv,	// address of buffer that contains data  
			cb,	// number of bytes to write 
			pcbWritten,	// address of number of bytes written 
			NULL 	// address of structure for overlapped I/O  
		   );

		return (rc) ? S_OK : E_FAIL;
	}

    virtual /* [local] */ HRESULT STDMETHODCALLTYPE Seek( 
        /* [in] */ LARGE_INTEGER dlibMove,
        /* [in] */ DWORD dwOrigin,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE SetSize( 
        /* [in] */ ULARGE_INTEGER libNewSize) { return E_FAIL; }
    
    virtual /* [local] */ HRESULT STDMETHODCALLTYPE CopyTo( 
        /* [unique][in] */ IStream __RPC_FAR *pstm,
        /* [in] */ ULARGE_INTEGER cb,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbRead,
        /* [out] */ ULARGE_INTEGER __RPC_FAR *pcbWritten) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Commit( 
        /* [in] */ DWORD grfCommitFlags) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Revert( void) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE LockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE UnlockRegion( 
        /* [in] */ ULARGE_INTEGER libOffset,
        /* [in] */ ULARGE_INTEGER cb,
        /* [in] */ DWORD dwLockType) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Stat( 
        /* [out] */ STATSTG __RPC_FAR *pstatstg,
        /* [in] */ DWORD grfStatFlag) { return E_FAIL; }
    
    virtual HRESULT STDMETHODCALLTYPE Clone( 
        /* [out] */ IStream __RPC_FAR *__RPC_FAR *ppstm) { return E_FAIL; }
private:
	HANDLE hFile;
    bool read;
};

#endif // _FILESTREAM_HXX
