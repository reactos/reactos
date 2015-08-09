
set(fusion_folder "${CMAKE_CURRENT_SOURCE_DIR}/..")
spec2def(fusion.dll fusion.spec)

add_library(fusion_2_0 SHARED
    $<TARGET_OBJECTS:fusion_lib>
    ${fusion_folder}/version.rc
    ${CMAKE_CURRENT_BINARY_DIR}/fusion.def)

set_module_type(fusion_2_0 win32dll)
target_link_libraries(fusion_2_0 uuid wine)
set_target_properties(fusion_2_0 PROPERTIES OUTPUT_NAME "fusion")
add_importlibs(fusion_2_0 advapi32 dbghelp shlwapi user32 msvcrt kernel32 ntdll)
add_cd_file(TARGET fusion_2_0 DESTINATION reactos/Microsoft.NET/Framework/v2.0.50727 FOR all)
