PROJECT(SHELL)

add_subdirectory(shellbars)
add_subdirectory(shellfind)

spec2def(browseui.dll browseui.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    ACLCustomMRU.cpp
    ACLCustomMRU.h
    aclhistory.cpp
    aclistisf.cpp
    aclmulti.cpp
    addressband.cpp
    addresseditbox.cpp
    bandproxy.cpp
    basebarsite.cpp
    brandband.cpp
    browseui.cpp
    browseuiord.cpp
    CAutoComplete.cpp
    CShellTaskScheduler.cpp
    CShellTaskScheduler.h
    CTaskbarList.cpp
    CTaskbarList.h
    commonbrowser.cpp
    desktopipc.cpp
    explorerband.cpp
    globalfoldersettings.cpp
    internettoolbar.cpp
    parsecmdline.cpp
    regtreeoptions.cpp
    shellbrowser.cpp
    toolsband.cpp
    travellog.cpp
    utility.cpp
    CProgressDialog.cpp
    CUserAssist.cpp
    CUserAssist.h)

list(APPEND PCH_SKIP_SOURCE
    dllinstall.c)

add_library(browseui MODULE
    ${SOURCE}
    ${PCH_SKIP_SOURCE}
    browseui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

set_module_type(browseui win32dll UNICODE)
target_link_libraries(browseui shellbars shellfind uuid wine cpprt atl_classes)
add_importlibs(browseui uxtheme shlwapi shell32 comctl32 gdi32 ole32 oleaut32 user32 advapi32 mpr msvcrt kernel32 ntdll)
add_pch(browseui precomp.h "${PCH_SKIP_SOURCE}")
add_cd_file(TARGET browseui DESTINATION reactos/system32 FOR all)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(browseui PRIVATE -Wno-unused-but-set-variable)
endif()

if(NOT MSVC)
    # Binutils linker bug
    if(LTCG)
        target_link_options(browseui PRIVATE "-Wl,--allow-multiple-definition")
    endif()
endif()

add_custom_command(TARGET browseui POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy
     "$<TARGET_FILE:browseui>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:browseui>"
  COMMENT "Copying to output directory")
