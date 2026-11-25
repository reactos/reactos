
list(APPEND _keyboard_layouts
    kbd101
    kbd101a
    kbd101b
    kbd101c
    kbd103
    kbd106
    kbd106n
    kbda1
    kbda2
    kbda3
    kbdal
    kbdarme
    kbdarmw
    kbdax2
    kbdaze
    kbdazel
    kbdbe
    kbdbene
    kbdbga
    kbdbgt
    kbdbhc
    kbdblr
    kbdbr
    kbdbu
    kbdbur
    kbdca
    kbdcan
    kbdcmk
    kbdcr
    kbdcz
    kbdcz1
    kbdcz2
    kbdda
    kbddv
    kbdeo
    kbdes
    kbdest
    kbdfa
    kbdfar
    kbdfc
    kbdfi
    kbdfi1
    kbdfo
    kbdfr
    kbdgae
    kbdgeo
    kbdgerg
    kbdgkl
    kbdgneo
    kbdgr
    kbdgr1
    kbdgre1
    kbdgrist
    kbdhe
    kbdhe220
    kbdhe319
    kbdheb
    kbdhela2
    kbdhela3
    kbdhept
    kbdhu
    kbdibm02
    kbdic
    kbdinasa
    kbdinbe2
    kbdinben
    kbdindev
    kbdinguj
    kbdinmal
    kbdir
    kbdit
    kbdit142
    kbdiulat
    kbdjpn
    kbdkaz
    kbdkor
    kbdkyr
    kbdla
    kbdlk41a
    kbdlk41j
    kbdlt
    kbdlt1
    kbdlv
    kbdlv1
    kbdmac
    kbdmaori
    kbdmlt47
    kbdmlt48
    kbdne
    kbdnec
    kbdnec95
    kbdnecat
    kbdnecnt
    kbdnepr
    kbdno
    kbdno1
    kbdpash
    kbdpl
    kbdpl1
    kbdpo
    kbdro
    kbdrost
    kbdru
    kbdru1
    kbdsf
    kbdsg
    kbdsl
    kbdsl1
    kbdsmsfi
    kbdsmsno
    kbdsp
    kbdsw
    kbdtat
    kbdth0
    kbdth1
    kbdth2
    kbdth3
    kbdtuf
    kbdtuq
    kbduk
    kbdukx
    kbdur
    kbdurs
    kbdus
    kbdusa
    kbdusl
    kbdusr
    kbdusx
    kbduzb
    kbdvntc
    kbdycc
    kbdycl)

foreach(_keyboard_layout ${_keyboard_layouts})
    spec2def(${_keyboard_layout}.dll ${_keyboard_layout}/${_keyboard_layout}.spec)

    add_library(${_keyboard_layout} MODULE
        ${_keyboard_layout}/${_keyboard_layout}.c
        ${_keyboard_layout}/${_keyboard_layout}.rc
        ${CMAKE_CURRENT_BINARY_DIR}/${_keyboard_layout}.def)

    if(_keyboard_layout STREQUAL "kbdjpn" OR _keyboard_layout STREQUAL "kbdkor")
        # kbdjpn and kbdkor are special keyboard layouts
        target_sources(${_keyboard_layout} PRIVATE jpnkor.c)
        add_importlibs(${_keyboard_layout} ntdll)
    endif()

    set_entrypoint(${_keyboard_layout} 0)
    if(ARCH STREQUAL "arm64")
        # ARM64 images must be based above 4GB
        set_image_base(${_keyboard_layout} 0x180000000)
    else()
        set_image_base(${_keyboard_layout} 0x5FFF0000)
    endif()
    set_subsystem(${_keyboard_layout} native)

    if(MSVC)
        # Merge the .text and .rdata section into the .data section
        target_link_options(${_keyboard_layout} PRIVATE "/ignore:4254;/SECTION:.data,ER;/MERGE:.text=.data;/MERGE:.rdata=.data;/MERGE:.bss=.data;/MERGE:.edata=.data")
    else()
        # Use a custom linker script
        add_linker_script(${_keyboard_layout} kbdlayout.lds)
    endif()

    # dynamic analysis switches
    if(STACK_PROTECTOR)
        target_compile_options(${_keyboard_layout} PRIVATE -fno-stack-protector)
    endif()

    if(RUNTIME_CHECKS)
        target_link_libraries(${_keyboard_layout} runtmchk)
    endif()

    add_dependencies(${_keyboard_layout} xdk)
    add_cd_file(TARGET ${_keyboard_layout} DESTINATION reactos/system32 FOR all)
endforeach()

target_compile_definitions(kbdjpn PRIVATE KBD_JAPANESE)
target_compile_definitions(kbdkor PRIVATE KBD_KOREAN)
