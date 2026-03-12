
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include)

list(APPEND SOURCE
    __mingw_vsprintf.c
    __throw_out_of_range_fmt.cpp
    StringCchCopyNA.c)

add_library(stdc++compat ${SOURCE})
set_target_cpp_properties(stdc++compat WITH_EXCEPTIONS)
add_dependencies(stdc++compat xdk)
target_link_libraries(stdc++compat libmsvcrt)
remove_target_compile_option(stdc++compat "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<IN_LIST:cppstl,$<TARGET_PROPERTY:LINK_LIBRARIES>>>>:-nostdinc>")
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    remove_target_compile_option(stdc++compat "$<$<AND:$<COMPILE_LANGUAGE:CXX>,$<NOT:$<IN_LIST:cppstl,$<TARGET_PROPERTY:LINK_LIBRARIES>>>>:-nostdlibinc>")

    if(REACTOS_CLANG_GCC_TOOLCHAIN AND CMAKE_CXX_COMPILER_TARGET)
        file(GLOB _mingw_cxx_include_versions LIST_DIRECTORIES TRUE
            "${REACTOS_CLANG_GCC_TOOLCHAIN}/${CMAKE_CXX_COMPILER_TARGET}/include/c++/*")
        list(SORT _mingw_cxx_include_versions COMPARE NATURAL ORDER DESCENDING)
        foreach(_include_dir IN LISTS _mingw_cxx_include_versions)
            if(IS_DIRECTORY "${_include_dir}" AND EXISTS "${_include_dir}/bits")
                set(_mingw_cxx_include_dir "${_include_dir}")
                break()
            endif()
        endforeach()

        set(_mingw_cxx_include_roots
            "${REACTOS_CLANG_GCC_TOOLCHAIN}/${CMAKE_CXX_COMPILER_TARGET}/include"
            "${REACTOS_CLANG_GCC_TOOLCHAIN}/${CMAKE_CXX_COMPILER_TARGET}/include/c++")
        if(_mingw_cxx_include_dir)
            list(APPEND _mingw_cxx_include_roots
                "${_mingw_cxx_include_dir}"
                "${_mingw_cxx_include_dir}/${CMAKE_CXX_COMPILER_TARGET}"
                "${_mingw_cxx_include_dir}/backward")
        endif()
        if(EXISTS "${REACTOS_CLANG_GCC_TOOLCHAIN}/${CMAKE_CXX_COMPILER_TARGET}/sysroot/usr/${CMAKE_CXX_COMPILER_TARGET}/include")
            list(APPEND _mingw_cxx_include_roots
                "${REACTOS_CLANG_GCC_TOOLCHAIN}/${CMAKE_CXX_COMPILER_TARGET}/sysroot/usr/${CMAKE_CXX_COMPILER_TARGET}/include")
        endif()

        list(REMOVE_DUPLICATES _mingw_cxx_include_roots)
        foreach(_include_dir IN LISTS _mingw_cxx_include_roots)
            if(EXISTS "${_include_dir}")
                target_include_directories(stdc++compat SYSTEM PRIVATE "${_include_dir}")
            endif()
        endforeach()
    endif()
endif()
target_compile_definitions(stdc++compat PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:PAL_STDCPP_COMPAT>" _CRT_DECLARE_NONSTDC_NAMES=1)
