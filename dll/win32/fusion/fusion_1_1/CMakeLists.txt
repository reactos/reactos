
set(fusion_folder "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_library(fusion_1_1 MODULE
    ${fusion_folder}/version.rc)
spec2def(fusion_1_1 fusion.dll fusion.spec)

set_module_type(fusion_1_1 win32dll)
target_link_libraries(fusion_1_1 fusion_common uuid wine)
set_target_properties(fusion_1_1 PROPERTIES OUTPUT_NAME "fusion" PDB_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
add_importlibs(fusion_1_1 advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion_1_1 DESTINATION reactos/Microsoft.NET/Framework/v1.1.4322 FOR all)
