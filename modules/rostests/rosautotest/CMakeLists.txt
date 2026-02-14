
list(APPEND SOURCE
    auto_array_ptr.h
    CConfiguration.cpp
    CConfiguration.h
    CFatalException.cpp
    CFatalException.h
    CInvalidParameterException.cpp
    CInvalidParameterException.h
    CJournaledTestList.cpp
    CJournaledTestList.h
    CPipe.cpp
    CPipe.h
    CPipedProcess.cpp
    CPipedProcess.h
    CProcess.cpp
    CProcess.h
    CSimpleException.cpp
    CSimpleException.h
    CTest.h
    CTestInfo.h
    CTestList.cpp
    CTestList.h
    CVirtualTestList.cpp
    CVirtualTestList.h
    CWebService.cpp
    CWebService.h
    CWebServiceLibCurl.cpp
    CWebServiceLibCurl.h
    CWebServiceWinInet.cpp
    CWebServiceWinInet.h
    CWineTest.cpp
    CWineTest.h
    main.cpp
    misc.cpp
    shutdown.cpp
    tools.cpp
    precomp.h)

add_executable(rosautotest ${SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/rosautotestmsg.rc)
target_link_libraries(rosautotest cppstl)
set_target_cpp_properties(rosautotest WITH_EXCEPTIONS)
set_module_type(rosautotest win32cui UNICODE)
add_importlibs(rosautotest advapi32 shell32 user32 wininet msvcrt kernel32 ntdll)
add_pch(rosautotest precomp.h SOURCE)
add_message_headers(ANSI rosautotestmsg.mc)
add_dependencies(rosautotest rosautotestmsg)
add_cd_file(TARGET rosautotest DESTINATION reactos/system32 FOR all)
