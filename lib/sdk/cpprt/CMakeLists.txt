
set_cpp(WITH_EXCEPTIONS)

list(APPEND SOURCE
    ehvec.cpp
    typeinfo.cpp)

if(ARCH STREQUAL "i386")
  add_asm_files(cpprt_asm i386/cpprt.s)
elseif(ARCH STREQUAL "amd64")
  add_asm_files(cpprt_asm amd64/cpprt.s)
endif()
add_library(cpprt ${SOURCE} ${cpprt_asm})
