
#ifndef  _TCHAR_H_
#define _TCHAR_H_

#include <string.h>

#ifdef _UNICODE

   #define  __T(q)   L##q
   
   #ifndef _TCHAR_DEFINED
      #ifndef RC_INVOKED
         typedef  wchar_t  TCHAR;
         typedef wchar_t _TCHAR;
      #endif   /* Not RC_INVOKED */
      #define _TCHAR_DEFINED
   #endif
   
   #define _TINT wint_t
   #define _TSCHAR wchar_t
   #define _TUCHAR wchar_t
   #define _TXCHAR wchar_t
   #define _TEOF WEOF
   #define _tenviron _wenviron 
   #define _tfinddata_t _wfinddata_t 
   
   /* dirent structures and functions */
   #define _tdirent  _wdirent
   #define _TDIR     _WDIR
   #define _topendir _wopendir
   #define _tclosedir   _wclosedir
   #define _treaddir _wreaddir
   #define _trewinddir  _wrewinddir
   #define _ttelldir _wtelldir
   #define _tseekdir _wseekdir
   
   #define _ttoi64     _wtoi64
   #define _i64tot     _i64tow
   #define _ui64tot    _ui64tow
   #define _tcsnccoll  _wstrncoll
   #define _tcsncoll   _wstrncoll
   #define _tcsncicoll _wstrnicoll
   #define _tfindfirsti64  _wfindfirsti64
   #define _tfindnexti64   _wfindnexti64
   #define _tfinddatai64_t _wfinddatai64_t
   
   #define _tunlink    _wunlink
   #define _tgetdcwd   _wgetdcwd

   #define _fgettc fgetwc 
   #define _fgettchar _fgetwchar 
   #define _fgetts fgetws 
   #define _fputtc fputwc 
   #define _fputtchar _fputwchar 
   #define _fputts fputws 
   #define _ftprintf fwprintf 
   #define _ftscanf fwscanf 
   #define _gettc getwc 
   #define _gettchar getwchar 
   #define _getts getws 
   #define _istalnum iswalnum 
   #define _istalpha iswalpha 
   #define _istascii iswascii 
   #define _istcntrl iswcntrl 
   #define _istdigit iswdigit 
   #define _istgraph iswgraph 
   #define _istlead(c) 0
   #define _istleadbyte(c) 0
   #define _istlegal(c) 1
   #define _istlower iswlower 
   #define _istprint iswprint 
   #define _istpunct iswpunct 
   #define _istspace iswspace 
   #define _istupper iswupper 
   #define _istxdigit iswxdigit 
   #define _itot _itow 
   #define _ltot _ltow 
   #define _puttc putwc 
   #define _puttchar putwchar 
   #define _putts putws 
   #define _tmain wmain 
   #define _sntprintf _snwprintf 
   #define _stprintf swprintf 
   #define _stscanf  swscanf 
   #define _taccess _waccess 
   #define _tasctime _wasctime 
   #define _tccpy(d,s) (*(d)=*(s))
   #define _tchdir _wchdir 
   #define _tclen(c) 2
   #define _tchmod _wchmod 
   #define _tcreat  _wcreat 
   #define _tcscat  wcscat 
   #define _tcschr  wcschr 
   #define _tcsclen  wcslen 
   #define _tcscmp  wcscmp 
   #define _tcscoll  wcscoll 
   #define _tcscpy  wcscpy 
   #define _tcscspn wcscspn 
   #define _tcsdec _wcsdec 
   #define _tcsdup _wcsdup 
   #define _tcsftime wcsftime 
   #define _tcsicmp _wcsicmp 
   #define _tcsicoll _wcsicoll 
   #define _tcsinc _wcsinc 
   #define _tcslen wcslen 
   #define _tcslwr _wcslwr 
   #define _tcsnbcnt _wcnscnt 
   #define _tcsncat wcsncat 
   #define _tcsnccat wcsncat 
   #define _tcsncmp wcsncmp 
   #define _tcsnccmp wcsncmp 
   #define _tcsnccnt _wcsncnt 
   #define _tcsnccpy wcsncpy 
   #define _tcsncicmp _wcsnicmp 
   #define _tcsncpy wcsncpy 
   #define _tcsncset _wcsnset 
   #define _tcsnextc _wcsnextc 
   #define _tcsnicmp _wcsnicmp 
   #define _tcsnicoll _wcsnicoll 
   #define _tcsninc _wcsninc 
   #define _tcsnccnt _wcsncnt 
   #define _tcsnset _wcsnset 
   #define _tcspbrk wcspbrk 
   #define _tcsspnp _wcsspnp 
   #define _tcsrchr wcsrchr 
   #define _tcsrev _wcsrev 
   #define _tcsset _wcsset 
   #define _tcsspn wcsspn 
   #define _tcsstr wcsstr 
   #define _tcstod wcstod 
   #define _tcstok wcstok 
   #define _tcstol wcstol 
   #define _tcstoul wcstoul 
   #define _tcsupr _wcsupr 
   #define _tcsxfrm wcsxfrm 
   #define _tctime _wctime 
   #define _texecl _wexecl 
   #define _texecle _wexecle 
   #define _texeclp _wexeclp 
   #define _texeclpe _wexeclpe 
   #define _texecv _wexecv 
   #define _texecve _wexecve 
   #define _texecvp _wexecvp 
   #define _texecvpe _wexecvpe 
   #define _tfdopen _wfdopen 
   #define _tfindfirst _wfindfirst 
   #define _tfindnext _wfindnext 
   #define _tfopen _wfopen 
   #define _tfreopen _wfreopen 
   #define _tfsopen _wfsopen 
   #define _tfullpath _wfullpath 
   #define _tgetcwd _wgetcwd 
   #define _tgetenv _wgetenv 
   #define _tmain wmain 
   #define _tmakepath _wmakepath 
   #define _tmkdir _wmkdir 
   #define _tmktemp _wmktemp 
   #define _tperror _wperror 
   #define _topen _wopen 
   #define _totlower towlower 
   #define _totupper towupper 
   #define _tpopen _wpopen 
   #define _tprintf wprintf 
   #define _tremove _wremove 
   #define _trename _wrename 
   #define _trmdir _wrmdir 
   #define _tsearchenv _wsearchenv 
   #define _tscanf  wscanf 
   #define _tsetlocale _wsetlocale 
   #define _tsopen  _wsopen 
   #define _tspawnl  _wspawnl 
   #define _tspawnle  _wspawnle 
   #define _tspawnlp _wspawnlp 
   #define _tspawnlpe _wspawnlpe 
   #define _tspawnv _wspawnv 
   #define _tspawnve  _wspawnve 
   #define _tspawnvp  _wspawnvp
   #define _tspawnvpe  _wspawnvpe 
   #define _tsplitpath  _wsplitpath 
   #define _tstat  _wstat 
   #define _tstrdate _wstrdate 
   #define _tstrtime  _wstrtime 
   #define _tsystem  _wsystem 
   #define _ttempnam  _wtempnam 
   #define _ttmpnam  _wtmpnam 
   #define _ttoi  _wtoi 
   #define _ttol  _wtol 
   #define _tutime  _wutime 
   #define _tWinMain  wWinMain 
   #define _ultot _ultow 
   #define _ungettc ungetwc 
   #define _vftprintf vfwprintf 
   #define _vsntprintf _vsnwprintf 
   #define _vstprintf vswprintf 
   #define _vtprintf vwprintf 
   
   #define _wcsdec(_wcs1, _wcs2) ((_wcs1)>=(_wcs2) ? NULL : (_wcs2)-1)
   #define _wcsinc(_wcs)  ((_wcs)+1)
   #define _wcsnextc(_wcs) ((unsigned int) *(_wcs))
   #define _wcsninc(_wcs, _inc) (((_wcs)+(_inc)))
   #define _wcsncnt(_wcs, _cnt) ((wcslen(_wcs)>_cnt) ? _count : wcslen(_wcs))
   #define _wcsspnp(_wcs1, _wcs2) ((*((_wcs1)+wcsspn(_wcs1,_wcs2))) ? ((_wcs1)+wcsspn(_wcs1,_wcs2)) : NULL)
   
