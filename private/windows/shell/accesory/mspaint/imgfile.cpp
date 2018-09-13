// File import/export for icons, cursors, and bitmaps

#include "stdafx.h"

#include <direct.h>
#include <sys\stat.h>

#include "global.h"
#include "pbrush.h"
#include "pbrusdoc.h"
#include "pbrusfrm.h"
#include "bmobject.h"
#include "imgsuprt.h"
#include "imgwnd.h"
#include "imgfile.h"
#include "ferr.h"
#include "cmpmsg.h"

#ifdef _DEBUG
#undef THIS_FILE
static CHAR BASED_CODE THIS_FILE[] = __FILE__;
#endif

#include "memtrace.h"

// base for temp file names
const TCHAR BASED_CODE CFileSaver::c_szAps [] = TEXT("TMP");

/***************************************************************************/

BOOL FileExists(const TCHAR* szFileName)
    {
    CFileStatus fs;
    return CFile::GetStatus(szFileName, fs) != 0;
    }

/***************************************************************************/

BOOL RenameFile(const TCHAR* szOldName, const TCHAR* szNewName)
    {
    TRACE2("RenameFile: \"%s\" to \"%s\"\n", szOldName, szNewName);

    TRY
        {
        if (FileExists(szNewName))
            CFile::Remove(szNewName);
        CFile::Rename(szOldName, szNewName);
        }
    CATCH(CFileException, e)
        {
        TRACE(TEXT("Rename failed!\n"));
        theApp.SetFileError( IDS_ERROR_EXPORT, e->m_cause );
        return FALSE;
        }
    END_CATCH

    return TRUE;
    }

/***************************************************************************/

CFileSaver::CFileSaver(const TCHAR* szFileName)
    {
    CString strDir = StripName(szFileName);
    CString strTempDir;

    GetTempPath( _MAX_PATH, strTempDir.GetBuffer( _MAX_PATH ) );

    strTempDir.ReleaseBuffer();

    GetTempFileName( strTempDir, c_szAps, 0, m_strTempName.GetBuffer( _MAX_PATH ));
    GetTempFileName( strTempDir, c_szAps, 0, m_strBackupName.GetBuffer( _MAX_PATH ));

    CFile::Remove( m_strTempName );
    CFile::Remove( m_strBackupName );

    m_strName = szFileName;
    m_strTempName = strDir + StripPath( m_strTempName );
    m_strBackupName = strDir + StripPath( m_strBackupName );
    }

/***************************************************************************/

CFileSaver::~CFileSaver()
    {
    if (FileExists(m_strTempName))
        CFile::Remove(m_strTempName);

    if (FileExists(m_strBackupName))
        CFile::Remove(m_strBackupName);
    }

/***************************************************************************/

BOOL CFileSaver::CanSave() const
    {
    CFileStatus fs;

    if ( CFile::GetStatus(m_strName, fs) != 0 )
                {
                if ((fs.m_attribute & CFile::readOnly) != 0)
                {
                theApp.SetFileError( IDS_ERROR_SAVE, ferrCantSaveReadOnly);
                return FALSE;
                }
                }

    return TRUE;
    }

/***************************************************************************/

BOOL CFileSaver::Finish()
    {
    if (FileExists(m_strName) != 0)
        {
        if (!RenameFile(m_strName, m_strBackupName))
            {
            CFile::Remove(m_strTempName);
            return FALSE;
            }
        }
    else
        {
        // no backup was made since the "original" didn't exists,
        // wipe out the name so we don't delete the file later...

        m_strBackupName.Empty();
        }

    if (!RenameFile(m_strTempName, m_strName))
        {
        if (!m_strBackupName.IsEmpty() &&
            RenameFile(m_strBackupName, m_strName))
            {
            CFile::Remove(m_strTempName);
            }

        return FALSE;
        }

    if (!m_strBackupName.IsEmpty())
        CFile::Remove(m_strBackupName);

    return TRUE;
    }

/***************************************************************************/
/* strrchrs() -- find the last instance in a string of any one of
**  a set of characters.  Return a pointer into the string at
**  the matchpoint.  Analogous to strrchr() in the CRT.
*/

TCHAR *strrchrs(TCHAR *szText, TCHAR * szSSet)
    {
    register TCHAR *pchSSet;
    register TCHAR *pchStep;
    register TCHAR *pchLast = NULL;

    if ((NULL == szText) || (NULL == szSSet))
        return NULL;
    for (pchStep = szText; TEXT('\0') != *pchStep; pchStep = CharNext(pchStep))
        for (pchSSet = szSSet; TEXT('\0') != *pchSSet; pchSSet = CharNext(pchSSet))
            if ((pchSSet[0] == pchStep[0]) && (
               #ifndef UNICODE
                !IsDBCSLeadByte((CHAR)pchSSet[0]) ||
               #endif // UNICODE
               (pchSSet[1] == pchStep[1])))
                    pchLast = pchStep;
    return pchLast;
    }

