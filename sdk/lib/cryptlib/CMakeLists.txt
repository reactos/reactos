
list(APPEND SOURCE
    aes.c
    des.c
    md4.c
    md5.c
    mvAesAlg.c
    rc4.c
    sha1.c
    util.c)

add_library(cryptlib ${SOURCE})
add_dependencies(cryptlib xdk)
