
#
# Define three Delay-Import binary test programs:
# - "nohook": delayload is used, but neither of the hook/failurehook is defined;
# - "globalhook": delayload is used, and the hooks are assigned at global scope;
# - "runtimehook": delayload is used, and the hooks are assigned at runtime.
#
list(APPEND DELAYIMP_TEST_NAME nohook globalhook runtimehook)
list(APPEND DELAYIMP_TEST_INDEX 0 1 2)
foreach(testname testid IN ZIP_LISTS DELAYIMP_TEST_NAME DELAYIMP_TEST_INDEX)
    add_executable(delayimp_${testname}_apitest delayimp.cpp)
    target_compile_definitions(delayimp_${testname}_apitest PRIVATE DELAYIMP_TEST=${testid})
    set_module_type(delayimp_${testname}_apitest win32cui)
    target_link_libraries(delayimp_${testname}_apitest ${PSEH_LIB})
    add_importlibs(delayimp_${testname}_apitest msvcrt kernel32)
    add_delay_importlibs(delayimp_${testname}_apitest winmm version dbghelp shlwapi sfc_os imagehlp)
    add_rostests_file(TARGET delayimp_${testname}_apitest)
endforeach()
