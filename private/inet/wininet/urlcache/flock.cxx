#include "flock.hxx"
#include <resource.h>
#include <cache.hxx>

#undef inet_ntoa
#undef inet_addr
#undef gethostname
#undef gethostbyname
#undef gethostbyaddr

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef sunos5
extern "C" int gethostname(char*,int);
#endif

extern HANDLE MwOpenProcess(pid_t, BOOL);
extern "C" MwAtExit(void (*f)(void));

//locally used functions
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len);
off_t lock_test(int fd, int type, off_t *offset, int whence, off_t *len);

#define REG_READONLYCACHE TEXT("Software\\Microsoft\\Internet Explorer\\Unix\\ReadOnlyCacheWarning")
#define REG_READONLYCACHEKEY TEXT("ShowCacheWarning")
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SECONDSINDAY 86400

#define REG_READONLYCACHE TEXT("Software\\Microsoft\\Internet Explorer\\Unix\\ReadOnlyCacheWarning")
#define REG_READONLYCACHEKEY TEXT("ShowCacheWarning")
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#define SECONDSINDAY 86400


// lock region relative to whence starting at offset upto len bytes
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len) {

  struct flock lock;

  lock.l_type = type;       //F_RDLCK, F_WRLCK, F_UNLCK     
  lock.l_start = offset;    //byte offset, relative to l_whence
  lock.l_whence = whence;   //SEEK_SET, SEEK_CUR, SEEK_END  
  lock.l_len = len;     //#bytes (0 means to EOF)

  return( fcntl(fd, cmd, &lock) );
}

// test region for locks relative to whence starting at offset for len bytes
off_t lock_test(int fd, int type, off_t *offset, int whence, off_t *len) {

  struct flock lock;

  lock.l_type = type;       //F_RDLCK, F_WRLCK, F_UNLCK 
  lock.l_start = *offset;   //byte offset, relative to l_whence     
  lock.l_whence = whence;   //SEEK_SET, SEEK_CUR, SEEK_END  
  lock.l_len = *len;        // #bytes (0 means to EOF)      

  if (fcntl(fd, F_GETLK, &lock) < 0)
    return(-1);

  if (lock.l_type == F_UNLCK)
    return(0);          // nobody has lock in this region
  else if (lock.l_type == F_RDLCK) {
    *offset = lock.l_start;
    *len = lock.l_len;
    return(lock.l_start);   // byte offset of host with read lock
  } else {          // dont support extended semantics of
    return(-1);         // write lock yet
  }
}


extern "C" void unixCleanupWininetCacheLockFile()
{
//    if(!g_ReadOnlyCaches)
        //unlink(szLockDBName);
}

BOOL CreateAtomicCacheLockFile(BOOL *pfReadOnlyCaches, char **pszLockingHost)
{
    int fdlockdbf, fdlock, envLen, hostbynameerr;
    off_t IPOffset=0, IPLen=0, ownIPOffset, ownIPLen;
    char *hostname, hostbynamebuf[512];
    char szLockFileName[MAX_PATH+1], szLockDBName[MAX_PATH+1];
    struct hostent hostbynameresult;
#ifdef ux10
    struct hostent_data hostentdata;
#endif

    char *pEnv = getenv("MWUSER_DIRECTORY");

    /* Don't process the ielock file for Mainwin Lite programs */
    if (MwIsInitLite())
       goto Cleanup;

    if (pEnv == 0)
    return FALSE;

    envLen = strlen(pEnv);
    if (envLen > MAX_PATH-256)
        return FALSE;

    strcpy(szLockFileName, pEnv);
    if (szLockFileName[envLen-1] != '/') {
      szLockFileName[envLen] = '/';
      szLockFileName[envLen+1] = 0x00;
    }
    strcpy(szLockDBName, pEnv);
    if (szLockDBName[envLen-1] != '/') {
      szLockDBName[envLen] = '/';
      szLockDBName[envLen+1] = 0x00;
    }
    strcat(szLockFileName, LF);
    strcat(szLockDBName, LOCKDBF);

    hostname = (char *)malloc(256*sizeof(char));
    if ((hostname == NULL) || (gethostname(hostname, 256) == -1)) {
      *pfReadOnlyCaches = TRUE;
      return FALSE;
    }

#ifdef sunos5
    if (!(gethostbyname_r(hostname, &hostbynameresult, hostbynamebuf,
                 sizeof(hostbynamebuf), &hostbynameerr))) {
      *pfReadOnlyCaches = TRUE;
      return FALSE;
    }
#endif
#ifdef ux10
    if (gethostbyname_r(hostname, &hostbynameresult, &hostentdata) < 0) {
      *pfReadOnlyCaches = TRUE;
      return FALSE;
    }
#endif
    struct in_addr *ptr = (struct in_addr *)*hostbynameresult.h_addr_list;
    ownIPOffset = inet_netof(*ptr);
    ownIPLen = inet_lnaof(*ptr);

    if ((fdlock = open(szLockFileName, O_WRONLY|O_CREAT|O_EXCL, FILE_MODE)) < 0) {
      if (errno == EEXIST) {
        if ((fdlock = open(szLockFileName, O_WRONLY)) < 0) {
          *pfReadOnlyCaches = TRUE;
          return FALSE;
        }
      } else {
        *pfReadOnlyCaches = TRUE;
        return FALSE;
      }
    }

    if (writew_lock(fdlock, 0, SEEK_SET, 0) < 0) {
      *pfReadOnlyCaches = TRUE;
      return FALSE;
    }

    /*under this lock, now do all the examination of szLockDBName*/
    if ((fdlockdbf = open(szLockDBName, O_RDWR|O_CREAT|O_EXCL, FILE_MODE)) < 0) {
      if (errno == EEXIST) {
        if ((fdlockdbf = open(szLockDBName, O_RDWR)) < 0) {
          *pfReadOnlyCaches = TRUE;
          un_lock(fdlock, 0, SEEK_SET, 0);
          return FALSE;
        }
      } else {
        *pfReadOnlyCaches = TRUE;
        un_lock(fdlock, 0, SEEK_SET, 0);
        return FALSE;
      }
    }

    /* check entire file for locking */
    if ((can_writelock(fdlockdbf, &IPOffset, SEEK_SET, &IPLen)) >= 0) {
      if ((IPOffset == 0) || ((IPOffset == ownIPOffset) && (IPLen == ownIPLen))){
    // either no IE writing to cache or IE on own host writing to cache
    // (IP address is identical)..either way we have write access
    *pfReadOnlyCaches = FALSE;
    *pszLockingHost = hostname;
    //lock at "network part" position for "host part" bytes
    read_lock(fdlockdbf, ownIPOffset, SEEK_SET, ownIPLen);
    un_lock(fdlock, 0, SEEK_SET, 0);
    return TRUE;
      } else {
    //some other host writing to cache
        *pfReadOnlyCaches = TRUE;
        u_long addr = inet_addr(inet_ntoa(inet_makeaddr(IPOffset, IPLen)));
        struct hostent * hp = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
        if (!hp)
          ;       //cant find hostname from offset & length of locked bytes..
        else
          *pszLockingHost = hp->h_name;
        un_lock(fdlock, 0, SEEK_SET, 0);
        return TRUE;
      }
    } else {
      //can_writelock returned -1 with some fcntl error
      *pfReadOnlyCaches = TRUE;
      un_lock(fdlock, 0, SEEK_SET, 0);
      return FALSE;
    }

Cleanup:
    return TRUE;
}

