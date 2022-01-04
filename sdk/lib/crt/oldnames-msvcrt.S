
#ifdef _M_IX86
#define SYM(name) _##name
#define IMPSYM(name) __imp__##name
#else
#define SYM(name) name
#define IMPSYM(name) __imp_##name
#endif

#if (defined(_M_IX86) || defined(_M_AMD64))

#include <asm.inc>

MACRO(CREATE_ALIAS1, alias, target)
#ifdef _USE_ML
    EXTERN SYM(&target):PROC
    ALIAS <SYM(&alias)> = <SYM(&target)>
#else
    .weakref SYM(&alias), SYM(&target)
#endif
ENDM

MACRO(CREATE_ALIAS2, alias, target)
#ifdef _USE_ML
    EXTERN IMPSYM(&target):PROC
    ALIAS <IMPSYM(&alias)> = <IMPSYM(&target)>
#else
    .weakref IMPSYM(&alias), IMPSYM(&target)
#endif
ENDM

MACRO(CREATE_ALIAS, alias, target)
    CREATE_ALIAS1 &alias, &target
    CREATE_ALIAS2 &alias, &target
ENDM

#elif defined(_M_ARM)

#include <kxarm.h>

    MACRO
    CREATE_ALIAS1   $alias, $target
    IMPORT  SYM($alias), WEAK SYM($target)
    MEND

    MACRO
    CREATE_ALIAS2   $alias, $target
    IMPORT  IMPSYM($alias), WEAK IMPSYM($target)
    MEND

    MACRO
    CREATE_ALIAS   $alias, $target
    CREATE_ALIAS1 $alias, $target
    CREATE_ALIAS2 $alias, $target
    MEND
