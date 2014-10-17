
include(ntdll_crt_apitest.cmake)
include(msvcrt_crt_apitest.cmake)

if(NOT ARCH STREQUAL "amd64")
  include(crtdll_crt_apitest.cmake)
endif()
