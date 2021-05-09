
include_directories(
    ${REACTOS_SOURCE_DIR}/sdk/lib/crt/include
    ${REACTOS_SOURCE_DIR}/sdk/include/c++)

list(APPEND SOURCE
    ehvec.cpp
    new_nothrow.cpp
    terminate.cpp
    typeinfo.cpp)

if(ARCH STREQUAL "i386")
  add_asm_files(cpprt_asm i386/cpprt.s)
  list(APPEND SOURCE i386/framehandler.c)
elseif(ARCH STREQUAL "amd64")
  add_asm_files(cpprt_asm amd64/cpprt.s)
elseif(ARCH STREQUAL "arm")
  add_asm_files(cpprt_asm arm/cpprt.s)
endif()

add_library(cpprt ${SOURCE} ${cpprt_asm})
set_target_cpp_properties(cpprt WITH_EXCEPTIONS)
add_dependencies(cpprt xdk)
