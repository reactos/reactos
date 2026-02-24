
list(APPEND SOURCE
    aes.c
    aes-ctr.c
    des.c
    md4.c
    md5.c
    mvAesAlg.c
    rc4.c
    sha1.c
    sha2.c
    util.c)

add_library(cryptlib ${SOURCE})
add_dependencies(cryptlib xdk)
target_include_directories(cryptlib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
