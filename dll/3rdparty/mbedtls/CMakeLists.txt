
spec2def(mbedtls.dll mbedtls.spec ADD_IMPORTLIB)

include_directories(${REACTOS_SOURCE_DIR}/include/reactos/libs
                    ${REACTOS_SOURCE_DIR}/include/reactos/zlib)

list(APPEND src
     _reactos_strsafe_wrapper.c
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
     ecdh.c
     ecdsa.c
     ecp.c
     ecp_curves.c
     entropy.c
     entropy_poll.c
     error.c
     gcm.c
     havege.c
     hmac_drbg.c
     md2.c
     md4.c
     md5.c
     md.c
     md_wrap.c
     memory_buffer_alloc.c
     net.c
     oid.c
     padlock.c
     pem.c
     pk.c
     pkcs11.c
     pkcs12.c
     pkcs5.c
     pkparse.c
     pk_wrap.c
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
     ssl_cookie.c
     ssl_srv.c
     ssl_ticket.c
     ssl_tls.c
     threading.c
     timing.c
     version.c
     version_features.c
     x509.c
     x509_create.c
     x509_crl.c
     x509_crt.c
     x509_csr.c
     x509write_crt.c
     x509write_csr.c
     xtea.c)

add_library(mbedtls SHARED
    ${src}
    mbedtls.rc
    ${CMAKE_CURRENT_BINARY_DIR}/mbedtls.def)

set_module_type(mbedtls win32dll)
target_link_libraries(mbedtls zlib)
add_importlibs(mbedtls advapi32 ws2_32 msvcrt kernel32)

# to use `_vsnprintf_s` looks like we have to define MINGW_HAS_SECURE_API
# and explicitly export it from msvcrt for it to work... what on earth?!
add_definitions(-DMINGW_HAS_SECURE_API -DCRTDLL)

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
