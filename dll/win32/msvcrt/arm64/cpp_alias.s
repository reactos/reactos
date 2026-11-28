/*
 * PROJECT:     ReactOS msvcrt.dll
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     ARM64 C++ MSVC aliases for cpp.c
 * COPYRIGHT:   Copyright 2014-2024 Wine team
 *              Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <kxarm64.h>

// See msvcrt/cpp.c, the 64 bit names are the same as on x64

    IMPORT |??3@YAXPEAX@Z|, WEAK |operator_delete|
    IMPORT |??_U@YAPEAX_K@Z|, WEAK |operator_new|
    IMPORT |??_U@YAPEAX_KHPEBDH@Z|, WEAK |operator_new_dbg|
    IMPORT |??_V@YAXPEAX@Z|, WEAK |operator_delete|
    IMPORT |??2@YAPEAX_K@Z|, WEAK |operator_new|
    IMPORT |??2@YAPEAX_KHPEBDH@Z|, WEAK |operator_new_dbg|
    IMPORT |?_query_new_handler@@YAP6AHI@ZXZ|, WEAK |_query_new_handler|
    IMPORT |?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z|, WEAK |_set_new_handler|
    IMPORT |?set_new_handler@@YAP6AXXZP6AXXZ@Z|, WEAK |set_new_handler|
    IMPORT |?_query_new_mode@@YAHXZ|, WEAK |_query_new_mode|
    IMPORT |?_set_new_mode@@YAHH@Z|, WEAK |_set_new_mode|
    IMPORT |?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z|, WEAK |_set_se_translator|
    IMPORT |?set_terminate@@YAP6AXXZP6AXXZ@Z|, WEAK |set_terminate|
    IMPORT |?set_unexpected@@YAP6AXXZP6AXXZ@Z|, WEAK |set_unexpected|
    IMPORT |?terminate@@YAXXZ|, WEAK |terminate|
    IMPORT |?unexpected@@YAXXZ|, WEAK |unexpected|
    IMPORT |?what@exception@@UEBAPEBDXZ|, WEAK |exception_what|
    IMPORT |??0exception@@QEAA@AEBQEBDH@Z|, WEAK |exception_ctor_noalloc|
    IMPORT |??0exception@@QEAA@AEBV0@@Z|, WEAK |exception_copy_ctor|
    IMPORT |??0exception@@QEAA@XZ|, WEAK |exception_default_ctor|
    IMPORT |??1exception@@UEAA@XZ|, WEAK |exception_dtor|
    IMPORT |??4exception@@QEAAAEAV0@AEBV0@@Z|, WEAK |exception_opequals|
    IMPORT |??1type_info@@UEAA@XZ|, WEAK |type_info_dtor|
    IMPORT |??0__non_rtti_object@@QEAA@AEBV0@@Z|, WEAK |__non_rtti_object_copy_ctor|
    IMPORT |??0__non_rtti_object@@QEAA@PEBD@Z|, WEAK |__non_rtti_object_ctor|
    IMPORT |??0bad_cast@@AAE@PBQBD@Z|, WEAK |bad_cast_ctor|
    IMPORT |??0bad_cast@@AEAA@PEBQEBD@Z|, WEAK |bad_cast_ctor|
    IMPORT |??0bad_cast@@QAE@ABQBD@Z|, WEAK |bad_cast_ctor|
    IMPORT |??0bad_cast@@QEAA@AEBQEBD@Z|, WEAK |bad_cast_ctor|
    IMPORT |??0bad_cast@@QEAA@AEBV0@@Z|, WEAK |bad_cast_copy_ctor|
    IMPORT |??0bad_cast@@QEAA@PEBD@Z|, WEAK |bad_cast_ctor_charptr|
    IMPORT |??0bad_typeid@@QEAA@AEBV0@@Z|, WEAK |bad_typeid_copy_ctor|
    IMPORT |??0bad_typeid@@QEAA@PEBD@Z|, WEAK |bad_typeid_ctor|
    IMPORT |??0exception@@QEAA@AEBQEBD@Z|, WEAK |exception_ctor|
    IMPORT |??1__non_rtti_object@@UEAA@XZ|, WEAK |__non_rtti_object_dtor|
    IMPORT |??1bad_cast@@UEAA@XZ|, WEAK |bad_cast_dtor|
    IMPORT |??1bad_typeid@@UEAA@XZ|, WEAK |bad_typeid_dtor|
    IMPORT |??4bad_cast@@QEAAAEAV0@AEBV0@@Z|, WEAK |bad_cast_opequals|
    IMPORT |??4bad_typeid@@QEAAAEAV0@AEBV0@@Z|, WEAK |bad_typeid_opequals|
    IMPORT |??8type_info@@QEBAHAEBV0@@Z|, WEAK |type_info_opequals_equals|
    IMPORT |??9type_info@@QEBAHAEBV0@@Z|, WEAK |type_info_opnot_equals|
    IMPORT |??_Fbad_cast@@QEAAXXZ|, WEAK |bad_cast_default_ctor|
    IMPORT |??_Fbad_typeid@@QEAAXXZ|, WEAK |bad_typeid_default_ctor|
    IMPORT |?_query_new_handler@@YAP6AH_K@ZXZ|, WEAK |_query_new_handler|
    IMPORT |?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z|, WEAK |_set_new_handler|
    IMPORT |?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z|, WEAK |_set_se_translator|
    IMPORT |?before@type_info@@QEBAHAEBV1@@Z|, WEAK |type_info_before|
    IMPORT |?name@type_info@@QEBAPEBDXZ|, WEAK |type_info_name|
    IMPORT |?raw_name@type_info@@QEBAPEBDXZ|, WEAK |type_info_raw_name|
    IMPORT |??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z|, WEAK |__non_rtti_object_opequals|

    END
