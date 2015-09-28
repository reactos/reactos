
spec2def(mbedtls.dll mbedtls.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs/mbedtls
                    ${REACTOS_SOURCE_DIR}/include/reactos/zlib)

list(APPEND src
     aes.c
     aesni.c
     arc4.c
     asn1parse.c
     asn1write.c
     base64.c
     bignum.c
     blowfish.c
     camellia.c
     ccm.c
     certs.c
     cipher.c
     cipher_wrap.c
     ctr_drbg.c
     debug.c
     des.c
     dhm.c
     ecp.c
     ecp_curves.c
     ecdh.c
     ecdsa.c
     entropy.c
     entropy_poll.c
     error.c
     gcm.c
     havege.c
     hmac_drbg.c
     md.c
     md_wrap.c
     md2.c
     md4.c
     md5.c
     memory_buffer_alloc.c
     net.c
     oid.c
     padlock.c
     pbkdf2.c
     pem.c
     pkcs5.c
     pkcs11.c
     pkcs12.c
     pk.c
     pk_wrap.c
     pkparse.c
     pkwrite.c
     platform.c
     ripemd160.c
     rsa.c
     sha1.c
     sha256.c
     sha512.c
     ssl_cache.c
     ssl_ciphersuites.c
     ssl_cli.c
     ssl_srv.c
     ssl_tls.c
     threading.c
     timing.c
     version.c
     version_features.c
     x509.c
     x509_crt.c
     x509_crl.c
     x509_csr.c
     x509_create.c
     x509write_crt.c
     x509write_csr.c
     xtea.c)

add_library(mbedtls SHARED
    ${src}
    ${CMAKE_CURRENT_BINARY_DIR}/mbedtls.def)

set_module_type(mbedtls win32dll)
target_link_libraries(mbedtls zlib)
add_importlibs(mbedtls advapi32 ws2_32 msvcrt kernel32)

if(MSVC)
    add_importlibs(mbedtls ntdll)
endif()
add_cd_file(TARGET mbedtls DESTINATION reactos/system32 FOR all)

if(NOT MSVC)
    add_target_compile_flags(mbedtls "-Wno-pointer-sign -Wno-unused-function")
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_target_compile_flags(mbedtls "-Wno-unused-but-set-variable")
    endif()
endif()
