
list(APPEND SOURCE
    se/access.c
    se/accesschk.c
    se/acl.c
    se/audit.c
    se/client.c
    se/objtype.c
    se/priv.c
    se/sd.c
    se/semgr.c
    se/sid.c
    se/sqos.c
    se/srm.c
    se/subject.c
    se/token.c
    se/tokenadj.c
    se/tokencls.c
    se/tokenlif.c
    )

if(DBG)
    list(APPEND SOURCE se/debug.c)
endif()
