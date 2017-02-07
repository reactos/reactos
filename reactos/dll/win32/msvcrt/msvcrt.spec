# msvcrt.dll - MS VC++ Run Time Library

@ cdecl $I10_OUTPUT() MSVCRT_I10_OUTPUT
@ cdecl -i386 -norelay ??0__non_rtti_object@@QAE@ABV0@@Z(ptr) __thiscall_MSVCRT___non_rtti_object_copy_ctor
@ cdecl -i386 -norelay ??0__non_rtti_object@@QAE@PBD@Z(ptr) __thiscall_MSVCRT___non_rtti_object_ctor
@ cdecl -i386 -norelay ??0bad_cast@@AAE@PBQBD@Z(ptr) __thiscall_MSVCRT_bad_cast_ctor
@ cdecl -i386 -norelay ??0bad_cast@@QAE@ABQBD@Z(ptr) __thiscall_MSVCRT_bad_cast_ctor
@ cdecl -i386 -norelay ??0bad_cast@@QAE@ABV0@@Z(ptr) __thiscall_MSVCRT_bad_cast_copy_ctor
@ cdecl -i386 -norelay ??0bad_cast@@QAE@PBD@Z(ptr) __thiscall_MSVCRT_bad_cast_ctor_charptr
@ cdecl -i386 -norelay ??0bad_typeid@@QAE@ABV0@@Z(ptr) __thiscall_MSVCRT_bad_typeid_copy_ctor
@ cdecl -i386 -norelay ??0bad_typeid@@QAE@PBD@Z(ptr) __thiscall_MSVCRT_bad_typeid_ctor
@ cdecl -i386 -norelay ??0exception@@QAE@ABQBD@Z(ptr) __thiscall_MSVCRT_exception_ctor
@ cdecl -i386 -norelay ??0exception@@QAE@ABQBDH@Z(ptr long) __thiscall_MSVCRT_exception_ctor_noalloc
@ cdecl -i386 -norelay ??0exception@@QAE@ABV0@@Z(ptr) __thiscall_MSVCRT_exception_copy_ctor
@ cdecl -i386 -norelay ??0exception@@QAE@XZ() __thiscall_MSVCRT_exception_default_ctor
@ cdecl -i386 -norelay ??1__non_rtti_object@@UAE@XZ() __thiscall_MSVCRT___non_rtti_object_dtor
@ cdecl -i386 -norelay ??1bad_cast@@UAE@XZ() __thiscall_MSVCRT_bad_cast_dtor
@ cdecl -i386 -norelay ??1bad_typeid@@UAE@XZ() __thiscall_MSVCRT_bad_typeid_dtor
@ cdecl -i386 -norelay ??1exception@@UAE@XZ() __thiscall_MSVCRT_exception_dtor
@ cdecl -i386 -norelay ??1type_info@@UAE@XZ() __thiscall_MSVCRT_type_info_dtor
@ cdecl -arch=win32 ??2@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl -arch=win64 ??2@YAPEAX_K@Z(double) MSVCRT_operator_new
# @ cdecl ??2@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win32 ??3@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -arch=win64 ??3@YAXPEAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -i386 -norelay ??4__non_rtti_object@@QAEAAV0@ABV0@@Z(ptr) __thiscall_MSVCRT___non_rtti_object_opequals
@ cdecl -i386 -norelay ??4bad_cast@@QAEAAV0@ABV0@@Z(ptr) __thiscall_MSVCRT_bad_cast_opequals
@ cdecl -i386 -norelay ??4bad_typeid@@QAEAAV0@ABV0@@Z(ptr) __thiscall_MSVCRT_bad_typeid_opequals
@ cdecl -i386 -norelay ??4exception@@QAEAAV0@ABV0@@Z(ptr) __thiscall_MSVCRT_exception_opequals
@ cdecl -i386 -norelay ??8type_info@@QBEHABV0@@Z(ptr) __thiscall_MSVCRT_type_info_opequals_equals
@ cdecl -i386 -norelay ??9type_info@@QBEHABV0@@Z(ptr) __thiscall_MSVCRT_type_info_opnot_equals
@ extern -i386 ??_7__non_rtti_object@@6B@ MSVCRT___non_rtti_object_vtable
@ extern -i386 ??_7bad_cast@@6B@ MSVCRT_bad_cast_vtable
@ extern -i386 ??_7bad_typeid@@6B@ MSVCRT_bad_typeid_vtable
@ extern -i386 ??_7exception@@6B@ MSVCRT_exception_vtable
@ cdecl -i386 -norelay ??_E__non_rtti_object@@UAEPAXI@Z(long) __thiscall_MSVCRT___non_rtti_object_vector_dtor
@ cdecl -i386 -norelay ??_Ebad_cast@@UAEPAXI@Z(long) __thiscall_MSVCRT_bad_cast_vector_dtor
@ cdecl -i386 -norelay ??_Ebad_typeid@@UAEPAXI@Z(long) __thiscall_MSVCRT_bad_typeid_vector_dtor
@ cdecl -i386 -norelay ??_Eexception@@UAEPAXI@Z(long) __thiscall_MSVCRT_exception_vector_dtor
@ cdecl -i386 -norelay ??_Fbad_cast@@QAEXXZ() __thiscall_MSVCRT_bad_cast_default_ctor
@ cdecl -i386 -norelay ??_Fbad_typeid@@QAEXXZ() __thiscall_MSVCRT_bad_typeid_default_ctor
@ cdecl -i386 -norelay ??_G__non_rtti_object@@UAEPAXI@Z(long) __thiscall_MSVCRT___non_rtti_object_scalar_dtor
@ cdecl -i386 -norelay ??_Gbad_cast@@UAEPAXI@Z(long) __thiscall_MSVCRT_bad_cast_scalar_dtor
@ cdecl -i386 -norelay ??_Gbad_typeid@@UAEPAXI@Z(long) __thiscall_MSVCRT_bad_typeid_scalar_dtor
@ cdecl -i386 -norelay ??_Gexception@@UAEPAXI@Z(long) __thiscall_MSVCRT_exception_scalar_dtor
@ cdecl -arch=win32 ??_U@YAPAXI@Z(long) MSVCRT_operator_new
@ cdecl -arch=win64 ??_U@YAPEAX_K@Z(long) MSVCRT_operator_new
# @ cdecl ??_U@YAPAXIHPBDH@Z(long long str long) MSVCRT_operator_new_dbg
@ cdecl -arch=win32 ??_V@YAXPAX@Z(ptr) MSVCRT_operator_delete
@ cdecl -arch=win64 ??_V@YAXPEAX@Z(ptr) MSVCRT_operator_delete
@ cdecl ?_query_new_handler@@YAP6AHI@ZXZ() MSVCRT__query_new_handler
@ cdecl ?_query_new_mode@@YAHXZ() MSVCRT__query_new_mode
@ cdecl ?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z(ptr) MSVCRT__set_new_handler
@ cdecl ?_set_new_mode@@YAHH@Z(long) MSVCRT__set_new_mode
@ cdecl ?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z(ptr) MSVCRT__set_se_translator
@ cdecl -i386 -norelay ?before@type_info@@QBEHABV1@@Z(ptr) __thiscall_MSVCRT_type_info_before
@ cdecl -i386 -norelay ?name@type_info@@QBEPBDXZ() __thiscall_MSVCRT_type_info_name
@ cdecl -i386 -norelay ?raw_name@type_info@@QBEPBDXZ() __thiscall_MSVCRT_type_info_raw_name
@ cdecl ?set_new_handler@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_new_handler
@ cdecl ?set_terminate@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_terminate
@ cdecl ?set_unexpected@@YAP6AXXZP6AXXZ@Z(ptr) MSVCRT_set_unexpected
@ cdecl ?terminate@@YAXXZ() MSVCRT_terminate
@ cdecl ?unexpected@@YAXXZ() MSVCRT_unexpected
@ cdecl -i386 -norelay ?what@exception@@UBEPBDXZ() __thiscall_MSVCRT_what_exception
@ cdecl -arch=i386 _CIacos()
@ cdecl -arch=i386 _CIasin()
@ cdecl -arch=i386 _CIatan()
@ cdecl -arch=i386 _CIatan2()
@ cdecl -arch=i386 _CIcos()
@ cdecl -arch=i386 _CIcosh()
@ cdecl -arch=i386 _CIexp()
@ cdecl -arch=i386 _CIfmod()
@ cdecl -arch=i386 _CIlog()
@ cdecl -arch=i386 _CIlog10()
@ cdecl -arch=i386 _CIpow()
@ cdecl -arch=i386 _CIsin()
@ cdecl -arch=i386 _CIsinh()
@ cdecl -arch=i386 _CIsqrt()
@ cdecl -arch=i386 _CItan()
@ cdecl -arch=i386 _CItanh()
# stub _CrtCheckMemory
# stub _CrtDbgBreak
# stub _CrtDbgReport
# stub _CrtDbgReportV
# stub _CrtDbgReportW
# stub _CrtDbgReportWV
# stub _CrtDoForAllClientObjects
# stub _CrtDumpMemoryLeaks
# stub _CrtIsMemoryBlock
# stub _CrtIsValidHeapPointer
# stub _CrtIsValidPointer
# stub _CrtMemCheckpoint
# stub _CrtMemDifference
# stub _CrtMemDumpAllObjectsSince
# stub _CrtMemDumpStatistics
# stub _CrtReportBlockType
# stub _CrtSetAllocHook
# stub _CrtSetBreakAlloc
# stub _CrtSetDbgBlockType
# stub _CrtSetDbgFlag
# stub _CrtSetDumpClient
# stub _CrtSetReportFile
# stub _CrtSetReportHook
# stub _CrtSetReportHook2
# stub _CrtSetReportMode
@ stdcall _CxxThrowException(long long)
@ cdecl -i386 -norelay _EH_prolog()
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ extern _HUGE
@ cdecl _Strftime(str long str ptr ptr)
@ cdecl _XcptFilter(long ptr)
@ cdecl __CppXcptFilter(long ptr)
# stub __CxxCallUnwindDelDtor
# stub __CxxCallUnwindDtor
# stub __CxxCallUnwindVecDtor
@ cdecl __CxxDetectRethrow(ptr)
# stub __CxxExceptionFilter
@ cdecl -i386 -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ cdecl -i386 -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -i386 -norelay __CxxFrameHandler3(ptr ptr ptr ptr) __CxxFrameHandler
@ stdcall -arch=x86_64 __C_specific_handler(ptr long ptr ptr)
@ stdcall -i386 __CxxLongjmpUnwind(ptr)
@ cdecl __CxxQueryExceptionSize()
# stub __CxxRegisterExceptionObject
# stub __CxxUnregisterExceptionObject
# stub __DestructExceptionObject
@ cdecl __RTCastToVoid(ptr) MSVCRT___RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) MSVCRT___RTDynamicCast
@ cdecl __RTtypeid(ptr) MSVCRT___RTtypeid
@ cdecl __STRINGTOLD(ptr ptr str long)
@ cdecl ___lc_codepage_func()
# @ cdecl ___lc_collate_cp_func()
@ cdecl ___lc_handle_func()
# @ cdecl ___mb_cur_max_func() MSVCRT___mb_cur_max_func
@ cdecl ___setlc_active_func()
@ cdecl ___unguarded_readlc_active_add_func()
@ extern __argc
@ extern __argv
@ extern __badioinfo __badioinfo
@ cdecl __crtCompareStringA(long long str long str long) kernel32.CompareStringA
@ cdecl __crtCompareStringW(long long wstr long wstr long) kernel32.CompareStringW
@ cdecl __crtGetLocaleInfoW(long long ptr long) kernel32.GetLocaleInfoW
@ cdecl __crtGetStringTypeW(long long wstr long ptr)
@ cdecl __crtLCMapStringA(long long str long ptr long long long)
# stub __crtLCMapStringW
@ cdecl __daylight() __p__daylight
@ cdecl __dllonexit(ptr ptr ptr)
@ cdecl __doserrno()
@ cdecl __fpecode()
@ cdecl __get_app_type()
@ cdecl __getmainargs(ptr ptr ptr long ptr)
@ extern __initenv
@ cdecl __iob_func()
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ extern __lc_codepage MSVCRT___lc_codepage
# @ stub __lc_collate # not in XP / 7
@ extern __lc_collate_cp MSVCRT___lc_collate_cp
@ extern __lc_handle MSVCRT___lc_handle
@ cdecl __lconv_init()
# stub __libm_sse2_acos
# stub __libm_sse2_acosf
# stub __libm_sse2_asin
# stub __libm_sse2_asinf
# stub __libm_sse2_atan
# stub __libm_sse2_atan2
# stub __libm_sse2_atanf
# stub __libm_sse2_cos
# stub __libm_sse2_cosf
# stub __libm_sse2_exp
# stub __libm_sse2_expf
# stub __libm_sse2_log
# stub __libm_sse2_log10
# stub __libm_sse2_log10f
# stub __libm_sse2_logf
# stub __libm_sse2_pow
# stub __libm_sse2_powf
# stub __libm_sse2_sin
# stub __libm_sse2_sinf
# stub __libm_sse2_tan
# stub __libm_sse2_tanf
@ extern __mb_cur_max
@ cdecl -arch=i386 __p___argc()
@ cdecl -arch=i386 __p___argv()
@ cdecl -arch=i386 __p___initenv()
@ cdecl -arch=i386 __p___mb_cur_max()
@ cdecl -arch=i386 __p___wargv()
@ cdecl -arch=i386 __p___winitenv()
@ cdecl -arch=i386 __p__acmdln()
@ cdecl -arch=i386 __p__amblksiz()
@ cdecl -arch=i386 __p__commode()
@ cdecl -arch=i386 __p__daylight()
@ cdecl -arch=i386 __p__dstbias()
@ cdecl -arch=i386 __p__environ()
@ cdecl -arch=i386 __p__fileinfo()
@ cdecl -arch=i386 __p__fmode()
@ cdecl -arch=i386 __p__iob() __iob_func
@ cdecl -arch=i386 __p__mbcasemap()
@ cdecl -arch=i386 __p__mbctype()
@ cdecl -arch=i386 __p__osver()
@ cdecl -arch=i386 __p__pctype()
@ cdecl -arch=i386 __p__pgmptr()
@ cdecl -arch=i386 __p__pwctype()
@ cdecl -arch=i386 __p__timezone()
@ cdecl -arch=i386 __p__tzname()
@ cdecl -arch=i386 __p__wcmdln()
@ cdecl -arch=i386 __p__wenviron()
@ cdecl -arch=i386 __p__winmajor()
@ cdecl -arch=i386 __p__winminor()
@ cdecl -arch=i386 __p__winver()
@ cdecl -arch=i386 __p__wpgmptr()
@ cdecl __pctype_func()
@ extern __pioinfo
@ cdecl __pwctype_func()
@ cdecl __pxcptinfoptrs()
@ cdecl __set_app_type(long)
@ extern __setlc_active
@ cdecl __setusermatherr(ptr)
# stub __strncnt
@ cdecl __threadhandle() kernel32.GetCurrentThread
@ cdecl __threadid() kernel32.GetCurrentThreadId
@ cdecl __toascii(long)
@ cdecl __uncaught_exception()
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ extern __unguarded_readlc_active
@ extern __wargv __wargv
@ cdecl __wcserror(wstr)
@ cdecl __wcserror_s(ptr long wstr)
# stub __wcsncnt
@ cdecl __wgetmainargs(ptr ptr ptr long ptr)
@ extern __winitenv
@ cdecl _abnormal_termination()
# stub _abs64
@ cdecl _access(str long)
@ cdecl _access_s(str long)
@ extern _acmdln
@ stdcall -arch=i386 _adj_fdiv_m16i(long)
@ stdcall -arch=i386 _adj_fdiv_m32(long)
@ stdcall -arch=i386 _adj_fdiv_m32i(long)
@ stdcall -arch=i386 _adj_fdiv_m64(double)
@ cdecl -arch=i386 _adj_fdiv_r()
@ stdcall -arch=i386 _adj_fdivr_m16i(long)
@ stdcall -arch=i386 _adj_fdivr_m32(long)
@ stdcall -arch=i386 _adj_fdivr_m32i(long)
@ stdcall -arch=i386 _adj_fdivr_m64(double)
@ cdecl -arch=i386 _adj_fpatan()
@ cdecl -arch=i386 _adj_fprem()
@ cdecl -arch=i386 _adj_fprem1()
@ cdecl -arch=i386 _adj_fptan()
@ extern -arch=i386 _adjust_fdiv
@ extern _aexit_rtn
@ cdecl _aligned_free(ptr)
# stub _aligned_free_dbg
@ cdecl _aligned_malloc(long long)
# stub _aligned_malloc_dbg
@ cdecl _aligned_offset_malloc(long long long)
# stub _aligned_offset_malloc_dbg
@ cdecl _aligned_offset_realloc(ptr long long long)
# stub _aligned_offset_realloc_dbg
@ cdecl _aligned_realloc(ptr long long)
# stub _aligned_realloc_dbg
@ cdecl _amsg_exit(long)
@ cdecl _assert(str str long)
@ cdecl _atodbl(ptr str)
# stub _atodbl_l
# @ cdecl _atof_l(str ptr)
# stub _atoflt_l
@ cdecl -ret64 _atoi64(str)
# stub _atoi64_l
# stub _atoi_l
# stub _atol_l
@ cdecl _atoldbl(ptr str)
# stub _atoldbl_l
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _beginthreadex(ptr long ptr ptr long ptr)
@ cdecl _c_exit()
@ cdecl _cabs(long)
@ cdecl _callnewh(long)
# stub _calloc_dbg
@ cdecl _cexit()
@ cdecl _cgets(str)
# stub _cgets_s
# stub _cgetws
# stub _cgetws_s
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign(double)
@ cdecl -i386 -norelay _chkesp()
@ cdecl _chmod(str long)
@ cdecl _chsize(long long)
# stub _chsize_s
# stub _chvalidator
# stub _chvalidator_l
@ cdecl _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ extern _commode
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
# @ cdecl _controlfp_s(ptr long long)
@ cdecl _copysign( double double )
@ varargs _cprintf(str)
# stub _cprintf_l
# stub _cprintf_p
# stub _cprintf_p_l
# stub _cprintf_s
# stub _cprintf_s_l
@ cdecl _cputs(str)
# stub _cputws
@ cdecl _creat(str long)
# stub _crtAssertBusy
# stub _crtBreakAlloc
# stub _crtDbgFlag
@ varargs _cscanf(str)
# @ varargs _cscanf_l(str ptr)
# @ varargs _cscanf_s(str)
# @ varargs _cscanf_s_l(str ptr)
@ cdecl _ctime32(ptr)
@ cdecl _ctime32_s(str long ptr)
@ cdecl _ctime64(ptr)
@ cdecl _ctime64_s(str long ptr)
@ extern _ctype
@ cdecl _cwait(ptr long long)
@ varargs _cwprintf(wstr)
# stub _cwprintf_l
# stub _cwprintf_p
# stub _cwprintf_p_l
# stub _cwprintf_s
# stub _cwprintf_s_l
# @ varargs _cwscanf(wstr)
# @ varargs _cwscanf_l(wstr ptr)
# @ varargs _cwscanf_s(wstr)
# @ varargs _cwscanf_s_l(wstr ptr)
@ extern _daylight
@ cdecl _difftime32(long long)
@ cdecl _difftime64(long long)
@ extern _dstbias
@ cdecl _dup(long)
@ cdecl _dup2(long long)
@ cdecl _ecvt(double long ptr ptr)
# stub _ecvt_s
@ cdecl _endthread()
@ cdecl _endthreadex(long)
@ extern _environ
@ cdecl _eof(long)
@ cdecl _errno()
@ cdecl -i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl -i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr)
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
# stub _expand_dbg
@ cdecl _fcloseall()
@ cdecl _fcvt(double long ptr ptr)
# stub _fcvt_s
@ cdecl _fdopen(long str)
@ cdecl _fgetchar()
@ cdecl _fgetwchar()
@ cdecl _filbuf(ptr)
@ extern _fileinfo
@ cdecl _filelength(long)
@ cdecl -ret64 _filelengthi64(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst(str ptr)
@ cdecl _findfirst64(str ptr)
@ cdecl _findfirsti64(str ptr)
@ cdecl _findnext(long ptr)
@ cdecl _findnext64(long ptr)
@ cdecl _findnexti64(long ptr)
@ cdecl _finite(double)
@ cdecl _flsbuf(long ptr)
@ cdecl _flushall()
@ extern _fmode
@ cdecl _fpclass(double)
@ cdecl _fpieee_flt(long ptr ptr)
@ cdecl _fpreset()
# stub _fprintf_l
# stub _fprintf_p
# stub _fprintf_p_l
# stub _fprintf_s_l
@ cdecl _fputchar(long)
@ cdecl _fputwchar(long)
# stub _free_dbg
# stub _freea
# stub _freea_s
# stub _fscanf_l
# @ varargs _fscanf_l(ptr str ptr)
# @ varargs _fscanf_s_l(ptr str ptr)
@ cdecl _fseeki64(ptr long long long)
@ cdecl _fsopen(str str long)
@ cdecl _fstat(long ptr)
@ cdecl _fstat64(long ptr)
@ cdecl _fstati64(long ptr)
@ cdecl -ret64 _ftelli64(ptr)
@ cdecl _ftime(ptr)
@ cdecl _ftime32(ptr)
# stub _ftime32_s
@ cdecl _ftime64(ptr)
# stub _ftime64_s
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl -arch=i386 -ret64 _ftol2() _ftol
@ cdecl -arch=i386 -ret64 _ftol2_sse() _ftol #FIXME: SSE variant should be implemented
# stub _ftol2_sse_excpt
@ cdecl _fullpath(ptr str long)
# stub _fullpath_dbg
@ cdecl _futime(long ptr)
@ cdecl _futime32(long ptr)
@ cdecl _futime64(long ptr)
# stub _fwprintf_l
# stub _fwprintf_p
# stub _fwprintf_p_l
# stub _fwprintf_s_l
# @ varargs _fwscanf_l(ptr wstr ptr)
# @ varargs _fwscanf_s_l(ptr wstr ptr)
@ cdecl _gcvt(double long str)
# stub _gcvt_s
@ cdecl _get_doserrno(ptr)
# stub _get_environ
@ cdecl _get_errno(ptr)
# stub _get_fileinfo
# stub _get_fmode
# @ cdecl _get_heap_handle()
@ cdecl _get_osfhandle(long)
@ cdecl _get_osplatform(ptr) 
# stub _get_osver
@ cdecl _get_output_format()
@ cdecl _get_pgmptr(ptr)
@ cdecl _get_sbh_threshold()
# stub _get_wenviron
# stub _get_winmajor
# stub _get_winminor
# stub _get_winver
@ cdecl _get_wpgmptr(ptr)
@ cdecl _get_terminate()
@ cdecl _get_tzname(ptr str long long)
@ cdecl _get_unexpected()
@ cdecl _getch()
@ cdecl _getche()
@ cdecl _getcwd(str long)
@ cdecl _getdcwd(long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives() kernel32.GetLogicalDrives
@ cdecl _getmaxstdio()
@ cdecl _getmbcp()
@ cdecl _getpid() kernel32.GetCurrentProcessId
@ cdecl _getsystime(ptr)
@ cdecl _getw(ptr)
# stub _getwch
# stub _getwche
@ cdecl _getws(ptr)
@ cdecl -i386 _global_unwind2(ptr)
@ cdecl _gmtime32(ptr)
@ cdecl _gmtime32_s(ptr ptr)
@ cdecl _gmtime64(ptr)
@ cdecl _gmtime64_s(ptr ptr)
@ cdecl _heapadd(ptr long)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapset(long)
@ cdecl _heapused(ptr ptr)
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _i64toa(long long ptr long)
@ cdecl _i64toa_s(long long ptr long long) 
@ cdecl _i64tow(long long ptr long)
@ cdecl _i64tow_s(long long ptr long long) 
@ cdecl _initterm(ptr ptr)
@ cdecl _initterm_e(ptr ptr)
@ cdecl -arch=i386 _inp(long) MSVCRT__inp
@ cdecl -arch=i386 _inpd(long) MSVCRT__inpd
@ cdecl -arch=i386 _inpw(long) MSVCRT__inpw
@ cdecl _invalid_parameter(wstr wstr wstr long long)
@ extern _iob
# stub _isalnum_l
# stub _isalpha_l
@ cdecl _isatty(long)
# stub _iscntrl_l
@ cdecl _isctype(long long)
# stub _isctype_l
# stub _isdigit_l
# stub _isgraph_l
# stub _isleadbyte_l
# stub _islower_l
@ cdecl _ismbbalnum(long)
# stub _ismbbalnum_l
@ cdecl _ismbbalpha(long)
# stub _ismbbalpha_l
@ cdecl _ismbbgraph(long)
# stub _ismbbgraph_l
@ cdecl _ismbbkalnum(long)
# stub _ismbbkalnum_l
@ cdecl _ismbbkana(long)
# stub _ismbbkana_l
@ cdecl _ismbbkprint(long)
# stub _ismbbkprint_l
@ cdecl _ismbbkpunct(long)
# stub _ismbbkpunct_l
@ cdecl _ismbblead(long)
# stub _ismbblead_l
@ cdecl _ismbbprint(long)
# stub _ismbbprint_l
@ cdecl _ismbbpunct(long)
# stub _ismbbpunct_l
@ cdecl _ismbbtrail(long)
# stub _ismbbtrail_l
@ cdecl _ismbcalnum(long)
# stub _ismbcalnum_l
@ cdecl _ismbcalpha(long)
# stub _ismbcalpha_l
@ cdecl _ismbcdigit(long)
# stub _ismbcdigit_l
@ cdecl _ismbcgraph(long)
# stub _ismbcgraph_l
@ cdecl _ismbchira(long)
# stub _ismbchira_l
@ cdecl _ismbckata(long)
# stub _ismbckata_l
@ cdecl _ismbcl0(long)
# stub _ismbcl0_l
@ cdecl _ismbcl1(long)
# stub _ismbcl1_l
@ cdecl _ismbcl2(long)
# stub _ismbcl2_l
@ cdecl _ismbclegal(long)
# stub _ismbclegal_l
@ cdecl _ismbclower(long)
# stub _ismbclower_l
@ cdecl _ismbcprint(long)
# stub _ismbcprint_l
@ cdecl _ismbcpunct(long)
# stub _ismbcpunct_l
@ cdecl _ismbcspace(long)
# stub _ismbcspace_l
@ cdecl _ismbcsymbol(long)
# stub _ismbcsymbol_l
@ cdecl _ismbcupper(long)
# stub _ismbcupper_l
@ cdecl _ismbslead(ptr ptr)
# stub _ismbslead_l
@ cdecl _ismbstrail(ptr ptr)
# stub _ismbstrail_l
@ cdecl _isnan(double)
# stub _isprint_l
# stub _isspace_l
# stub _isupper_l
# stub _iswalnum_l
# stub _iswalpha_l
# stub _iswcntrl_l
# stub _iswctype_l
# stub _iswdigit_l
# stub _iswgraph_l
# stub _iswlower_l
# stub _iswprint_l
# stub _iswpunct_l
# stub _iswspace_l
# stub _iswupper_l
# stub _iswxdigit_l
# stub _isxdigit_l
@ cdecl _itoa(long ptr long)
@ cdecl _itoa_s(long ptr long long)
@ cdecl _itow(long ptr long)
@ cdecl _itow_s(long ptr long long)
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl _lfind(ptr ptr ptr long ptr)
# stub _lfind_s
@ cdecl _loaddll(str)
@ cdecl -i386 _local_unwind2(ptr long)
@ cdecl -i386 _local_unwind4(ptr ptr long)
@ cdecl _localtime32(ptr)
@ cdecl _localtime32_s(ptr ptr)
@ cdecl _localtime64(ptr)
@ cdecl _localtime64_s(ptr ptr)
@ cdecl _lock(long)
@ cdecl _locking(long long long)
@ cdecl _logb(double)
@ cdecl -i386 _longjmpex(ptr long) longjmp
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr long long ptr)
# stub _lsearch_s
@ cdecl _lseek(long long long)
@ cdecl -ret64 _lseeki64(long double long)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltoa_s(long ptr long long)
@ cdecl _ltow(long ptr long)
@ cdecl _ltow_s(long ptr long long)
@ cdecl _makepath(ptr str str str str)
@ cdecl _makepath_s(ptr long str str str str)
# stub _malloc_dbg
@ cdecl _mbbtombc(long)
# stub _mbbtombc_l
@ cdecl _mbbtype(long long)
@ extern _mbcasemap
@ cdecl _mbccpy (str str)
# stub _mbccpy_l
# stub _mbccpy_s
# stub _mbccpy_s_l
@ cdecl _mbcjistojms(long)
# stub _mbcjistojms_l
@ cdecl _mbcjmstojis(long)
# stub _mbcjmstojis_l
@ cdecl _mbclen(ptr)
# stub _mbclen_l
@ cdecl _mbctohira(long)
# stub _mbctohira_l
@ cdecl _mbctokata(long)
# stub _mbctokata_l
@ cdecl _mbctolower(long)
# stub _mbctolower_l
@ cdecl _mbctombb(long)
# stub _mbctombb_l
@ cdecl _mbctoupper(long)
# stub _mbctoupper_l
@ extern _mbctype
# stub _mblen_l
@ cdecl _mbsbtype(str long)
# stub _mbsbtype_l
@ cdecl _mbscat(str str)
# stub _mbscat_s
# stub _mbscat_s_l
@ cdecl _mbschr(str long)
# stub _mbschr_l
@ cdecl _mbscmp(str str)
# stub _mbscmp_l
@ cdecl _mbscoll(str str)
# stub _mbscoll_l
@ cdecl _mbscpy(ptr str)
# stub _mbscpy_s
# stub _mbscpy_s_l
@ cdecl _mbscspn(str str)
# stub _mbscspn_l
@ cdecl _mbsdec(ptr ptr)
# stub _mbsdec_l
@ cdecl _mbsdup(str)
# stub _strdup_dbg
@ cdecl _mbsicmp(str str)
# stub _mbsicmp_l
@ cdecl _mbsicoll(str str)
# stub _mbsicoll_l
@ cdecl _mbsinc(str)
# stub _mbsinc_l
@ cdecl _mbslen(str)
# stub _mbslen_l
@ cdecl _mbslwr(str)
# stub _mbslwr_l
# stub _mbslwr_s
# stub _mbslwr_s_l
@ cdecl _mbsnbcat(str str long)
# stub _mbsnbcat_l
# stub _mbsnbcat_s
# stub _mbsnbcat_s_l
@ cdecl _mbsnbcmp(str str long)
# stub _mbsnbcmp_l
@ cdecl _mbsnbcnt(ptr long)
# stub _mbsnbcnt_l
@ cdecl _mbsnbcoll(str str long)
# stub _mbsnbcoll_l
@ cdecl _mbsnbcpy(ptr str long)
# stub _mbsnbcpy_l
@ cdecl _mbsnbcpy_s(ptr long str long)
# stub _mbsnbcpy_s_l
@ cdecl _mbsnbicmp(str str long)
# stub _mbsnbicmp_l
@ cdecl _mbsnbicoll(str str long)
# stub _mbsnbicoll_l
@ cdecl _mbsnbset(str long long)
# stub _mbsnbset_l
# stub _mbsnbset_s
# stub _mbsnbset_s_l
@ cdecl _mbsncat(str str long)
# stub _mbsncat_l
# stub _mbsncat_s
# stub _mbsncat_s_l
@ cdecl _mbsnccnt(str long)
# stub _mbsnccnt_l
@ cdecl _mbsncmp(str str long)
# stub _mbsncmp_l
@ cdecl _mbsncoll(str str long)
# stub _mbsncoll_l
@ cdecl _mbsncpy(str str long)
# stub _mbsncpy_l
# stub _mbsncpy_s
# stub _mbsncpy_s_l
@ cdecl _mbsnextc(str)
# stub _mbsnextc_l
@ cdecl _mbsnicmp(str str long)
# stub _mbsnicmp_l
@ cdecl _mbsnicoll(str str long)
# stub _mbsnicoll_l
@ cdecl _mbsninc(str long)
# stub _mbsninc_l
# stub _mbsnlen
# stub _mbsnlen_l
@ cdecl _mbsnset(str long long)
# stub _mbsnset_l
# stub _mbsnset_s
# stub _mbsnset_s_l
@ cdecl _mbspbrk(str str)
# stub _mbspbrk_l
@ cdecl _mbsrchr(str long)
# stub _mbsrchr_l
@ cdecl _mbsrev(str)
# stub _mbsrev_l
@ cdecl _mbsset(str long)
# stub _mbsset_l
# stub _mbsset_s
# stub _mbsset_s_l
@ cdecl _mbsspn(str str)
# stub _mbsspn_l
@ cdecl _mbsspnp(str str)
# stub _mbsspnp_l
@ cdecl _mbsstr(str str)
# stub _mbsstr_l
@ cdecl _mbstok(str str)
# stub _mbstok_l
# stub _mbstok_s
# stub _mbstok_s_l
# stub _mbstowcs_l
# stub _mbstowcs_s_l
@ cdecl _mbstrlen(str)
# stub _mbstrlen_l
# stub _mbstrnlen
# stub _mbstrnlen_l
@ cdecl _mbsupr(str)
# stub _mbsupr_l
# stub _mbsupr_s
# stub _mbsupr_s_l
# stub _mbtowc_l
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
# stub _memicmp_l
@ cdecl _mkdir(str)
@ cdecl _mkgmtime(ptr)
@ cdecl _mkgmtime32(ptr)
@ cdecl _mkgmtime64(ptr)
@ cdecl _mktemp(str)
# stub _mktemp_s
@ cdecl _mktime32(ptr)
@ cdecl _mktime64(ptr)
@ cdecl _msize(ptr)
# stub _msize_debug
@ cdecl _nextafter(double double)
@ cdecl _onexit(ptr)
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ extern _osplatform
@ extern _osver
@ cdecl _outp(long long) MSVCRT__outp
@ cdecl _outpd(long long) MSVCRT__outpd
@ cdecl _outpw(long long) MSVCRT__outpw
@ cdecl _pclose(ptr)
@ extern _pctype
@ extern _pgmptr
@ cdecl _pipe(ptr long long)
@ cdecl _popen(str str)
# stub _printf_l
# stub _printf_p
# stub _printf_p_l
# stub _printf_s_l
@ cdecl _purecall()
@ cdecl _putch(long)
@ cdecl _putenv(str)
# stub _putenv_s
@ cdecl _putw(long ptr)
@ cdecl _putwch(long)
@ cdecl _putws(wstr)
# extern _pwctype
@ cdecl _read(long ptr long)
# stub _realloc_dbg
@ cdecl _resetstkoflw()
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
# stub _rotl64
@ cdecl _rotr(long long)
# stub _rotr64
@ cdecl -arch=i386 _safe_fdiv()
@ cdecl -arch=i386 _safe_fdivr()
@ cdecl -arch=i386 _safe_fprem()
@ cdecl -arch=i386 _safe_fprem1()
@ cdecl _scalb(double long)
# @ varargs _scanf_l(str ptr)
# @ varargs _scanf_s_l(str ptr)
# stub _scprintf
# stub _scprintf_l
# stub _scprintf_p_l
# stub _scwprintf
# stub _scwprintf_l
# stub _scwprintf_p_l
@ cdecl _searchenv(str str ptr)
@ cdecl _searchenv_s(str str ptr long)
@ stdcall -i386 _seh_longjmp_unwind4(ptr)
@ stdcall -i386 _seh_longjmp_unwind(ptr)
# stub _set_SSE2_enable
# stub _set_controlfp
@ cdecl _set_doserrno(long)
@ cdecl _set_errno(long)
@ cdecl _set_error_mode(long)
# stub _set_fileinfo
# stub _set_fmode
# stub _set_output_format
@ cdecl _set_sbh_threshold(long)
@ cdecl _seterrormode(long)
@ cdecl -arch=i386,x86_64 -norelay _setjmp(ptr)
@ cdecl -arch=i386 -norelay _setjmp3(ptr long)
@ cdecl -arch=x86_64 -norelay _setjmpex(ptr ptr)
@ cdecl _setmaxstdio(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ cdecl _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _snprintf(ptr long str)
# stub _snprintf_c
# stub _snprintf_c_l
# stub _snprintf_l
# stub _snprintf_s
# stub _snprintf_s_l
# stub _snscanf
# stub _snscanf_l
# stub _snscanf_s
# stub _snscanf_s_l
@ varargs _snwprintf(ptr long wstr)
# stub _snwprintf_l
# stub _snwprintf_s
# stub _snwprintf_s_l
# stub _snwscanf
# stub _snwscanf_l
# stub _snwscanf_s
# stub _snwscanf_s_l
@ varargs _sopen(str long long)
@ cdecl _sopen_s(ptr str long long long)
@ varargs _spawnl(long str str)
@ varargs _spawnle(long str str)
@ varargs _spawnlp(long str str)
@ varargs _spawnlpe(long str str)
@ cdecl _spawnv(long str ptr)
@ cdecl _spawnve(long str ptr ptr)
@ cdecl _spawnvp(long str ptr)
@ cdecl _spawnvpe(long str ptr ptr)
@ cdecl _splitpath(str ptr ptr ptr ptr)
# stub _splitpath_s
# stub _sprintf_l
# stub _sprintf_p_l
# stub _sprintf_s_l
# @ varargs _sscanf_l(str str ptr)
# @ varargs _sscanf_s_l(str str ptr)
@ cdecl _stat(str ptr)
@ cdecl _stat64(str ptr)
@ cdecl _stati64(str ptr)
@ cdecl _statusfp()
@ cdecl _strcmpi(str str)
# stub _strcoll_l
@ cdecl _strdate(ptr)
# stub _strdate_s
@ cdecl _strdup(str)
# stub _strdup_dbg
@ cdecl _strerror(long)
# stub _strerror_s
@ cdecl _stricmp(str str)
# stub _stricmp_l
@ cdecl _stricoll(str str)
# stub _stricoll_l
@ cdecl _strlwr(str)
# stub _strlwr_l
# stub _strlwr_s
# stub _strlwr_s_l
@ cdecl _strncoll(str str long)
# stub _strncoll_l
@ cdecl _strnicmp(str str long)
# stub _strnicmp_l
@ cdecl _strnicoll(str str long)
# stub _strnicoll_l
@ cdecl _strnset(str long long)
# stub _strnset_s
@ cdecl _strrev(str)
@ cdecl _strset(str long)
# stub _strset_s
@ cdecl _strtime(ptr)
# stub _strtime_s
# @ cdecl _strtod_l(str ptr ptr)
@ cdecl _strtoi64(str ptr long)
# @ cdecl _strtoi64_l(str ptr long ptr)
# stub _strtol_l
@ cdecl _strtoui64(str ptr long) strtoull
# @ cdecl _strtoui64_l(str ptr long ptr)
# stub _strtoul_l
@ cdecl _strupr(str)
# stub _strupr_l
# stub _strupr_s
# stub _strupr_s_l
# stub _strxfrm_l
@ cdecl _swab(str str long)
@ varargs _swprintf(ptr str) swprintf
# stub _swprintf_c
# stub _swprintf_c_l
# stub _swprintf_p_l
# stub _swprintf_s_l
# @ varargs _swscanf_l(wstr wstr ptr)
# @ varargs _swscanf_s_l(wstr wstr ptr)
@ extern _sys_errlist
@ extern _sys_nerr
@ cdecl _tell(long)
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str)
# stub _tempnam_dbg
@ cdecl _time32(ptr)
@ cdecl _time64(ptr)
@ extern _timezone
@ cdecl _tolower(long)
# stub _tolower_l
@ cdecl _toupper(long)
# stub _toupper_l
# stub _towlower_l
# stub _towupper_l
@ extern _tzname
@ cdecl _tzset()
@ cdecl _ui64toa(long long ptr long)
@ cdecl _ui64toa_s(long long ptr long long)
@ cdecl _ui64tow(long long ptr long)
@ cdecl _ui64tow_s(long long ptr long long)
@ cdecl _ultoa(long ptr long)
# stub _ultoa_s
@ cdecl _ultow(long ptr long)
# stub _ultow_s
@ cdecl _umask(long)
# stub _umask_s
@ cdecl _ungetch(long)
# stub _ungetwch
@ cdecl _unlink(str)
@ cdecl _unloaddll(long)
@ cdecl _unlock(long)
@ cdecl _utime32(str ptr)
@ cdecl _utime64(str ptr)
# stub _vcprintf
# stub _vcprintf_l
# stub _vcprintf_p
# stub _vcprintf_p_l
# stub _vcprintf_s
# stub _vcprintf_s_l
@ cdecl _vcwprintf(wstr ptr)
# stub _vcwprintf_l
# stub _vcwprintf_p
# stub _vcwprintf_p_l
# stub _vcwprintf_s
# stub _vcwprintf_s_l
# stub _vfprintf_l
# stub _vfprintf_p
# stub _vfprintf_p_l
# stub _vfprintf_s_l
# stub _vfwprintf_l
# stub _vfwprintf_p
# stub _vfwprintf_p_l
# stub _vfwprintf_s_l
# stub _vprintf_l
# stub _vprintf_p
# stub _vprintf_p_l
# stub _vprintf_s_l
@ cdecl _utime(str ptr)
@ cdecl _vscprintf(str ptr)
# stub _vscprintf_l
# stub _vscprintf_p_l
@ cdecl _vscwprintf(wstr ptr)
# stub _vscwprintf_l
# stub _vscwprintf_p_l
@ cdecl _vsnprintf(ptr long str ptr)
@ cdecl _vsnprintf_c(ptr long str ptr) _vsnprintf
# @ cdecl _vsnprintf_c_l(ptr long str ptr ptr) _vsnprintf_l
# @ cdecl _vsnprintf_l(ptr long str ptr ptr)
# @ cdecl _vsnprintf_s(ptr long long str ptr)
# @ cdecl _vsnprintf_s_l(ptr long long str ptr ptr)
@ cdecl _vsnwprintf(ptr long wstr ptr)
# @ cdecl _vsnwprintf_l(ptr long wstr ptr ptr)
# @ cdecl _vsnwprintf_s(ptr long long wstr ptr)
# @ cdecl _vsnwprintf_s_l(ptr long long wstr ptr ptr)
# stub _vsprintf_l
@ cdecl _vsprintf_p(ptr long str ptr)
# stub _vsprintf_p_l
# stub _vsprintf_s_l
# @ cdecl _vswprintf(ptr wstr ptr)
@ cdecl _vswprintf_c(ptr long wstr ptr) _vsnwprintf
# @ cdecl _vswprintf_c_l(ptr long wstr ptr ptr) _vsnwprintf_l
# @ cdecl _vswprintf_l(ptr wstr ptr ptr)
# @ cdecl _vswprintf_p_l(ptr long wstr ptr ptr) _vsnwprintf_l
# @ cdecl _vswprintf_s_l(ptr long wstr ptr ptr)
# stub _vwprintf_l
# stub _vwprintf_p
# stub _vwprintf_p_l
# stub _vwprintf_s_l
@ cdecl _waccess(wstr long)
@ cdecl _waccess_s(wstr long)
@ cdecl _wasctime(ptr)
# stub _wasctime_s
# stub _wassert
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ extern _wcmdln
@ cdecl _wcreat(wstr long)
# stub _wcscoll_l
@ cdecl _wcsdup(wstr)
# stub _wcsdup_dbg
@ cdecl _wcserror(long)
@ cdecl _wcserror_s(ptr long long)
# stub _wcsftime_l
@ cdecl _wcsicmp(wstr wstr)
# stub _wcsicmp_l
@ cdecl _wcsicoll(wstr wstr)
# stub _wcsicoll_l
@ cdecl _wcslwr(wstr)
# stub _wcslwr_l
# stub _wcslwr_s
# stub _wcslwr_s_l
@ cdecl _wcsncoll(wstr wstr long)
# stub _wcsncoll_l
@ cdecl _wcsnicmp(wstr wstr long)
# stub _wcsnicmp_l
@ cdecl _wcsnicoll(wstr wstr long)
# stub _wcsnicoll_l
@ cdecl _wcsnset(wstr long long)
# stub _wcsnset_s
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
# stub _wcsset_s
@ cdecl _wcstoi64(wstr ptr long)
# @ cdecl _wcstoi64_l(wstr ptr long ptr)
# stub _wcstol_l
# stub _wcstombs_l
# @ cdecl _wcstombs_s_l(ptr ptr long wstr long ptr)
@ cdecl _wcstoui64(wstr ptr long)
# @ cdecl _wcstoui64_l(wstr ptr long ptr)
# stub _wcstoul_l
@ cdecl _wcsupr(wstr)
# stub _wcsupr_l
@ cdecl _wcsupr_s(wstr long)
# stub _wcsupr_s_l
# stub _wcsxfrm_l
@ cdecl _wctime(ptr)
@ cdecl _wctime32(ptr)
# stub _wctime32_s
@ cdecl _wctime64(ptr)
# stub _wctime64_s
# stub _wctomb_l
# stub _wctomb_s_l
# stub _wctype
@ extern _wenviron
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr)
@ cdecl _wfindfirst(wstr ptr)
# stub _wfindfirst64
@ cdecl _wfindfirsti64(wstr ptr)
@ cdecl _wfindnext(long ptr)
# stub _wfindnext64
@ cdecl _wfindnexti64(long ptr)
@ cdecl _wfopen(wstr wstr)
@ cdecl _wfopen_s(ptr wstr wstr)
@ cdecl _wfreopen(wstr wstr ptr)
# stub _wfreopen_s
@ cdecl _wfsopen(wstr wstr long)
@ cdecl _wfullpath(ptr wstr long)
# stub _wfullpath_dbg
@ cdecl _wgetcwd(wstr long)
@ cdecl _wgetdcwd(long wstr long)
@ cdecl _wgetenv(wstr)
# stub _wgetenv_s
@ extern _winmajor
@ extern _winminor
# stub _winput_s
@ extern _winver
@ cdecl _wmakepath(ptr wstr wstr wstr wstr)
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
# stub _wmktemp_s
@ varargs _wopen(wstr long)
# stub _woutput_s
@ cdecl _wperror(wstr)
@ extern _wpgmptr
@ cdecl _wpopen(wstr wstr)
# stub _wprintf_l
# stub _wprintf_p
# stub _wprintf_p_l
# stub _wprintf_s_l
@ cdecl _wputenv(wstr)
# stub _wputenv_s
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long)
@ cdecl _wrmdir(wstr)
# @ varargs _wscanf_l(wstr ptr)
# @ varargs _wscanf_s_l(wstr ptr)
@ cdecl _wsearchenv(wstr wstr ptr)
# stub _wsearchenv_s
@ cdecl _wsetlocale(long wstr)
@ varargs _wsopen(wstr long long)
@ cdecl _wsopen_s(ptr wstr long long long)
@ varargs _wspawnl(long wstr wstr)
@ varargs _wspawnle(long wstr wstr)
@ varargs _wspawnlp(long wstr wstr)
@ varargs _wspawnlpe(long wstr wstr)
@ cdecl _wspawnv(long wstr ptr)
@ cdecl _wspawnve(long wstr ptr ptr)
@ cdecl _wspawnvp(long wstr ptr)
@ cdecl _wspawnvpe(long wstr ptr ptr)
@ cdecl _wsplitpath(wstr ptr ptr ptr ptr)
# @ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long)
@ cdecl _wstat(wstr ptr)
@ cdecl _wstati64(wstr ptr)
@ cdecl _wstat64(wstr ptr)
@ cdecl _wstrdate(ptr)
# stub _wstrdate_s
@ cdecl _wstrtime(ptr)
# stub _wstrtime_s
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr)
# stub _wtempnam_dbg
@ cdecl _wtmpnam(ptr)
# stub _wtmpnam_s
# @ cdecl _wtof(wstr)
# @ cdecl _wtof_l(wstr ptr)
@ cdecl _wtoi(wstr)
@ cdecl _wtoi64(wstr)
# stub _wtoi64_l
# stub _wtoi_l
@ cdecl _wtol(wstr)
# stub _wtol_l
@ cdecl _wunlink(wstr)
@ cdecl _wutime(wstr ptr)
@ cdecl _wutime32(wstr ptr)
@ cdecl _wutime64(wstr ptr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double )
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl asctime(ptr)
# stub asctime_s
@ cdecl asin(double)
@ cdecl atan(double)
@ cdecl atan2(double double)
@ extern atexit # <-- keep this as an extern, thank you
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl bsearch(ptr ptr long long ptr)
# stub bsearch_s
# @ cdecl btowc(long)
@ cdecl calloc(long long)
@ cdecl ceil(double)
@ cdecl -arch=x86_64 ceilf(double)
@ cdecl clearerr(ptr)
# stub clearerr_s
@ cdecl clock()
@ cdecl cos(double)
@ cdecl -arch=x86_64 cosf(double)
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
@ cdecl fgetws(ptr long ptr)
@ cdecl floor(double)
@ cdecl -arch=x86_64 floorf(long)
@ cdecl fmod(double double)
@ cdecl -arch=x86_64 fmodf(long)
@ cdecl fopen(str str)
@ cdecl fopen_s(ptr str str)
@ varargs fprintf(ptr str)
@ varargs fprintf_s(ptr str)
@ cdecl fputc(long ptr)
@ cdecl fputs(str ptr)
@ cdecl fputwc(long ptr)
@ cdecl fputws(wstr ptr)
@ cdecl fread(ptr long long ptr)
@ cdecl free(ptr)
@ cdecl freopen(str str ptr)
# stub freopen_s
@ cdecl frexp(double ptr)
@ varargs fscanf(ptr str)
# @ varargs fscanf_s(ptr str)
@ cdecl fseek(ptr long long)
@ cdecl fsetpos(ptr ptr)
@ cdecl ftell(ptr)
@ varargs fwprintf(ptr wstr)
@ varargs fwprintf_s(ptr wstr)
@ cdecl fwrite(ptr long long ptr)
@ varargs fwscanf(ptr wstr)
# @ varargs fwscanf_s(ptr wstr)
@ cdecl getc(ptr)
@ cdecl getchar()
@ cdecl getenv(str)
# stub getenv_s
@ cdecl gets(str)
@ cdecl getwc(ptr)
@ cdecl getwchar()
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
@ cdecl ldexp(double long)
@ cdecl ldiv(long long)
@ cdecl localeconv()
@ cdecl localtime(ptr)
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl -arch=x86_64 logf(double)
@ cdecl -i386 longjmp(ptr long)
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
# stub mbrlen
# stub mbrtowc
# stub mbsdup_dbg
# stub mbsrtowcs
# stub mbsrtowcs_s
@ cdecl mbstowcs(ptr str long)
# stub mbstowcs_s
@ cdecl mbtowc(wstr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memcpy_s(ptr long ptr long) memmove_s
@ cdecl memmove(ptr ptr long)
@ cdecl memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long)
@ cdecl mktime(ptr)
@ cdecl modf(double ptr)
@ cdecl perror(str)
@ cdecl pow(double double)
@ cdecl -arch=x86_64 powf(long)
@ varargs printf(str)
@ varargs printf_s(str)
@ cdecl putc(long ptr)
@ cdecl putchar(long)
@ cdecl puts(str)
@ cdecl putwc(long ptr) fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr)
# stub qsort_s
@ cdecl raise(long)
@ cdecl rand()
@ cdecl rand_s(ptr)
@ cdecl realloc(ptr long)
@ cdecl remove(str)
@ cdecl rename(str str)
@ cdecl rewind(ptr)
@ varargs scanf(str)
# @ varargs scanf_s(str)
@ cdecl setbuf(ptr ptr)
@ cdecl -arch=x86_64 -norelay -private setjmp(ptr ptr) _setjmp
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long)
@ cdecl signal(long long)
@ cdecl sin(double)
@ cdecl -arch=x86_64 sinf(long)
@ cdecl sinh(double)
@ varargs sprintf(ptr str)
# @ varargs sprintf_s(ptr long str)
@ cdecl sqrt(double)
@ cdecl -arch=x86_64 sqrtf(long)
@ cdecl srand(long)
@ varargs sscanf(str str)
# @ varargs sscanf_s(str str)
@ cdecl strcat(str str)
@ cdecl strcat_s(str long str)
@ cdecl strchr(str long)
@ cdecl strcmp(str str)
@ cdecl strcoll(str str)
@ cdecl strcpy(ptr str)
@ cdecl strcpy_s(ptr long str)
@ cdecl strcspn(str str)
@ cdecl strerror(long)
@ cdecl strerror_s(ptr long long)
@ cdecl strftime(str long str ptr)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
# stub strncat_s
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strncpy_s(ptr long str long)
@ cdecl strnlen(str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtok(str str)
@ cdecl strtok_s(ptr str ptr)
@ cdecl strtol(str ptr long)
@ cdecl strtoul(str ptr long)
@ cdecl strxfrm(ptr str long)
@ varargs swprintf(ptr wstr)
# @ varargs swprintf_s(ptr long wstr)
@ varargs swscanf(wstr wstr)
# @ varargs swscanf_s(wstr wstr)
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl tanh(double)
@ cdecl time(ptr)
@ cdecl tmpfile()
# stub tmpfile_s
@ cdecl tmpnam(ptr)
# stub tmpnam_s
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl ungetc(long ptr)
@ cdecl ungetwc(long ptr)
# stub utime
@ cdecl vfprintf(ptr str ptr)
@ cdecl vfprintf_s(ptr str ptr)
@ cdecl vfwprintf(ptr wstr ptr)
@ cdecl vfwprintf_s(ptr wstr ptr)
@ cdecl vprintf(str ptr)
@ cdecl vprintf_s(str ptr)
# stub vsnprintf
@ cdecl vsprintf(ptr str ptr)
# @ cdecl vsprintf_s(ptr long str ptr)
@ cdecl vswprintf(ptr wstr ptr)
# @ cdecl vswprintf_s(ptr long wstr ptr)
@ cdecl vwprintf(wstr ptr)
@ cdecl vwprintf_s(wstr ptr)
# stub wcrtomb
# stub wcrtomb_s
@ cdecl wcscat(wstr wstr)
@ cdecl wcscat_s(wstr long wstr)
@ cdecl wcschr(wstr long)
@ cdecl wcscmp(wstr wstr)
@ cdecl wcscoll(wstr wstr)
@ cdecl wcscpy(ptr wstr)
@ cdecl wcscpy_s(ptr long wstr)
@ cdecl wcscspn(wstr wstr)
@ cdecl wcsftime(ptr long wstr ptr)
@ cdecl wcslen(wstr)
@ cdecl wcsncat(wstr wstr long)
@ cdecl wcsncat_s(wstr long wstr long)
@ cdecl wcsncmp(wstr wstr long)
@ cdecl wcsncpy(ptr wstr long)
@ cdecl wcsncpy_s(ptr long wstr long)
# stub wcsnlen
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
# stub wcsrtombs
# stub wcsrtombs_s
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstok(wstr wstr)
@ cdecl wcstok_s(ptr wstr ptr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstombs(ptr ptr long)
# @ cdecl wcstombs_s(ptr ptr long wstr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
# stub wctob
@ cdecl wctomb(ptr long)
# stub wctomb_s
@ varargs wprintf(wstr)
@ varargs wprintf_s(wstr)
@ varargs wscanf(wstr)
# @ varargs wscanf_s(wstr)

# Functions not exported in native dll:
@ cdecl _get_invalid_parameter_handler()
@ cdecl _set_invalid_parameter_handler(ptr)
