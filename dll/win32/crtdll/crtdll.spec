# Old C runtime library.

@ cdecl ??2@YAPAXI@Z(long) operator_new # void * __cdecl operator new(unsigned int)
@ cdecl ??3@YAXPAX@Z(ptr) operator_delete # void __cdecl operator delete(void *)
@ cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) _set_new_handler # int (__cdecl*__cdecl _set_new_handler(int (__cdecl*)(unsigned int)))(unsigned int)
@ cdecl _CIacos()
@ cdecl _CIasin()
@ cdecl _CIatan()
@ cdecl _CIatan2()
@ cdecl _CIcos()
@ cdecl _CIcosh()
@ cdecl _CIexp()
@ cdecl _CIfmod()
@ cdecl _CIlog()
@ cdecl _CIlog10()
@ cdecl _CIpow()
@ cdecl _CIsin()
@ cdecl _CIsinh()
@ cdecl _CIsqrt()
@ cdecl _CItan()
@ cdecl _CItanh()
@ extern _HUGE_dll _HUGE
@ cdecl _XcptFilter()
@ cdecl __GetMainArgs(ptr ptr ptr long)
@ extern __argc_dll __argc
@ extern __argv_dll __argv
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ cdecl __fpecode()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ extern __mb_cur_max_dll __mb_cur_max
@ cdecl __pxcptinfoptrs()
@ cdecl __threadhandle()
@ cdecl __threadid()
@ cdecl __toascii(long)
@ cdecl _abnormal_termination()
@ cdecl _access(str long)
@ extern _acmdln_dll _acmdln
@ extern _aexit_rtn_dll _aexit_rtn
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long)
@ extern _basemajor_dll CRTDLL__basemajor_dll
@ extern _baseminor_dll CRTDLL__baseminor_dll
@ extern _baseversion_dll CRTDLL__baseversion_dll
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _c_exit()
@ cdecl _cabs(long)
@ cdecl _cexit()
@ cdecl _cgets(str)
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign( double )
@ cdecl _chmod(str long)
@ cdecl _chsize(long long)
@ cdecl _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ extern _commode_dll _commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _copysign( double double )
@ varargs _cprintf(str)
@ extern _cpumode_dll CRTDLL__cpumode_dll
@ cdecl _cputs(str)
@ cdecl _creat(str long)
@ varargs _cscanf(str)
@ extern _ctype
@ cdecl _cwait(ptr long long)
@ extern _daylight_dll _daylight
@ cdecl _dup(long)
@ cdecl _dup2(long long)
@ cdecl _ecvt( double long ptr ptr)
@ cdecl _endthread()
@ extern _environ_dll _environ
@ cdecl _eof(long)
@ cdecl _errno()
@ cdecl _except_handler2(ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str str)
@ cdecl _execve(str str str)
@ cdecl _execvp(str str)
@ cdecl _execvpe(str str str)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
@ cdecl _fcloseall()
@ cdecl _fcvt( double long ptr ptr)
@ cdecl _fdopen(long str)
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr)
@ extern _fileinfo_dll
@ cdecl _filelength(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ cdecl _findnext(long ptr)
@ cdecl _finite( double )
@ cdecl _flsbuf(long ptr)
@ cdecl _flushall()
@ extern _fmode_dll _fmode
@ cdecl _fpclass(double)
@ cdecl _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
@ cdecl _fsopen(str str long)
@ cdecl _fstat(long ptr) CRTDLL__fstat
@ cdecl _ftime(ptr)
@ cdecl -ret64 _ftol()
@ cdecl _fullpath(ptr str long)
@ cdecl _futime(long ptr)
@ cdecl _gcvt( double long str)
@ cdecl _get_osfhandle(long)
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives()
@ cdecl _getpid()
@ cdecl _getsystime(ptr)
@ cdecl _getw(ptr)
@ cdecl _global_unwind2(ptr)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _initterm(ptr ptr)
@ extern _iob
@ cdecl _isatty(long)
@ cdecl _isctype(long long)
@ cdecl _ismbbalnum(long)
@ cdecl _ismbbalpha(long)
@ cdecl _ismbbgraph(long)
@ cdecl _ismbbkalnum(long)
@ cdecl _ismbbkana(long)
@ cdecl _ismbbkpunct(long)
@ cdecl _ismbblead(long)
@ cdecl _ismbbprint(long)
@ cdecl _ismbbpunct(long)
@ cdecl _ismbbtrail(long)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcdigit(long)
@ cdecl _ismbchira(long)
@ cdecl _ismbckata(long)
@ cdecl _ismbcl0(long)
@ cdecl _ismbcl1(long)
@ cdecl _ismbcl2(long)
@ cdecl _ismbclegal(long)
@ cdecl _ismbclower(long)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcsymbol(long)
@ cdecl _ismbcupper(long)
@ cdecl _ismbslead(ptr ptr)
@ cdecl _ismbstrail(ptr ptr)
@ cdecl _isnan( double )
@ cdecl _itoa(long ptr long)
@ cdecl _itow(long ptr long)
#  @ cdecl _j0(double)
#  @ cdecl _j1(double)
#  @ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _loaddll(str)
@ cdecl _local_unwind2(ptr long)
@ cdecl _locking(long long long)
@ cdecl _logb( double )
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr long long ptr)
@ cdecl _lseek(long long long)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltow(long ptr long)
@ cdecl _makepath(str str str str str)
@ cdecl _matherr(ptr)
@ cdecl _mbbtombc(long)
@ cdecl _mbbtype(long long)
@ cdecl _mbccpy(str str)
@ cdecl _mbcjistojms(long)
@ cdecl _mbcjmstojis(long)
@ cdecl _mbclen(ptr)
@ cdecl _mbctohira(long)
@ cdecl _mbctokata(long)
@ cdecl _mbctolower(long)
@ cdecl _mbctombb(long)
@ cdecl _mbctoupper(long)
@ extern _mbctype
@ cdecl _mbsbtype(str long)
@ cdecl _mbscat(str str)
@ cdecl _mbschr(str long)
@ cdecl _mbscmp(str str)
@ cdecl _mbscpy(ptr str)
@ cdecl _mbscspn(str str)
@ cdecl _mbsdec(ptr ptr)
@ cdecl _mbsdup(str)
@ cdecl _mbsicmp(str str)
@ cdecl _mbsinc(str)
@ cdecl _mbslen(str)
@ cdecl _mbslwr(str)
@ cdecl _mbsnbcat(str str long)
@ cdecl _mbsnbcmp(str str long)
@ cdecl _mbsnbcnt(ptr long)
@ cdecl _mbsnbcpy(ptr str long)
@ cdecl _mbsnbicmp(str str long)
@ cdecl _mbsnbset(str long long)
@ cdecl _mbsncat(str str long)
@ cdecl _mbsnccnt(str long)
@ cdecl _mbsncmp(str str long)
@ cdecl _mbsncpy(str str long)
@ cdecl _mbsnextc(str)
@ cdecl _mbsnicmp(str str long)
@ cdecl _mbsninc(str long)
@ cdecl _mbsnset(str long long)
@ cdecl _mbspbrk(str str)
@ cdecl _mbsrchr(str long)
@ cdecl _mbsrev(str)
@ cdecl _mbsset(str long)
@ cdecl _mbsspn(str str)
@ cdecl _mbsspnp(str str)
@ cdecl _mbsstr(str str)
@ cdecl _mbstok(str str)
@ cdecl _mbstrlen(str)
@ cdecl _mbsupr(str)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl _mkdir(str)
@ cdecl _mktemp(str)
@ cdecl _msize(ptr)
@ cdecl _nextafter(double double) nextafter
@ cdecl _onexit(ptr)
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ extern _osmajor_dll CRTDLL__osmajor_dll
@ extern _osminor_dll CRTDLL__osminor_dll
@ extern _osmode_dll CRTDLL__osmode_dll
@ extern _osver_dll _osver
@ extern _osversion_dll CRTDLL__osversion_dll
@ cdecl _pclose(ptr)
@ extern _pctype_dll _pctype
@ extern _pgmptr_dll _pgmptr
@ cdecl _pipe(ptr long long)
@ cdecl _popen(str str)
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
@ cdecl _putw(long ptr)
@ extern _pwctype_dll _pwctype
@ cdecl _read(long ptr long)
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
@ cdecl _rotr(long long)
@ cdecl _scalb( double long)
@ cdecl _searchenv(str str ptr)
@ cdecl _seterrormode(long)
@ cdecl -i386 _setjmp(ptr)
@ cdecl _setmode(long long)
@ cdecl _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(str long str)
@ varargs _snwprintf(wstr long wstr)
@ varargs _sopen(str long long)
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
@ cdecl _stat(str ptr) CRTDLL__stat
@ cdecl _statusfp()
@ cdecl _strcmpi(str str)
@ cdecl _strdate(ptr)
@ cdecl _strdec(str str)
@ cdecl _strdup(str)
@ cdecl _strerror(long)
@ cdecl _stricmp(str str)
@ cdecl _stricoll(str str)
@ cdecl _strinc(str)
@ cdecl _strlwr(str)
@ cdecl _strncnt(str long)
@ cdecl _strnextc(str)
@ cdecl _strnicmp(str str long)
@ cdecl _strninc(str long)
@ cdecl _strnset(str long long)
@ cdecl _strrev(str)
@ cdecl _strset(str long)
@ cdecl _strspnp(str str)
@ cdecl _strtime(ptr)
@ cdecl _strupr(str)
@ cdecl _swab(str str long)
@ extern _sys_errlist
@ extern _sys_nerr_dll _sys_nerr
@ cdecl _tell(long)
@ cdecl _tempnam(str str)
@ extern _timezone_dll _timezone
@ cdecl _tolower(long)
@ cdecl _toupper(long)
@ extern _tzname
@ cdecl _tzset()
@ cdecl _ultoa(long ptr long)
@ cdecl _ultow(long ptr long)
@ cdecl _umask(long)
@ cdecl _ungetch(long)
@ cdecl _unlink(str)
@ cdecl _unloaddll(long)
@ cdecl _utime(str ptr)
@ cdecl _vsnprintf(ptr long ptr ptr)
@ cdecl _vsnwprintf(ptr long wstr long)
@ cdecl _wcsdup(wstr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcsicoll(wstr wstr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl _wcsupr(wstr)
@ extern _winmajor_dll _winmajor
@ extern _winminor_dll _winminor
@ extern _winver_dll _winver
@ cdecl _write(long ptr long)
@ cdecl _wtoi(wstr)
@ cdecl _wtol(wstr)
#  @ cdecl _y0(double)
#  @ cdecl _y1(double)
#  @ cdecl _yn(long double )
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl asctime(ptr)
@ cdecl asin(double)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl atexit(ptr)
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl calloc(long long)
@ cdecl ceil(double)
@ cdecl clearerr(ptr)
@ cdecl clock()
@ cdecl cos(double)
@ cdecl cosh(double)
@ cdecl ctime(ptr)
@ cdecl difftime(long long)
@ cdecl div(long long)
@ cdecl exit(long)
@ cdecl exp(double)
@ cdecl fabs(double)
@ cdecl fclose(ptr)
@ cdecl feof(ptr)
@ cdecl ferror(ptr)
@ cdecl fflush(ptr)
@ cdecl fgetc(ptr)
@ cdecl fgetpos(ptr ptr)
@ cdecl fgets(ptr long ptr)
@ cdecl fgetwc(ptr)
@ cdecl floor(double)
@ cdecl fmod(double double)
@ cdecl fopen(str str)
@ varargs fprintf(ptr str)
@ cdecl fputc(long ptr)
@ cdecl fputs(str ptr)
@ cdecl fputwc(long ptr)
@ cdecl fread(ptr long long ptr)
@ cdecl free(ptr)
@ cdecl freopen(str str ptr)
@ cdecl frexp(double ptr)
@ varargs fscanf(ptr str)
@ cdecl fseek(ptr long long)
@ cdecl fsetpos(ptr ptr)
@ cdecl ftell(ptr)
@ varargs fwprintf(ptr wstr)
@ cdecl fwrite(ptr long long ptr)
@ varargs fwscanf(ptr wstr)
@ cdecl getc(ptr)
@ cdecl getchar()
@ cdecl getenv(str)
@ cdecl gets(str)
@ cdecl gmtime(ptr)
@ cdecl is_wctype(long long)
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl iscntrl(long)
@ cdecl isdigit(long)
@ cdecl isgraph(long)
@ cdecl isleadbyte(long)
@ cdecl islower(long)
@ cdecl isprint(long)
@ cdecl ispunct(long)
@ cdecl isspace(long)
@ cdecl isupper(long)
@ cdecl iswalnum(long)
@ cdecl iswalpha(long)
@ cdecl iswascii(long)
@ cdecl iswcntrl(long)
@ cdecl iswctype(long long)
@ cdecl iswdigit(long)
@ cdecl iswgraph(long)
@ cdecl iswlower(long)
@ cdecl iswprint(long)
@ cdecl iswpunct(long)
@ cdecl iswspace(long)
@ cdecl iswupper(long)
@ cdecl iswxdigit(long)
@ cdecl isxdigit(long)
@ cdecl labs(long)
@ cdecl ldexp( double long)
@ cdecl ldiv(long long)
@ cdecl localeconv()
@ cdecl localtime(ptr)
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl -i386 longjmp(ptr long)
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
@ cdecl mbstowcs(ptr str long)
@ cdecl mbtowc(wstr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memset(ptr long long)
@ cdecl mktime(ptr)
@ cdecl modf(double ptr)
@ cdecl perror(str)
@ cdecl pow(double double)
@ varargs printf(str)
@ cdecl putc(long ptr)
@ cdecl putchar(long)
@ cdecl puts(str)
@ cdecl qsort(ptr long long ptr)
@ cdecl raise(long)
@ cdecl rand()
@ cdecl realloc(ptr long)
@ cdecl remove(str)
@ cdecl rename(str str)
@ cdecl rewind(ptr)
@ varargs scanf(str)
@ cdecl setbuf(ptr ptr)
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long)
@ cdecl signal(long long)
@ cdecl sin(double)
@ cdecl sinh(double)
@ varargs sprintf(ptr str)
@ cdecl sqrt(double)
@ cdecl srand(long)
@ varargs sscanf(str str)
@ cdecl strcat(str str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl strftime(str long str ptr)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtok(str str)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ cdecl strxfrm(ptr str long)
@ varargs swprintf(wstr wstr)
@ varargs swscanf(wstr wstr)
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl tanh(double)
@ cdecl time(ptr)
@ cdecl tmpfile()
@ cdecl tmpnam(str)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl ungetc(long ptr)
@ cdecl ungetwc(long ptr)
@ cdecl vfprintf(ptr str long)
@ cdecl vfwprintf(ptr wstr long)
@ cdecl vprintf(str long)
@ cdecl vsprintf(ptr str ptr)
@ cdecl vswprintf(ptr wstr long)
@ cdecl vwprintf(wstr long)
@ cdecl wcscat(wstr wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscoll(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcsftime(ptr long wstr ptr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstok(wstr wstr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
@ cdecl wctomb(ptr long)
@ varargs wprintf(wstr)
@ varargs wscanf(wstr)
