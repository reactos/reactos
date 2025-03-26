
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include
    ${REACTOS_SOURCE_DIR}/sdk/include/c++)

# We need different things, depending on the compiler
if(MSVC)
    list(APPEND SOURCE
        ehvec.cpp
        new_nothrow.cpp
        terminate.cpp
        typeinfo.cpp)
    if(ARCH STREQUAL "i386")
        add_asm_files(cpprt_asm i386/cpprt.s)
    elseif(ARCH STREQUAL "amd64")
        add_asm_files(cpprt_asm amd64/cpprt.s)
    elseif(ARCH STREQUAL "arm")
        add_asm_files(cpprt_asm arm/cpprt.s)
    endif()
else()
    list(APPEND SOURCE
        __cxa_pure_virtual.cpp)
endif()

add_library(cpprt ${SOURCE} ${cpprt_asm})
set_target_cpp_properties(cpprt WITH_EXCEPTIONS)
add_dependencies(cpprt xdk)

# On GCC builds, we need to link against libsupc++
if(NOT MSVC)
    target_link_libraries(cpprt libsupc++)
endif()
