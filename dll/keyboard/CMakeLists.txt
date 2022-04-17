
list(APPEND _keyboard_layouts
    kbda1
    kbda2
    kbda3
    kbdal
    kbdarme
    kbdarmw
    kbdaze
    kbdazel
    kbdbe
    kbdbga
    kbdbgm
    kbdbgt
    kbdblr
    kbdbr
    kbdbur
    kbdcan
    kbdcr
    kbdcz
    kbdcz1
    kbdda
    kbddv
    kbdeo
    kbdes
    kbdest
    kbdfc
    kbdfi
    kbdfr
    kbdgeo
    kbdgerg
    kbdgneo
    kbdgr
    kbdgrist
    kbdhe
    kbdheb
    kbdhu
    kbdic
    kbdinasa
    kbdinben
    kbdindev
    kbdinguj
    kbdinmal
    kbdir
    kbdit
    kbdja
    kbdkaz
    kbdko
    kbdla
    kbdlt1
    kbdlv
    kbdmac
    kbdne
    kbdno
    kbdpl
    kbdpl1
    kbdpo
    kbdro
    kbdrost
    kbdru
    kbdru1
    kbdsf
    kbdsg
    kbdsk
    kbdsk1
    kbdsw
    kbdtat
    kbdth0
    kbdth1
    kbdth2
    kbdth3
    kbdtuf
    kbdtuq
    kbduk
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

    target_include_directories(${_keyboard_layout} PRIVATE ${REACTOS_SOURCE_DIR}/ntoskrnl/include)
    set_entrypoint(${_keyboard_layout} 0)
    set_image_base(${_keyboard_layout} 0x5FFF0000)
    set_subsystem(${_keyboard_layout} native)

    if(MSVC)
        # Merge the .text and .rdata section into the .data section
        target_link_options(${_keyboard_layout} PRIVATE "/ignore:4254;/SECTION:.data,ER;/MERGE:.text=.data;/MERGE:.rdata=.data;/MERGE:.bss=.data;/MERGE:.edata=.data")
    else()
        # Use a custom linker script
        target_link_options(${_keyboard_layout} PRIVATE "-Wl,-T,${CMAKE_CURRENT_SOURCE_DIR}/kbdlayout.lds")

        # Avoid "universal character names are only valid in C++ and C99" error.
        set_property(TARGET ${_keyboard_layout} PROPERTY C_STANDARD 99)
    endif()

    # dynamic analysis switches
    if(STACK_PROTECTOR)
        target_compile_options(${_keyboard_layout} PRIVATE -fno-stack-protector)
    endif()

    if(RUNTIME_CHECKS)
        target_link_libraries(${_keyboard_layout} runtmchk)
    endif()

    add_dependencies(${_keyboard_layout} psdk)
    add_cd_file(TARGET ${_keyboard_layout} DESTINATION reactos/system32 FOR all)
endforeach()
