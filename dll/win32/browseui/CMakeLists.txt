PROJECT(SHELL)

add_subdirectory(shellbars)

set_cpp(WITH_RUNTIME)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/atl)
spec2def(browseui.dll browseui.spec ADD_IMPORTLIB)

list(APPEND SOURCE
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
    precomp.h)

add_library(browseui SHARED
    ${SOURCE}
    dllinstall.c
    browseui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

set_module_type(browseui win32dll UNICODE)
target_link_libraries(browseui shellbars atlnew uuid wine)
add_importlibs(browseui shlwapi shell32 comctl32 gdi32 ole32 oleaut32 user32 advapi32 msvcrt kernel32 ntdll)
add_pch(browseui precomp.h SOURCE)
add_cd_file(TARGET browseui DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(browseui "-Wno-unused-but-set-variable")
    endif()

    # Binutils linker bug
    if(LTCG)
        add_target_link_flags(browseui "-Wl,--allow-multiple-definition")
    endif()
endif()

add_custom_command(TARGET browseui POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:browseui>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:browseui>" 
  COMMENT "Copying to output directory")

add_custom_command(TARGET browseui POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:browseui>"
     "$<TARGET_FILE_DIR:filebrowser>/$<TARGET_FILE_NAME:browseui>" 
  COMMENT "Copying to output directory")
