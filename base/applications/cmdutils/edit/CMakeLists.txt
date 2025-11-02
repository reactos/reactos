
PROJECT(edit)

list(APPEND DFLAT_SOURCE
    fdeditw32/applicat.c
    ## fdeditw32/barchart.c
    fdeditw32/box.c
    fdeditw32/button.c
    fdeditw32/calendar.c
    fdeditw32/checkbox.c
    fdeditw32/clipbord.c
    fdeditw32/combobox.c
    fdeditw32/config.c
    ## fdeditw32/console.c      ## Win32 implementation in console_w32.c
    fdeditw32/decomp.c
    fdeditw32/dfalloc.c
    fdeditw32/dialbox.c
    fdeditw32/direct.c
    fdeditw32/editbox.c
    fdeditw32/editor.c
    fdeditw32/fileopen.c
    fdeditw32/helpbox.c
    fdeditw32/keys.c
    fdeditw32/listbox.c
    fdeditw32/lists.c
    fdeditw32/log.c
    fdeditw32/menu.c
    fdeditw32/menubar.c
    fdeditw32/message.c
    ## fdeditw32/mouse.c        ## Win32 implementation in console_w32.c
    fdeditw32/msgbox.c
    fdeditw32/normal.c
    fdeditw32/pictbox.c
    fdeditw32/popdown.c
    fdeditw32/radio.c
    fdeditw32/rect.c
    fdeditw32/search.c
    fdeditw32/slidebox.c
    fdeditw32/spinbutt.c
    fdeditw32/statbar.c
    fdeditw32/sysmenu.c
    fdeditw32/text.c
    fdeditw32/textbox.c
    ## fdeditw32/video.c        ## Win32 implementation in video_w32.c
    fdeditw32/watch.c
    fdeditw32/window.c
)
list(APPEND DFLAT_SOURCE_WIN32
    fdeditw32/clipbord_w32.c    ## Win32 implementation for some clipbord.c helpers
    fdeditw32/console_w32.c
    fdeditw32/country_w32.c
    fdeditw32/dir_w32.c         ## Win32 implementation of directory/drive functions
    fdeditw32/timer_w32.c       ## Win32 implementation for timer functions in message.c
    fdeditw32/video_w32.c
)
add_library(dflat20_static ${DFLAT_SOURCE} ${DFLAT_SOURCE_WIN32})
target_compile_definitions(dflat20_static PRIVATE
    _CRT_DECLARE_NONSTDC_NAMES=1 # This must be set to 1
    _CRT_NONSTDC_NO_DEPRECATE
    INCLUDE_PICTUREBOX
    INCLUDE_MULTI_WINDOWS
    # INCLUDE_WINDOWOPTIONS
    INCLUDE_MAXIMIZE
    INCLUDE_MINIMIZE
    INCLUDE_RESTORE
    INCLUDE_EXTENDEDSELECTIONS
    INCLUDE_SHELLDOS
    _FDEDIT_PRIVATE_CLIPBOARD
)

add_executable(huffc fdeditw32/huffc.c fdeditw32/htree.c)
set_module_type(huffc win32cui) ## UNICODE
add_importlibs(huffc msvcrt kernel32)

add_executable(fixhelp fdeditw32/decomp.c fdeditw32/fixhelp.c)
set_module_type(fixhelp win32cui) ## UNICODE
add_importlibs(fixhelp msvcrt kernel32)

# add_custom_target(edit_help DEPENDS ${rosapps_converted_item})

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/edit.hlp
    COMMAND huffc fdeditw32/edit.txt ${CMAKE_CURRENT_BINARY_DIR}/edit.hlp
    COMMAND fixhelp ${CMAKE_CURRENT_BINARY_DIR}/edit.hlp
    DEPENDS huffc fixhelp)


list(APPEND EDIT_SOURCE
    fdeditw32/dialogs.c
    fdeditw32/edit.c
    fdeditw32/menus.c
    fdeditw32/rdonly.c
)
add_executable(edit ${EDIT_SOURCE} edit.rc)
target_compile_definitions(edit PRIVATE
    ##_CRT_DECLARE_NONSTDC_NAMES=1 # This must be set to 1
    ##_CRT_NONSTDC_NO_DEPRECATE
    INCLUDE_PICTUREBOX
    INCLUDE_MULTI_WINDOWS
    # INCLUDE_WINDOWOPTIONS
    INCLUDE_MAXIMIZE
    INCLUDE_MINIMIZE
    INCLUDE_RESTORE
    INCLUDE_EXTENDEDSELECTIONS
    INCLUDE_SHELLDOS
    _FDEDIT_PRIVATE_CLIPBOARD
)

set_module_type(edit win32cui) ## UNICODE
target_link_libraries(edit dflat20_static oldnames)
add_importlibs(edit user32 msvcrt kernel32)
#set_target_properties(edit PROPERTIES OUTPUT_NAME "edit" SUFFIX ".com")
add_cd_file(TARGET edit DESTINATION reactos/system32 FOR all)