#else

   #define  __T(q)   q
   
   #ifndef _TCHAR_DEFINED
      #ifndef RC_INVOKED
         typedef char   TCHAR;
         typedef char   _TCHAR;
      #endif
      #define _TCHAR_DEFINED
   #endif

   #define _TINT int
   #define _TSCHAR signed char
   #define _TUCHAR unsigned char
   #define _TXCHAR char
   #define _TEOF EOF
   #define _tenviron _environ 
   #define _tfinddata_t _finddata_t 
   
   /* dirent structures and functions */   
   #define _tdirent  dirent
   #define _TDIR     DIR
   #define _topendir opendir
   #define _tclosedir   closedir
   #define _treaddir readdir
   #define _trewinddir  rewinddir
   #define _ttelldir telldir
   #define _tseekdir seekdir

   #define _ttoi64     _atoi64
   #define _i64tot     _i64toa
   #define _ui64tot    _ui64toa
   #define _tcsnccoll  _strncoll
   #define _tcsncoll   _strncoll
   #define _tcsncicoll _strnicoll
   #define _tfindfirsti64  _findfirsti64
   #define _tfindnexti64   _findnexti64
   #define _tfinddatai64_t _finddatai64_t
   
   #define _tunlink    _unlink
   #define _tgetdcwd   _getdcwd

   #define _fgettc fgetc
   #define _fgettchar fgetchar
   #define _fgetts fgets
   #define _fputtc fputc
   #define _fputtchar fputchar
   #define _fputts fputs
   #define _ftprintf fprintf
   #define _ftscanf fscanf
   #define _gettc getc
   #define _gettchar getchar
   #define _getts gets
   #define _istalnum isalnum
   #define _istalpha isalpha
   #define _istascii __isascii
   #define _istcntrl iscntrl
   #define _istdigit isdigit
   #define _istgraph isgraph
   #define _istlead(c) 0
   #define _istleadbyte(c) 0
   #define _istlegal(c) 1
   #define _istlower islower 
   #define _istprint isprint 
   #define _istpunct ispunct 
   #define _istspace isspace 
   #define _istupper isupper 
   #define _istxdigit isxdigit 
   #define _itot _itoa 
   #define _ltot _ltoa  
   #define _puttc putc 
   #define _puttchar putchar
   #define _putts puts 
   #define _tmain main  
   #define _sntprintf _snprintf 
   #define _stprintf sprintf 
   #define _stscanf sscanf 
   #define _taccess _access 
   #define _tasctime asctime 
   #define _tccpy(d,s) (*(d)=*(s))
   #define _tchdir _chdir  
   #define _tclen(c) 1
   #define _tchmod _chmod 
   #define _tcreat _creat 
   #define _tcscat strcat 
   #define _tcschr strchr 
   #define _tcsclen strlen  
   #define _tcscmp strcmp 
   #define _tcscoll strcoll 
   #define _tcscpy strcpy
   #define _tcscspn strcspn 
   #define _tcsdec _strdec 
   #define _tcsdup _strdup 
   #define _tcsftime strftime 
   #define _tcsicmp _stricmp 
   #define _tcsicoll _stricoll 
   #define _tcsinc _strinc
   #define _tcslen strlen
   #define _tcslwr _strlwr 
   #define _tcsnbcnt _strncnt 
   #define _tcsncat strncat  
   #define _tcsnccat strncat 
   #define _tcsncmp strncmp 
   #define _tcsnccmp strncmp 
   #define _tcsnccnt _strncnt 
   #define _tcsnccpy strncpy 
   #define _tcsncicmp _strnicmp 
   #define _tcsncpy strncpy 
   #define _tcsncset _strnset 
   #define _tcsnextc _strnextc 
   #define _tcsnicmp _strnicmp 
   #define _tcsnicoll _strnicoll 
   #define _tcsninc _strninc 
   #define _tcsnccnt _strncnt 
   #define _tcsnset _strnset 
   #define _tcspbrk strpbrk 
   #define _tcsspnp _strspnp 
   #define _tcsrchr strrchr 
   #define _tcsrev _strrev 
   #define _tcsset _strset 
   #define _tcsspn strspn 
   #define _tcsstr strstr  
   #define _tcstod strtod 
   #define _tcstok strtok 
   #define _tcstol strtol 
   #define _tcstoul strtoul 
   #define _tcsupr _strupr 
   #define _tcsxfrm strxfrm 
   #define _tctime ctime 
   #define _texecl _execl 
   #define _texecle _execle 
   #define _texeclp _execlp 
   #define _texeclpe _execlpe 
   #define _texecv _execv 
   #define _texecve _execve 
   #define _texecvp _execvp 
   #define _texecvpe _execvpe 
   #define _tfdopen _fdopen 
   #define _tfindfirst _findfirst 
   #define _tfindnext _findnext 
   #define _tfopen fopen  
   #define _tfreopen freopen  
   #define _tfsopen _fsopen 
   #define _tfullpath _fullpath 
   #define _tgetcwd _getcwd 
   #define _tgetenv getenv 
   #define _tmain main 
   #define _tmakepath _makepath 
   #define _tmkdir _mkdir 
   #define _tmktemp _mktemp 
   #define _tperror perror 
   #define _topen _open 
   #define _totlower tolower  
   #define _totupper toupper 
   #define _tpopen _popen 
   #define _tprintf printf 
   #define _tremove remove 
   #define _trename rename 
   #define _trmdir _rmdir 
   #define _tsearchenv _searchenv 
   #define _tscanf scanf 
   #define _tsetlocale setlocale 
   #define _tsopen _sopen 
   #define _tspawnl _spawnl 
   #define _tspawnle _spawnle  
   #define _tspawnlp _spawnlp 
   #define _tspawnlpe _spawnlpe 
   #define _tspawnv _spawnv 
   #define _tspawnve _spawnve 
   #define _tspawnvp _spawnvp 
   #define _tspawnvpe _spawnvpe 
   #define _tsplitpath _splitpath 
   #define _tstat _stat 
   #define _tstrdate _strdate 
   #define _tstrtime _strtime 
   #define _tsystem system
   #define _ttempnam _tempnam 
   #define _ttmpnam tmpnam
   #define _ttoi atoi 
   #define _ttol atol 
   #define _tutime _utime 
   #define _tWinMain WinMain  
   #define _ultot _ultoa 
   #define _ungettc ungetc 
   #define _vftprintf vfprintf 
   #define _vsntprintf _vsnprintf 
   #define _vstprintf vsprintf 
   #define _vtprintf vprintf
   
   #define _strdec(_str1, _str2) ((_str1)>=(_str2) ? NULL : (_str2)-1)
   #define _strinc(_str)  ((_str)+1)
   #define _strnextc(_str) ((unsigned int) *(_str))
   #define _strninc(_str, _inc) (((_str)+(_inc)))
   #define _strncnt(_str, _cnt) ((strlen(_str)>_cnt) ? _count : strlen(_str))
   #define _strspnp(_str1, _str2) ((*((_str1)+strspn(_str1,_str2))) ? ((_str1)+strspn(_str1,_str2)) : NULL)
   
#endif
   
   


#define _TEXT(x)  __T(x)
#define _T(x)     __T(x)

#endif /* _TCHAR_H_ */

