
add_definitions(
    -D__WINESRC__
    -D__ROS_LONG64__)

spec2def(dpnhupnp.dll dpnhupnp.spec)

list(APPEND SOURCE
    main.c
    ${CMAKE_CURRENT_BINARY_DIR}/dpnhupnp.def)

add_library(dpnhupnp MODULE ${SOURCE} version.rc)
set_module_type(dpnhupnp win32dll)
target_link_libraries(dpnhupnp wine wine_dll_register wine_dll_canunload dxguid uuid oldnames)
add_importlibs(dpnhupnp ole32 msvcrt kernel32 ntdll)
add_dependencies(dpnhupnp wineheaders)
add_cd_file(TARGET dpnhupnp DESTINATION reactos/system32 FOR all)
set_wine_module(dpnhupnp)
