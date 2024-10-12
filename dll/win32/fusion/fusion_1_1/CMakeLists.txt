
set(baseaddress_fusion_1_1 ${baseaddress_fusion})

set(fusion_folder "${CMAKE_CURRENT_SOURCE_DIR}/..")
spec2def(fusion.dll fusion.spec)

add_library(fusion_1_1 MODULE
    ${fusion_folder}/version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion_1_1 win32dll)
target_link_libraries(fusion_1_1 fusion_common uuid wine oldnames)
set_target_properties(fusion_1_1 PROPERTIES OUTPUT_NAME "fusion" PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_importlibs(fusion_1_1 bcrypt advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion_1_1 DESTINATION reactos/Microsoft.NET/Framework/v1.1.4322 FOR all)
