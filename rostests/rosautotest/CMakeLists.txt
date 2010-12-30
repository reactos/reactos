
set_unicode()
set_cpp()

list(APPEND SOURCE
    CConfiguration.cpp
    CFatalException.cpp
    CInvalidParameterException.cpp
    CJournaledTestList.cpp
    CProcess.cpp
    CSimpleException.cpp
    CTest.cpp
    CTestInfo.cpp
    CTestList.cpp
    CVirtualTestList.cpp
    CWebService.cpp
    CWineTest.cpp
    main.cpp
    shutdown.cpp
    tools.cpp)

add_executable(rosautotest ${SOURCE})

target_link_libraries(rosautotest wine)

set_module_type(rosautotest win32cui)
add_importlibs(rosautotest advapi32 shell32 user32 wininet msvcrt kernel32 ntdll)
