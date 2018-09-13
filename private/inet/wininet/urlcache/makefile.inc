..\inc\urlcache.h: urlcache.w
    wcshdr < urlcache.w > $@

!IF "$(PROCESSOR_ARCHITECTURE)" == "x86"

urlcache.sym: urlcache.map
    ..\win32s\tools\mapsympe -o urlcache.sym urlcache.map
    binplace urlcache.map
    binplace urlcache.sym

!ENDIF
