
set_cpp(WITH_RUNTIME WITH_STL)

list(APPEND SOURCE
	Fraginator.cpp
	MainDialog.cpp
	ReportDialog.cpp)

list(APPEND UNFRAG_SOURCE
	Unfrag.cpp
	Defragment.cpp
	DriveVolume.cpp)
    
add_executable(frag ${SOURCE} ${UNFRAG_SOURCE} Fraginator.rc)
set_module_type(frag win32gui UNICODE)
add_importlibs(frag user32 advapi32 shell32 comctl32 msvcrt kernel32 ntdll)
add_cd_file(TARGET frag DESTINATION reactos/system32 FOR all)

add_executable(unfrag ${UNFRAG_SOURCE})
add_target_compile_definitions(unfrag "_CUI_")
set_module_type(unfrag win32cui UNICODE)
add_importlibs(unfrag advapi32 msvcrt kernel32 ntdll)
add_cd_file(TARGET unfrag DESTINATION reactos/system32 FOR all)