BOOL DeleteAtomicCacheLockFile()
{
    /* Don't process for MainWin Lite programs */
    /* Right now, the code below does not make sense because all
     * we do is return TRUE. So, commenting out this code for now.
     */
#if 0
    if (MwIsInitLite())
       goto Cleanup;

Cleanup:
#endif /* 0 */
    //unlink(szLockDBName);
    return TRUE;
}

#if 0 // Back out till we get a consensus on this

BOOL CALLBACK ReadOnlyCache_DlgProc(HWND   hDlg,
                                    UINT   uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam) {
     switch (uMsg) {
            case WM_INITDIALOG:
            {
                 LPTSTR lpszMessageStr = (LPTSTR)lParam;
                 TCHAR  pszText[MAX_PATH];
                 TCHAR  pszFormattedText[MAX_PATH];

                 if (lpszMessageStr)
                 {
                    if (LoadString(GlobalDllHandle,
                                   IDS_READONLYCACHE,
                                   pszText,
                                   ARRAYSIZE(pszText))) {
                       wsprintf(pszFormattedText,pszText, lpszMessageStr);
                       SetDlgItemText(hDlg, IDC_READONLYCACHE, pszFormattedText);
                    }
                 }

                 SetFocus(GetDlgItem(hDlg, IDOK));
            }
            break;

            case WM_COMMAND:
                 switch (LOWORD(wParam))
                 {
                        case IDOK:
                        {
                             if (IsDlgButtonChecked(hDlg, IDC_DONT_WANT_WARNING))
                                EndDialog(hDlg, 1);
                             else
                                EndDialog(hDlg, 0);
                             break;
                        }

                        default:
                             return FALSE;
                 }
                 return TRUE;
            case WM_CLOSE:
            {
                 if (IsDlgButtonChecked(hDlg, IDC_DONT_WANT_WARNING))
                    EndDialog(hDlg, 1);
                 else
                    EndDialog(hDlg, 0);
            }
            return TRUE;
     }

     return FALSE;
}

void ShowReadOnlyCacheDialog(char* pszHostName) {
     DWORD dwError = E_FAIL;
     HKEY  hKey = NULL;
     DWORD dwValue = 0;
     DWORD dwValueType;
     DWORD dwValueSize = sizeof(DWORD);

     if ((dwError = REGOPENKEYEX(HKEY_CURRENT_USER,
                            REG_READONLYCACHE,
                            0,
                            KEY_READ|KEY_WRITE,
                            &hKey)) != ERROR_SUCCESS)
     {
        goto Cleanup;
     }

     if ((dwError = RegQueryValueEx(hKey,
                               REG_READONLYCACHEKEY,
                               0,
                               &dwValueType,
                               (LPBYTE)&dwValue,
                               &dwValueSize)) != ERROR_SUCCESS)
     {
        goto Cleanup;
     }

     if (dwValue)
     {
        int fRet = 0;

        if ((fRet = DialogBoxParam(GlobalDllHandle,
                              MAKEINTRESOURCE(IDD_READONLYCACHE),
                              NULL,
                              ReadOnlyCache_DlgProc,
                              (LPARAM)pszHostName)) < 0)
        {
           goto Cleanup;
        }

        /*
         * we are here, because the registry told us to show this dialog.
         * now, we check if fRet == TRUE, in which case we don't show this
     * dialog in the future. And, we update the registry.
         */

        if (fRet == 1) {
           /* ShowCacheWarning will be set to False in the registry */
           dwValue = 0;

           /*
            * we don't check for the error here, because we close the key next
            * and if we did not save successfully, we will show this dialog again
            */

           RegSetValueEx(hKey,
                         REG_READONLYCACHEKEY,
                         0,
                         dwValueType,
                         (LPBYTE)&dwValue,
                         dwValueSize);
        }
     }

Cleanup:

     if (hKey)
        REGCLOSEKEY(hKey);

     return;
}
#endif

