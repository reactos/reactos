
list(APPEND SOURCE
	Fraginator.cpp
	MainDialog.cpp
	ReportDialog.cpp)

list(APPEND UNFRAG_SOURCE
	Unfrag.cpp
	Defragment.cpp
	DriveVolume.cpp)

add_executable(frag ${SOURCE} ${UNFRAG_SOURCE} Fraginator.rc)
target_link_libraries(frag cppstl)
set_target_cpp_properties(frag WITH_EXCEPTIONS)
set_module_type(frag win32gui UNICODE)
add_importlibs(frag user32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET frag DESTINATION reactos/system32 FOR all)

add_executable(unfrag ${UNFRAG_SOURCE})
target_link_libraries(unfrag cppstl)
set_target_cpp_properties(unfrag WITH_EXCEPTIONS)
target_compile_definitions(unfrag PRIVATE _CUI_)
set_module_type(unfrag win32cui UNICODE)
add_importlibs(unfrag advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET unfrag DESTINATION reactos/system32 FOR all)
