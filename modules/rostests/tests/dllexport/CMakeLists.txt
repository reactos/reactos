
spec2def(dllexport_test_dll1.dll dllexport_test_dll1.spec ADD_IMPORTLIB)
spec2def(dllexport_test_dll2.dll dllexport_test_dll2.spec ADD_IMPORTLIB)

set(baseaddress_dllexport_test_dll1 0x1000000)

add_library(dllexport_test_dll1 MODULE
    dllexport_test_dll1.c
    ${CMAKE_CURRENT_BINARY_DIR}/dllexport_test_dll1.def)

set_module_type(dllexport_test_dll1 module)
add_importlibs(dllexport_test_dll1 dllexport_test_dll2)

set(baseaddress_dllexport_test_dll2 0x2000000)

add_library(dllexport_test_dll2 MODULE
    dllexport_test_dll2.c
    ${CMAKE_CURRENT_BINARY_DIR}/dllexport_test_dll2.def)

set_module_type(dllexport_test_dll2 module)

add_executable(dllexport_test dllexport_test.c)
set_module_type(dllexport_test win32cui)
add_importlibs(dllexport_test dllexport_test_dll1 msvcrt kernel32 ntdll)

if(MSVC)
    add_compile_flags("/Zc:wchar_t-")
endif()

# framedyn is skipped in the clang-cl build
if(NOT USE_CLANG_CL)

add_library(dllimport_test MODULE dllimport_framedyn.cpp)
set_module_type(dllimport_test module)
add_importlibs(dllimport_test framedyn)

if(NOT MSVC)
    target_link_libraries(dllimport_test framedynex)
endif()

endif()
