
set(baseaddress_fusion_2_0 ${baseaddress_fusion})

set(fusion_folder "${CMAKE_CURRENT_SOURCE_DIR}/..")
spec2def(fusion.dll fusion.spec)

add_library(fusion_2_0 MODULE
    ${fusion_folder}/version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion_2_0 win32dll)
target_link_libraries(fusion_2_0 fusion_common uuid wine oldnames)
set_target_properties(fusion_2_0 PROPERTIES OUTPUT_NAME "fusion" PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_importlibs(fusion_2_0 bcrypt advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion_2_0 DESTINATION reactos/Microsoft.NET/Framework/v2.0.50727 FOR all)
