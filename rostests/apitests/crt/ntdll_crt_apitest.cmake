
list(APPEND SOURCE_NTDLL
#    _CIcos.c
#    _CIlog.c
#    _CIsin.c
#    _CIsqrt.c
#    __isascii.c
#    __iscsym.c
#    __iscsymf.c
#    __toascii.c
#    _atoi64.c
#    _fltused.c
#    _i64toa.c
#    _i64tow.c
#    _itoa.c
#    _itow.c
#    _lfind.c
#    _ltoa.c
#    _ltow.c
#    _memccpy.c
#    _memicmp.c
#    _snprintf.c
#    _snwprintf.c
#    _splitpath.c
    # _strcmpi == _stricmp
#    _stricmp.c
#    _strlwr.c
#    _strnicmp.c
#    _strupr.c
#    _tolower.c
#    _toupper.c
#    _ui64toa.c
#    _ui64tow.c
#    _ultoa.c
#    _ultow.c
    _vscwprintf.c
    _vsnprintf.c
    _vsnwprintf.c
#    _wcsicmp.c
#    _wcslwr.c
#    _wcsnicmp.c
#    _wcsupr.c
#    _wtoi.c
#    _wtoi64.c
#    _wtol.c
#    abs.c
#    atan.c
#    atoi.c
#    atol.c
#    bsearch.c
#    ceil.c
#    cos.c
#    fabs.c
#    floor.c
#    isalnum.c
#    isalpha.c
#    iscntrl.c
#    isdigit.c
#    isgraph.c
#    islower.c
#    isprint.c
#    ispunct.c
#    isspace.c
#    isupper.c
#    iswalpha.c
#    iswctype.c
#    iswdigit.c
#    iswlower.c
#    iswspace.c
#    iswxdigit.c
#    isxdigit.c
#    labs.c
#    log.c
    mbstowcs.c
#    memchr.c
#    memcmp.c
    # memcpy == memmove
#    memmove.c
#    memset.c
#    pow.c
#    qsort.c
#    sin.c
    sprintf.c
#    sqrt.c
#    sscanf.c
#    strcat.c
#    strchr.c
#    strcmp.c
    strcpy.c
#    strcspn.c
    strlen.c
#    strncat.c
#    strncmp.c
#    strncpy.c
#    strpbrk.c
#    strrchr.c
#    strspn.c
#    strstr.c
#    strtol.c
#    strtoul.c
#    swprintf.c
#    tan.c
#    tolower.c
#    toupper.c
#    towlower.c
#    towupper.c
#    vsprintf.c
#    wcscat.c
#    wcschr.c
#    wcscmp.c
#    wcscpy.c
#    wcscspn.c
#    wcslen.c
#    wcsncat.c
#    wcsncmp.c
#    wcsncpy.c
#    wcspbrk.c
#    wcsrchr.c
#    wcsspn.c
#    wcsstr.c
#    wcstok.c
#    wcstol.c
    wcstombs.c
#    wcstoul.c
)

if(ARCH STREQUAL "i386")
    list(APPEND SOURCE_NTDLL
    #    _CIpow.c
    #    _ftol.c
    #    _alldiv.c
    #    _alldvrm.c
    #    _allmul.c
    #    _allrem.c
    #    _allshl.c
    #    _allshr.c
    #    _alloca_probe.c
    #    _aulldiv.c
    #    _aulldvrm.c
    #    _aullrem.c
    #    _aullshr.c
    #    _chkstk.c
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND SOURCE_NTDLL
    #    __C_specific_handler
    #    _setjmp.c
    #    _setjmpex.c
    #    _local_unwind.c
    #    longjmp.c
    )
endif()

add_executable(ntdll_crt_apitest testlist.c ${SOURCE_NTDLL})
add_target_compile_definitions(ntdll_crt_apitest TEST_NTDLL)
target_link_libraries(ntdll_crt_apitest wine ${PSEH_LIB})
set_module_type(ntdll_crt_apitest win32cui)
add_importlibs(ntdll_crt_apitest ntdll msvcrt kernel32)
add_cd_file(TARGET ntdll_crt_apitest DESTINATION reactos/bin FOR all)
