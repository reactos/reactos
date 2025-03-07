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
@ cdecl -stub _Cbuild(ptr double double)
@ stub _Cmulcc
@ stub _Cmulcr
@ cdecl _CreateFrameInfo(ptr ptr)
@ cdecl -dbg _CrtCheckMemory()
@ cdecl -dbg _CrtDbgReport(long str long str str)
@ cdecl -dbg _CrtDbgReportW(long wstr long wstr wstr)
@ cdecl -dbg _CrtDoForAllClientObjects(ptr ptr)
@ cdecl -dbg _CrtDumpMemoryLeaks()
@ cdecl -dbg _CrtGetAllocHook()
@ cdecl -dbg _CrtGetDebugFillThreshold(long)
@ cdecl -dbg _CrtGetDumpClient()
@ cdecl -dbg _CrtGetReportHook()
@ cdecl -dbg _CrtIsMemoryBlock(ptr long ptr ptr ptr)
@ cdecl -dbg _CrtIsValidHeapPointer(ptr)
@ cdecl -dbg _CrtIsValidPointer() # FIXME: params
@ cdecl -dbg _CrtMemCheckpoint() # FIXME: params
@ cdecl -dbg _CrtMemDifference() # FIXME: params
@ cdecl -dbg _CrtMemDumpAllObjectsSince(ptr)
@ cdecl -dbg _CrtMemDumpStatistics() # FIXME: params
@ cdecl -dbg _CrtReportBlockType() # FIXME: params
@ cdecl -dbg _CrtSetAllocHook() # FIXME: params
@ cdecl -dbg _CrtSetBreakAlloc() # FIXME: params
@ cdecl -dbg _CrtSetDbgBlockType() # FIXME: params
@ cdecl -dbg _CrtSetDbgFlag() # FIXME: params
@ cdecl -dbg _CrtSetDebugFillThreshold() # FIXME: params
@ cdecl -dbg _CrtSetDumpClient() # FIXME: params
@ cdecl -dbg _CrtSetReportFile() # FIXME: params
@ cdecl -dbg _CrtSetReportHook() # FIXME: params
@ cdecl -dbg _CrtSetReportHook2() # FIXME: params
@ cdecl -dbg _CrtSetReportHookW2() # FIXME: params
@ cdecl -dbg _CrtSetReportMode() # FIXME: params
@ stdcall _CxxThrowException(ptr ptr)
@ cdecl -arch=i386 -norelay _EH_prolog()
@ cdecl _Exit(long)
@ stub _FCbuild
@ stub _FCmulcc
@ stub _FCmulcr
@ cdecl _FindAndUnlinkFrame(ptr)
@ stub _GetImageBase
@ stub _GetThrowImageBase
@ cdecl _Getdays()
@ cdecl _Getmonths()
@ cdecl _Gettnames()
@ cdecl _IsExceptionObjectToBeDestroyed(ptr)
@ stub _LCbuild
@ stub _LCmulcc
@ stub _LCmulcr
@ stub _SetImageBase
@ stub _SetThrowImageBase
@ stub _NLG_Dispatch2
@ stub _NLG_Return
@ stub _NLG_Return2
@ cdecl -stub _SetWinRTOutOfMemoryExceptionCallback(ptr)
@ cdecl _Strftime(ptr long str ptr ptr)
@ cdecl -dbg _VCrtDbgReportA(long ptr str long str str ptr)
@ cdecl -dbg _VCrtDbgReportW(long ptr wstr long wstr wstr ptr)
@ cdecl _W_Getdays()
@ cdecl _W_Getmonths()
@ cdecl _W_Gettnames()
@ cdecl _Wcsftime(ptr long wstr ptr ptr)
@ cdecl __AdjustPointer(ptr ptr)
@ stub __BuildCatchObject
@ stub __BuildCatchObjectHelper
@ cdecl -arch=!i386 __C_specific_handler(ptr long ptr ptr) ntdll.__C_specific_handler
@ cdecl -stub -arch=!i386 __C_specific_handler_noexcept(ptr long ptr ptr)
@ cdecl __CxxDetectRethrow(ptr)
@ cdecl __CxxExceptionFilter(ptr ptr long ptr)
@ cdecl -norelay __CxxFrameHandler(ptr ptr ptr ptr)
@ cdecl -norelay __CxxFrameHandler2(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -norelay __CxxFrameHandler3(ptr ptr ptr ptr) __CxxFrameHandler
@ cdecl -stub -arch=x86_64 __CxxFrameHandler4(ptr long ptr ptr)
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr)
@ cdecl __CxxQueryExceptionSize()
@ cdecl __CxxRegisterExceptionObject(ptr ptr)
@ cdecl __CxxUnregisterExceptionObject(ptr long)
@ cdecl __DestructExceptionObject(ptr)
@ stub __FrameUnwindFilter
@ stub __GetPlatformExceptionInfo
@ stub __NLG_Dispatch2
@ stub __NLG_Return2
@ cdecl __RTCastToVoid(ptr)
@ cdecl __RTDynamicCast(ptr long ptr ptr long)
@ cdecl __RTtypeid(ptr)
@ stub __TypeMatch
@ cdecl ___lc_codepage_func()
@ cdecl ___lc_collate_cp_func()
@ cdecl ___lc_locale_name_func()
@ cdecl ___mb_cur_max_func()
@ cdecl ___mb_cur_max_l_func(ptr)
@ cdecl __acrt_iob_func(long)
@ cdecl __conio_common_vcprintf(int64 str ptr ptr)
@ cdecl __conio_common_vcprintf_p(int64 str ptr ptr)
@ cdecl __conio_common_vcprintf_s(int64 str ptr ptr)
@ cdecl __conio_common_vcscanf(int64 str ptr ptr)
@ cdecl __conio_common_vcwprintf(int64 wstr ptr ptr)
@ cdecl __conio_common_vcwprintf_p(int64 wstr ptr ptr)
@ cdecl __conio_common_vcwprintf_s(int64 wstr ptr ptr)
@ cdecl __conio_common_vcwscanf(int64 wstr ptr ptr)
@ cdecl -stub -arch=i386 __control87_2(long long ptr ptr)
@ cdecl __current_exception()
@ cdecl __current_exception_context()
@ cdecl __daylight()
@ cdecl __dcrt_get_wide_environment_from_os() # FIXME: params
@ cdecl __dcrt_initial_narrow_environment()
@ cdecl __doserrno()
@ cdecl __dstbias()
@ cdecl -stub __fpe_flt_rounds()
@ cdecl __fpecode()
@ cdecl __initialize_lconv_for_unsigned_char()
@ cdecl -stub __intrinsic_abnormal_termination() # CHECKME
@ cdecl -stub -norelay __intrinsic_setjmp(ptr) # _setjmp
@ cdecl -stub -arch=!i386 -norelay __intrinsic_setjmpex(ptr ptr) # _setjmpex
@ cdecl __isascii(long)
@ cdecl __iscsym(long)
@ cdecl __iscsymf(long)
@ cdecl __iswcsym(long)
@ cdecl __iswcsymf(long)
@ stdcall -arch=arm __jump_unwind(ptr ptr) ntdll.__jump_unwind
@ cdecl -stub -arch=i386 -norelay __libm_sse2_acos()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_acosf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_asin()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_asinf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_atan()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_atan2()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_atanf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_cos()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_cosf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_exp()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_expf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_log()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_log10()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_log10f()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_logf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_pow()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_powf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_sin()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_sinf()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_tan()
@ cdecl -stub -arch=i386 -norelay __libm_sse2_tanf()
@ cdecl __p___argc()
@ cdecl __p___argv()
@ cdecl __p___wargv()
@ cdecl __p__acmdln()
@ cdecl __p__commode()
@ cdecl -dbg __p__crtBreakAlloc()
@ cdecl -dbg __p__crtDbgFlag()
@ cdecl __p__environ()
@ cdecl __p__fmode()
@ cdecl __p__mbcasemap()
@ cdecl __p__mbctype()
@ cdecl __p__pgmptr()
@ cdecl __p__wcmdln()
@ cdecl __p__wenviron()
@ cdecl __p__wpgmptr()
@ cdecl __pctype_func()
@ cdecl __processing_throw()
@ cdecl __pwctype_func()
@ cdecl __pxcptinfoptrs()
@ cdecl __report_gsfailure()
@ cdecl __setusermatherr(ptr)
@ cdecl __std_exception_copy(ptr ptr)
@ cdecl __std_exception_destroy(ptr)
@ cdecl __std_terminate() terminate
@ cdecl __std_type_info_compare(ptr ptr)
@ cdecl __std_type_info_destroy_list(ptr)
@ cdecl __std_type_info_hash(ptr)
@ cdecl __std_type_info_name(ptr ptr)
@ cdecl __stdio_common_vfprintf(int64 ptr str ptr ptr)
@ cdecl __stdio_common_vfprintf_p(int64 ptr str ptr ptr)
@ cdecl __stdio_common_vfprintf_s(int64 ptr str ptr ptr)
@ cdecl __stdio_common_vfscanf(int64 ptr str ptr ptr)
@ cdecl __stdio_common_vfwprintf(int64 ptr wstr ptr ptr)
@ cdecl __stdio_common_vfwprintf_p(int64 ptr wstr ptr ptr)
@ cdecl __stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr)
@ cdecl __stdio_common_vfwscanf(int64 ptr wstr ptr ptr)
@ cdecl __stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr)
@ cdecl __stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr)
@ cdecl -norelay __stdio_common_vsprintf(int64 ptr long str ptr ptr)
@ cdecl __stdio_common_vsprintf_p(int64 ptr long str ptr ptr)
@ cdecl __stdio_common_vsprintf_s(int64 ptr long str ptr ptr)
@ cdecl __stdio_common_vsscanf(int64 ptr long str ptr ptr)
@ cdecl __stdio_common_vswprintf(int64 ptr long wstr ptr ptr)
@ cdecl __stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr)
@ cdecl __stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr)
@ cdecl __stdio_common_vswscanf(int64 ptr long wstr ptr ptr)
@ cdecl __strncnt(str long)
@ cdecl __sys_errlist()
@ cdecl __sys_nerr()
@ cdecl __threadhandle()
@ cdecl __threadid()
@ cdecl __timezone()
@ cdecl __toascii(long)
@ cdecl __tzname()
@ cdecl __unDName(ptr str long ptr ptr long)
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long)
@ cdecl __uncaught_exception()
@ cdecl -stub __uncaught_exceptions()
@ cdecl __wcserror(wstr)
@ cdecl __wcserror_s(ptr long wstr)
@ cdecl __wcsncnt(wstr long)
@ cdecl -ret64 _abs64(int64)
@ cdecl _access(str long)
@ cdecl _access_s(str long)
@ cdecl _aligned_free(ptr)
@ cdecl -dbg _aligned_free_dbg(ptr)
@ cdecl _aligned_malloc(long long)
@ cdecl -dbg _aligned_malloc_dbg(long long str long)
@ cdecl _aligned_msize(ptr long long)
@ cdecl -dbg _aligned_msize_dbg(ptr long long)
@ cdecl _aligned_offset_malloc(long long long)
@ cdecl -dbg _aligned_offset_malloc_dbg(long long long str long)
@ cdecl _aligned_offset_realloc(ptr long long long)
@ cdecl -dbg _aligned_offset_realloc_dbg(ptr long long long str long)
@ cdecl _aligned_offset_recalloc(ptr long long long long)
@ cdecl -dbg _aligned_offset_recalloc_dbg(ptr long long long long str long)
@ cdecl _aligned_realloc(ptr long long)
@ cdecl -dbg _aligned_realloc_dbg(ptr long long str long)
@ cdecl _aligned_recalloc(ptr long long long)
@ cdecl -dbg _aligned_recalloc_dbg(ptr long long long str long)
@ cdecl _assert(str str long)
@ cdecl _atodbl(ptr str)
@ cdecl _atodbl_l(ptr str ptr)
@ cdecl _atof_l(str ptr)
@ cdecl _atoflt(ptr str)
@ cdecl _atoflt_l(ptr str ptr)
@ cdecl -ret64 _atoi64(str)
@ cdecl -ret64 _atoi64_l(str ptr)
@ cdecl _atoi_l(str ptr)
@ cdecl _atol_l(str ptr)
@ cdecl _atoldbl(ptr str)
@ cdecl _atoldbl_l(ptr str ptr)
@ cdecl -ret64 _atoll_l(str ptr)
@ cdecl _beep(long long)
@ cdecl _beginthread(ptr long ptr)
@ cdecl _beginthreadex(ptr long ptr ptr long ptr)
@ cdecl _byteswap_uint64(int64)
@ cdecl _byteswap_ulong(long)
@ cdecl _byteswap_ushort(long)
@ cdecl _c_exit()
@ cdecl -stub _cabs(long)
@ cdecl _callnewh(long)
@ cdecl _calloc_base(long long)
@ cdecl -dbg _calloc_dbg(long long long str long)
@ cdecl _cexit()
@ cdecl _cgets(ptr)
@ cdecl _cgets_s(str long ptr)
@ cdecl _cgetws(wstr)
@ cdecl _cgetws_s(wstr long ptr)
@ cdecl _chdir(str)
@ cdecl _chdrive(long)
@ cdecl _chgsign(double)
@ cdecl _chgsignf(float)
@ cdecl -arch=i386 -norelay _chkesp()
@ cdecl _chmod(str long)
@ cdecl _chsize(long long)
@ cdecl _chsize_s(long int64)
@ cdecl -dbg _chvalidator(long long)
@ cdecl -dbg _chvalidator_l(ptr long long)
@ cdecl _clearfp()
@ cdecl _close(long)
@ cdecl _commit(long)
@ cdecl _configthreadlocale(long)
@ cdecl _configure_narrow_argv(long)
@ cdecl _configure_wide_argv(long)
@ cdecl _control87(long long)
@ cdecl _controlfp(long long)
@ cdecl _controlfp_s(ptr long long)
@ cdecl _copysign(double double)
@ cdecl _copysignf(float float) copysignf
@ cdecl _cputs(str)
@ cdecl _cputws(wstr)
@ cdecl _creat(str long)
@ cdecl _create_locale(long str)
@ cdecl _crt_at_quick_exit(ptr)
@ cdecl _crt_atexit(ptr)
@ cdecl -stub _crt_debugger_hook(long)
@ cdecl _ctime32(ptr)
@ cdecl _ctime32_s(str long ptr)
@ cdecl _ctime64(ptr)
@ cdecl _ctime64_s(str long ptr)
@ cdecl _cwait(ptr long long)
@ cdecl -stub _d_int(ptr long)
@ cdecl _dclass(double)
@ cdecl -stub _dexp(ptr double long)
@ cdecl _difftime32(long long)
@ cdecl _difftime64(int64 int64)
@ cdecl -stub _dlog(double long)
@ cdecl -stub _dnorm(ptr)
@ cdecl -stub _dpcomp(double double)
@ cdecl -stub _dpoly(double ptr long)
@ cdecl -stub _dscale(ptr long)
@ cdecl -stub _dsign(double)
@ cdecl -stub _dsin(double long)
@ cdecl _dtest(ptr)
@ cdecl -stub _dunscale(ptr ptr)
@ cdecl _dup(long)
@ cdecl _dup2(long long)
@ cdecl _dupenv_s(ptr ptr str)
@ cdecl -dbg _dupenv_s_dbg(ptr ptr str long ptr long)
@ cdecl _ecvt(double long ptr ptr)
@ cdecl _ecvt_s(str long double long ptr ptr)
@ cdecl _endthread()
@ cdecl _endthreadex(long)
@ cdecl _eof(long)
@ cdecl _errno()
@ cdecl -stub _except1(long long double double long ptr)
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr)
@ cdecl -arch=i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr)
@ varargs _execl(str str)
@ varargs _execle(str str)
@ varargs _execlp(str str)
@ varargs _execlpe(str str)
@ cdecl _execute_onexit_table(ptr)
@ cdecl _execv(str ptr)
@ cdecl _execve(str ptr ptr)
@ cdecl _execvp(str ptr)
@ cdecl _execvpe(str ptr ptr)
@ cdecl _exit(long)
@ cdecl _expand(ptr long)
@ cdecl -dbg _expand_dbg(ptr long long str long)
@ cdecl _fclose_nolock(ptr)
@ cdecl _fcloseall()
@ cdecl _fcvt(double long ptr ptr)
@ cdecl _fcvt_s(ptr long double long ptr ptr)
@ cdecl -stub _fd_int(ptr long)
@ cdecl _fdclass(float)
@ cdecl -stub _fdexp(ptr float long)
@ cdecl -stub _fdlog(float long)
@ cdecl -stub _fdnorm(ptr)
@ cdecl _fdopen(long str)
@ cdecl -stub _fdpcomp(float float)
@ cdecl -stub _fdpoly(float ptr long)
@ cdecl -stub _fdscale(ptr long)
@ cdecl -stub _fdsign(float)
@ cdecl -stub _fdsin(float long)
@ cdecl _fdtest(ptr)
@ cdecl -stub _fdunscale(ptr ptr)
@ cdecl _fflush_nolock(ptr)
@ cdecl _fgetc_nolock(ptr)
@ cdecl _fgetchar()
@ cdecl _fgetwc_nolock(ptr)
@ cdecl _fgetwchar()
@ cdecl _filelength(long)
@ cdecl -ret64 _filelengthi64(long)
@ cdecl _fileno(ptr)
@ cdecl _findclose(long)
@ cdecl _findfirst32(str ptr)
@ cdecl _findfirst32i64(str ptr)
@ cdecl _findfirst64(str ptr)
@ cdecl _findfirst64i32(str ptr)
@ cdecl _findnext32(long ptr)
@ cdecl _findnext32i64(long ptr)
@ cdecl _findnext64(long ptr)
@ cdecl _findnext64i32(long ptr)
@ cdecl _finite(double)
@ cdecl -stub -arch=!i386 _finitef(float)
@ cdecl _flushall()
@ cdecl _fpclass(double)
@ cdecl -stub -arch=!i386 _fpclassf(float)
@ cdecl _fpieee_flt(long ptr ptr)
@ cdecl -stub _fpreset()
@ cdecl _fputc_nolock(long ptr)
@ cdecl _fputchar(long)
@ cdecl _fputwc_nolock(long ptr)
@ cdecl _fputwchar(long)
@ cdecl _fread_nolock(ptr long long ptr)
@ cdecl _fread_nolock_s(ptr long long long ptr)
@ cdecl _free_base(ptr)
@ cdecl -dbg _free_dbg(ptr long)
@ cdecl _free_locale(ptr)
@ cdecl _fseek_nolock(ptr long long)
@ cdecl _fseeki64(ptr int64 long)
@ cdecl _fseeki64_nolock(ptr int64 long)
@ cdecl _fsopen(str str long)
@ cdecl _fstat32(long ptr)
@ cdecl _fstat32i64(long ptr)
@ cdecl _fstat64(long ptr)
@ cdecl _fstat64i32(long ptr)
@ cdecl _ftell_nolock(ptr)
@ cdecl -ret64 _ftelli64(ptr)
@ cdecl -ret64 _ftelli64_nolock(ptr)
@ cdecl _ftime32(ptr)
@ cdecl _ftime32_s(ptr)
@ cdecl _ftime64(ptr)
@ cdecl _ftime64_s(ptr)
@ cdecl -arch=i386 -ret64 _ftol()
@ cdecl _fullpath(ptr str long)
@ cdecl -dbg _fullpath_dbg(ptr str long long str long)
@ cdecl _futime32(long ptr)
@ cdecl _futime64(long ptr)
@ cdecl _fwrite_nolock(ptr long long ptr)
@ cdecl _gcvt(double long str)
@ cdecl _gcvt_s(ptr long double long)
@ cdecl -stub -arch=win64 _get_FMA3_enable()
@ cdecl _get_current_locale()
@ cdecl _get_daylight(ptr)
@ cdecl _get_doserrno(ptr)
@ cdecl _get_dstbias(ptr)
@ cdecl _get_errno(ptr)
@ cdecl _get_fmode(ptr)
@ cdecl _get_heap_handle()
@ cdecl _get_initial_narrow_environment()
@ cdecl _get_initial_wide_environment()
@ cdecl _get_invalid_parameter_handler()
@ cdecl _get_narrow_winmain_command_line()
@ cdecl _get_osfhandle(long)
@ cdecl _get_pgmptr(ptr)
@ cdecl _get_printf_count_output()
@ cdecl -stub _get_purecall_handler()
@ cdecl _get_stream_buffer_pointers(ptr ptr ptr ptr)
@ cdecl _get_terminate()
@ cdecl _get_thread_local_invalid_parameter_handler()
@ cdecl _get_timezone(ptr)
@ cdecl _get_tzname(ptr str long long)
@ cdecl _get_unexpected()
@ cdecl _get_wide_winmain_command_line()
@ cdecl _get_wpgmptr(ptr)
@ cdecl _getc_nolock(ptr) _fgetc_nolock
@ cdecl _getch()
@ cdecl _getch_nolock()
@ cdecl _getche()
@ cdecl _getche_nolock()
@ cdecl _getcwd(ptr long)
@ cdecl -dbg _getcwd_dbg(ptr long long str long)
@ cdecl _getdcwd(long ptr long)
@ cdecl -dbg _getdcwd_dbg(long ptr long long str long)
@ cdecl _getdiskfree(long ptr)
@ cdecl _getdllprocaddr(long str long)
@ cdecl _getdrive()
@ cdecl _getdrives()
@ cdecl _getmaxstdio()
@ cdecl _getmbcp()
@ cdecl _getpid()
@ cdecl _getsystime(ptr)
@ cdecl _getw(ptr)
@ cdecl _getwc_nolock(ptr)
@ cdecl _getwch()
@ cdecl _getwch_nolock()
@ cdecl _getwche()
@ cdecl _getwche_nolock()
@ cdecl _getws(ptr)
@ cdecl _getws_s(ptr long)
@ cdecl -arch=i386 _global_unwind2(ptr)
@ cdecl _gmtime32(ptr)
@ cdecl _gmtime32_s(ptr ptr)
@ cdecl _gmtime64(ptr)
@ cdecl _gmtime64_s(ptr ptr)
@ cdecl _heapchk()
@ cdecl _heapmin()
@ cdecl _heapwalk(ptr)
@ cdecl _hypot(double double)
@ cdecl _hypotf(float float)
@ cdecl _i64toa(int64 ptr long)
@ cdecl _i64toa_s(int64 ptr long long)
@ cdecl _i64tow(int64 ptr long)
@ cdecl _i64tow_s(int64 ptr long long)
@ cdecl _initialize_narrow_environment()
@ cdecl _initialize_onexit_table(ptr)
@ cdecl _initialize_wide_environment()
@ cdecl _initterm(ptr ptr)
@ cdecl _initterm_e(ptr ptr)
@ cdecl _invalid_parameter(wstr wstr wstr long long)
@ cdecl _invalid_parameter_noinfo()
@ cdecl _invalid_parameter_noinfo_noreturn()
@ cdecl _invoke_watson(wstr wstr wstr long long)
@ cdecl _is_exception_typeof(ptr ptr)
@ cdecl _isalnum_l(long ptr)
@ cdecl _isalpha_l(long ptr)
@ cdecl _isatty(long)
@ cdecl _isblank_l(long ptr)
@ cdecl _iscntrl_l(long ptr)
@ cdecl _isctype(long long)
@ cdecl _isctype_l(long long ptr)
@ cdecl _isdigit_l(long ptr)
@ cdecl _isgraph_l(long ptr)
@ cdecl _isleadbyte_l(long ptr)
@ cdecl _islower_l(long ptr)
@ cdecl _ismbbalnum(long)
@ cdecl _ismbbalnum_l(long ptr)
@ cdecl _ismbbalpha(long)
@ cdecl _ismbbalpha_l(long ptr)
@ cdecl _ismbbblank(long)
@ cdecl _ismbbblank_l(long ptr)
@ cdecl _ismbbgraph(long)
@ cdecl _ismbbgraph_l(long ptr)
@ cdecl _ismbbkalnum(long)
@ cdecl _ismbbkalnum_l(long ptr)
@ cdecl _ismbbkana(long)
@ cdecl _ismbbkana_l(long ptr)
@ cdecl _ismbbkprint(long)
@ cdecl _ismbbkprint_l(long ptr)
@ cdecl _ismbbkpunct(long)
@ cdecl _ismbbkpunct_l(long ptr)
@ cdecl _ismbblead(long)
@ cdecl _ismbblead_l(long ptr)
@ cdecl _ismbbprint(long)
@ cdecl _ismbbprint_l(long ptr)
@ cdecl _ismbbpunct(long)
@ cdecl _ismbbpunct_l(long ptr)
@ cdecl _ismbbtrail(long)
@ cdecl _ismbbtrail_l(long ptr)
@ cdecl _ismbcalnum(long)
@ cdecl _ismbcalnum_l(long ptr)
@ cdecl _ismbcalpha(long)
@ cdecl _ismbcalpha_l(long ptr)
@ cdecl _ismbcblank(long)
@ cdecl _ismbcblank_l(long ptr)
@ cdecl _ismbcdigit(long)
@ cdecl _ismbcdigit_l(long ptr)
@ cdecl _ismbcgraph(long)
@ cdecl _ismbcgraph_l(long ptr)
@ cdecl _ismbchira(long)
@ cdecl _ismbchira_l(long ptr)
@ cdecl _ismbckata(long)
@ cdecl _ismbckata_l(long ptr)
@ cdecl _ismbcl0(long)
@ cdecl _ismbcl0_l(long ptr)
@ cdecl _ismbcl1(long)
@ cdecl _ismbcl1_l(long ptr)
@ cdecl _ismbcl2(long)
@ cdecl _ismbcl2_l(long ptr)
@ cdecl _ismbclegal(long)
@ cdecl _ismbclegal_l(long ptr)
@ cdecl _ismbclower(long)
@ cdecl _ismbclower_l(long ptr)
@ cdecl _ismbcprint(long)
@ cdecl _ismbcprint_l(long ptr)
@ cdecl _ismbcpunct(long)
@ cdecl _ismbcpunct_l(long ptr)
@ cdecl _ismbcspace(long)
@ cdecl _ismbcspace_l(long ptr)
@ cdecl _ismbcsymbol(long)
@ cdecl _ismbcsymbol_l(long ptr)
@ cdecl _ismbcupper(long)
@ cdecl _ismbcupper_l(long ptr)
@ cdecl _ismbslead(ptr ptr)
@ cdecl _ismbslead_l(ptr ptr ptr)
@ cdecl _ismbstrail(ptr ptr)
@ cdecl _ismbstrail_l(ptr ptr ptr)
@ cdecl _isnan(double)
@ cdecl -stub -arch=x86_64 _isnanf(float)
@ cdecl _isprint_l(long ptr)
@ cdecl _ispunct_l(long ptr)
@ cdecl _isspace_l(long ptr)
@ cdecl _isupper_l(long ptr)
@ cdecl _iswalnum_l(long ptr)
@ cdecl _iswalpha_l(long ptr)
@ cdecl _iswblank_l(long ptr)
@ cdecl _iswcntrl_l(long ptr)
@ cdecl _iswcsym_l(long ptr)
@ cdecl _iswcsymf_l(long ptr)
@ cdecl _iswctype_l(long long ptr)
@ cdecl _iswdigit_l(long ptr)
@ cdecl _iswgraph_l(long ptr)
@ cdecl _iswlower_l(long ptr)
@ cdecl _iswprint_l(long ptr)
@ cdecl _iswpunct_l(long ptr)
@ cdecl _iswspace_l(long ptr)
@ cdecl _iswupper_l(long ptr)
@ cdecl _iswxdigit_l(long ptr)
@ cdecl _isxdigit_l(long ptr)
@ cdecl _itoa(long ptr long)
@ cdecl _itoa_s(long ptr long long)
@ cdecl _itow(long ptr long)
@ cdecl _itow_s(long ptr long long)
@ cdecl _j0(double)
@ cdecl _j1(double)
@ cdecl _jn(long double)
@ cdecl _kbhit()
@ cdecl -stub _ld_int(ptr long)
@ cdecl _ldclass(double) _dclass
@ cdecl _ldexp(ptr double long) _dexp
@ cdecl _ldlog(double long) _dlog
@ cdecl _ldpcomp(double double) _dpcomp
@ cdecl _ldpoly(double ptr long) _dpoly
@ cdecl _ldscale(ptr long) _dscale
@ cdecl _ldsign(double) _dsign
@ cdecl _ldsin(double long) _dsin
@ cdecl _ldtest(ptr) _dtest
@ cdecl _ldunscale(ptr ptr) _dunscale
@ cdecl _lfind(ptr ptr ptr long ptr)
@ cdecl _lfind_s(ptr ptr ptr long ptr ptr)
@ cdecl -stub -arch=i386 -norelay _libm_sse2_acos_precise() #__libm_sse2_acos
@ cdecl -stub -arch=i386 -norelay _libm_sse2_asin_precise() #__libm_sse2_asin
@ cdecl -stub -arch=i386 -norelay _libm_sse2_atan_precise() #__libm_sse2_atan
@ cdecl -stub -arch=i386 -norelay _libm_sse2_cos_precise() #__libm_sse2_cos
@ cdecl -stub -arch=i386 -norelay _libm_sse2_exp_precise() #__libm_sse2_exp
@ cdecl -stub -arch=i386 -norelay _libm_sse2_log10_precise() #__libm_sse2_log10
@ cdecl -stub -arch=i386 -norelay _libm_sse2_log_precise() #__libm_sse2_log
@ cdecl -stub -arch=i386 -norelay _libm_sse2_pow_precise() #__libm_sse2_pow
@ cdecl -stub -arch=i386 -norelay _libm_sse2_sin_precise() #__libm_sse2_sin
@ cdecl -stub -arch=i386 -norelay _libm_sse2_sqrt_precise() #__libm_sse2_sqrt
@ cdecl -stub -arch=i386 -norelay _libm_sse2_tan_precise() #__libm_sse2_tan
@ cdecl _loaddll(str)
@ cdecl -arch=win64 _local_unwind(ptr ptr) ntdll._local_unwind
@ cdecl -arch=i386 _local_unwind2(ptr long)
@ cdecl -arch=i386 _local_unwind4(ptr ptr long)
@ cdecl _localtime32(ptr)
@ cdecl _localtime32_s(ptr ptr)
@ cdecl _localtime64(ptr)
@ cdecl _localtime64_s(ptr ptr)
@ cdecl _lock_file(ptr)
@ cdecl _lock_locales()
@ cdecl _locking(long long long)
@ cdecl _logb(double) # logb
@ cdecl -arch=!i386 _logbf(float) logbf
@ cdecl -stub -arch=i386 _longjmpex(ptr long)
@ cdecl _lrotl(long long)
@ cdecl _lrotr(long long)
@ cdecl _lsearch(ptr ptr ptr long ptr)
@ cdecl _lsearch_s(ptr ptr ptr long ptr ptr)
@ cdecl _lseek(long long long)
@ cdecl -ret64 _lseeki64(long int64 long)
@ cdecl _ltoa(long ptr long)
@ cdecl _ltoa_s(long ptr long long)
@ cdecl _ltow(long ptr long)
@ cdecl _ltow_s(long ptr long long)
@ cdecl _makepath(ptr str str str str)
@ cdecl _makepath_s(ptr long str str str str)
@ cdecl _malloc_base(long)
@ cdecl -dbg _malloc_dbg(long long str long)
@ cdecl _mbbtombc(long)
@ cdecl _mbbtombc_l(long ptr)
@ cdecl _mbbtype(long long)
@ cdecl _mbbtype_l(long long ptr)
@ extern _mbcasemap
@ cdecl _mbccpy(ptr ptr)
@ cdecl _mbccpy_l(ptr ptr ptr)
@ cdecl _mbccpy_s(ptr long ptr ptr)
@ cdecl _mbccpy_s_l(ptr long ptr ptr ptr)
@ cdecl _mbcjistojms(long)
@ cdecl _mbcjistojms_l(long ptr)
@ cdecl _mbcjmstojis(long)
@ cdecl _mbcjmstojis_l(long ptr)
@ cdecl _mbclen(ptr)
@ cdecl _mbclen_l(ptr ptr)
@ cdecl _mbctohira(long)
@ cdecl _mbctohira_l(long ptr)
@ cdecl _mbctokata(long)
@ cdecl _mbctokata_l(long ptr)
@ cdecl _mbctolower(long)
@ cdecl _mbctolower_l(long ptr)
@ cdecl _mbctombb(long)
@ cdecl _mbctombb_l(long ptr)
@ cdecl _mbctoupper(long)
@ cdecl _mbctoupper_l(long ptr)
@ cdecl _mblen_l(str long ptr)
@ cdecl _mbsbtype(str long)
@ cdecl _mbsbtype_l(str long ptr)
@ cdecl _mbscat_s(ptr long str)
@ cdecl _mbscat_s_l(ptr long str ptr)
@ cdecl _mbschr(str long)
@ cdecl _mbschr_l(str long ptr)
@ cdecl _mbscmp(str str)
@ cdecl _mbscmp_l(str str ptr)
@ cdecl _mbscoll(str str)
@ cdecl _mbscoll_l(str str ptr)
@ cdecl _mbscpy_s(ptr long str)
@ cdecl _mbscpy_s_l(ptr long str ptr)
@ cdecl _mbscspn(str str)
@ cdecl _mbscspn_l(str str ptr)
@ cdecl _mbsdec(ptr ptr)
@ cdecl _mbsdec_l(ptr ptr ptr)
@ cdecl _mbsdup(str) _strdup
@ stub _mbsdup_dbg() # FIXME params
@ cdecl _mbsicmp(str str)
@ cdecl _mbsicmp_l(str str ptr)
@ cdecl _mbsicoll(str str)
@ cdecl _mbsicoll_l(str str ptr)
@ cdecl _mbsinc(str)
@ cdecl _mbsinc_l(str ptr)
@ cdecl _mbslen(str)
@ cdecl _mbslen_l(str ptr)
@ cdecl _mbslwr(str)
@ cdecl _mbslwr_l(str ptr)
@ cdecl _mbslwr_s(str long)
@ cdecl _mbslwr_s_l(str long ptr)
@ cdecl _mbsnbcat(str str long)
@ cdecl _mbsnbcat_l(str str long ptr)
@ cdecl _mbsnbcat_s(str long ptr long)
@ cdecl _mbsnbcat_s_l(str long ptr long ptr)
@ cdecl _mbsnbcmp(str str long)
@ cdecl _mbsnbcmp_l(str str long ptr)
@ cdecl _mbsnbcnt(ptr long)
@ cdecl _mbsnbcnt_l(ptr long ptr)
@ cdecl _mbsnbcoll(str str long)
@ cdecl _mbsnbcoll_l(str str long ptr)
@ cdecl _mbsnbcpy(ptr str long)
@ cdecl _mbsnbcpy_l(ptr str long ptr)
@ cdecl _mbsnbcpy_s(ptr long str long)
@ cdecl _mbsnbcpy_s_l(ptr long str long ptr)
@ cdecl _mbsnbicmp(str str long)
@ cdecl _mbsnbicmp_l(str str long ptr)
@ cdecl _mbsnbicoll(str str long)
@ cdecl _mbsnbicoll_l(str str long ptr)
@ cdecl _mbsnbset(ptr long long)
@ cdecl _mbsnbset_l(ptr long long ptr)
@ cdecl _mbsnbset_s(ptr long long long)
@ cdecl _mbsnbset_s_l(ptr long long long ptr)
@ cdecl _mbsncat(str str long)
@ cdecl _mbsncat_l(str str long ptr)
@ cdecl _mbsncat_s(ptr long str long)
@ cdecl _mbsncat_s_l(ptr long str long ptr)
@ cdecl _mbsnccnt(str long)
@ cdecl _mbsnccnt_l(str long ptr)
@ cdecl _mbsncmp(str str long)
@ cdecl _mbsncmp_l(str str long ptr)
@ cdecl _mbsncoll(str str long)
@ cdecl _mbsncoll_l(str str long ptr)
@ cdecl _mbsncpy(ptr str long)
@ cdecl _mbsncpy_l(ptr str long ptr)
@ cdecl _mbsncpy_s(ptr long str long)
@ cdecl _mbsncpy_s_l(ptr long str long ptr)
@ cdecl _mbsnextc(str)
@ cdecl _mbsnextc_l(str ptr)
@ cdecl _mbsnicmp(str str long)
@ cdecl _mbsnicmp_l(str str long ptr)
@ cdecl _mbsnicoll(str str long)
@ cdecl _mbsnicoll_l(str str long ptr)
@ cdecl _mbsninc(str long)
@ cdecl _mbsninc_l(str long ptr)
@ cdecl _mbsnlen(str long)
@ cdecl _mbsnlen_l(str long ptr)
@ cdecl _mbsnset(ptr long long)
@ cdecl _mbsnset_l(ptr long long ptr)
@ cdecl _mbsnset_s(ptr long long long)
@ cdecl _mbsnset_s_l(ptr long long long ptr)
@ cdecl _mbspbrk(str str)
@ cdecl _mbspbrk_l(str str ptr)
@ cdecl _mbsrchr(str long)
@ cdecl _mbsrchr_l(str long ptr)
@ cdecl _mbsrev(str)
@ cdecl _mbsrev_l(str ptr)
@ cdecl _mbsset(ptr long)
@ cdecl _mbsset_l(ptr long ptr)
@ cdecl _mbsset_s(ptr long long)
@ cdecl _mbsset_s_l(ptr long long ptr)
@ cdecl _mbsspn(str str)
@ cdecl _mbsspn_l(str str ptr)
@ cdecl _mbsspnp(str str)
@ cdecl _mbsspnp_l(str str ptr)
@ cdecl _mbsstr(str str)
@ cdecl _mbsstr_l(str str ptr)
@ cdecl _mbstok(str str)
@ cdecl _mbstok_l(str str ptr)
@ cdecl _mbstok_s(str str ptr)
@ cdecl _mbstok_s_l(str str ptr ptr)
@ cdecl _mbstowcs_l(ptr str long ptr)
@ cdecl _mbstowcs_s_l(ptr ptr long str long ptr)
@ cdecl _mbstrlen(str)
@ cdecl _mbstrlen_l(str ptr)
@ cdecl _mbstrnlen(str long)
@ cdecl _mbstrnlen_l(str long ptr)
@ cdecl _mbsupr(str)
@ cdecl _mbsupr_l(str ptr)
@ cdecl _mbsupr_s(str long)
@ cdecl _mbsupr_s_l(str long ptr)
@ cdecl _mbtowc_l(ptr str long ptr)
@ cdecl _memccpy(ptr ptr long long)
@ cdecl _memicmp(str str long)
@ cdecl _memicmp_l(str str long ptr)
@ cdecl _mkdir(str)
@ cdecl _mkgmtime32(ptr)
@ cdecl _mkgmtime64(ptr)
@ cdecl _mktemp(str)
@ cdecl _mktemp_s(str long)
@ cdecl _mktime32(ptr)
@ cdecl _mktime64(ptr)
@ cdecl _msize(ptr)
@ cdecl -dbg _msize_dbg(ptr long)
@ cdecl _nextafter(double double) nextafter
@ cdecl -arch=x86_64 _nextafterf(float float) nextafterf
@ cdecl -arch=i386 _o__CIacos() _CIacos
@ cdecl -arch=i386 _o__CIasin() _CIasin
@ cdecl -arch=i386 _o__CIatan() _CIatan
@ cdecl -arch=i386 _o__CIatan2() _CIatan2
@ cdecl -arch=i386 _o__CIcos() _CIcos
@ cdecl -arch=i386 _o__CIcosh() _CIcosh
@ cdecl -arch=i386 _o__CIexp() _CIexp
@ cdecl -arch=i386 _o__CIfmod() _CIfmod
@ cdecl -arch=i386 _o__CIlog() _CIlog
@ cdecl -arch=i386 _o__CIlog10() _CIlog10
@ cdecl -arch=i386 _o__CIpow() _CIpow
@ cdecl -arch=i386 _o__CIsin() _CIsin
@ cdecl -arch=i386 _o__CIsinh() _CIsinh
@ cdecl -arch=i386 _o__CIsqrt() _CIsqrt
@ cdecl -arch=i386 _o__CItan() _CItan
@ cdecl -arch=i386 _o__CItanh() _CItanh
@ cdecl _o__Getdays() _Getdays
@ cdecl _o__Getmonths() _Getmonths
@ cdecl _o__Gettnames() _Gettnames
@ cdecl _o__Strftime(ptr long str ptr ptr) _Strftime
@ cdecl _o__W_Getdays() _W_Getdays
@ cdecl _o__W_Getmonths() _W_Getmonths
@ cdecl _o__W_Gettnames() _W_Gettnames
@ cdecl _o__Wcsftime(ptr long wstr ptr ptr) _Wcsftime
@ cdecl _o____lc_codepage_func() ___lc_codepage_func
@ cdecl _o____lc_collate_cp_func() ___lc_collate_cp_func
@ cdecl _o____lc_locale_name_func() ___lc_locale_name_func
@ cdecl _o____mb_cur_max_func() ___mb_cur_max_func
@ cdecl _o___acrt_iob_func(long) __acrt_iob_func
@ cdecl _o___conio_common_vcprintf(int64 str ptr ptr) __conio_common_vcprintf
@ cdecl _o___conio_common_vcprintf_p() __conio_common_vcprintf_p # FIXME: params
@ cdecl _o___conio_common_vcprintf_s() __conio_common_vcprintf_s # FIXME: params
@ cdecl _o___conio_common_vcscanf() __conio_common_vcscanf # FIXME: params
@ cdecl _o___conio_common_vcwprintf(int64 wstr ptr ptr) __conio_common_vcwprintf
@ cdecl _o___conio_common_vcwprintf_p() __conio_common_vcwprintf_p # FIXME: params
@ cdecl _o___conio_common_vcwprintf_s() __conio_common_vcwprintf_s # FIXME: params
@ cdecl _o___conio_common_vcwscanf() __conio_common_vcwscanf # FIXME: params
@ cdecl _o___daylight() __daylight
@ cdecl _o___dstbias() __dstbias
@ cdecl _o___fpe_flt_rounds() __fpe_flt_rounds
@ cdecl -arch=i386 -norelay _o___libm_sse2_acos() __libm_sse2_acos
@ cdecl -arch=i386 -norelay _o___libm_sse2_acosf() __libm_sse2_acosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_asin() __libm_sse2_asin
@ cdecl -arch=i386 -norelay _o___libm_sse2_asinf() __libm_sse2_asinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan() __libm_sse2_atan
@ cdecl -arch=i386 -norelay _o___libm_sse2_atan2() __libm_sse2_atan2
@ cdecl -arch=i386 -norelay _o___libm_sse2_atanf() __libm_sse2_atanf
@ cdecl -arch=i386 -norelay _o___libm_sse2_cos() __libm_sse2_cos
@ cdecl -arch=i386 -norelay _o___libm_sse2_cosf() __libm_sse2_cosf
@ cdecl -arch=i386 -norelay _o___libm_sse2_exp() __libm_sse2_exp
@ cdecl -arch=i386 -norelay _o___libm_sse2_expf() __libm_sse2_expf
@ cdecl -arch=i386 -norelay _o___libm_sse2_log() __libm_sse2_log
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10() __libm_sse2_log10
@ cdecl -arch=i386 -norelay _o___libm_sse2_log10f() __libm_sse2_log10f
@ cdecl -arch=i386 -norelay _o___libm_sse2_logf() __libm_sse2_logf
@ cdecl -arch=i386 -norelay _o___libm_sse2_pow() __libm_sse2_pow
@ cdecl -arch=i386 -norelay _o___libm_sse2_powf() __libm_sse2_powf
@ cdecl -arch=i386 -norelay _o___libm_sse2_sin() __libm_sse2_sin
@ cdecl -arch=i386 -norelay _o___libm_sse2_sinf() __libm_sse2_sinf
@ cdecl -arch=i386 -norelay _o___libm_sse2_tan() __libm_sse2_tan
@ cdecl -arch=i386 -norelay _o___libm_sse2_tanf() __libm_sse2_tanf
@ cdecl _o___p___argc() __p___argc
@ cdecl _o___p___argv() __p___argv
@ cdecl _o___p___wargv() __p___wargv
@ cdecl _o___p__acmdln() __p__acmdln
@ cdecl _o___p__commode() __p__commode
@ cdecl _o___p__environ() __p__environ
@ cdecl _o___p__fmode() __p__fmode
@ cdecl _o___p__mbcasemap() __p__mbcasemap # FIXME: params
@ cdecl _o___p__mbctype() __p__mbctype
@ cdecl _o___p__pgmptr() __p__pgmptr
@ cdecl _o___p__wcmdln() __p__wcmdln
@ cdecl _o___p__wenviron() __p__wenviron
@ cdecl _o___p__wpgmptr() __p__wpgmptr
@ cdecl _o___pctype_func() __pctype_func
@ cdecl _o___pwctype_func() __pwctype_func # FIXME: params
@ cdecl _o___std_exception_copy(ptr ptr) __std_exception_copy
@ cdecl _o___std_exception_destroy(ptr) __std_exception_destroy
@ cdecl _o___std_type_info_destroy_list(ptr) __std_type_info_destroy_list
@ cdecl _o___std_type_info_name(ptr ptr) __std_type_info_name
@ cdecl _o___stdio_common_vfprintf(int64 ptr str ptr ptr) __stdio_common_vfprintf
@ cdecl _o___stdio_common_vfprintf_p(int64 ptr str ptr ptr) __stdio_common_vfprintf_p
@ cdecl _o___stdio_common_vfprintf_s(int64 ptr str ptr ptr) __stdio_common_vfprintf_s
@ cdecl _o___stdio_common_vfscanf(int64 ptr str ptr ptr) __stdio_common_vfscanf
@ cdecl _o___stdio_common_vfwprintf(int64 ptr wstr ptr ptr) __stdio_common_vfwprintf
@ cdecl _o___stdio_common_vfwprintf_p(int64 ptr wstr ptr ptr) __stdio_common_vfwprintf_p
@ cdecl _o___stdio_common_vfwprintf_s(int64 ptr wstr ptr ptr) __stdio_common_vfwprintf_s
@ cdecl _o___stdio_common_vfwscanf(int64 ptr wstr ptr ptr) __stdio_common_vfwscanf
@ cdecl _o___stdio_common_vsnprintf_s(int64 ptr long long str ptr ptr) __stdio_common_vsnprintf_s
@ cdecl _o___stdio_common_vsnwprintf_s(int64 ptr long long wstr ptr ptr) __stdio_common_vsnwprintf_s
@ cdecl _o___stdio_common_vsprintf(int64 ptr long str ptr ptr) __stdio_common_vsprintf
@ cdecl _o___stdio_common_vsprintf_p(int64 ptr long str ptr ptr) __stdio_common_vsprintf_p
@ cdecl _o___stdio_common_vsprintf_s(int64 ptr long str ptr ptr) __stdio_common_vsprintf_s
@ cdecl _o___stdio_common_vsscanf(int64 ptr long str ptr ptr) __stdio_common_vsscanf
@ cdecl _o___stdio_common_vswprintf(int64 ptr long wstr ptr ptr) __stdio_common_vswprintf
@ cdecl _o___stdio_common_vswprintf_p(int64 ptr long wstr ptr ptr) __stdio_common_vswprintf_p
@ cdecl _o___stdio_common_vswprintf_s(int64 ptr long wstr ptr ptr) __stdio_common_vswprintf_s
@ cdecl _o___stdio_common_vswscanf(int64 ptr long wstr ptr ptr) __stdio_common_vswscanf
@ cdecl _o___timezone() __timezone
@ cdecl _o___tzname() __tzname
@ cdecl _o___wcserror(wstr) __wcserror
@ cdecl _o__access(str long) _access
@ cdecl _o__access_s(str long) _access_s
@ cdecl _o__aligned_free(ptr) _aligned_free
@ cdecl _o__aligned_malloc(long long) _aligned_malloc
@ cdecl _o__aligned_msize(ptr long long) _aligned_msize
@ cdecl _o__aligned_offset_malloc(long long long) _aligned_offset_malloc
@ cdecl _o__aligned_offset_realloc(ptr long long long) _aligned_offset_realloc
@ cdecl _o__aligned_offset_recalloc() _aligned_offset_recalloc # FIXME: params
@ cdecl _o__aligned_realloc(ptr long long) _aligned_realloc
@ cdecl _o__aligned_recalloc() _aligned_recalloc # FIXME: params
@ cdecl _o__atodbl(ptr str) _atodbl
@ cdecl _o__atodbl_l(ptr str ptr) _atodbl_l
@ cdecl _o__atof_l(str ptr) _atof_l
@ cdecl _o__atoflt(ptr str) _atoflt
@ cdecl _o__atoflt_l(ptr str ptr) _atoflt_l
@ cdecl -ret64 _o__atoi64(str) _atoi64
@ cdecl -ret64 _o__atoi64_l(str ptr) _atoi64_l
@ cdecl _o__atoi_l(str ptr) _atoi_l
@ cdecl _o__atol_l(str ptr) _atol_l
@ cdecl _o__atoldbl(ptr str) _atoldbl
@ cdecl _o__atoldbl_l(ptr str ptr) _atoldbl_l
@ cdecl -ret64 _o__atoll_l(str ptr) _atoll_l
@ cdecl _o__beep(long long) _beep
@ cdecl _o__beginthread(ptr long ptr) _beginthread
@ cdecl _o__beginthreadex(ptr long ptr ptr long ptr) _beginthreadex
@ cdecl _o__cabs(long) _cabs
@ cdecl _o__callnewh(long) _callnewh
@ cdecl _o__calloc_base(long long) _calloc_base
@ cdecl _o__cexit() _cexit
@ cdecl _o__cgets(ptr) _cgets
@ cdecl _o__cgets_s() _cgets_s # FIXME: params
@ cdecl _o__cgetws() _cgetws # FIXME: params
@ cdecl _o__cgetws_s() _cgetws_s # FIXME: params
@ cdecl _o__chdir(str) _chdir
@ cdecl _o__chdrive(long) _chdrive
@ cdecl _o__chmod(str long) _chmod
@ cdecl _o__chsize(long long) _chsize
@ cdecl _o__chsize_s(long int64) _chsize_s
@ cdecl _o__close(long) _close
@ cdecl _o__commit(long) _commit
@ cdecl _o__configthreadlocale(long) _configthreadlocale
@ cdecl _o__configure_narrow_argv(long) _configure_narrow_argv
@ cdecl _o__configure_wide_argv(long) _configure_wide_argv
@ cdecl _o__controlfp_s(ptr long long) _controlfp_s
@ cdecl _o__cputs(str) _cputs
@ cdecl _o__cputws(wstr) _cputws
@ cdecl _o__creat(str long) _creat
@ cdecl _o__create_locale(long str) _create_locale
@ cdecl _o__crt_atexit(ptr) _crt_atexit
@ cdecl _o__ctime32_s(str long ptr) _ctime32_s
@ cdecl _o__ctime64_s(str long ptr) _ctime64_s
@ cdecl _o__cwait(ptr long long) _cwait
@ cdecl _o__d_int(ptr long) _d_int
@ cdecl _o__dclass(double) _dclass
@ cdecl _o__difftime32(long long) _difftime32
@ cdecl _o__difftime64(int64 int64) _difftime64
@ cdecl _o__dlog(double long) _dlog
@ cdecl _o__dnorm(ptr) _dnorm
@ cdecl _o__dpcomp(double double) _dpcomp
@ cdecl _o__dpoly(double ptr long) _dpoly
@ cdecl _o__dscale(ptr long) _dscale
@ cdecl _o__dsign(double) _dsign
@ cdecl _o__dsin(double long) _dsin
@ cdecl _o__dtest(ptr) _dtest
@ cdecl _o__dunscalee(ptr ptr) _dunscale
@ cdecl _o__dup(long) _dup
@ cdecl _o__dup2(long long) _dup2
@ cdecl _o__dupenv_s(ptr ptr str) _dupenv_s
@ cdecl _o__ecvt(double long ptr ptr) _ecvt
@ cdecl _o__ecvt_s(str long double long ptr ptr) _ecvt_s
@ cdecl _o__endthread() _endthread
@ cdecl _o__endthreadex(long) _endthreadex
@ cdecl _o__eof(long) _eof
@ cdecl _o__errno() _errno
@ cdecl _o__except1(long long double double long ptr) _except1
@ cdecl _o__execute_onexit_table(ptr) _execute_onexit_table
@ cdecl _o__execv(str ptr) _execv
@ cdecl _o__execve(str ptr ptr) _execve
@ cdecl _o__execvp(str ptr) _execvp
@ cdecl _o__execvpe(str ptr ptr) _execvpe
@ cdecl _o__exit(long) _exit
@ cdecl _o__expand(ptr long) _expand
@ cdecl _o__fclose_nolock(ptr) _fclose_nolock
@ cdecl _o__fcloseall() _fcloseall
@ cdecl _o__fcvt(double long ptr ptr) _fcvt
@ cdecl _o__fcvt_s(ptr long double long ptr ptr) _fcvt_s
@ cdecl _o__fd_int(ptr long) _fd_int
@ cdecl _o__fdclass(float) _fdclass
@ cdecl _o__fdexp(ptr float long) _fdexp
@ cdecl _o__fdlog(float long) _fdlog
@ cdecl _o__fdopen(long str) _fdopen
@ cdecl _o__fdpcomp(float float) _fdpcomp
@ cdecl _o__fdpoly(float ptr long) _fdpoly
@ cdecl _o__fdscale(ptr long) _fdscale
@ cdecl _o__fdsign(float) _fdsign
@ cdecl _o__fdsin(float long) _fdsin
@ cdecl _o__fflush_nolock(ptr) _fflush_nolock
@ cdecl _o__fgetc_nolock(ptr) _fgetc_nolock
@ cdecl _o__fgetchar() _fgetchar
@ cdecl _o__fgetwc_nolock(ptr) _fgetwc_nolock
@ cdecl _o__fgetwchar() _fgetwchar
@ cdecl _o__filelength(long) _filelength
@ cdecl -ret64 _o__filelengthi64(long) _filelengthi64
@ cdecl _o__fileno(ptr) _fileno
@ cdecl _o__findclose(long) _findclose
@ cdecl _o__findfirst32(str ptr) _findfirst32
@ cdecl _o__findfirst32i64() _findfirst32i64 # FIXME: params
@ cdecl _o__findfirst64(str ptr) _findfirst64
@ cdecl _o__findfirst64i32(str ptr) _findfirst64i32
@ cdecl _o__findnext32(long ptr) _findnext32
@ cdecl _o__findnext32i64() _findnext32i64 # FIXME: params
@ cdecl _o__findnext64(long ptr) _findnext64
@ cdecl _o__findnext64i32(long ptr) _findnext64i32
@ cdecl _o__flushall() _flushall
@ cdecl _o__fpclass(double) _fpclass
@ cdecl -arch=!i386 _o__fpclassf(float) _fpclassf
@ cdecl _o__fputc_nolock(long ptr) _fputc_nolock
@ cdecl _o__fputchar(long) _fputchar
@ cdecl _o__fputwc_nolock(long ptr) _fputwc_nolock
@ cdecl _o__fputwchar(long) _fputwchar
@ cdecl _o__fread_nolock(ptr long long ptr) _fread_nolock
@ cdecl _o__fread_nolock_s(ptr long long long ptr) _fread_nolock_s
@ cdecl _o__free_base(ptr) _free_base
@ cdecl _o__free_locale(ptr) _free_locale
@ cdecl _o__fseek_nolock(ptr long long) _fseek_nolock
@ cdecl _o__fseeki64(ptr int64 long) _fseeki64
@ cdecl _o__fseeki64_nolock(ptr int64 long) _fseeki64_nolock
@ cdecl _o__fsopen(str str long) _fsopen
@ cdecl _o__fstat32(long ptr) _fstat32
@ cdecl _o__fstat32i64(long ptr) _fstat32i64
@ cdecl _o__fstat64(long ptr) _fstat64
@ cdecl _o__fstat64i32(long ptr) _fstat64i32
@ cdecl _o__ftell_nolock(ptr) _ftell_nolock
@ cdecl -ret64 _o__ftelli64(ptr) _ftelli64
@ cdecl -ret64 _o__ftelli64_nolock(ptr) _ftelli64_nolock
@ cdecl _o__ftime32(ptr) _ftime32
@ cdecl _o__ftime32_s(ptr) _ftime32_s
@ cdecl _o__ftime64(ptr) _ftime64
@ cdecl _o__ftime64_s(ptr) _ftime64_s
@ cdecl _o__fullpath(ptr str long) _fullpath
@ cdecl _o__futime32(long ptr) _futime32
@ cdecl _o__futime64(long ptr) _futime64
@ cdecl _o__fwrite_nolock(ptr long long ptr) _fwrite_nolock
@ cdecl _o__gcvt(double long str) _gcvt
@ cdecl _o__gcvt_s(ptr long double long) _gcvt_s
@ cdecl _o__get_daylight(ptr) _get_daylight
@ cdecl _o__get_doserrno(ptr) _get_doserrno
@ cdecl _o__get_dstbias(ptr) _get_dstbias
@ cdecl _o__get_errno(ptr) _get_errno
@ cdecl _o__get_fmode(ptr) _get_fmode
@ cdecl _o__get_heap_handle() _get_heap_handle
@ cdecl _o__get_initial_narrow_environment() _get_initial_narrow_environment
@ cdecl _o__get_initial_wide_environment() _get_initial_wide_environment
@ cdecl _o__get_invalid_parameter_handler() _get_invalid_parameter_handler
@ cdecl _o__get_narrow_winmain_command_line() _get_narrow_winmain_command_line
@ cdecl _o__get_osfhandle(long) _get_osfhandle
@ cdecl _o__get_pgmptr(ptr) _get_pgmptr
@ cdecl _o__get_stream_buffer_pointers(ptr ptr ptr ptr) _get_stream_buffer_pointers
@ cdecl _o__get_terminate() _get_terminate
@ cdecl _o__get_thread_local_invalid_parameter_handler() _get_thread_local_invalid_parameter_handler
@ cdecl _o__get_timezone(ptr) _get_timezone
@ cdecl _o__get_tzname(ptr str long long) _get_tzname
@ cdecl _o__get_wide_winmain_command_line() _get_wide_winmain_command_line
@ cdecl _o__get_wpgmptr(ptr) _get_wpgmptr
@ cdecl _o__getc_nolock(ptr) _fgetc_nolock
@ cdecl _o__getch() _getch
@ cdecl _o__getch_nolock() _getch_nolock
@ cdecl _o__getche() _getche
@ cdecl _o__getche_nolock() _getche_nolock
@ cdecl _o__getcwd(str long) _getcwd
@ cdecl _o__getdcwd(long str long) _getdcwd
@ cdecl _o__getdiskfree(long ptr) _getdiskfree
@ cdecl _o__getdllprocaddr(long str long) _getdllprocaddr
@ cdecl _o__getdrive() _getdrive
@ cdecl _o__getdrives() _getdrives # kernel32.GetLogicalDrives
@ cdecl _o__getmbcp() _getmbcp
@ cdecl _o__getsystime() _getsystime # FIXME: params
@ cdecl _o__getw(ptr) _getw
@ cdecl _o__getwc_nolock(ptr) _fgetwc_nolock
@ cdecl _o__getwch() _getwch
@ cdecl _o__getwch_nolock() _getwch_nolock
@ cdecl _o__getwche() _getwche
@ cdecl _o__getwche_nolock() _getwche_nolock
@ cdecl _o__getws(ptr) _getws
@ cdecl _o__getws_s() _getws_s # FIXME: params
@ cdecl _o__gmtime32(ptr) _gmtime32
@ cdecl _o__gmtime32_s(ptr ptr) _gmtime32_s
@ cdecl _o__gmtime64(ptr) _gmtime64
@ cdecl _o__gmtime64_s(ptr ptr) _gmtime64_s
@ cdecl _o__heapchk() _heapchk
@ cdecl _o__heapmin() _heapmin
@ cdecl _o__hypot(double double) _hypot
@ cdecl _o__hypotf(float float) _hypotf
@ cdecl _o__i64toa(int64 ptr long) _i64toa
@ cdecl _o__i64toa_s(int64 ptr long long) _i64toa_s
@ cdecl _o__i64tow(int64 ptr long) _i64tow
@ cdecl _o__i64tow_s(int64 ptr long long) _i64tow_s
@ cdecl _o__initialize_narrow_environment() _initialize_narrow_environment
@ cdecl _o__initialize_onexit_table(ptr) _initialize_onexit_table
@ cdecl _o__initialize_wide_environment() _initialize_wide_environment
@ cdecl _o__invalid_parameter_noinfo() _invalid_parameter_noinfo
@ cdecl _o__invalid_parameter_noinfo_noreturn() _invalid_parameter_noinfo_noreturn
@ cdecl _o__isatty(long) _isatty
@ cdecl _o__isctype(long long) _isctype
@ cdecl _o__isctype_l(long long ptr) _isctype_l
@ cdecl _o__isleadbyte_l(long ptr) _isleadbyte_l
@ cdecl _o__ismbbalnum() _ismbbalnum # FIXME: params
@ cdecl _o__ismbbalnum_l() _ismbbalnum_l # FIXME: params
@ cdecl _o__ismbbalpha() _ismbbalpha # FIXME: params
@ cdecl _o__ismbbalpha_l() _ismbbalpha_l # FIXME: params
@ cdecl _o__ismbbblank() _ismbbblank # FIXME: params
@ cdecl _o__ismbbblank_l() _ismbbblank_l # FIXME: params
@ cdecl _o__ismbbgraph() _ismbbgraph # FIXME: params
@ cdecl _o__ismbbgraph_l() _ismbbgraph_l # FIXME: params
@ cdecl _o__ismbbkalnum() _ismbbkalnum # FIXME: params
@ cdecl _o__ismbbkalnum_l() _ismbbkalnum_l # FIXME: params
@ cdecl _o__ismbbkana(long) _ismbbkana
@ cdecl _o__ismbbkana_l(long ptr) _ismbbkana_l
@ cdecl _o__ismbbkprint() _ismbbkprint # FIXME: params
@ cdecl _o__ismbbkprint_l() _ismbbkprint_l # FIXME: params
@ cdecl _o__ismbbkpunct() _ismbbkpunct # FIXME: params
@ cdecl _o__ismbbkpunct_l() _ismbbkpunct_l # FIXME: params
@ cdecl _o__ismbblead(long) _ismbblead
@ cdecl _o__ismbblead_l(long ptr) _ismbblead_l
@ cdecl _o__ismbbprint() _ismbbprint # FIXME: params
@ cdecl _o__ismbbprint_l() _ismbbprint_l # FIXME: params
@ cdecl _o__ismbbpunct() _ismbbpunct # FIXME: params
@ cdecl _o__ismbbpunct_l() _ismbbpunct_l # FIXME: params
@ cdecl _o__ismbbtrail(long) _ismbbtrail
@ cdecl _o__ismbbtrail_l(long ptr) _ismbbtrail_l
@ cdecl _o__ismbcalnum(long) _ismbcalnum
@ cdecl _o__ismbcalnum_l(long ptr) _ismbcalnum_l
@ cdecl _o__ismbcalpha(long) _ismbcalpha
@ cdecl _o__ismbcalpha_l(long ptr) _ismbcalpha_l
@ cdecl _o__ismbcblank() _ismbcblank # FIXME: params
@ cdecl _o__ismbcblank_l() _ismbcblank_l # FIXME: params
@ cdecl _o__ismbcdigit(long) _ismbcdigit
@ cdecl _o__ismbcdigit_l(long ptr) _ismbcdigit_l
@ cdecl _o__ismbcgraph(long) _ismbcgraph
@ cdecl _o__ismbcgraph_l(long ptr) _ismbcgraph_l
@ cdecl _o__ismbchira(long) _ismbchira
@ cdecl _o__ismbchira_l(long ptr) _ismbchira_l
@ cdecl _o__ismbckata(long) _ismbckata
@ cdecl _o__ismbckata_l(long ptr) _ismbckata_l
@ cdecl _o__ismbcl0(long) _ismbcl0
@ cdecl _o__ismbcl0_l(long ptr) _ismbcl0_l
@ cdecl _o__ismbcl1(long) _ismbcl1
@ cdecl _o__ismbcl1_l(long ptr) _ismbcl1_l
@ cdecl _o__ismbcl2(long) _ismbcl2
@ cdecl _o__ismbcl2_l(long ptr) _ismbcl2_l
@ cdecl _o__ismbclegal(long) _ismbclegal
@ cdecl _o__ismbclegal_l(long ptr) _ismbclegal_l
@ cdecl _o__ismbclower() _ismbclower # FIXME: params
@ cdecl _o__ismbclower_l(long ptr) _ismbclower_l
@ cdecl _o__ismbcprint(long) _ismbcprint
@ cdecl _o__ismbcprint_l(long ptr) _ismbcprint_l
@ cdecl _o__ismbcpunct(long) _ismbcpunct
@ cdecl _o__ismbcpunct_l(long ptr) _ismbcpunct_l
@ cdecl _o__ismbcspace(long) _ismbcspace
@ cdecl _o__ismbcspace_l(long ptr) _ismbcspace_l
@ cdecl _o__ismbcsymbol(long) _ismbcsymbol
@ cdecl _o__ismbcsymbol_l(long ptr) _ismbcsymbol_l
@ cdecl _o__ismbcupper(long) _ismbcupper
@ cdecl _o__ismbcupper_l(long ptr) _ismbcupper_l
@ cdecl _o__ismbslead(ptr ptr) _ismbslead
@ cdecl _o__ismbslead_l(ptr ptr ptr) _ismbslead_l
@ cdecl _o__ismbstrail(ptr ptr) _ismbstrail
@ cdecl _o__ismbstrail_l(ptr ptr ptr) _ismbstrail_l
@ cdecl _o__iswctype_l(long long ptr) _iswctype_l
@ cdecl _o__itoa(long ptr long) _itoa
@ cdecl _o__itoa_s(long ptr long long) _itoa_s
@ cdecl _o__itow(long ptr long) _itow
@ cdecl _o__itow_s(long ptr long long) _itow_s
@ cdecl _o__j0(double) _j0
@ cdecl _o__j1(double) _j1
@ cdecl _o__jn(long double) _jn
@ cdecl _o__kbhit() _kbhit
@ cdecl _o__ld_int(ptr long) _ld_int
@ cdecl _o__ldclass(double) _dclass
@ cdecl _o__ldexp(ptr double long) _dexp
@ cdecl _o__ldlog(double long) _dlog
@ cdecl _o__ldpcomp(double double) _dpcomp
@ cdecl _o__ldpoly(double ptr long) _dpoly
@ cdecl _o__ldscale(ptr long) _dscale
@ cdecl _o__ldsign(double) _dsign
@ cdecl _o__ldsin(double long) _dsin
@ cdecl _o__ldtest(ptr) _dtest
@ cdecl _o__ldunscale(ptr ptr) _dunscale
@ cdecl _o__lfind(ptr ptr ptr long ptr) _lfind
@ cdecl _o__lfind_s(ptr ptr ptr long ptr ptr) _lfind_s
@ cdecl -arch=i386 -norelay _o__libm_sse2_acos_precise() _libm_sse2_acos_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_asin_precise() _libm_sse2_asin_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_atan_precise() _libm_sse2_atan_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_cos_precise() _libm_sse2_cos_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_exp_precise() _libm_sse2_exp_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_log10_precise() _libm_sse2_log10_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_log_precise() _libm_sse2_log_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_pow_precise() _libm_sse2_pow_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_sin_precise() _libm_sse2_sin_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_sqrt_precise() _libm_sse2_sqrt_precise
@ cdecl -arch=i386 -norelay _o__libm_sse2_tan_precise() _libm_sse2_tan_precise
@ cdecl _o__loaddll(str) _loaddll
@ cdecl _o__localtime32(ptr) _localtime32
@ cdecl _o__localtime32_s(ptr ptr) _localtime32_s
@ cdecl _o__localtime64(ptr) _localtime64
@ cdecl _o__localtime64_s(ptr ptr) _localtime64_s
@ cdecl _o__lock_file(ptr) _lock_file
@ cdecl _o__locking(long long long) _locking
@ cdecl _o__logb(double) logb
@ cdecl -arch=!i386 _o__logbf(float) logbf
@ cdecl _o__lsearch(ptr ptr ptr long ptr) _lsearch
@ cdecl _o__lsearch_s() _lsearch_s # FIXME: params
@ cdecl _o__lseek(long long long) _lseek
@ cdecl -ret64 _o__lseeki64(long int64 long) _lseeki64
@ cdecl _o__ltoa(long ptr long) _ltoa
@ cdecl _o__ltoa_s(long ptr long long) _ltoa_s
@ cdecl _o__ltow(long ptr long) _ltow
@ cdecl _o__ltow_s(long ptr long long) _ltow_s
@ cdecl _o__makepath(ptr str str str str) _makepath
@ cdecl _o__makepath_s(ptr long str str str str) _makepath_s
@ cdecl _o__malloc_base(long) _malloc_base
@ cdecl _o__mbbtombc(long) _mbbtombc
@ cdecl _o__mbbtombc_l(long ptr) _mbbtombc_l
@ cdecl _o__mbbtype(long long) _mbbtype
@ cdecl _o__mbbtype_l(long long ptr) _mbbtype_l
@ cdecl _o__mbccpy(ptr ptr) _mbccpy
@ cdecl _o__mbccpy_l(ptr ptr ptr) _mbccpy_l
@ cdecl _o__mbccpy_s(ptr long ptr ptr) _mbccpy_s
@ cdecl _o__mbccpy_s_l(ptr long ptr ptr ptr) _mbccpy_s_l
@ cdecl _o__mbcjistojms(long) _mbcjistojms
@ cdecl _o__mbcjistojms_l(long ptr) _mbcjistojms_l
@ cdecl _o__mbcjmstojis(long) _mbcjmstojis
@ cdecl _o__mbcjmstojis_l(long ptr) _mbcjmstojis_l
@ cdecl _o__mbclen(ptr) _mbclen
@ cdecl _o__mbclen_l(ptr ptr) _mbclen_l
@ cdecl _o__mbctohira(long) _mbctohira
@ cdecl _o__mbctohira_l(long ptr) _mbctohira_l
@ cdecl _o__mbctokata(long) _mbctokata
@ cdecl _o__mbctokata_l(long ptr) _mbctokata_l
@ cdecl _o__mbctolower(long) _mbctolower
@ cdecl _o__mbctolower_l(long ptr) _mbctolower_l
@ cdecl _o__mbctombb(long) _mbctombb
@ cdecl _o__mbctombb_l(long ptr) _mbctombb_l
@ cdecl _o__mbctoupper(long) _mbctoupper
@ cdecl _o__mbctoupper_l(long ptr) _mbctoupper_l
@ cdecl _o__mblen_l(str long ptr) _mblen_l
@ cdecl _o__mbsbtype(str long) _mbsbtype
@ cdecl _o__mbsbtype_l(str long ptr) _mbsbtype_l
@ cdecl _o__mbscat_s(ptr long str) _mbscat_s
@ cdecl _o__mbscat_s_l(ptr long str ptr) _mbscat_s_l
@ cdecl _o__mbschr(str long) _mbschr
@ cdecl _o__mbschr_l(str long ptr) _mbschr_l
@ cdecl _o__mbscmp(str str) _mbscmp
@ cdecl _o__mbscmp_l(str str ptr) _mbscmp_l
@ cdecl _o__mbscoll(str str) _mbscoll
@ cdecl _o__mbscoll_l(str str ptr) _mbscoll_l
@ cdecl _o__mbscpy_s(ptr long str) _mbscpy_s
@ cdecl _o__mbscpy_s_l(ptr long str ptr) _mbscpy_s_l
@ cdecl _o__mbscspn(str str) _mbscspn
@ cdecl _o__mbscspn_l(str str ptr) _mbscspn_l
@ cdecl _o__mbsdec(ptr ptr) _mbsdec
@ cdecl _o__mbsdec_l(ptr ptr ptr) _mbsdec_l
@ cdecl _o__mbsicmp(str str) _mbsicmp
@ cdecl _o__mbsicmp_l(str str ptr) _mbsicmp_l
@ cdecl _o__mbsicoll(str str) _mbsicoll
@ cdecl _o__mbsicoll_l(str str ptr) _mbsicoll_l
@ cdecl _o__mbsinc(str) _mbsinc
@ cdecl _o__mbsinc_l(str ptr) _mbsinc_l
@ cdecl _o__mbslen(str) _mbslen
@ cdecl _o__mbslen_l(str ptr) _mbslen_l
@ cdecl _o__mbslwr(str) _mbslwr
@ cdecl _o__mbslwr_l(str ptr) _mbslwr_l
@ cdecl _o__mbslwr_s(str long) _mbslwr_s
@ cdecl _o__mbslwr_s_l(str long ptr) _mbslwr_s_l
@ cdecl _o__mbsnbcat(str str long) _mbsnbcat
@ cdecl _o__mbsnbcat_l(str str long ptr) _mbsnbcat_l
@ cdecl _o__mbsnbcat_s(str long ptr long) _mbsnbcat_s
@ cdecl _o__mbsnbcat_s_l(str long ptr long ptr) _mbsnbcat_s_l
@ cdecl _o__mbsnbcmp(str str long) _mbsnbcmp
@ cdecl _o_mbsnbcmp_l(str str long ptr) _mbsnbcmp_l
@ cdecl _o__mbsnbcnt(ptr long) _mbsnbcnt
@ cdecl _o__mbsnbcnt_l(ptr long ptr) _mbsnbcnt_l
@ cdecl _o__mbsnbcoll(str str long) _mbsnbcoll
@ cdecl _o__mbsnbcoll_l(str str long ptr) _mbsnbcoll_l
@ cdecl _o__mbsnbcpy(ptr str long) _mbsnbcpy
@ cdecl _o__mbsnbcpy_l(ptr str long ptr) _mbsnbcpy_l
@ cdecl _o__mbsnbcpy_s(ptr long str long) _mbsnbcpy_s
@ cdecl _o__mbsnbcpy_s_l(ptr long str long ptr) _mbsnbcpy_s_l
@ cdecl _o__mbsnbicmp(str str long) _mbsnbicmp
@ cdecl _o__mbsnbicmp_l(str str long ptr) _mbsnbicmp_l
@ cdecl _o__mbsnbicoll(str str long) _mbsnbicoll
@ cdecl _o__mbsnbicoll_l(str str long ptr) _mbsnbicoll_l
@ cdecl _o__mbsnbset(ptr long long) _mbsnbset
@ cdecl _o__mbsnbset_l(str long long ptr) _mbsnbset_l
@ cdecl _o__mbsnbset_s() _mbsnbset_s # FIXME: params
@ cdecl _o__mbsnbset_s_l() _mbsnbset_s_l # FIXME: params
@ cdecl _o__mbsncat(str str long) _mbsncat
@ cdecl _o__mbsncat_l(str str long ptr) _mbsncat_l
@ cdecl _o__mbsncat_s(ptr long str long) _mbsncat_s
@ cdecl _o__mbsncat_s_l(ptr long str long ptr) _mbsncat_s_l
@ cdecl _o__mbsnccnt(str long) _mbsnccnt
@ cdecl _o__mbsnccnt_l(str long ptr) _mbsnccnt_l
@ cdecl _o__mbsncmp(str str long) _mbsncmp
@ cdecl _o__mbsncmp_l(str str long ptr) _mbsncmp_l
@ cdecl _o__mbsncoll() _mbsncoll # FIXME: params
@ cdecl _o__mbsncoll_l() _mbsncoll_l # FIXME: params
@ cdecl _o__mbsncpy(ptr str long) _mbsncpy
@ cdecl _o__mbsncpy_l(ptr str long ptr) _mbsncpy_l
@ cdecl _o__mbsncpy_s() _mbsncpy_s # FIXME: params
@ cdecl _o__mbsncpy_s_l() _mbsncpy_s_l # FIXME: params
@ cdecl _o__mbsnextc(str) _mbsnextc
@ cdecl _o__mbsnextc_l(str ptr) _mbsnextc_l
@ cdecl _o__mbsnicmp(str str long) _mbsnicmp
@ cdecl _o__mbsnicmp_l(str str long ptr) _mbsnicmp_l
@ cdecl _o__mbsnicoll() _mbsnicoll # FIXME: params
@ cdecl _o__mbsnicoll_l() _mbsnicoll_l # FIXME: params
@ cdecl _o__mbsninc(str long) _mbsninc
@ cdecl _o__mbsninc_l() _mbsninc_l # FIXME: params
@ cdecl _o__mbsnlen(str long) _mbsnlen
@ cdecl _o__mbsnlen_l(str long ptr) _mbsnlen_l
@ cdecl _o__mbsnset(ptr long long) _mbsnset
@ cdecl _o__mbsnset_l(ptr long long ptr) _mbsnset_l
@ cdecl _o__mbsnset_s() _mbsnset_s # FIXME: params
@ cdecl _o__mbsnset_s_l() _mbsnset_s_l # FIXME: params
@ cdecl _o__mbspbrk(str str) _mbspbrk
@ cdecl _o__mbspbrk_l(str str ptr) _mbspbrk_l
@ cdecl _o__mbsrchr(str long) _mbsrchr
@ cdecl _o__mbsrchr_l(str long ptr) _mbsrchr_l
@ cdecl _o__mbsrev(str) _mbsrev
@ cdecl _o__mbsrev_l(str ptr) _mbsrev_l
@ cdecl _o__mbsset(ptr long) _mbsset
@ cdecl _o__mbsset_l(ptr long ptr) _mbsset_l
@ cdecl _o__mbsset_s() _mbsset_s # FIXME: params
@ cdecl _o__mbsset_s_l() _mbsset_s_l # FIXME: params
@ cdecl _o__mbsspn(str str) _mbsspn
@ cdecl _o__mbsspn_l(str str ptr) _mbsspn_l
@ cdecl _o__mbsspnp(str str) _mbsspnp
@ cdecl _o__mbsspnp_l(str str ptr) _mbsspnp_l
@ cdecl _o__mbsstr(str str) _mbsstr
@ cdecl _o__mbsstr_l() _mbsstr_l # FIXME: params
@ cdecl _o__mbstok(str str) _mbstok
@ cdecl _o__mbstok_l(str str ptr) _mbstok_l
@ cdecl _o__mbstok_s(str str ptr) _mbstok_s
@ cdecl _o__mbstok_s_l(str str ptr ptr) _mbstok_s_l
@ cdecl _o__mbstowcs_l(ptr str long ptr) _mbstowcs_l
@ cdecl _o__mbstowcs_s_l(ptr ptr long str long ptr) _mbstowcs_s_l
@ cdecl _o__mbstrlen(str) _mbstrlen
@ cdecl _o__mbstrlen_l(str ptr) _mbstrlen_l
@ cdecl _o__mbstrnlen() _mbstrnlen # FIXME: params
@ cdecl _o__mbstrnlen_l() _mbstrnlen_l # FIXME: params
@ cdecl _o__mbsupr(str) _mbsupr
@ cdecl _o__mbsupr_l(str ptr) _mbsupr_l
@ cdecl _o__mbsupr_s(str long) _mbsupr_s
@ cdecl _o__mbsupr_s_l(str long ptr) _mbsupr_s_l
@ cdecl _o__mbtowc_l(ptr str long ptr) _mbtowc_l
@ cdecl _o__memicmp(str str long) _memicmp
@ cdecl _o__memicmp_l(str str long ptr) _memicmp_l
@ cdecl _o__mkdir(str) _mkdir
@ cdecl _o__mkgmtime32(ptr) _mkgmtime32
@ cdecl _o__mkgmtime64(ptr) _mkgmtime64
@ cdecl _o__mktemp(str) _mktemp
@ cdecl _o__mktemp_s(str long) _mktemp_s
@ cdecl _o__mktime32(ptr) _mktime32
@ cdecl _o__mktime64(ptr) _mktime64
@ cdecl _o__msize(ptr) _msize
@ cdecl _o__nextafter(double double) nextafter
@ cdecl -arch=x86_64 _o__nextafterf(float float) nextafterf
@ cdecl _o__open_osfhandle(long long) _open_osfhandle
@ cdecl _o__pclose(ptr) _pclose
@ cdecl _o__pipe(ptr long long) _pipe
@ cdecl _o__popen(str str) _popen
@ cdecl _o__purecall() _purecall
@ cdecl _o__putc_nolock(long ptr) _fputc_nolock
@ cdecl _o__putch(long) _putch
@ cdecl _o__putch_nolock(long) _putch_nolock
@ cdecl _o__putenv(str) _putenv
@ cdecl _o__putenv_s(str str) _putenv_s
@ cdecl _o__putw(long ptr) _putw
@ cdecl _o__putwc_nolock(long ptr) _fputwc_nolock
@ cdecl _o__putwch(long) _putwch
@ cdecl _o__putwch_nolock(long) _putwch_nolock
@ cdecl _o__putws(wstr) _putws
@ cdecl _o__read(long ptr long) _read
@ cdecl _o__realloc_base(ptr long) _realloc_base
@ cdecl _o__recalloc(ptr long long) _recalloc
@ cdecl _o__register_onexit_function(ptr ptr) _register_onexit_function
@ cdecl _o__resetstkoflw() _resetstkoflw
@ cdecl _o__rmdir(str) _rmdir
@ cdecl _o__rmtmp() _rmtmp
@ cdecl _o__scalb(double long) scalbn
@ cdecl -arch=x86_64 _o__scalbf(float long) scalbnf
@ cdecl _o__searchenv(str str ptr) _searchenv
@ cdecl _o__searchenv_s(str str ptr long) _searchenv_s
@ cdecl _o__seh_filter_dll(long ptr) _seh_filter_dll # __CppXcptFilter
@ cdecl _o__seh_filter_exe(long ptr) _seh_filter_exe # _XcptFilter
@ cdecl _o__set_abort_behavior(long long) _set_abort_behavior
@ cdecl _o__set_app_type(long) _set_app_type
@ cdecl _o__set_doserrno(long) _set_doserrno
@ cdecl _o__set_errno(long) _set_errno
@ cdecl _o__set_fmode(long) _set_fmode
@ cdecl _o__set_invalid_parameter_handler(ptr) _set_invalid_parameter_handler
@ cdecl _o__set_new_handler(ptr) _set_new_handler
@ cdecl _o__set_new_mode(long) _set_new_mode
@ cdecl _o__set_thread_local_invalid_parameter_handler(ptr) _set_thread_local_invalid_parameter_handler
@ cdecl _o__seterrormode(long) _seterrormode
@ cdecl _o__setmbcp(long) _setmbcp
@ cdecl _o__setmode(long long) _setmode
@ cdecl _o__setsystime() _setsystime # FIXME: params
@ cdecl _o__sleep(long) _sleep
@ varargs _o__sopen(str long long) _sopen
@ cdecl _o__sopen_dispatch(str long long long ptr long) _sopen_dispatch
@ cdecl _o__sopen_s(ptr str long long long) _sopen_s
@ cdecl _o__spawnv(long str ptr) _spawnv
@ cdecl _o__spawnve(long str ptr ptr) _spawnve
@ cdecl _o__spawnvp(long str ptr) _spawnvp
@ cdecl _o__spawnvpe(long str ptr ptr) _spawnvpe
@ cdecl _o__splitpath(str ptr ptr ptr ptr) _splitpath
@ cdecl _o__splitpath_s(str ptr long ptr long ptr long ptr long) _splitpath_s
@ cdecl _o__stat32(str ptr) _stat32
@ cdecl _o__stat32i64(str ptr) _stat32i64
@ cdecl _o__stat64(str ptr) _stat64
@ cdecl _o__stat64i32(str ptr) _stat64i32
@ cdecl _o__strcoll_l(str str ptr) _strcoll_l
@ cdecl _o__strdate(ptr) _strdate
@ cdecl _o__strdate_s(ptr long) _strdate_s
@ cdecl _o__strdup(str) _strdup
@ cdecl _o__strerror(long) _strerror
@ cdecl _o__strerror_s() _strerror_s # FIXME: params
@ cdecl _o__strftime_l(ptr long str ptr ptr) _strftime_l
@ cdecl _o__stricmp(str str) _stricmp
@ cdecl _o__stricmp_l(str str ptr) _stricmp_l
@ cdecl _o__stricoll(str str) _stricoll
@ cdecl _o__stricoll_l(str str ptr) _stricoll_l
@ cdecl _o__strlwr(str) _strlwr
@ cdecl _o__strlwr_l(str ptr) _strlwr_l
@ cdecl _o__strlwr_s(ptr long) _strlwr_s
@ cdecl _o__strlwr_s_l(ptr long ptr) _strlwr_s_l
@ cdecl _o__strncoll(str str long) _strncoll
@ cdecl _o__strncoll_l(str str long ptr) _strncoll_l
@ cdecl _o__strnicmp(str str long) _strnicmp
@ cdecl _o__strnicmp_l(str str long ptr) _strnicmp_l
@ cdecl _o__strnicoll(str str long) _strnicoll
@ cdecl _o__strnicoll_l(str str long ptr) _strnicoll_l
@ cdecl _o__strnset_s(str long long long) _strnset_s
@ cdecl _o__strset_s() _strset_s # FIXME: params
@ cdecl _o__strtime(ptr) _strtime
@ cdecl _o__strtime_s(ptr long) _strtime_s
@ cdecl _o__strtod_l(str ptr ptr) _strtod_l
@ cdecl _o__strtof_l(str ptr ptr) _strtof_l
@ cdecl -ret64 _o__strtoi64(str ptr long) _strtoi64
@ cdecl -ret64 _o__strtoi64_l(str ptr long ptr) _strtoi64_l
@ cdecl _o__strtol_l(str ptr long ptr) _strtol_l
@ cdecl _o__strtold_l(str ptr ptr) _strtod_l
@ cdecl -ret64 _o__strtoll_l(str ptr long ptr) _strtoi64_l
@ cdecl -ret64 _o__strtoui64(str ptr long) _strtoui64
@ cdecl -ret64 _o__strtoui64_l(str ptr long ptr) _strtoui64_l
@ cdecl _o__strtoul_l(str ptr long ptr) _strtoul_l
@ cdecl -ret64 _o__strtoull_l(str ptr long ptr) _strtoui64_l
@ cdecl _o__strupr(str) _strupr
@ cdecl _o__strupr_l(str ptr) _strupr_l
@ cdecl _o__strupr_s(str long) _strupr_s
@ cdecl _o__strupr_s_l(str long ptr) _strupr_s_l
@ cdecl _o__strxfrm_l(ptr str long ptr) _strxfrm_l
@ cdecl _o__swab(str str long) _swab
@ cdecl _o__tell(long) _tell
@ cdecl -ret64 _o__telli64(long) _telli64
@ cdecl _o__timespec32_get(ptr long) _timespec32_get
@ cdecl _o__timespec64_get(ptr long) _timespec64_get
@ cdecl _o__tolower(long) _tolower
@ cdecl _o__tolower_l(long ptr) _tolower_l
@ cdecl _o__toupper(long) _toupper
@ cdecl _o__toupper_l(long ptr) _toupper_l
@ cdecl _o__towlower_l(long ptr) _towlower_l
@ cdecl _o__towupper_l(long ptr) _towupper_l
@ cdecl _o__tzset() _tzset
@ cdecl _o__ui64toa(int64 ptr long) _ui64toa
@ cdecl _o__ui64toa_s(int64 ptr long long) _ui64toa_s
@ cdecl _o__ui64tow(int64 ptr long) _ui64tow
@ cdecl _o__ui64tow_s(int64 ptr long long) _ui64tow_s
@ cdecl _o__ultoa(long ptr long) _ultoa
@ cdecl _o__ultoa_s(long ptr long long) _ultoa_s
@ cdecl _o__ultow(long ptr long) _ultow
@ cdecl _o__ultow_s(long ptr long long) _ultow_s
@ cdecl _o__umask(long) _umask
@ cdecl _o__umask_s() _umask_s # FIXME: params
@ cdecl _o__ungetc_nolock(long ptr) _ungetc_nolock
@ cdecl _o__ungetch(long) _ungetch
@ cdecl _o__ungetch_nolock(long) _ungetch_nolock
@ cdecl _o__ungetwc_nolock(long ptr) _ungetwc_nolock
@ cdecl _o__ungetwch(long) _ungetwch
@ cdecl _o__ungetwch_nolock(long) _ungetwch_nolock
@ cdecl _o__unlink(str) _unlink
@ cdecl _o__unloaddll(long) _unloaddll
@ cdecl _o__unlock_file(ptr) _unlock_file
@ cdecl _o__utime32(str ptr) _utime32
@ cdecl _o__utime64(str ptr) _utime64
@ cdecl _o__waccess(wstr long) _waccess
@ cdecl _o__waccess_s(wstr long) _waccess_s
@ cdecl _o__wasctime(ptr) _wasctime
@ cdecl _o__wasctime_s(ptr long ptr) _wasctime_s
@ cdecl _o__wchdir(wstr) _wchdir
@ cdecl _o__wchmod(wstr long) _wchmod
@ cdecl _o__wcreat(wstr long) _wcreat
@ cdecl _o__wcreate_locale(long wstr) _wcreate_locale
@ cdecl _o__wcscoll_l(wstr wstr ptr) _wcscoll_l
@ cdecl _o__wcsdup(wstr) _wcsdup
@ cdecl _o__wcserror(long) _wcserror
@ cdecl _o__wcserror_s(ptr long long) _wcserror_s
@ cdecl _o__wcsftime_l(ptr long wstr ptr ptr) _wcsftime_l
@ cdecl _o__wcsicmp(wstr wstr) _wcsicmp
@ cdecl _o__wcsicmp_l(wstr wstr ptr) _wcsicmp_l
@ cdecl _o__wcsicoll(wstr wstr) _wcsicoll
@ cdecl _o__wcsicoll_l(wstr wstr ptr) _wcsicoll_l
@ cdecl _o__wcslwr(wstr) _wcslwr
@ cdecl _o__wcslwr_l(wstr ptr) _wcslwr_l
@ cdecl _o__wcslwr_s(wstr long) _wcslwr_s
@ cdecl _o__wcslwr_s_l(wstr long ptr) _wcslwr_s_l
@ cdecl _o__wcsncoll(wstr wstr long) _wcsncoll
@ cdecl _o__wcsncoll_l(wstr wstr long ptr) _wcsncoll_l
@ cdecl _o__wcsnicmp(wstr wstr long) _wcsnicmp
@ cdecl _o__wcsnicmp_l(wstr wstr long ptr) _wcsnicmp_l
@ cdecl _o__wcsnicoll(wstr wstr long) _wcsnicoll
@ cdecl _o__wcsnicoll_l(wstr wstr long ptr) _wcsnicoll_l
@ cdecl _o__wcsnset(wstr long long) _wcsnset
@ cdecl _o__wcsnset_s(wstr long long long) _wcsnset_s
@ cdecl _o__wcsset(wstr long) _wcsset
@ cdecl _o__wcsset_s(wstr long long) _wcsset_s
@ cdecl _o__wcstod_l(wstr ptr ptr) _wcstod_l
@ cdecl _o__wcstof_l(wstr ptr ptr) _wcstof_l
@ cdecl -ret64 _o__wcstoi64(wstr ptr long) _wcstoi64
@ cdecl -ret64 _o__wcstoi64_l(wstr ptr long ptr) _wcstoi64_l
@ cdecl _o__wcstol_l(wstr ptr long ptr) _wcstol_l
@ cdecl _o__wcstold_l(wstr ptr ptr) _wcstod_l
@ cdecl -ret64 _o__wcstoll_l(wstr ptr long ptr) _wcstoi64_l
@ cdecl _o__wcstombs_l(ptr ptr long ptr) _wcstombs_l
@ cdecl _o__wcstombs_s_l(ptr ptr long wstr long ptr) _wcstombs_s_l
@ cdecl -ret64 _o__wcstoui64(wstr ptr long) _wcstoui64
@ cdecl -ret64 _o__wcstoui64_l(wstr ptr long ptr) _wcstoui64_l
@ cdecl _o__wcstoul_l(wstr ptr long ptr) _wcstoul_l
@ cdecl -ret64 _o__wcstoull_l(wstr ptr long ptr) _wcstoui64_l
@ cdecl _o__wcsupr(wstr) _wcsupr
@ cdecl _o__wcsupr_l(wstr ptr) _wcsupr_l
@ cdecl _o__wcsupr_s(wstr long) _wcsupr_s
@ cdecl _o__wcsupr_s_l(wstr long ptr) _wcsupr_s_l
@ cdecl _o__wcsxfrm_l(ptr wstr long ptr) _wcsxfrm_l
@ cdecl _o__wctime32(ptr) _wctime32
@ cdecl _o__wctime32_s(ptr long ptr) _wctime32_s
@ cdecl _o__wctime64(ptr) _wctime64
@ cdecl _o__wctime64_s(ptr long ptr) _wctime64_s
@ cdecl _o__wctomb_l(ptr long ptr) _wctomb_l
@ cdecl _o__wctomb_s_l(ptr ptr long long ptr) _wctomb_s_l
@ cdecl _o__wdupenv_s(ptr ptr wstr) _wdupenv_s
@ cdecl _o__wexecv(wstr ptr) _wexecv
@ cdecl _o__wexecve(wstr ptr ptr) _wexecve
@ cdecl _o__wexecvp(wstr ptr) _wexecvp
@ cdecl _o__wexecvpe(wstr ptr ptr) _wexecvpe
@ cdecl _o__wfdopen(long wstr) _wfdopen
@ cdecl _o__wfindfirst32(wstr ptr) _wfindfirst32
@ cdecl _o__wfindfirst32i64() _wfindfirst32i64 # FIXME: params
@ cdecl _o__wfindfirst64(wstr ptr) _wfindfirst64
@ cdecl _o__wfindfirst64i32(wstr ptr) _wfindfirst64i32
@ cdecl _o__wfindnext32(long ptr) _wfindnext32
@ cdecl _o__wfindnext32i64() _wfindnext32i64 # FIXME: params
@ cdecl _o__wfindnext64(long ptr) _wfindnext64
@ cdecl _o__wfindnext64i32(long ptr) _wfindnext64i32
@ cdecl _o__wfopen(wstr wstr) _wfopen
@ cdecl _o__wfopen_s(ptr wstr wstr) _wfopen_s
@ cdecl _o__wfreopen(wstr wstr ptr) _wfreopen
@ cdecl _o__wfreopen_s(ptr wstr wstr ptr) _wfreopen_s
@ cdecl _o__wfsopen(wstr wstr long) _wfsopen
@ cdecl _o__wfullpath(ptr wstr long) _wfullpath
@ cdecl _o__wgetcwd(wstr long) _wgetcwd
@ cdecl _o__wgetdcwd(long wstr long) _wgetdcwd
@ cdecl _o__wgetenv(wstr) _wgetenv
@ cdecl _o__wgetenv_s(ptr ptr long wstr) _wgetenv_s
@ cdecl _o__wmakepath(ptr wstr wstr wstr wstr) _wmakepath
@ cdecl _o__wmakepath_s(ptr long wstr wstr wstr wstr) _wmakepath_s
@ cdecl _o__wmkdir(wstr) _wmkdir
@ cdecl _o__wmktemp(wstr) _wmktemp
@ cdecl _o__wmktemp_s(wstr long) _wmktemp_s
@ cdecl _o__wperror(wstr) _wperror
@ cdecl _o__wpopen(wstr wstr) _wpopen
@ cdecl _o__wputenv(wstr) _wputenv
@ cdecl _o__wputenv_s(wstr wstr) _wputenv_s
@ cdecl _o__wremove(wstr) _wremove
@ cdecl _o__wrename(wstr wstr) _wrename
@ cdecl _o__write(long ptr long) _write
@ cdecl _o__wrmdir(wstr) _wrmdir
@ cdecl _o__wsearchenv(wstr wstr ptr) _wsearchenv
@ cdecl _o__wsearchenv_s(wstr wstr ptr long) _wsearchenv_s
@ cdecl _o__wsetlocale(long wstr) _wsetlocale
@ cdecl _o__wsopen_dispatch(wstr long long long ptr long) _wsopen_dispatch
@ cdecl _o__wsopen_s(ptr wstr long long long) _wsopen_s
@ cdecl _o__wspawnv(long wstr ptr) _wspawnv
@ cdecl _o__wspawnve(long wstr ptr ptr) _wspawnve
@ cdecl _o__wspawnvp(long wstr ptr) _wspawnvp
@ cdecl _o__wspawnvpe(long wstr ptr ptr) _wspawnvpe
@ cdecl _o__wsplitpath(wstr ptr ptr ptr ptr) _wsplitpath
@ cdecl _o__wsplitpath_s(wstr ptr long ptr long ptr long ptr long) _wsplitpath_s
@ cdecl _o__wstat32(wstr ptr) _wstat32
@ cdecl _o__wstat32i64(wstr ptr) _wstat32i64
@ cdecl _o__wstat64(wstr ptr) _wstat64
@ cdecl _o__wstat64i32(wstr ptr) _wstat64i32
@ cdecl _o__wstrdate(ptr) _wstrdate
@ cdecl _o__wstrdate_s(ptr long) _wstrdate_s
@ cdecl _o__wstrtime(ptr) _wstrtime
@ cdecl _o__wstrtime_s(ptr long) _wstrtime_s
@ cdecl _o__wsystem(wstr) _wsystem
@ cdecl _o__wtmpnam_s(ptr long) _wtmpnam_s
@ cdecl _o__wtof(wstr) _wtof
@ cdecl _o__wtof_l(wstr ptr) _wtof_l
@ cdecl _o__wtoi(wstr) _wtoi
@ cdecl -ret64 _o__wtoi64(wstr) _wtoi64
@ cdecl -ret64 _o__wtoi64_l(wstr ptr) _wtoi64_l
@ cdecl _o__wtoi_l(wstr ptr) _wtoi_l
@ cdecl _o__wtol(wstr) _wtol
@ cdecl _o__wtol_l(wstr ptr) _wtol_l
@ cdecl -ret64 _o__wtoll(wstr) _wtoll
@ cdecl -ret64 _o__wtoll_l(wstr ptr) _wtoll_l
@ cdecl _o__wunlink(wstr) _wunlink
@ cdecl _o__wutime32(wstr ptr) _wutime32
@ cdecl _o__wutime64(wstr ptr) _wutime64
@ cdecl _o__y0(double) _y0
@ cdecl _o__y1(double) _y1
@ cdecl _o__yn(long double) _yn
@ cdecl _o_abort() abort
@ cdecl _o_acos(double) acos
@ cdecl -arch=!i386 _o_acosf(float) acosf
@ cdecl _o_acosh(double) acosh
@ cdecl _o_acoshf(float) acoshf
@ cdecl _o_acoshl(double) acosh
@ cdecl _o_asctime(ptr) asctime
@ cdecl _o_asctime_s(ptr long ptr) asctime_s
@ cdecl _o_asin(double) asin
@ cdecl -arch=!i386 _o_asinf(float) asinf
@ cdecl _o_asinh(double) asinh
@ cdecl _o_asinhf(float) asinhf
@ cdecl _o_asinhl(double) asinh
@ cdecl _o_atan(double) atan
@ cdecl _o_atan2(double double) atan2
@ cdecl -arch=!i386 _o_atan2f(float float) atan2f
@ cdecl -arch=!i386 _o_atanf(float) atanf
@ cdecl _o_atanh(double) atanh
@ cdecl _o_atanhf(float) atanhf
@ cdecl _o_atanhl(double) atanh
@ cdecl _o_atof(str) atof
@ cdecl _o_atoi(str) atoi
@ cdecl _o_atol(str) atol
@ cdecl -ret64 _o_atoll(str) atoll
@ cdecl _o_bsearch(ptr ptr long long ptr) bsearch
@ cdecl _o_bsearch_s(ptr ptr long long ptr ptr) bsearch_s
@ cdecl _o_btowc(long) btowc
@ cdecl _o_calloc(long long) calloc
@ cdecl _o_cbrt(double) cbrt
@ cdecl _o_cbrtf(float) cbrtf
@ cdecl _o_ceil(double) ceil
@ cdecl -arch=!i386 _o_ceilf(float) ceilf
@ cdecl _o_clearerr(ptr) clearerr
@ cdecl _o_clearerr_s(ptr) clearerr_s
@ cdecl _o_cos(double) cos
@ cdecl -arch=!i386 _o_cosf(float) cosf
@ cdecl _o_cosh(double) cosh
@ cdecl -arch=!i386 _o_coshf(float) coshf
@ cdecl _o_erf(double) erf
@ cdecl _o_erfc(double) erfc
@ cdecl _o_erfcf(float) erfcf
@ cdecl _o_erfcl(double) erfc
@ cdecl _o_erff(float) erff
@ cdecl _o_erfl(double) erf
@ cdecl _o_exit(long) exit
@ cdecl _o_exp(double) exp
@ cdecl _o_exp2(double) exp2
@ cdecl _o_exp2f(float) exp2f
@ cdecl _o_exp2l(double) exp2
@ cdecl -arch=!i386 _o_expf(float) expf
@ cdecl _o_fabs(double) fabs
@ cdecl _o_fclose(ptr) fclose
@ cdecl _o_feof(ptr) feof
@ cdecl _o_ferror(ptr) ferror
@ cdecl _o_fflush(ptr) fflush
@ cdecl _o_fgetc(ptr) fgetc
@ cdecl _o_fgetpos(ptr ptr) fgetpos
@ cdecl _o_fgets(ptr long ptr) fgets
@ cdecl _o_fgetwc(ptr) fgetwc
@ cdecl _o_fgetws(ptr long ptr) fgetws
@ cdecl _o_floor(double) floor
@ cdecl -arch=!i386 _o_floorf(float) floorf
@ cdecl _o_fma(double double double) fma
@ cdecl _o_fmaf(float float float) fmaf
@ cdecl _o_fmal(double double double) fma
@ cdecl _o_fmod(double double) fmod
@ cdecl -arch=!i386 _o_fmodf(float float) fmodf
@ cdecl _o_fopen(str str) fopen
@ cdecl _o_fopen_s(ptr str str) fopen_s
@ cdecl _o_fputc(long ptr) fputc
@ cdecl _o_fputs(str ptr) fputs
@ cdecl _o_fputwc(long ptr) fputwc
@ cdecl _o_fputws(wstr ptr) fputws
@ cdecl _o_fread(ptr long long ptr) fread
@ cdecl _o_fread_s(ptr long long long ptr) fread_s
@ cdecl _o_free(ptr) free
@ cdecl _o_freopen(str str ptr) freopen
@ cdecl _o_freopen_s(ptr str str ptr) freopen_s
@ cdecl _o_frexp(double ptr) frexp
@ cdecl _o_fseek(ptr long long) fseek
@ cdecl _o_fsetpos(ptr ptr) fsetpos
@ cdecl _o_ftell(ptr) ftell
@ cdecl _o_fwrite(ptr long long ptr) fwrite
@ cdecl _o_getc(ptr) getc
@ cdecl _o_getchar() getchar
@ cdecl _o_getenv(str) getenv
@ cdecl _o_getenv_s(ptr ptr long str) getenv_s
@ cdecl _o_gets(str) gets
@ cdecl _o_gets_s(ptr long) gets_s
@ cdecl _o_getwc(ptr) getwc
@ cdecl _o_getwchar() getwchar
@ cdecl _o_hypot(double double) _hypot
@ cdecl _o_is_wctype(long long) iswctype
@ cdecl _o_isalnum(long) isalnum
@ cdecl _o_isalpha(long) isalpha
@ cdecl _o_isblank(long) isblank
@ cdecl _o_iscntrl(long) iscntrl
@ cdecl _o_isdigit(long) isdigit
@ cdecl _o_isgraph(long) isgraph
@ cdecl _o_isleadbyte(long) isleadbyte
@ cdecl _o_islower(long) islower
@ cdecl _o_isprint(long) isprint
@ cdecl _o_ispunct(long) ispunct
@ cdecl _o_isspace(long) isspace
@ cdecl _o_isupper(long) isupper
@ cdecl _o_iswalnum(long) iswalnum
@ cdecl _o_iswalpha(long) iswalpha
@ cdecl _o_iswascii(long) iswascii
@ cdecl _o_iswblank(long) iswblank
@ cdecl _o_iswcntrl(long) iswcntrl
@ cdecl _o_iswctype(long long) iswctype
@ cdecl _o_iswdigit(long) iswdigit
@ cdecl _o_iswgraph(long) iswgraph
@ cdecl _o_iswlower(long) iswlower
@ cdecl _o_iswprint(long) iswprint
@ cdecl _o_iswpunct(long) iswpunct
@ cdecl _o_iswspace(long) iswspace
@ cdecl _o_iswupper(long) iswupper
@ cdecl _o_iswxdigit(long) iswxdigit
@ cdecl _o_isxdigit(long) isxdigit
@ cdecl _o_ldexp(double long) ldexp
@ cdecl _o_lgamma(double) lgamma
@ cdecl _o_lgammaf(float) lgammaf
@ cdecl _o_lgammal(double) lgamma
@ cdecl -ret64 _o_llrint(double) llrint
@ cdecl -ret64 _o_llrintf(float) llrintf
@ cdecl -ret64 _o_llrintl(double) llrint
@ cdecl -ret64 _o_llround(double) llround
@ cdecl -ret64 _o_llroundf(float) llroundf
@ cdecl -ret64 _o_llroundl(double) llround
@ cdecl _o_localeconv() localeconv
@ cdecl _o_log(double) log
@ cdecl _o_log10(double) log10
@ cdecl -arch=!i386 _o_log10f(float) log10f
@ cdecl _o_log1p(double) log1p
@ cdecl _o_log1pf(float) log1pf
@ cdecl _o_log1pl(double) log1p
@ cdecl _o_log2(double) log2
@ cdecl _o_log2f(float) log2f
@ cdecl _o_log2l(double) log2
@ cdecl _o_logb(double) logb
@ cdecl _o_logbf(float) logbf
@ cdecl _o_logbl(double) logb
@ cdecl -arch=!i386 _o_logf(float) logf
@ cdecl _o_lrint(double) lrint
@ cdecl _o_lrintf(float) lrintf
@ cdecl _o_lrintl(double) lrint
@ cdecl _o_lround(double) lround
@ cdecl _o_lroundf(float) lroundf
@ cdecl _o_lroundl(double) lround
@ cdecl _o_malloc(long) malloc
@ cdecl _o_mblen(ptr long) mblen
@ cdecl _o_mbrlen(ptr long ptr) mbrlen
@ cdecl _o_mbrtoc16() mbrtoc16 # FIXME: params
@ cdecl _o_mbrtoc32() mbrtoc32 # FIXME: params
@ cdecl _o_mbrtowc(ptr str long ptr) mbrtowc
@ cdecl _o_mbsrtowcs(ptr ptr long ptr) mbsrtowcs
@ cdecl _o_mbsrtowcs_s(ptr ptr long ptr long ptr) mbsrtowcs_s
@ cdecl _o_mbstowcs(ptr str long) mbstowcs
@ cdecl _o_mbstowcs_s(ptr ptr long str long) mbstowcs_s
@ cdecl _o_mbtowc(ptr str long) mbtowc
@ cdecl _o_memcpy_s(ptr long ptr long) memcpy_s
@ cdecl _o_memset(ptr long long) memset
@ cdecl _o_modf(double ptr) modf
@ cdecl -arch=!i386 _o_modff(float ptr) modff
@ cdecl _o_nan(str) nan
@ cdecl _o_nanf(str) nanf
@ cdecl _o_nanl(str) nan
@ cdecl _o_nearbyint(double) nearbyint
@ cdecl _o_nearbyintf(float) nearbyintf
@ cdecl _o_nearbyintl(double) nearbyint
@ cdecl _o_nextafter(double double) nextafter
@ cdecl _o_nextafterf(float float) nextafterf
@ cdecl _o_nextafterl(double double) nextafter
@ cdecl _o_nexttoward(double double) nexttoward
@ cdecl _o_nexttowardf(float double) nexttowardf
@ cdecl _o_nexttowardl(double double) nexttoward
@ cdecl _o_pow(double double) pow
@ cdecl -arch=!i386 _o_powf(float float) powf
@ cdecl _o_putc(long ptr) putc
@ cdecl _o_putchar(long) putchar
@ cdecl _o_puts(str) puts
@ cdecl _o_putwc(long ptr) fputwc
@ cdecl _o_putwchar(long) _fputwchar
@ cdecl _o_qsort(ptr long long ptr) qsort
@ cdecl _o_qsort_s(ptr long long ptr ptr) qsort_s
@ cdecl _o_raise(long) raise
@ cdecl _o_rand() rand
@ cdecl _o_rand_s(ptr) rand_s
@ cdecl _o_realloc(ptr long) realloc
@ cdecl _o_remainder(double double) remainder
@ cdecl _o_remainderf(float float) remainderf
@ cdecl _o_remainderl(double double) remainder
@ cdecl _o_remove(str) remove
@ cdecl _o_remquo(double double ptr) remquo
@ cdecl _o_remquof(float float ptr) remquof
@ cdecl _o_remquol(double double ptr) remquo
@ cdecl _o_rename(str str) rename
@ cdecl _o_rewind(ptr) rewind
@ cdecl _o_rint(double) rint
@ cdecl _o_rintf(float) rintf
@ cdecl _o_rintl(double) rint
@ cdecl _o_round(double) round
@ cdecl _o_roundf(float) roundf
@ cdecl _o_roundl(double) round
@ cdecl _o_scalbln(double long) scalbn
@ cdecl _o_scalblnf(float long) scalbnf
@ cdecl _o_scalblnl(double long) scalbn
@ cdecl _o_scalbn(double long) scalbn
@ cdecl _o_scalbnf(float long) scalbnf
@ cdecl _o_scalbnl(double long) scalbn
@ cdecl _o_set_terminate(ptr) set_terminate
@ cdecl _o_setbuf(ptr ptr) setbuf
@ cdecl _o_setlocale(long str) setlocale
@ cdecl _o_setvbuf(ptr str long long) setvbuf
@ cdecl _o_sin(double) sin
@ cdecl -arch=!i386 _o_sinf(float) sinf
@ cdecl _o_sinh(double) sinh
@ cdecl -arch=!i386 _o_sinhf(float) sinhf
@ cdecl _o_sqrt(double) sqrt
@ cdecl -arch=!i386 _o_sqrtf(float) sqrtf
@ cdecl _o_srand(long) srand
@ cdecl _o_strcat_s(str long str) strcat_s
@ cdecl _o_strcoll(str str) strcoll
@ cdecl _o_strcpy_s(ptr long str) strcpy_s
@ cdecl _o_strerror(long) strerror
@ cdecl _o_strerror_s(ptr long long) strerror_s
@ cdecl _o_strftime(ptr long str ptr) strftime
@ cdecl _o_strncat_s(str long str long) strncat_s
@ cdecl _o_strncpy_s(ptr long str long) strncpy_s
@ cdecl _o_strtod(str ptr) strtod
@ cdecl _o_strtof(str ptr) strtof
@ cdecl _o_strtok(str str) strtok
@ cdecl _o_strtok_s(ptr str ptr) strtok_s
@ cdecl _o_strtol(str ptr long) strtol
@ cdecl _o_strtold(str ptr) strtod
@ cdecl -ret64 _o_strtoll(str ptr long) _strtoi64
@ cdecl _o_strtoul(str ptr long) strtoul
@ cdecl -ret64 _o_strtoull(str ptr long) _strtoui64
@ cdecl _o_system(str) system
@ cdecl _o_tan(double) tan
@ cdecl -arch=!i386 _o_tanf(float) tanf
@ cdecl _o_tanh(double) tanh
@ cdecl -arch=!i386 _o_tanhf(float) tanhf
@ cdecl _o_terminate() terminate
@ cdecl _o_tgamma(double) tgamma
@ cdecl _o_tgammaf(float) tgammaf
@ cdecl _o_tgammal(double) tgamma
@ cdecl _o_tmpfile_s(ptr) tmpfile_s
@ cdecl _o_tmpnam_s(ptr long) tmpnam_s
@ cdecl _o_tolower(long) tolower
@ cdecl _o_toupper(long) toupper
@ cdecl _o_towlower(long) towlower
@ cdecl _o_towupper(long) towupper
@ cdecl _o_ungetc(long ptr) ungetc
@ cdecl _o_ungetwc(long ptr) ungetwc
@ cdecl _o_wcrtomb(ptr long ptr) wcrtomb
@ cdecl _o_wcrtomb_s(ptr ptr long long ptr) wcrtomb_s
@ cdecl _o_wcscat_s(wstr long wstr) wcscat_s
@ cdecl _o_wcscoll(wstr wstr) wcscoll
@ cdecl _o_wcscpy(ptr wstr) wcscpy
@ cdecl _o_wcscpy_s(ptr long wstr) wcscpy_s
@ cdecl _o_wcsftime(ptr long wstr ptr) wcsftime
@ cdecl _o_wcsncat_s(wstr long wstr long) wcsncat_s
@ cdecl _o_wcsncpy_s(ptr long wstr long) wcsncpy_s
@ cdecl _o_wcsrtombs(ptr ptr long ptr) wcsrtombs
@ cdecl _o_wcsrtombs_s(ptr ptr long ptr long ptr) wcsrtombs_s
@ cdecl _o_wcstod(wstr ptr) wcstod
@ cdecl _o_wcstof(ptr ptr) wcstof
@ cdecl _o_wcstok(wstr wstr ptr) wcstok
@ cdecl _o_wcstok_s(ptr wstr ptr) wcstok_s
@ cdecl _o_wcstol(wstr ptr long) wcstol
@ cdecl _o_wcstold(wstr ptr ptr) wcstod
@ cdecl -ret64 _o_wcstoll(wstr ptr long) _wcstoi64
@ cdecl _o_wcstombs(ptr ptr long) wcstombs
@ cdecl _o_wcstombs_s(ptr ptr long wstr long) wcstombs_s
@ cdecl _o_wcstoul(wstr ptr long) wcstoul
@ cdecl -ret64 _o_wcstoull(wstr ptr long) _wcstoui64
@ cdecl _o_wctob(long) wctob
@ cdecl _o_wctomb(ptr long) wctomb
@ cdecl _o_wctomb_s(ptr ptr long long) wctomb_s
@ cdecl _o_wmemcpy_s(ptr long ptr long) wmemcpy_s
@ cdecl _o_wmemmove_s(ptr long ptr long) wmemmove_s
@ varargs _open(str long)
@ cdecl _open_osfhandle(long long)
@ cdecl _pclose(ptr)
@ cdecl _pipe(ptr long long)
@ cdecl _popen(str str)
@ cdecl _purecall()
@ cdecl _putc_nolock(long ptr) _fputc_nolock
@ cdecl _putch(long)
@ cdecl _putch_nolock(long)
@ cdecl _putenv(str)
@ cdecl _putenv_s(str str)
@ cdecl _putw(long ptr)
@ cdecl _putwc_nolock(long ptr) _fputwc_nolock
@ cdecl _putwch(long)
@ cdecl _putwch_nolock(long)
@ cdecl _putws(wstr)
@ cdecl _query_app_type()
@ cdecl _query_new_handler()
@ cdecl _query_new_mode()
@ cdecl _read(long ptr long)
@ cdecl _realloc_base(ptr long)
@ cdecl -dbg _realloc_dbg(ptr long long str long)
@ cdecl _recalloc(ptr long long)
@ cdecl -dbg _recalloc_dbg(ptr long long long str long)
@ cdecl _register_onexit_function(ptr ptr)
@ cdecl _register_thread_local_exe_atexit_callback(ptr)
@ cdecl _resetstkoflw()
@ cdecl _rmdir(str)
@ cdecl _rmtmp()
@ cdecl _rotl(long long)
@ cdecl -ret64 _rotl64(int64 long)
@ cdecl _rotr(long long)
@ cdecl -ret64 _rotr64(int64 long)
@ cdecl _scalb(double long) scalbn # double _scalb(double x, long exp);
@ cdecl -arch=x86_64 _scalbf(float long) scalbnf # float _scalbf(float x, long exp);
@ cdecl _searchenv(str str ptr)
@ cdecl _searchenv_s(str str ptr long)
@ cdecl _seh_filter_dll(long ptr) # __CppXcptFilter
@ cdecl _seh_filter_exe(long ptr) # _XcptFilter
@ cdecl -arch=win64 _set_FMA3_enable(long)
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr)
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr)
@ cdecl -stub -arch=i386 _set_SSE2_enable(long)
@ cdecl _set_abort_behavior(long long)
@ cdecl _set_app_type(long)
@ cdecl _set_controlfp(long long) _controlfp
@ cdecl _set_doserrno(long)
@ cdecl _set_errno(long)
@ cdecl _set_error_mode(long)
@ cdecl _set_fmode(long)
@ cdecl _set_invalid_parameter_handler(ptr)
@ cdecl _set_new_handler(ptr)
@ cdecl _set_new_mode(long)
@ cdecl _set_printf_count_output(long)
@ cdecl _set_purecall_handler(ptr)
@ cdecl _set_se_translator(ptr)
@ cdecl _set_thread_local_invalid_parameter_handler(ptr)
@ cdecl _seterrormode(long)
@ cdecl -arch=i386 -norelay _setjmp3(ptr long)
@ cdecl _setmaxstdio(long)
@ cdecl _setmbcp(long)
@ cdecl _setmode(long long)
@ cdecl _setsystime(ptr long)
@ cdecl _sleep(long)
@ varargs _sopen(str long long)
@ cdecl _sopen_dispatch(str long long long ptr long)
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
@ cdecl _splitpath_s(str ptr long ptr long ptr long ptr long)
@ cdecl _stat32(str ptr)
@ cdecl _stat32i64(str ptr)
@ cdecl _stat64(str ptr)
@ cdecl _stat64i32(str ptr)
@ cdecl _statusfp()
@ cdecl -stub -arch=i386 _statusfp2(ptr ptr)
@ cdecl _strcoll_l(str str ptr)
@ cdecl _strdate(ptr)
@ cdecl _strdate_s(ptr long)
@ cdecl _strdup(str)
@ cdecl -dbg _strdup_dbg(str long str long)
@ cdecl _strerror(long)
@ cdecl _strerror_s(str)
@ cdecl _strftime_l(ptr long str ptr ptr)
@ cdecl _stricmp(str str)
@ cdecl _stricmp_l(str str ptr)
@ cdecl _stricoll(str str)
@ cdecl _stricoll_l(str str ptr)
@ cdecl _strlwr(str)
@ cdecl _strlwr_l(str ptr)
@ cdecl _strlwr_s(ptr long)
@ cdecl _strlwr_s_l(ptr long ptr)
@ cdecl _strncoll(str str long)
@ cdecl _strncoll_l(str str long ptr)
@ cdecl _strnicmp(str str long)
@ cdecl _strnicmp_l(str str long ptr)
@ cdecl _strnicoll(str str long)
@ cdecl _strnicoll_l(str str long ptr)
@ cdecl _strnset(str long long)
@ cdecl _strnset_s(str long long long)
@ cdecl _strrev(str)
@ cdecl _strset(ptr long)
@ cdecl _strset_s(ptr long long)
@ cdecl _strtime(ptr)
@ cdecl _strtime_s(ptr long)
@ cdecl _strtod_l(str ptr ptr)
@ cdecl _strtof_l(str ptr ptr)
@ cdecl -ret64 _strtoi64(str ptr long)
@ cdecl -ret64 _strtoi64_l(str ptr long ptr)
@ cdecl -ret64 _strtoimax_l(str ptr long ptr) _strtoi64_l
@ cdecl _strtol_l(str ptr long ptr)
@ cdecl _strtold_l(str ptr ptr) _strtod_l
@ cdecl -ret64 _strtoll_l(str ptr long ptr) _strtoi64_l
@ cdecl -ret64 _strtoui64(str ptr long)
@ cdecl -ret64 _strtoui64_l(str ptr long ptr)
@ cdecl _strtoul_l(str ptr long ptr)
@ cdecl -ret64 _strtoull_l(str ptr long ptr) _strtoui64_l
@ cdecl -ret64 _strtoumax_l(str ptr long ptr) _strtoui64_l
@ cdecl _strupr(str)
@ cdecl _strupr_l(str ptr)
@ cdecl _strupr_s(str long)
@ cdecl _strupr_s_l(str long ptr)
@ cdecl _strxfrm_l(ptr str long ptr)
@ cdecl _swab(str str long)
@ cdecl _tell(long)
@ cdecl -ret64 _telli64(long)
@ cdecl _tempnam(str str)
@ cdecl -dbg _tempnam_dbg(str str long str long)
@ cdecl _time32(ptr)
@ cdecl _time64(ptr)
@ cdecl _timespec32_get(ptr long)
@ cdecl _timespec64_get(ptr long)
@ cdecl _tolower(long)
@ cdecl _tolower_l(long ptr)
@ cdecl _toupper(long)
@ cdecl _toupper_l(long ptr)
@ cdecl _towlower_l(long ptr)
@ cdecl _towupper_l(long ptr)
@ cdecl _tzset()
@ cdecl _ui64toa(int64 ptr long)
@ cdecl _ui64toa_s(int64 ptr long long)
@ cdecl _ui64tow(int64 ptr long)
@ cdecl _ui64tow_s(int64 ptr long long)
@ cdecl _ultoa(long ptr long)
@ cdecl _ultoa_s(long ptr long long)
@ cdecl _ultow(long ptr long)
@ cdecl _ultow_s(long ptr long long)
@ cdecl _umask(long)
@ cdecl _umask_s(long)
@ cdecl _ungetc_nolock(long ptr)
@ cdecl _ungetch(long)
@ cdecl _ungetch_nolock(long)
@ cdecl _ungetwc_nolock(long ptr)
@ cdecl _ungetwch(long)
@ cdecl _ungetwch_nolock(long)
@ cdecl _unlink(str)
@ cdecl _unloaddll(long)
@ cdecl _unlock_file(ptr)
@ cdecl _unlock_locales()
@ cdecl _utime32(str ptr)
@ cdecl _utime64(str ptr)
@ cdecl _waccess(wstr long)
@ cdecl _waccess_s(wstr long)
@ cdecl _wasctime(ptr)
@ cdecl _wasctime_s(ptr long ptr)
@ cdecl _wassert(wstr wstr long)
@ cdecl _wchdir(wstr)
@ cdecl _wchmod(wstr long)
@ cdecl _wcreat(wstr long)
@ cdecl _wcreate_locale(long wstr)
@ cdecl _wcscoll_l(wstr wstr ptr)
@ cdecl _wcsdup(wstr)
@ cdecl -dbg _wcsdup_dbg(wstr long str long)
@ cdecl _wcserror(long)
@ cdecl _wcserror_s(ptr long long)
@ cdecl _wcsftime_l(ptr long wstr ptr ptr)
@ cdecl _wcsicmp(wstr wstr)
@ cdecl _wcsicmp_l(wstr wstr ptr)
@ cdecl _wcsicoll(wstr wstr)
@ cdecl _wcsicoll_l(wstr wstr ptr)
@ cdecl _wcslwr(wstr)
@ cdecl _wcslwr_l(wstr ptr)
@ cdecl _wcslwr_s(wstr long)
@ cdecl _wcslwr_s_l(wstr long ptr)
@ cdecl _wcsncoll(wstr wstr long)
@ cdecl _wcsncoll_l(wstr wstr long ptr)
@ cdecl _wcsnicmp(wstr wstr long)
@ cdecl _wcsnicmp_l(wstr wstr long ptr)
@ cdecl _wcsnicoll(wstr wstr long)
@ cdecl _wcsnicoll_l(wstr wstr long ptr)
@ cdecl _wcsnset(wstr long long)
@ cdecl _wcsnset_s(wstr long long long)
@ cdecl _wcsrev(wstr)
@ cdecl _wcsset(wstr long)
@ cdecl _wcsset_s(wstr long long)
@ cdecl _wcstod_l(wstr ptr ptr)
@ cdecl _wcstof_l(wstr ptr ptr)
@ cdecl -ret64 _wcstoi64(wstr ptr long)
@ cdecl -ret64 _wcstoi64_l(wstr ptr long ptr)
@ cdecl _wcstoimax_l(wstr ptr long ptr)
@ cdecl _wcstol_l(wstr ptr long ptr)
@ cdecl _wcstold_l(wstr ptr ptr) _wcstod_l
@ cdecl -ret64 _wcstoll_l(wstr ptr long ptr) _wcstoi64_l
@ cdecl _wcstombs_l(ptr ptr long ptr)
@ cdecl _wcstombs_s_l(ptr ptr long wstr long ptr)
@ cdecl -ret64 _wcstoui64(wstr ptr long)
@ cdecl -ret64 _wcstoui64_l(wstr ptr long ptr)
@ cdecl _wcstoul_l(wstr ptr long ptr)
@ cdecl -ret64 _wcstoull_l(wstr ptr long ptr) _wcstoui64_l
@ cdecl _wcstoumax_l(wstr ptr long ptr)
@ cdecl _wcsupr(wstr)
@ cdecl _wcsupr_l(wstr ptr)
@ cdecl _wcsupr_s(wstr long)
@ cdecl _wcsupr_s_l(wstr long ptr)
@ cdecl _wcsxfrm_l(ptr wstr long ptr)
@ cdecl _wctime32(ptr)
@ cdecl _wctime32_s(ptr long ptr)
@ cdecl _wctime64(ptr)
@ cdecl _wctime64_s(ptr long ptr)
@ cdecl _wctomb_l(ptr long ptr)
@ cdecl _wctomb_s_l(ptr ptr long long ptr)
@ extern _wctype
@ cdecl _wdupenv_s(ptr ptr wstr)
@ cdecl -dbg _wdupenv_s_dbg(ptr ptr wstr long str long)
@ varargs _wexecl(wstr wstr)
@ varargs _wexecle(wstr wstr)
@ varargs _wexeclp(wstr wstr)
@ varargs _wexeclpe(wstr wstr)
@ cdecl _wexecv(wstr ptr)
@ cdecl _wexecve(wstr ptr ptr)
@ cdecl _wexecvp(wstr ptr)
@ cdecl _wexecvpe(wstr ptr ptr)
@ cdecl _wfdopen(long wstr)
@ cdecl _wfindfirst32(wstr ptr)
@ cdecl _wfindfirst32i64(wstr ptr)
@ cdecl _wfindfirst64(wstr ptr)
@ cdecl _wfindfirst64i32(wstr ptr)
@ cdecl _wfindnext32(long ptr)
@ cdecl _wfindnext32i64(long ptr)
@ cdecl _wfindnext64(long ptr)
@ cdecl _wfindnext64i32(long ptr)
@ cdecl _wfopen(wstr wstr)
@ cdecl _wfopen_s(ptr wstr wstr)
@ cdecl _wfreopen(wstr wstr ptr)
@ cdecl _wfreopen_s(ptr wstr wstr ptr)
@ cdecl _wfsopen(wstr wstr long)
@ cdecl _wfullpath(ptr wstr long)
@ cdecl -dbg _wfullpath_dbg(ptr wstr long long str long)
@ cdecl _wgetcwd(wstr long)
@ cdecl -dbg _wgetcwd_dbg(ptr long long str long)
@ cdecl _wgetdcwd(long ptr long)
@ cdecl -dbg _wgetdcwd_dbg(long ptr long long str long)
@ cdecl _wgetenv(wstr)
@ cdecl _wgetenv_s(ptr ptr long wstr)
@ cdecl _wmakepath(ptr wstr wstr wstr wstr)
@ cdecl _wmakepath_s(ptr long wstr wstr wstr wstr)
@ cdecl _wmkdir(wstr)
@ cdecl _wmktemp(wstr)
@ cdecl _wmktemp_s(wstr long)
@ varargs _wopen(wstr long)
@ cdecl _wperror(wstr)
@ cdecl _wpopen(wstr wstr)
@ cdecl _wputenv(wstr)
@ cdecl _wputenv_s(wstr wstr)
@ cdecl _wremove(wstr)
@ cdecl _wrename(wstr wstr)
@ cdecl _write(long ptr long)
@ cdecl _wrmdir(wstr)
@ cdecl _wsearchenv(wstr wstr ptr)
@ cdecl _wsearchenv_s(wstr wstr ptr long)
@ cdecl _wsetlocale(long wstr)
@ varargs _wsopen(wstr long long)
@ cdecl _wsopen_dispatch(wstr long long long ptr long)
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
@ cdecl _wsplitpath_s(wstr ptr long ptr long ptr long ptr long)
@ cdecl _wstat32(wstr ptr)
@ cdecl _wstat32i64(wstr ptr)
@ cdecl _wstat64(wstr ptr)
@ cdecl _wstat64i32(wstr ptr)
@ cdecl _wstrdate(ptr)
@ cdecl _wstrdate_s(ptr long)
@ cdecl _wstrtime(ptr)
@ cdecl _wstrtime_s(ptr long)
@ cdecl _wsystem(wstr)
@ cdecl _wtempnam(wstr wstr)
@ cdecl -dbg _wtempnam_dbg(wstr wstr long str long)
@ cdecl _wtmpnam(ptr)
@ cdecl _wtmpnam_s(ptr long)
@ cdecl _wtof(wstr)
@ cdecl _wtof_l(wstr ptr)
@ cdecl _wtoi(wstr)
@ cdecl -ret64 _wtoi64(wstr)
@ cdecl -ret64 _wtoi64_l(wstr ptr)
@ cdecl _wtoi_l(wstr ptr)
@ cdecl _wtol(wstr)
@ cdecl _wtol_l(wstr ptr)
@ cdecl -ret64 _wtoll(wstr)
@ cdecl -ret64 _wtoll_l(wstr ptr)
@ cdecl _wunlink(wstr)
@ cdecl _wutime32(wstr ptr)
@ cdecl _wutime64(wstr ptr)
@ cdecl _y0(double)
@ cdecl _y1(double)
@ cdecl _yn(long double)
@ cdecl abort()
@ cdecl abs(long)
@ cdecl acos(double)
@ cdecl -arch=!i386 acosf(float)
@ cdecl -stub acosh(double)
@ cdecl -stub acoshf(float)
@ cdecl -stub acoshl(double)
@ cdecl asctime(ptr)
@ cdecl asctime_s(ptr long ptr)
@ cdecl asin(double)
@ cdecl -arch=!i386 asinf(float)
@ cdecl -stub asinh(double)
@ cdecl -stub asinhf(float)
@ cdecl -stub asinhl(double) asinh
@ cdecl atan(double)
@ cdecl atan2(double double)
@ cdecl -arch=!i386 atan2f(float float)
@ cdecl -arch=!i386 atanf(float)
@ cdecl -stub atanh(double)
@ cdecl -stub atanhf(float)
@ cdecl -stub atanhl(double)
@ cdecl atof(str)
@ cdecl atoi(str)
@ cdecl atol(str)
@ cdecl -ret64 atoll(str)
@ cdecl bsearch(ptr ptr long long ptr)
@ cdecl bsearch_s(ptr ptr long long ptr ptr)
@ cdecl btowc(long)
@ cdecl c16rtomb(ptr long ptr)
@ cdecl c32rtomb(ptr long ptr)
@ stub cabs
@ stub cabsf
@ stub cabsl
@ stub cacos
@ stub cacosf
@ stub cacosh
@ stub cacoshf
@ stub cacoshl
@ stub cacosl
@ cdecl calloc(long long)
@ stub carg
@ stub cargf
@ stub cargl
@ stub casin
@ stub casinf
@ stub casinh
@ stub casinhf
@ stub casinhl
@ stub casinl
@ stub catan
@ stub catanf
@ stub catanh
@ stub catanhf
@ stub catanhl
@ stub catanl
@ cdecl -stub cbrt(double)
@ cdecl -stub cbrtf(float)
@ cdecl -stub cbrtl(double) cbrt
@ stub ccos
@ stub ccosf
@ stub ccosh
@ stub ccoshf
@ stub ccoshl
@ stub ccosl
@ cdecl ceil(double)
@ cdecl -arch=!i386 ceilf(float)
@ stub cexp
@ stub cexpf
@ stub cexpl
@ stub cimag
@ stub cimagf
@ stub cimagl
@ cdecl clearerr(ptr)
@ cdecl clearerr_s(ptr)
@ cdecl clock()
@ stub clog
@ stub clog10
@ stub clog10f
@ stub clog10l
@ stub clogf
@ stub clogl
@ stub conj
@ stub conjf
@ stub conjl
@ cdecl -stub copysign(double double)
@ cdecl -stub copysignf(float float)
@ cdecl -stub copysignl(double double) copysign
@ cdecl cos(double)
@ cdecl -arch=!i386 cosf(float)
@ cdecl cosh(double)
@ cdecl -arch=!i386 coshf(float)
@ stub cpow
@ stub cpowf
@ stub cpowl
@ stub cproj
@ stub cprojf
@ stub cprojl
@ cdecl -stub creal(int128)
@ stub crealf
@ stub creall
@ stub csin
@ stub csinf
@ stub csinh
@ stub csinhf
@ stub csinhl
@ stub csinl
@ stub csqrt
@ stub csqrtf
@ stub csqrtl
@ stub ctan
@ stub ctanf
@ stub ctanh
@ stub ctanhf
@ stub ctanhl
@ stub ctanl
@ cdecl -ret64 div(long long)
@ cdecl -stub erf(double)
@ cdecl -stub erfc(double)
@ cdecl -stub erfcf(float)
@ cdecl -stub erfcl(double) erfc
@ cdecl -stub erff(float)
@ cdecl -stub erfl(double) erf
@ cdecl exit(long)
@ cdecl exp(double)
@ cdecl exp2(double)
@ cdecl exp2f(float)
@ cdecl exp2l(double) exp2
@ cdecl -arch=!i386 expf(float)
@ cdecl -stub expm1(double)
@ cdecl -stub expm1f(float)
@ cdecl expm1l(double) expm1
@ cdecl fabs(double)
@ cdecl -stub -arch=arm,arm64 fabsf(float)
@ cdecl fclose(ptr)
@ cdecl -stub fdim(double double)
@ cdecl -stub fdimf(float float)
@ cdecl fdiml(double double) fdim
@ cdecl -stub feclearexcept(long)
@ cdecl -stub fegetenv(ptr)
@ cdecl -stub fegetexceptflag(ptr long)
@ cdecl -stub fegetround()
@ cdecl -stub feholdexcept(ptr)
@ cdecl feof(ptr)
@ cdecl ferror(ptr)
@ cdecl -stub fesetenv(ptr)
@ cdecl -stub fesetexceptflag(ptr long)
@ cdecl -stub fesetround(long)
@ cdecl -stub fetestexcept(long)
@ cdecl fflush(ptr)
@ cdecl fgetc(ptr)
@ cdecl fgetpos(ptr ptr)
@ cdecl fgets(ptr long ptr)
@ cdecl fgetwc(ptr)
@ cdecl fgetws(ptr long ptr)
@ cdecl floor(double)
@ cdecl -arch=!i386 floorf(float)
@ cdecl fma(double double double)
@ cdecl fmaf(float float float)
@ cdecl fmal(double double double) fma
@ cdecl -stub fmax(double double)
@ cdecl -stub fmaxf(float float)
@ cdecl fmaxl(double double) fmax
@ cdecl -stub fmin(double double)
@ cdecl -stub fminf(float float)
@ cdecl fminl(double double) fmin
@ cdecl fmod(double double)
@ cdecl -arch=!i386 fmodf(float float)
@ cdecl fopen(str str)
@ cdecl fopen_s(ptr str str)
@ cdecl fputc(long ptr)
@ cdecl fputs(str ptr)
@ cdecl fputwc(long ptr)
@ cdecl fputws(wstr ptr)
@ cdecl fread(ptr long long ptr)
@ cdecl fread_s(ptr long long long ptr)
@ cdecl free(ptr)
@ cdecl freopen(str str ptr)
@ cdecl freopen_s(ptr str str ptr)
@ cdecl frexp(double ptr)
@ cdecl fseek(ptr long long)
@ cdecl fsetpos(ptr ptr)
@ cdecl ftell(ptr)
@ cdecl fwrite(ptr long long ptr)
@ cdecl getc(ptr)
@ cdecl getchar()
@ cdecl getenv(str)
@ cdecl getenv_s(ptr ptr long str)
@ cdecl gets(str)
@ cdecl gets_s(ptr long)
@ cdecl getwc(ptr)
@ cdecl getwchar()
@ cdecl hypot(double double) _hypot
@ cdecl -stub ilogb(double)
@ cdecl -stub ilogbf(float)
@ cdecl ilogbl(double) ilogb
@ cdecl -ret64 imaxabs(int64)
@ cdecl -ret64 imaxdiv(int64 int64)
@ cdecl is_wctype(long long)
@ cdecl isalnum(long)
@ cdecl isalpha(long)
@ cdecl isblank(long)
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
@ cdecl iswblank(long)
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
@ cdecl -ret64 ldiv(long long)
@ cdecl -stub lgamma(double)
@ cdecl -stub lgammaf(float)
@ cdecl lgammal(double) lgamma
@ cdecl -ret64 llabs(int64)
@ cdecl -norelay lldiv(int64 int64)
@ cdecl -stub -ret64 llrint(double)
@ cdecl -stub -ret64 llrintf(float)
@ cdecl -ret64 llrintl(double) llrint
@ cdecl -stub -ret64 llround(double)
@ cdecl -stub -ret64 llroundf(float)
@ cdecl -ret64 llroundl(double) llround
@ cdecl localeconv()
@ cdecl log(double)
@ cdecl log10(double)
@ cdecl -arch=!i386 log10f(float)
@ cdecl -stub log1p(double)
@ cdecl -stub log1pf(float)
@ cdecl log1pl(double) log1p
@ cdecl log2(double)
@ cdecl log2f(float)
@ cdecl log2l(double) log2
@ cdecl -stub logb(double)
@ cdecl -stub logbf(float)
@ cdecl logbl(double) logb
@ cdecl -arch=!i386 logf(float)
@ cdecl longjmp(ptr long)
@ cdecl lrint(double)
@ cdecl lrintf(float)
@ cdecl lrintl(double) lrint
@ cdecl -stub lround(double)
@ cdecl -stub lroundf(float)
@ cdecl lroundl(double) lround
@ cdecl malloc(long)
@ cdecl mblen(ptr long)
@ cdecl mbrlen(ptr long ptr)
@ cdecl mbrtoc16(ptr ptr long)
@ cdecl mbrtoc32(ptr ptr long)
@ cdecl mbrtowc(ptr str long ptr)
@ cdecl mbsrtowcs(ptr ptr long ptr)
@ cdecl mbsrtowcs_s(ptr ptr long ptr long ptr)
@ cdecl mbstowcs(ptr str long)
@ cdecl mbstowcs_s(ptr ptr long str long)
@ cdecl mbtowc(ptr str long)
@ cdecl memchr(ptr long long)
@ cdecl memcmp(ptr ptr long)
@ cdecl memcpy(ptr ptr long)
@ cdecl memcpy_s(ptr long ptr long)
@ cdecl memmove(ptr ptr long)
@ cdecl memmove_s(ptr long ptr long)
@ cdecl memset(ptr long long)
@ cdecl modf(double ptr)
@ cdecl -stub -arch=!i386 modff(float ptr)
@ cdecl -stub nan(str)
@ cdecl -stub nanf(str)
@ cdecl nanl(str) nan
@ cdecl -stub nearbyint(double)
@ cdecl -stub nearbyintf(float)
@ cdecl nearbyintl(double) nearbyint
@ cdecl -stub nextafter(double double)
@ cdecl -stub nextafterf(float float)
@ cdecl nextafterl(double double) nextafter
@ cdecl -stub nexttoward(double double) nexttoward
@ cdecl -stub nexttowardf(float double) nexttowardf
@ cdecl nexttowardl(double double) nexttoward
@ stub norm
@ stub normf
@ stub norml
@ cdecl perror(str)
@ cdecl pow(double double)
@ cdecl -arch=!i386 powf(float float)
@ cdecl putc(long ptr)
@ cdecl putchar(long)
@ cdecl puts(str)
@ cdecl putwc(long ptr) fputwc
@ cdecl putwchar(long) _fputwchar
@ cdecl qsort(ptr long long ptr)
@ cdecl qsort_s(ptr long long ptr ptr)
@ cdecl quick_exit(long)
@ cdecl raise(long)
@ cdecl rand()
@ cdecl rand_s(ptr)
@ cdecl realloc(ptr long)
@ cdecl -stub remainder(double double)
@ cdecl -stub remainderf(float float)
@ cdecl remainderl(double double) remainder
@ cdecl remove(str)
@ cdecl -stub remquo(double double ptr)
@ cdecl -stub remquof(float float ptr)
@ cdecl remquol(double double ptr) remquo
@ cdecl rename(str str)
@ cdecl rewind(ptr)
@ cdecl -stub rint(double)
@ cdecl -stub rintf(float)
@ cdecl rintl(double) rint
@ cdecl -stub round(double)
@ cdecl -stub roundf(float)
@ cdecl roundl(double) round
@ cdecl scalbln(double long) scalbn # double scalbln(double x, long exp);
@ cdecl scalblnf(float long) scalbnf # float scalblnf(float x, long exp);
@ cdecl scalblnl(double long) scalbn # long double scalblnl(long double x, long exp);
@ cdecl -stub scalbn(double long) # double scalbn(double x, int exp);
@ cdecl -stub scalbnf(float long) # float scalbnf(float x, int exp);
@ cdecl scalbnl(double long) scalbn # long double scalbnl(long double x, int exp);
@ cdecl set_terminate(ptr)
@ cdecl set_unexpected(ptr)
@ cdecl setbuf(ptr ptr)
@ cdecl -arch=arm,x86_64 -norelay -private setjmp(ptr ptr) _setjmp
@ cdecl setlocale(long str)
@ cdecl setvbuf(ptr str long long)
@ cdecl signal(long long)
@ cdecl sin(double)
@ cdecl -arch=!i386 sinf(float)
@ cdecl sinh(double)
@ cdecl -arch=!i386 sinhf(float)
@ cdecl sqrt(double)
@ cdecl -arch=!i386 sqrtf(float)
@ cdecl srand(long)
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
@ cdecl strftime(ptr long str ptr)
@ cdecl strlen(str)
@ cdecl strncat(str str long)
@ cdecl strncat_s(str long str long)
@ cdecl strncmp(str str long)
@ cdecl strncpy(ptr str long)
@ cdecl strncpy_s(ptr long str long)
@ cdecl strnlen(str long)
@ cdecl strpbrk(str str)
@ cdecl strrchr(str long)
@ cdecl strspn(str str)
@ cdecl strstr(str str)
@ cdecl strtod(str ptr)
@ cdecl strtof(str ptr)
@ cdecl -ret64 strtoimax(str ptr long) _strtoi64
@ cdecl strtok(str str)
@ cdecl strtok_s(ptr str ptr)
@ cdecl strtol(str ptr long)
@ cdecl strtold(str ptr) strtod
@ cdecl -ret64 strtoll(str ptr long) _strtoi64
@ cdecl strtoul(str ptr long)
@ cdecl -ret64 strtoull(str ptr long) _strtoui64
@ cdecl -ret64 strtoumax(str ptr long) _strtoui64
@ cdecl strxfrm(ptr str long)
@ cdecl system(str)
@ cdecl tan(double)
@ cdecl -arch=!i386 tanf(float)
@ cdecl tanh(double)
@ cdecl -arch=!i386 tanhf(float)
@ cdecl terminate()
@ cdecl -stub tgamma(double)
@ cdecl -stub tgammaf(float)
@ cdecl tgammal(double) tgamma
@ cdecl tmpfile()
@ cdecl tmpfile_s(ptr)
@ cdecl tmpnam(ptr)
@ cdecl tmpnam_s(ptr long)
@ cdecl tolower(long)
@ cdecl toupper(long)
@ cdecl towctrans(long long)
@ cdecl towlower(long)
@ cdecl towupper(long)
@ cdecl -stub trunc(double)
@ cdecl -stub truncf(float)
@ cdecl truncl(double) trunc
@ stub unexpected
@ cdecl ungetc(long ptr)
@ cdecl ungetwc(long ptr)
@ cdecl wcrtomb(ptr long ptr)
@ cdecl wcrtomb_s(ptr ptr long long ptr)
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
@ cdecl wcsnlen(wstr long)
@ cdecl wcspbrk(wstr wstr)
@ cdecl wcsrchr(wstr long)
@ cdecl wcsrtombs(ptr ptr long ptr)
@ cdecl wcsrtombs_s(ptr ptr long ptr long ptr)
@ cdecl wcsspn(wstr wstr)
@ cdecl wcsstr(wstr wstr)
@ cdecl wcstod(wstr ptr)
@ cdecl wcstof(ptr ptr)
@ cdecl wcstoimax(wstr ptr long)
@ cdecl wcstok(wstr wstr ptr)
@ cdecl wcstok_s(ptr wstr ptr)
@ cdecl wcstol(wstr ptr long)
@ cdecl wcstold(wstr ptr) wcstod
@ cdecl -ret64 wcstoll(wstr ptr long) _wcstoi64
@ cdecl wcstombs(ptr ptr long)
@ cdecl wcstombs_s(ptr ptr long wstr long)
@ cdecl wcstoul(wstr ptr long)
@ cdecl -ret64 wcstoull(wstr ptr long) _wcstoui64
@ cdecl wcstoumax(wstr ptr long)
@ cdecl wcsxfrm(ptr wstr long)
@ cdecl wctob(long)
@ cdecl wctomb(ptr long)
@ cdecl wctomb_s(ptr ptr long long)
@ cdecl wctrans(str)
@ cdecl wctype(str)
@ cdecl wmemcpy_s(ptr long ptr long)
@ cdecl wmemmove_s(ptr long ptr long)
