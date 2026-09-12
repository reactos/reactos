
remove_definitions(-D_WIN32_WINNT=0x502 -DWINVER=0x502)
add_definitions(-D_WIN32_WINNT=0x600 -DWINVER=0x600 -D__WINESRC__ -DSYMCRYPT_PLATFORM_WINE)

include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/symcrypt/inc)
include_directories(${REACTOS_SOURCE_DIR}/sdk/lib/3rdparty/symcrypt/lib)

list(APPEND SOURCE
	lib/3des.c
	lib/AesTables.c
	lib/DesTables.c
	lib/FatalIntercept.c
	lib/IEEE802_11SaeCustom.c
	lib/ScsTable.c
	lib/a_dispatch.c
	lib/aes-asm.c
	lib/aes-c.c
	lib/aes-default-bc.c
	lib/aes-default.c
	lib/aes-key.c
	lib/aes-neon.c
	lib/aes-xmm.c
	lib/aes-ymm.c
	lib/aesCtrDrbg.c
	lib/aescmac.c
	lib/aeskw.c
	lib/blockciphermodes.c
	lib/ccm.c
	lib/chacha20.c
	lib/chacha20_poly1305.c
	lib/cpuid.c
	lib/cpuid_um.c
	lib/crt.c
	lib/desx.c
	lib/dh.c
	lib/dl_internal_groups.c
	lib/dlgroup.c
	lib/dlkey.c
	lib/dsa.c
	lib/ec_dh.c
	lib/ec_dispatch.c
	lib/ec_dsa.c
	lib/ec_internal_curve_params.c
	lib/ec_internal_curves.c
	lib/ec_montgomery.c
	lib/ec_mul.c
	lib/ec_short_weierstrass.c
	lib/ec_twisted_edwards.c
	lib/eckey.c
	lib/ecpoint.c
	lib/ecurve.c
	lib/env_windowsUserModeWin8_1.c
	lib/equal.c
	lib/fdef369_mod.c
	lib/fdef_general.c
	lib/fdef_int.c
	lib/fdef_mod.c
	lib/gcm.c
	lib/gen_int.c
	lib/ghash.c
	lib/hash.c
	lib/hkdf.c
	lib/hmac.c
	lib/hmacmd5.c
	lib/hmacsha1.c
	lib/hmacsha224.c
	lib/hmacsha256.c
	lib/hmacsha384.c
	lib/hmacsha3_224.c
	lib/hmacsha3_256.c
	lib/hmacsha3_384.c
	lib/hmacsha3_512.c
	lib/hmacsha512.c
	lib/hmacsha512_224.c
	lib/hmacsha512_256.c
	lib/kmac.c
	lib/libmain.c
	lib/lms.c
	lib/marvin32.c
	lib/md2.c
	lib/md4.c
	lib/md5.c
	lib/mldsa.c
	lib/mldsa_primitives.c
	lib/mlkem.c
	lib/mlkem_primitives.c
	lib/modexp.c
	lib/paddingPkcs7.c
	lib/pbkdf2.c
	lib/pbkdf2_hmacsha1.c
	lib/pbkdf2_hmacsha256.c
	lib/poly1305.c
	lib/primes.c
	lib/rc2.c
	lib/rc4.c
	lib/rdrand.c
	lib/rdseed.c
	lib/recoding.c
	lib/rsa_enc.c
	lib/rsa_padding.c
	lib/rsakey.c
	lib/scsTools.c
	lib/selftest.c
	lib/session.c
	lib/sha1.c
	lib/sha256-xmm.c
	lib/sha256.c
	lib/sha3.c
	lib/sha3_224.c
	lib/sha3_256.c
	lib/sha3_384.c
	lib/sha3_512.c
	lib/sha512.c
	lib/shake.c
	lib/sp800_108.c
	lib/sp800_108_hmacsha1.c
	lib/sp800_108_hmacsha256.c
	lib/sp800_108_hmacsha512.c
	lib/srtp_kdf.c
	lib/ssh_kdf.c
	lib/ssh_kdf_sha256.c
	lib/ssh_kdf_sha512.c
	lib/sskdf.c
	lib/tlsCbcVerify.c
	lib/tlsprf.c
	lib/xmss.c
	lib/xtsaes.c)

if(ARCH STREQUAL "i386")
    list(APPEND ASM_SOURCE
        lib/i386/aesasm.S
	    lib/i386/fdef_asm.S
	    lib/i386/wipe.S
    )
elseif(ARCH STREQUAL "amd64")
    list(APPEND ASM_SOURCE
        lib/amd64/aesasm.S
        lib/amd64/fdef369_asm.S
        lib/amd64/fdef_asm.S
        lib/amd64/fdef_mulx.S
        lib/amd64/sha256xmm_asm.S
        lib/amd64/wipe.S
    )
elseif(ARCH STREQUAL "arm")
    list(APPEND ASM_SOURCE
        lib/arm/aesasm.S
        lib/arm/fdef_asm.S
        lib/arm/wipe.S
    )
elseif(ARCH STREQUAL "arm64")
    list(APPEND ASM_SOURCE
	    lib/arm64/fdef369_asm.S
	    lib/arm64/fdef_asm.S
	    lib/arm64/wipe.S
    )
elseif(ARCH STREQUAL "arm64ec")
    list(APPEND ASM_SOURCE
	    lib/arm64ec/fdef369_asm.S
	    lib/arm64ec/fdef_asm.S
	    lib/arm64ec/wipe.S
    )
endif()

#add_asm_files(symcrypt_asm ${ASM_SOURCE}) # Disabled for now, only works on GCC
add_library(symcrypt ${SOURCE})

if(MSVC)
	# warning C4068: unknown pragma 'GCC'
    target_compile_options(symcrypt PRIVATE /wd4068)
else()
    target_compile_options(symcrypt PRIVATE -Wno-pointer-sign -Wno-unused-function -Wno-missing-braces -Wno-unused-but-set-variable -Wno-comment -Wno-parentheses)
endif()

add_dependencies(symcrypt psdk)
