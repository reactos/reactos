
add_definitions(-DWINE_REGISTER_DLL -DPROXY_DELEGATION -DPROXY_CLSID=CLSID_PSFactoryBuffer)
spec2def(ieproxy.dll ieproxy.spec ADD_IMPORTLIB)
generate_idl_iids(ieproxy_ieautomation.idl)
add_rpcproxy_files(ieproxy_ieautomation.idl ieproxy_perhist.idl)

add_library(ieproxy MODULE
    ${CMAKE_CURRENT_BINARY_DIR}/ieproxy.def
    ${CMAKE_CURRENT_BINARY_DIR}/ieproxy_stubs.c
    ${CMAKE_CURRENT_BINARY_DIR}/proxy.dlldata.c
    ${CMAKE_CURRENT_BINARY_DIR}/ieproxy_ieautomation_p.c
    ${CMAKE_CURRENT_BINARY_DIR}/ieproxy_ieautomation_i.c
    ${CMAKE_CURRENT_BINARY_DIR}/ieproxy_perhist_p.c)

set_module_type(ieproxy win32dll)
add_idl_reg_scripts(ieproxy registry ieproxy_ieautomation.idl ieproxy_perhist.idl)
target_link_libraries(ieproxy uuid wine wine_dll_register wine_dll_canunload)
add_importlibs(ieproxy rpcrt4 ole32 oleaut32 msvcrt kernel32 ntdll)
add_cd_file(TARGET ieproxy DESTINATION reactos/system32 FOR all)
set_wine_module(ieproxy)
