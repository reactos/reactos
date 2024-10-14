
file(GLOB welcome_rc_deps res/*.*)
add_rc_deps(welcome.rc ${welcome_rc_deps})
add_executable(welcome welcome.c welcome.rc)
set_module_type(welcome win32gui UNICODE)
add_importlibs(welcome gdi32 user32 shell32 msvcrt kernel32 ntdll)

# Reduce the required subsystem to WinNT 4.0 for i386 builds only.
if(ARCH STREQUAL "i386")
    if(MSVC)
        # NOTE: We cannot use the following command:
        #   target_link_options(welcome PRIVATE "/SUBSYSTEM:WINDOWS,4.00")
        # because it would act at the level of the LINK.EXE linker flags,
        # which only accepts a subsystem version >= 5.10 (Windows XP+) on
        # latest MSVC versions.
        # So to work around this problem, we use a post-build command by
        # employing EDITBIN.EXE that does not check the subsystem version.
        #
        add_custom_command(
            TARGET welcome POST_BUILD
            COMMAND editbin.exe /NOLOGO /SUBSYSTEM:WINDOWS,4.00 $<TARGET_FILE:welcome>
            VERBATIM)
    else()
        # The binutils linker does not set a lower limit on the subsystem.
        # Otherwise we would use: objcopy --subsystem windows:4.00 $<TARGET_FILE:welcome>
        target_link_options(welcome PRIVATE "-Wl,--subsystem,windows:4.00")
    endif()
endif()

add_cd_file(TARGET welcome DESTINATION reactos NO_CAB FOR bootcd)
