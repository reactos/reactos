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

    IMPORT MANGLED(??3@YAXPEAX@Z), WEAK MANGLED(operator_delete)
    IMPORT MANGLED(??_U@YAPEAX_K@Z), WEAK MANGLED(operator_new)
    IMPORT MANGLED(??_U@YAPEAX_KHPEBDH@Z), WEAK MANGLED(operator_new_dbg)
    IMPORT MANGLED(??_V@YAXPEAX@Z), WEAK MANGLED(operator_delete)
    IMPORT MANGLED(??2@YAPEAX_K@Z), WEAK MANGLED(operator_new)
    IMPORT MANGLED(??2@YAPEAX_KHPEBDH@Z), WEAK MANGLED(operator_new_dbg)
    IMPORT MANGLED(?_query_new_handler@@YAP6AHI@ZXZ), WEAK MANGLED(_query_new_handler)
    IMPORT MANGLED(?_set_new_handler@@YAP6AHI@ZP6AHI@Z@Z), WEAK MANGLED(_set_new_handler)
    IMPORT MANGLED(?set_new_handler@@YAP6AXXZP6AXXZ@Z), WEAK MANGLED(set_new_handler)
    IMPORT MANGLED(?_query_new_mode@@YAHXZ), WEAK MANGLED(_query_new_mode)
    IMPORT MANGLED(?_set_new_mode@@YAHH@Z), WEAK MANGLED(_set_new_mode)
    IMPORT MANGLED(?_set_se_translator@@YAP6AXIPAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z), WEAK MANGLED(_set_se_translator)
    IMPORT MANGLED(?set_terminate@@YAP6AXXZP6AXXZ@Z), WEAK MANGLED(set_terminate)
    IMPORT MANGLED(?set_unexpected@@YAP6AXXZP6AXXZ@Z), WEAK MANGLED(set_unexpected)
    IMPORT MANGLED(?terminate@@YAXXZ), WEAK MANGLED(terminate)
    IMPORT MANGLED(?unexpected@@YAXXZ), WEAK MANGLED(unexpected)
    IMPORT MANGLED(?what@exception@@UEBAPEBDXZ), WEAK MANGLED(exception_what)
    IMPORT MANGLED(??0exception@@QEAA@AEBQEBDH@Z), WEAK MANGLED(exception_ctor_noalloc)
    IMPORT MANGLED(??0exception@@QEAA@AEBV0@@Z), WEAK MANGLED(exception_copy_ctor)
    IMPORT MANGLED(??0exception@@QEAA@XZ), WEAK MANGLED(exception_default_ctor)
    IMPORT MANGLED(??1exception@@UEAA@XZ), WEAK MANGLED(exception_dtor)
    IMPORT MANGLED(??4exception@@QEAAAEAV0@AEBV0@@Z), WEAK MANGLED(exception_opequals)
    IMPORT MANGLED(??1type_info@@UEAA@XZ), WEAK MANGLED(type_info_dtor)
    IMPORT MANGLED(??0__non_rtti_object@@QEAA@AEBV0@@Z), WEAK MANGLED(__non_rtti_object_copy_ctor)
    IMPORT MANGLED(??0__non_rtti_object@@QEAA@PEBD@Z), WEAK MANGLED(__non_rtti_object_ctor)
    IMPORT MANGLED(??0bad_cast@@AAE@PBQBD@Z), WEAK MANGLED(bad_cast_ctor)
    IMPORT MANGLED(??0bad_cast@@AEAA@PEBQEBD@Z), WEAK MANGLED(bad_cast_ctor)
    IMPORT MANGLED(??0bad_cast@@QAE@ABQBD@Z), WEAK MANGLED(bad_cast_ctor)
    IMPORT MANGLED(??0bad_cast@@QEAA@AEBQEBD@Z), WEAK MANGLED(bad_cast_ctor)
    IMPORT MANGLED(??0bad_cast@@QEAA@AEBV0@@Z), WEAK MANGLED(bad_cast_copy_ctor)
    IMPORT MANGLED(??0bad_cast@@QEAA@PEBD@Z), WEAK MANGLED(bad_cast_ctor_charptr)
    IMPORT MANGLED(??0bad_typeid@@QEAA@AEBV0@@Z), WEAK MANGLED(bad_typeid_copy_ctor)
    IMPORT MANGLED(??0bad_typeid@@QEAA@PEBD@Z), WEAK MANGLED(bad_typeid_ctor)
    IMPORT MANGLED(??0exception@@QEAA@AEBQEBD@Z), WEAK MANGLED(exception_ctor)
    IMPORT MANGLED(??1__non_rtti_object@@UEAA@XZ), WEAK MANGLED(__non_rtti_object_dtor)
    IMPORT MANGLED(??1bad_cast@@UEAA@XZ), WEAK MANGLED(bad_cast_dtor)
    IMPORT MANGLED(??1bad_typeid@@UEAA@XZ), WEAK MANGLED(bad_typeid_dtor)
    IMPORT MANGLED(??4bad_cast@@QEAAAEAV0@AEBV0@@Z), WEAK MANGLED(bad_cast_opequals)
    IMPORT MANGLED(??4bad_typeid@@QEAAAEAV0@AEBV0@@Z), WEAK MANGLED(bad_typeid_opequals)
    IMPORT MANGLED(??8type_info@@QEBAHAEBV0@@Z), WEAK MANGLED(type_info_opequals_equals)
    IMPORT MANGLED(??9type_info@@QEBAHAEBV0@@Z), WEAK MANGLED(type_info_opnot_equals)
    IMPORT MANGLED(??_Fbad_cast@@QEAAXXZ), WEAK MANGLED(bad_cast_default_ctor)
    IMPORT MANGLED(??_Fbad_typeid@@QEAAXXZ), WEAK MANGLED(bad_typeid_default_ctor)
    IMPORT MANGLED(?_query_new_handler@@YAP6AH_K@ZXZ), WEAK MANGLED(_query_new_handler)
    IMPORT MANGLED(?_set_new_handler@@YAP6AH_K@ZP6AH0@Z@Z), WEAK MANGLED(_set_new_handler)
    IMPORT MANGLED(?_set_se_translator@@YAP6AXIPEAU_EXCEPTION_POINTERS@@@ZP6AXI0@Z@Z), WEAK MANGLED(_set_se_translator)
    IMPORT MANGLED(?before@type_info@@QEBAHAEBV1@@Z), WEAK MANGLED(type_info_before)
    IMPORT MANGLED(?name@type_info@@QEBAPEBDXZ), WEAK MANGLED(type_info_name)
    IMPORT MANGLED(?raw_name@type_info@@QEBAPEBDXZ), WEAK MANGLED(type_info_raw_name)
    IMPORT MANGLED(??4__non_rtti_object@@QEAAAEAV0@AEBV0@@Z), WEAK MANGLED(__non_rtti_object_opequals)

    END
