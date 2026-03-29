
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include
    ${REACTOS_SOURCE_DIR}/sdk/include/c++)

list(APPEND SOURCE
    __mingw_vsprintf.c
    __throw_out_of_range_fmt.cpp
    StringCchCopyNA.c)

if(DLL_EXPORT_VERSION LESS 0x600)
    list(APPEND SOURCE
        fseeki64_compat.c
        mbsrtowcs_compat.c
        rand_s.c
        snprintf_compat.c
        strnlen_compat.c
        wchar_compat.c)
endif()

add_library(stdc++compat ${SOURCE})
set_target_cpp_properties(stdc++compat WITH_EXCEPTIONS)
set_target_properties(stdc++compat PROPERTIES USE_SYSTEM_CXX_HEADERS TRUE)
add_dependencies(stdc++compat xdk)
target_link_libraries(stdc++compat PRIVATE libmsvcrt cxx_system_includes)
target_compile_definitions(stdc++compat PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:PAL_STDCPP_COMPAT>" _CRT_DECLARE_NONSTDC_NAMES=1)