#else
#error "Unsupported platform."
#endif

    /* Do not remove indentation, this would break ARM build! */
    CREATE_ALIAS access, _access
    CREATE_ALIAS chdir, _chdir
    CREATE_ALIAS chmod, _chmod
    CREATE_ALIAS chsize, _chsize
    CREATE_ALIAS close, _close
    CREATE_ALIAS creat, _creat
    CREATE_ALIAS cwait, _cwait
    CREATE_ALIAS2 daylight, _daylight
    CREATE_ALIAS dup, _dup
    CREATE_ALIAS dup2, _dup2
    CREATE_ALIAS ecvt, _ecvt
    CREATE_ALIAS eof, _eof
    CREATE_ALIAS execl, _execl
    CREATE_ALIAS execle, _execle
    CREATE_ALIAS execlp, _execlp
    CREATE_ALIAS execlpe, _execlpe
    CREATE_ALIAS execv, _execv
    CREATE_ALIAS execve, _execve
    CREATE_ALIAS execvp, _execvp
    CREATE_ALIAS execvpe, _execvpe
    CREATE_ALIAS fcvt, _fcvt
    CREATE_ALIAS fdopen, _fdopen
    CREATE_ALIAS fgetchar, _fgetchar
    CREATE_ALIAS fgetwchar, _fgetwchar
    CREATE_ALIAS filelength, _filelength
    CREATE_ALIAS fileno, _fileno
    CREATE_ALIAS fpreset, _fpreset
    CREATE_ALIAS fputchar, _fputchar
    CREATE_ALIAS fputwchar, _fputwchar
    CREATE_ALIAS fstat, _fstat
    CREATE_ALIAS ftime, _ftime
    CREATE_ALIAS gcvt, _gcvt
    CREATE_ALIAS getch, _getch
    CREATE_ALIAS getche, _getche
    CREATE_ALIAS getcwd, _getcwd
    CREATE_ALIAS getpid, _getpid
    CREATE_ALIAS getw, _getw
    CREATE_ALIAS heapwalk, _heapwalk
    CREATE_ALIAS isatty, _isatty
    CREATE_ALIAS isascii, __isascii
    CREATE_ALIAS itoa, _itoa
    CREATE_ALIAS kbhit, _kbhit
    CREATE_ALIAS lfind, _lfind
    CREATE_ALIAS lsearch, _lsearch
    CREATE_ALIAS lseek, _lseek
    CREATE_ALIAS ltoa, _ltoa
    CREATE_ALIAS memccpy, _memccpy
    CREATE_ALIAS memicmp, _memicmp
    CREATE_ALIAS mkdir, _mkdir
    CREATE_ALIAS mktemp, _mktemp
    CREATE_ALIAS open, _open
    CREATE_ALIAS pclose, _pclose
    CREATE_ALIAS popen, _popen
    CREATE_ALIAS putch, _putch
    CREATE_ALIAS putenv, _putenv
    CREATE_ALIAS putw, _putw
    CREATE_ALIAS read, _read
    CREATE_ALIAS rmdir, _rmdir
    CREATE_ALIAS rmtmp, _rmtmp
    CREATE_ALIAS searchenv, _searchenv
    CREATE_ALIAS setmode, _setmode
    CREATE_ALIAS snprintf, _snprintf
    CREATE_ALIAS sopen, _sopen
    CREATE_ALIAS spawnl, _spawnl
    CREATE_ALIAS spawnle, _spawnle
    CREATE_ALIAS spawnlp, _spawnlp
    CREATE_ALIAS spawnlpe, _spawnlpe
    CREATE_ALIAS spawnv, _spawnv
    CREATE_ALIAS spawnve, _spawnve
    CREATE_ALIAS spawnvp, _spawnvp
    CREATE_ALIAS spawnvpe, _spawnvpe
    CREATE_ALIAS stat, _stat
    CREATE_ALIAS strcmpi, _strcmpi
    CREATE_ALIAS strdup, _strdup
    CREATE_ALIAS stricmp, _stricmp
    CREATE_ALIAS stricoll, _stricoll
    CREATE_ALIAS strlwr, _strlwr
    CREATE_ALIAS strnicmp, _strnicmp
    CREATE_ALIAS strnset, _strnset
    CREATE_ALIAS strrev, _strrev
    CREATE_ALIAS strset, _strset
    CREATE_ALIAS strupr, _strupr
    CREATE_ALIAS swab, _swab
    CREATE_ALIAS tell, _tell
    CREATE_ALIAS tempnam, _tempnam
    CREATE_ALIAS2 timezone, _timezone
    CREATE_ALIAS2 tzname, _tzname
    CREATE_ALIAS tzset, _tzset
    CREATE_ALIAS umask, _umask
    CREATE_ALIAS ungetch, _ungetch
    CREATE_ALIAS unlink, _unlink
    CREATE_ALIAS utime, _utime
    CREATE_ALIAS wcsdup, _wcsdup
    CREATE_ALIAS wcsicmp, _wcsicmp
    CREATE_ALIAS wcsicoll, _wcsicoll
    CREATE_ALIAS wcslwr, _wcslwr
    CREATE_ALIAS wcsnicmp, _wcsnicmp
    CREATE_ALIAS wcsnset, _wcsnset
    CREATE_ALIAS wcsrev, _wcsrev
    CREATE_ALIAS wcsset, _wcsset
    CREATE_ALIAS wcsupr, _wcsupr
    CREATE_ALIAS wpopen, _wpopen
    CREATE_ALIAS write, _write
    // non-ANSI functions declared in math.h
    CREATE_ALIAS j0, _j0
    CREATE_ALIAS j1, _j1
    CREATE_ALIAS jn, _jn
    CREATE_ALIAS y0, _y0
    CREATE_ALIAS y1, _y1
    CREATE_ALIAS yn, _yn
    CREATE_ALIAS chgsign, _chgsign
    CREATE_ALIAS scalb, _scalb
    CREATE_ALIAS finite, _finite
    CREATE_ALIAS fpclass, _fpclass
    // C99 functions
    CREATE_ALIAS cabs, _cabs
    CREATE_ALIAS hypot, _hypot
    CREATE_ALIAS logb, _logb
    CREATE_ALIAS nextafter, _nextafter

    END

