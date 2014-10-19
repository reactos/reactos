PROJECT(SHELL)

set_cpp(WITH_RUNTIME)

remove_definitions(-D_WIN32_WINNT=0x502)
add_definitions(-D_WIN32_WINNT=0x600)

include_directories(${REACTOS_SOURCE_DIR}/lib/atl)
spec2def(browseui.dll browseui.spec ADD_IMPORTLIB)

list(APPEND SOURCE
    aclmulti.cpp
    addressband.cpp
    addresseditbox.cpp
    bandproxy.cpp
    bandsite.cpp
    bandsitemenu.cpp
    basebar.cpp
    basebarsite.cpp
    brandband.cpp
    browseui.cpp
    browseuiord.cpp
    commonbrowser.cpp
    desktopipc.cpp
    globalfoldersettings.cpp
    internettoolbar.cpp
    parsecmdline.cpp
    regtreeoptions.cpp
    shellbrowser.cpp
    toolsband.cpp
    travellog.cpp
    utility.cpp
    precomp.h)

add_library(browseui SHARED
    ${SOURCE}
    dllinstall.c
    browseui.rc
    ${CMAKE_CURRENT_BINARY_DIR}/browseui.def)

set_module_type(browseui win32dll UNICODE)
target_link_libraries(browseui atlnew uuid wine)
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