/***************************************************************************/
/* MkPath() -- Make any directories necessary to ensure that a
**  directory name passed in exists.  Essentially, if the
**  argument exists and is a directory, return success.  If
**  not, strip off the last path component and recurse,
**  creating the directories on returning up the stack.
*/
int MkPath(TCHAR *szPath)
    {
    TCHAR *pchSlash;
    TCHAR chSep;
    DWORD dwAtts;

    //Does it exist?
    if ( (dwAtts = GetFileAttributes(szPath)) & FILE_ATTRIBUTE_DIRECTORY )
        {
        return 0;
        }
    else if (-1 != dwAtts)
        {
        return -1;
        }

    //Can we create it?
    else
        {
        if ( CreateDirectory(szPath, NULL))
            return 0;
        // are we out of path components?
        else
            {
            if (NULL == (pchSlash = strrchrs(szPath, TEXT("\\/"))))
                return -1;
            // Can we make its parent directory?
            else
                                {
                if ((chSep = *pchSlash), (*pchSlash = TEXT('\0')), MkPath(szPath))
                    {
                    #ifndef DEBUG
                    *pchSlash = chSep;
                    #endif
                    return -1;
                    }
                // Can we make it now that we've made its parent?
                else
                                        {
                    if ((*pchSlash = chSep), (TEXT('\0') != pchSlash[1]))
                        {
                        if (!CreateDirectory (szPath, NULL))
                        {
                           return -1;
                        }
                        return 0;
                        }
                    else //don't try trailing slash
                        return 0;
                                        }
                                }
                        }
                }
    }

/***************************************************************************/

void MkFullPath(CString& strFullPath, const CString& strRelPath,
    BOOL bPathOnly)
    {
    strFullPath.Empty();

    ASSERT(strRelPath.GetLength() > 0);
    if (strRelPath[0] != TEXT('\\') &&
        (strRelPath.GetLength() <= 1 || (
        #ifndef UNICODE
        !IsDBCSLeadByte((CHAR)strRelPath[0]) &&
        #endif // UNICODE
         strRelPath[1] != TEXT(':'))))
        {
        CHAR *szPathName = _getdcwd(0, NULL, 1);
        #ifdef UNICODE
        WCHAR *szPW = new WCHAR[lstrlenA (szPathName)+1];
        AtoW (szPathName, szPW);
        strFullPath = szPW;
        delete szPW;
        #else
        strFullPath = szPathName;
        #endif //UNICODE
        free(szPathName);

        if (strFullPath.Right(1) != TEXT('\\'))
            strFullPath += (TCHAR)TEXT('\\');
        }

    if (bPathOnly)
        {
        int iLastSep = strRelPath.ReverseFind(TEXT('\\'));
        if (iLastSep == -1)
            iLastSep = strRelPath.GetLength();
        strFullPath += strRelPath.Left(iLastSep);
        }
    else
        {
        strFullPath += strRelPath;
        }
    }

/***************************************************************************/

BOOL OpenSubFile(CFile& file, const CFileSaver& saver, UINT nOpenFlags,
                CFileException* pError)
    {
    BOOL bResult = file.Open(saver.GetSafeName(), nOpenFlags, pError);

    if (!bResult && (pError->m_cause == CFileException::badPath ||
            pError->m_cause == CFileException::accessDenied))
        {
        CString strFileName = saver.GetRealName();
        CString strPathName;

        MkFullPath(strPathName, strFileName, TRUE);
        strPathName.MakeUpper();

        // suppress the message box upon return, but keep error info!
        // (ie. user only needs one message box)
        pError->m_cause = -pError->m_cause;

        strFileName.MakeUpper();
        int nResult = CmpMessageBoxPrintf(IDS_QUERY_MKDIR, AFX_IDS_APP_TITLE,
            MB_YESNO | MB_ICONQUESTION,
            (LPCTSTR)strPathName, (LPCTSTR) strFileName);
        if (nResult == IDYES)
            {
            MkPath(strPathName.GetBuffer(strPathName.GetLength()));
            strPathName.ReleaseBuffer();
            bResult = file.Open(saver.GetSafeName(), nOpenFlags, pError);
            }
        }

    return bResult;
    }

/***************************************************************************/

