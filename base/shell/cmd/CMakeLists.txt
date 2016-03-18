
add_definitions(
    -DANONYMOUSUNIONS
    -D_DEBUG_MEM)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/wine)

list(APPEND SOURCE
    alias.c
    assoc.c
    attrib.c
    batch.c
    beep.c
    call.c
    chcp.c
    choice.c
    cls.c
    cmd.c
    cmddbg.c
    cmdinput.c
    cmdtable.c
    color.c
    console.c
    copy.c
    date.c
    del.c
    delay.c
    dir.c
    dirstack.c
    echo.c
    error.c
    filecomp.c
    for.c
    free.c
    goto.c
    history.c
    if.c
    internal.c
    label.c
    locale.c
    memory.c
    misc.c
    mklink.c
    move.c
    msgbox.c
    parser.c
    path.c
    pause.c
    prompt.c
    redir.c
    ren.c
    replace.c
    screen.c
    set.c
    setlocal.c
    shift.c
    start.c
    strtoclr.c
    time.c
    timer.c
    title.c
    type.c
    ver.c
    verify.c
    vol.c
    where.c
    window.c
    precomp.h)

add_rc_deps(cmd.rc ${CMAKE_CURRENT_SOURCE_DIR}/res/terminal.ico)
add_executable(cmd ${SOURCE} cmd.rc)
target_link_libraries(cmd wine)
set_module_type(cmd win32cui UNICODE)
add_importlibs(cmd advapi32 user32 msvcrt kernel32 ntdll)
add_pch(cmd precomp.h SOURCE)
add_cd_file(TARGET cmd DESTINATION reactos/system32 FOR all)
