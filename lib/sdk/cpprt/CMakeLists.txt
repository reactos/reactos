
set_cpp(WITH_EXCEPTIONS)

include_directories(${REACTOS_SOURCE_DIR}/lib/sdk/crt/include)

list(APPEND SOURCE
    ehvec.cpp
    typeinfo.cpp)

if(ARCH STREQUAL "i386")
  add_asm_files(cpprt_asm i386/cpprt.s)
  list(APPEND SOURCE i386/framehandler.c)
elseif(ARCH STREQUAL "amd64")
  add_asm_files(cpprt_asm amd64/cpprt.s)
endif()

add_library(cpprt ${SOURCE} ${cpprt_asm})
