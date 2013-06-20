
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

set_module_type(rosautotest win32cui UNICODE)
add_importlibs(rosautotest advapi32 shell32 user32 wininet msvcrt kernel32 ntdll)
add_pch(rosautotest precomp.h)

add_cd_file(TARGET rosautotest DESTINATION reactos/system32 FOR all)
