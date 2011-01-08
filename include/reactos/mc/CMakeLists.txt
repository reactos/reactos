
list(APPEND SOURCE
    bugcodes.mc
    errcodes.mc
    neteventmsg.mc
    ntstatus.mc
    pciclass.mc)

add_message_headers(${SOURCE})
