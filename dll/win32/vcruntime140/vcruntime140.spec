@ cdecl _CreateFrameInfo(ptr ptr) ucrtbase._CreateFrameInfo
@ stdcall _CxxThrowException(ptr ptr) ucrtbase._CxxThrowException
@ cdecl -arch=i386 -norelay _EH_prolog() ucrtbase._EH_prolog
@ cdecl _FindAndUnlinkFrame(ptr) ucrtbase._FindAndUnlinkFrame
@ cdecl _IsExceptionObjectToBeDestroyed(ptr) ucrtbase._IsExceptionObjectToBeDestroyed
@ stub _NLG_Dispatch2
@ stub _NLG_Return
@ stub _NLG_Return2
@ cdecl _SetWinRTOutOfMemoryExceptionCallback(ptr) ucrtbase._SetWinRTOutOfMemoryExceptionCallback
@ cdecl __AdjustPointer(ptr ptr) ucrtbase.__AdjustPointer
@ stub __BuildCatchObject
@ stub __BuildCatchObjectHelper
@ stdcall -arch=!i386 __C_specific_handler(ptr long ptr ptr) ucrtbase.__C_specific_handler
@ stub __C_specific_handler_noexcept
@ cdecl __CxxDetectRethrow(ptr) ucrtbase.__CxxDetectRethrow
@ cdecl __CxxExceptionFilter(ptr ptr long ptr) ucrtbase.__CxxExceptionFilter
@ cdecl -norelay __CxxFrameHandler(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler
@ cdecl -norelay __CxxFrameHandler2(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler2
@ cdecl -norelay __CxxFrameHandler3(ptr ptr ptr ptr) ucrtbase.__CxxFrameHandler3
@ stdcall -arch=i386 __CxxLongjmpUnwind(ptr) ucrtbase.__CxxLongjmpUnwind
@ cdecl __CxxQueryExceptionSize() ucrtbase.__CxxQueryExceptionSize
@ cdecl __CxxRegisterExceptionObject(ptr ptr) ucrtbase.__CxxRegisterExceptionObject
@ cdecl __CxxUnregisterExceptionObject(ptr long) ucrtbase.__CxxUnregisterExceptionObject
@ cdecl __DestructExceptionObject(ptr) ucrtbase.__DestructExceptionObject
@ stub __FrameUnwindFilter
@ stub __GetPlatformExceptionInfo
@ stub __NLG_Dispatch2
@ stub __NLG_Return2
@ cdecl __RTCastToVoid(ptr) ucrtbase.__RTCastToVoid
@ cdecl __RTDynamicCast(ptr long ptr ptr long) ucrtbase.__RTDynamicCast
@ cdecl __RTtypeid(ptr) ucrtbase.__RTtypeid
@ stub __TypeMatch
@ cdecl __current_exception() ucrtbase.__current_exception
@ cdecl __current_exception_context() ucrtbase.__current_exception_context
@ cdecl -norelay __intrinsic_setjmp(ptr) ucrtbase.__intrinsic_setjmp
@ cdecl -arch=!i386 -norelay __intrinsic_setjmpex(ptr ptr) ucrtbase.__intrinsic_setjmpex
@ stdcall -arch=arm __jump_unwind(ptr ptr) ucrtbase.__jump_unwind
@ cdecl __processing_throw() ucrtbase.__processing_throw
@ stub __report_gsfailure
@ cdecl __std_exception_copy(ptr ptr) ucrtbase.__std_exception_copy
@ cdecl __std_exception_destroy(ptr) ucrtbase.__std_exception_destroy
@ cdecl __std_terminate() ucrtbase.__std_terminate
@ cdecl __std_type_info_compare(ptr ptr) ucrtbase.__std_type_info_compare
@ cdecl __std_type_info_destroy_list(ptr) ucrtbase.__std_type_info_destroy_list
@ cdecl __std_type_info_hash(ptr) ucrtbase.__std_type_info_hash
@ cdecl __std_type_info_name(ptr ptr) ucrtbase.__std_type_info_name
@ cdecl __telemetry_main_invoke_trigger(ptr)
@ cdecl __telemetry_main_return_trigger(ptr)
@ cdecl __unDName(ptr str long ptr ptr long) ucrtbase.__unDName
@ cdecl __unDNameEx(ptr str long ptr ptr ptr long) ucrtbase.__unDNameEx
@ cdecl __uncaught_exception() ucrtbase.__uncaught_exception
@ cdecl __uncaught_exceptions()
@ stub __vcrt_GetModuleFileNameW
@ stub __vcrt_GetModuleHandleW
@ cdecl __vcrt_InitializeCriticalSectionEx(ptr long long)
@ stub __vcrt_LoadLibraryExW
@ cdecl -arch=i386 -norelay _chkesp() ucrtbase._chkesp
@ cdecl -arch=i386 _except_handler2(ptr ptr ptr ptr) ucrtbase._except_handler2
@ cdecl -arch=i386 _except_handler3(ptr ptr ptr ptr) ucrtbase._except_handler3
@ cdecl -arch=i386 _except_handler4_common(ptr ptr ptr ptr ptr ptr) ucrtbase._except_handler4_common
@ cdecl _get_purecall_handler() ucrtbase._get_purecall_handler
@ cdecl _get_unexpected() ucrtbase._get_unexpected
@ cdecl -arch=i386 _global_unwind2(ptr) ucrtbase._global_unwind2
@ stub _is_exception_typeof
@ cdecl -arch=i386 _local_unwind2(ptr long) ucrtbase._local_unwind2
@ cdecl -arch=i386 _local_unwind4(ptr ptr long) ucrtbase._local_unwind4
@ cdecl -arch=i386 _longjmpex(ptr long) ucrtbase._longjmpex
@ cdecl -arch=win64 _local_unwind(ptr ptr) ucrtbase._local_unwind
@ cdecl _purecall() ucrtbase._purecall
@ stdcall -arch=i386 _seh_longjmp_unwind4(ptr) ucrtbase._seh_longjmp_unwind4
@ stdcall -arch=i386 _seh_longjmp_unwind(ptr) ucrtbase._seh_longjmp_unwind
@ cdecl _set_purecall_handler(ptr) ucrtbase._set_purecall_handler
@ cdecl _set_se_translator(ptr) ucrtbase._set_se_translator
@ cdecl -arch=i386 -norelay _setjmp3(ptr long) ucrtbase._setjmp3
@ cdecl longjmp(ptr long) ucrtbase.longjmp
@ cdecl memchr(ptr long long) ucrtbase.memchr
@ cdecl memcmp(ptr ptr long) ucrtbase.memcmp
@ cdecl memcpy(ptr ptr long) ucrtbase.memcpy
@ cdecl memmove(ptr ptr long) ucrtbase.memmove
@ cdecl memset(ptr long long) ucrtbase.memset
@ cdecl set_unexpected(ptr) ucrtbase.set_unexpected
@ cdecl strchr(str long) ucrtbase.strchr
@ cdecl strrchr(str long) ucrtbase.strrchr
@ cdecl strstr(str str) ucrtbase.strstr
@ stub unexpected
@ cdecl wcschr(wstr long) ucrtbase.wcschr
@ cdecl wcsrchr(wstr long) ucrtbase.wcsrchr
@ cdecl wcsstr(wstr wstr) ucrtbase.wcsstr
