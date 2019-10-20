
add_definitions(-D__WINESRC__)
include_directories(${REACTOS_SOURCE_DIR}/sdk/include/reactos/wine)
spec2def(hnetcfg.dll hnetcfg.spec)

list(APPEND SOURCE
    apps.c
    hnetcfg.c
    manager.c
    policy.c
    port.c
    profile.c
    service.c
    precomp.h)

add_library(hnetcfg MODULE
    ${SOURCE}
    hnetcfg.rc
    ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg.def)

add_typelib(hnetcfg_tlb.idl)

list(APPEND hnetcfg_rc_deps
    ${CMAKE_CURRENT_SOURCE_DIR}/hnetcfg.rgs
    ${CMAKE_CURRENT_SOURCE_DIR}/hnetcfg_tlb.rgs
    ${CMAKE_CURRENT_BINARY_DIR}/hnetcfg_tlb.tlb)

set_source_files_properties(hnetcfg.rc PROPERTIES OBJECT_DEPENDS "${hnetcfg_rc_deps}")
set_module_type(hnetcfg win32dll)
add_dependencies(hnetcfg stdole2)
target_link_libraries(hnetcfg wine uuid)
add_importlibs(hnetcfg ole32 oleaut32 advapi32 mpr msvcrt kernel32 ntdll)
add_pch(hnetcfg precomp.h SOURCE)
add_cd_file(TARGET hnetcfg DESTINATION reactos/system32 FOR all)
