
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND GCC_MSVCRT_SUPPORT_SOURCE
    __mingw_vsprintf.c
    StringCchCopyNA.c)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    list(APPEND GCC_MSVCRT_SUPPORT_SOURCE
        K32EnumProcessModules.c)
endif()

if(DLL_EXPORT_VERSION LESS 0x600)
    list(APPEND GCC_MSVCRT_SUPPORT_SOURCE
        mbsrtowcs.c
        msvcrt_support.c
        rand_s.c)
endif()

add_library(gcc_msvcrt_support ${GCC_MSVCRT_SUPPORT_SOURCE})
add_dependencies(gcc_msvcrt_support xdk)
target_link_libraries(gcc_msvcrt_support libmsvcrt)
target_compile_definitions(gcc_msvcrt_support PRIVATE _CRT_DECLARE_NONSTDC_NAMES=1)

add_library(stdc++compat __throw_out_of_range_fmt.cpp)
set_target_cpp_properties(stdc++compat WITH_EXCEPTIONS)
add_dependencies(stdc++compat xdk)
target_link_libraries(stdc++compat PUBLIC gcc_msvcrt_support)
remove_target_compile_option(stdc++compat "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<IN_LIST:cppstl,$<TARGET_PROPERTY:LINK_LIBRARIES>>>>:-nostdinc>")
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(NOT DEFINED LLVM_LIBCXX_INCLUDE_MIRROR_DIR)
        message(FATAL_ERROR "LLVM libc++ include mirror was not configured")
    endif()
    target_include_directories(stdc++compat BEFORE PRIVATE "${LLVM_LIBCXX_INCLUDE_MIRROR_DIR}")
endif()
target_compile_definitions(stdc++compat PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:PAL_STDCPP_COMPAT>" _CRT_DECLARE_NONSTDC_NAMES=1)
