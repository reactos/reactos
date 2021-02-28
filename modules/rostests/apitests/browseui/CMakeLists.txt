
include_directories(
    includes)

list(APPEND SOURCE
    ACListISF.cpp
    IACLCustomMRU.cpp
	  IAutoComplete.cpp
    SHEnumClassesOfCategories.cpp
    SHExplorerParseCmdLine.c
    testlist.c)

add_executable(browseui_apitest ${SOURCE})

target_link_libraries(browseui_apitest uuid wine cpprt atl_classes)
set_target_cpp_properties(browseui_apitest WITH_EXCEPTIONS WITH_RTTI)
set_module_type(browseui_apitest win32cui)
add_importlibs(browseui_apitest advapi32 shell32 ole32 shlwapi msvcrt user32 kernel32 ntdll)
add_rostests_file(TARGET browseui_apitest)
