
spec2def(msvcrt_mini.dll msvcrt_mini.spec ADD_IMPORTLIB)
remove_definitions(-D_DLL -D__USE_CRTIMP)
include(ntdll_crt_apitest.cmake)
include(crtdll_crt_apitest.cmake)
include(msvcrt_crt_apitest.cmake)
