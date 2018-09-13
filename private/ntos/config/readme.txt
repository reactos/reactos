11-Nov-92

Conventions:

    BUGBUG - usual meaning

    PERFNOTE    - place where we should write some code to be more
                  time or space efficient, but which functions correctly
                  as is today.

    WARNNOTE    - some code that "works", but may someday break.
                  (a bugbug that we aren't going to fix)

                  OR - code that works reliably, but has behavior
                        that people may not like.



Build Products:

    config.lib  - linked into ntoskrnl.exe

    bconfig.lib - linked into boot loader (ntldr, osloader)

    uconfig.lib - linked into tools in sdktools\regini

    sconfig.lib - linked into setup
