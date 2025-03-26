
list(APPEND SOURCE
    sio/PRead.c
    sio/PWrite.c
    sio/SAcceptA.c
    sio/SAcceptS.c
    sio/SBind.c
    sio/SClose.c
    sio/SConnect.c
    sio/SConnectByName.c
    sio/SNew.c
    sio/SRead.c
    sio/SReadline.c
    sio/SRecv.c
    sio/SRecvfrom.c
    sio/SRecvmsg.c
    sio/SSelect.c
    sio/SSend.c
    sio/SSendto.c
    sio/SSendtoByName.c
    sio/SWrite.c
    sio/SocketUtil.c
    sio/StrAddr.c
    sio/UAcceptA.c
    sio/UAcceptS.c
    sio/UBind.c
    sio/UConnect.c
    sio/UConnectByName.c
    sio/UNew.c
    sio/URecvfrom.c
    sio/USendto.c
    sio/USendtoByName.c
    sio/SError.c
    sio/SWait.c
    sio/main.c
    Strn/Dynscat.c
    Strn/Strncpy.c
    Strn/Strncat.c
    Strn/Strntok.c
    Strn/Strnpcpy.c
    Strn/Strnpcat.c
    Strn/strtokc.c
    Strn/version.c
    libncftp/open.c
    libncftp/cmds.c
    libncftp/util.c
    libncftp/rcmd.c
    libncftp/ftp.c
    libncftp/io.c
    libncftp/errno.c
    libncftp/linelist.c
    libncftp/glob.c
    ncftp/cmds.c
    ncftp/cmdlist.c
    ncftp/getopt.c
    ncftp/ls.c
    ncftp/main.c
    ncftp/version.c
    ncftp/shell.c
    ncftp/util.c
    ncftp/readln.c
    ncftp/progress.c
    ncftp/bookmark.c
    ncftp/pref.c
    ncftp/preffw.c
    ncftp/trace.c
    ncftp/spool.c
    ncftp/log.c
    ncftp/getline.c)

add_executable(ncftp ${SOURCE} ncftp.rc)
set_module_type(ncftp win32cui)
target_compile_definitions(ncftp PRIVATE
    HAVE_UTIME_H
    HAVE_UNISTD_H
    _CRT_DECLARE_NONSTDC_NAMES=1
    strcasecmp=_stricmp
)
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_options(ncftp PRIVATE -Wno-unused-but-set-variable)
endif()
add_importlibs(ncftp advapi32 user32 ws2_32 msvcrt kernel32)
add_cd_file(TARGET ncftp DESTINATION reactos/system32 FOR all)
