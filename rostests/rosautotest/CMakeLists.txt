
set_cpp(WITH_EXCEPTIONS WITH_STL)

list(APPEND SOURCE
    CConfiguration.cpp
    CFatalException.cpp
    CInvalidParameterException.cpp
    CJournaledTestList.cpp
    CPipe.cpp
    CPipedProcess.cpp
    CProcess.cpp
    CSimpleException.cpp
    CTest.cpp
    CTestInfo.cpp
    CTestList.cpp
    CVirtualTestList.cpp
    CWebService.cpp
    CWineTest.cpp
    main.cpp
    misc.cpp
    shutdown.cpp
    tools.cpp
    precomp.h)

add_executable(rosautotest ${SOURCE} ${REACTOS_BINARY_DIR}/include/reactos/rosautotestmsg.rc)
set_module_type(rosautotest win32cui UNICODE)
add_importlibs(rosautotest advapi32 shell32 user32 wininet msvcrt kernel32 ntdll)
add_pch(rosautotest precomp.h SOURCE)
add_message_headers(ANSI rosautotestmsg.mc)
add_dependencies(rosautotest rosautotestmsg)
add_cd_file(TARGET rosautotest DESTINATION reactos/system32 FOR all)
