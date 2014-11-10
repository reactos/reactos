/*
 * Copyright 2011 Jacek Caban for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "mshtml_private.h"

/*
 * This object wraps any unrecognized interface overriding its IUnknown methods, allowing
 * us to return external interface from our QI implementation preserving COM rules.
 * This can't be done right and it seems to be broken by design.
 */
typedef struct {
    IUnknown IUnknown_iface;
    IUnknown *iface;
    IUnknown *ref_unk;
    LONG ref;
} iface_wrapper_t;

static inline iface_wrapper_t *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, iface_wrapper_t, IUnknown_iface);
}

static HRESULT WINAPI wrapper_QueryInterface(IUnknown *iface, REFIID riid, void **ppv)
{
    iface_wrapper_t *This = impl_from_IUnknown(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_mshtml_guid(riid), ppv);

    return IUnknown_QueryInterface(This->ref_unk, riid, ppv);
}

static HRESULT WINAPI wrapper_AddRef(IUnknown *iface)
{
    iface_wrapper_t *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static HRESULT WINAPI wrapper_Release(IUnknown *iface)
{
    iface_wrapper_t *This = impl_from_IUnknown(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->iface);
        IUnknown_Release(This->ref_unk);
        heap_free(This);
    }

    return ref;
}

#ifdef __i386__

#ifdef _MSC_VER
#define DEFINE_WRAPPER_FUNC(n, off, x) HRESULT wrapper_func_##n(IUnknown*);
#else
#define DEFINE_WRAPPER_FUNC(n, off, x)          \
    HRESULT wrapper_func_##n(IUnknown*);        \
    __ASM_GLOBAL_FUNC(wrapper_func_##n,         \
        "movl 4(%esp), %eax\n\t"                \
        "movl 4(%eax), %eax\n\t"                \
        "movl %eax, 4(%esp)\n\t"                \
        "movl 0(%eax), %eax\n\t"                \
        "jmp *" #off "(%eax)\n\t")
#endif

#elif defined(__x86_64__)

#define DEFINE_WRAPPER_FUNC(n, x, off)          \
    HRESULT WINAPI wrapper_func_##n(IUnknown*); \
    __ASM_GLOBAL_FUNC(wrapper_func_##n,         \
        "movq 8(%rcx), %rcx\n\t"                \
        "movq 0(%rcx), %rax\n\t"                \
        "jmp *" #off "(%rax)\n\t")

#else

#define DEFINE_WRAPPER_FUNC(n, x, off)                           \
    static HRESULT WINAPI wrapper_func_##n(IUnknown *iface) {    \
        ERR("Not implemented for this architecture\n");          \
        return E_NOTIMPL;                                        \
    }

#endif

/* DEFINE_WRAPPER_FUNC takes 3 arguments: index in vtbl, 32-bit offset in vtbl and 64-bit offset in vtbl */
DEFINE_WRAPPER_FUNC(3, 12, 24)
DEFINE_WRAPPER_FUNC(4, 16, 32)
DEFINE_WRAPPER_FUNC(5, 20, 40)
DEFINE_WRAPPER_FUNC(6, 24, 48)
DEFINE_WRAPPER_FUNC(7, 28, 56)
DEFINE_WRAPPER_FUNC(8, 32, 64)
DEFINE_WRAPPER_FUNC(9, 36, 72)
DEFINE_WRAPPER_FUNC(10, 40, 80)
DEFINE_WRAPPER_FUNC(11, 44, 88)
DEFINE_WRAPPER_FUNC(12, 48, 96)
DEFINE_WRAPPER_FUNC(13, 52, 104)
DEFINE_WRAPPER_FUNC(14, 56, 112)
DEFINE_WRAPPER_FUNC(15, 60, 120)
DEFINE_WRAPPER_FUNC(16, 64, 128)
DEFINE_WRAPPER_FUNC(17, 68, 136)
DEFINE_WRAPPER_FUNC(18, 72, 144)
DEFINE_WRAPPER_FUNC(19, 76, 152)
DEFINE_WRAPPER_FUNC(20, 80, 160)
DEFINE_WRAPPER_FUNC(21, 84, 168)
DEFINE_WRAPPER_FUNC(22, 88, 176)
DEFINE_WRAPPER_FUNC(23, 92, 184)
DEFINE_WRAPPER_FUNC(24, 96, 192)
DEFINE_WRAPPER_FUNC(25, 100, 200)
DEFINE_WRAPPER_FUNC(26, 104, 208)
DEFINE_WRAPPER_FUNC(27, 108, 216)
DEFINE_WRAPPER_FUNC(28, 112, 224)
DEFINE_WRAPPER_FUNC(29, 116, 232)
DEFINE_WRAPPER_FUNC(30, 120, 240)
DEFINE_WRAPPER_FUNC(31, 124, 248)
DEFINE_WRAPPER_FUNC(32, 128, 256)
DEFINE_WRAPPER_FUNC(33, 132, 264)
DEFINE_WRAPPER_FUNC(34, 136, 272)
DEFINE_WRAPPER_FUNC(35, 140, 280)
DEFINE_WRAPPER_FUNC(36, 144, 288)
DEFINE_WRAPPER_FUNC(37, 148, 296)
DEFINE_WRAPPER_FUNC(38, 152, 304)
DEFINE_WRAPPER_FUNC(39, 156, 312)
DEFINE_WRAPPER_FUNC(40, 160, 320)
DEFINE_WRAPPER_FUNC(41, 164, 328)
DEFINE_WRAPPER_FUNC(42, 168, 336)
DEFINE_WRAPPER_FUNC(43, 172, 344)
DEFINE_WRAPPER_FUNC(44, 176, 352)
DEFINE_WRAPPER_FUNC(45, 180, 360)
DEFINE_WRAPPER_FUNC(46, 184, 368)
DEFINE_WRAPPER_FUNC(47, 188, 376)
DEFINE_WRAPPER_FUNC(48, 192, 384)
DEFINE_WRAPPER_FUNC(49, 196, 392)
DEFINE_WRAPPER_FUNC(50, 200, 400)
DEFINE_WRAPPER_FUNC(51, 204, 408)
DEFINE_WRAPPER_FUNC(52, 208, 416)
DEFINE_WRAPPER_FUNC(53, 212, 424)
DEFINE_WRAPPER_FUNC(54, 216, 432)
DEFINE_WRAPPER_FUNC(55, 220, 440)
DEFINE_WRAPPER_FUNC(56, 224, 448)
DEFINE_WRAPPER_FUNC(57, 228, 456)
DEFINE_WRAPPER_FUNC(58, 232, 464)
DEFINE_WRAPPER_FUNC(59, 236, 472)
DEFINE_WRAPPER_FUNC(60, 240, 480)
DEFINE_WRAPPER_FUNC(61, 244, 488)
DEFINE_WRAPPER_FUNC(62, 248, 496)
DEFINE_WRAPPER_FUNC(63, 252, 504)
DEFINE_WRAPPER_FUNC(64, 256, 512)
DEFINE_WRAPPER_FUNC(65, 260, 520)
DEFINE_WRAPPER_FUNC(66, 264, 528)
DEFINE_WRAPPER_FUNC(67, 268, 536)
DEFINE_WRAPPER_FUNC(68, 272, 544)
DEFINE_WRAPPER_FUNC(69, 276, 552)
DEFINE_WRAPPER_FUNC(70, 280, 560)
DEFINE_WRAPPER_FUNC(71, 284, 568)
DEFINE_WRAPPER_FUNC(72, 288, 576)
DEFINE_WRAPPER_FUNC(73, 292, 584)
DEFINE_WRAPPER_FUNC(74, 296, 592)
DEFINE_WRAPPER_FUNC(75, 300, 600)
DEFINE_WRAPPER_FUNC(76, 304, 608)
DEFINE_WRAPPER_FUNC(77, 308, 616)
DEFINE_WRAPPER_FUNC(78, 312, 624)
DEFINE_WRAPPER_FUNC(79, 316, 632)
DEFINE_WRAPPER_FUNC(80, 320, 640)
DEFINE_WRAPPER_FUNC(81, 324, 648)
DEFINE_WRAPPER_FUNC(82, 328, 656)
DEFINE_WRAPPER_FUNC(83, 332, 664)
DEFINE_WRAPPER_FUNC(84, 336, 672)
DEFINE_WRAPPER_FUNC(85, 340, 680)
DEFINE_WRAPPER_FUNC(86, 344, 688)
DEFINE_WRAPPER_FUNC(87, 348, 696)
DEFINE_WRAPPER_FUNC(88, 352, 704)
DEFINE_WRAPPER_FUNC(89, 356, 712)
DEFINE_WRAPPER_FUNC(90, 360, 720)
DEFINE_WRAPPER_FUNC(91, 364, 728)
DEFINE_WRAPPER_FUNC(92, 368, 736)
DEFINE_WRAPPER_FUNC(93, 372, 744)
DEFINE_WRAPPER_FUNC(94, 376, 752)
DEFINE_WRAPPER_FUNC(95, 380, 760)
DEFINE_WRAPPER_FUNC(96, 384, 768)
DEFINE_WRAPPER_FUNC(97, 388, 776)
DEFINE_WRAPPER_FUNC(98, 392, 784)
DEFINE_WRAPPER_FUNC(99, 396, 792)

/* The size was found by testing when calls start crashing. It looks like MS wraps up to 100 functions. */
static const void *wrapper_vtbl[] = {
    wrapper_QueryInterface,
    wrapper_AddRef,
    wrapper_Release,
    wrapper_func_3,
    wrapper_func_4,
    wrapper_func_5,
    wrapper_func_6,
    wrapper_func_7,
    wrapper_func_8,
    wrapper_func_9,
    wrapper_func_10,
    wrapper_func_11,
    wrapper_func_12,
    wrapper_func_13,
    wrapper_func_14,
    wrapper_func_15,
    wrapper_func_16,
    wrapper_func_17,
    wrapper_func_18,
    wrapper_func_19,
    wrapper_func_20,
    wrapper_func_21,
    wrapper_func_22,
    wrapper_func_23,
    wrapper_func_24,
    wrapper_func_25,
    wrapper_func_26,
    wrapper_func_27,
    wrapper_func_28,
    wrapper_func_29,
    wrapper_func_30,
    wrapper_func_31,
    wrapper_func_32,
    wrapper_func_33,
    wrapper_func_34,
    wrapper_func_35,
    wrapper_func_36,
    wrapper_func_37,
    wrapper_func_38,
    wrapper_func_39,
    wrapper_func_40,
    wrapper_func_41,
    wrapper_func_42,
    wrapper_func_43,
    wrapper_func_44,
    wrapper_func_45,
    wrapper_func_46,
    wrapper_func_47,
    wrapper_func_48,
    wrapper_func_49,
    wrapper_func_50,
    wrapper_func_51,
    wrapper_func_52,
    wrapper_func_53,
    wrapper_func_54,
    wrapper_func_55,
    wrapper_func_56,
    wrapper_func_57,
    wrapper_func_58,
    wrapper_func_59,
    wrapper_func_60,
    wrapper_func_61,
    wrapper_func_62,
    wrapper_func_63,
    wrapper_func_64,
    wrapper_func_65,
    wrapper_func_66,
    wrapper_func_67,
    wrapper_func_68,
    wrapper_func_69,
    wrapper_func_70,
    wrapper_func_71,
    wrapper_func_72,
    wrapper_func_73,
    wrapper_func_74,
    wrapper_func_75,
    wrapper_func_76,
    wrapper_func_77,
    wrapper_func_78,
    wrapper_func_79,
    wrapper_func_80,
    wrapper_func_81,
    wrapper_func_82,
    wrapper_func_83,
    wrapper_func_84,
    wrapper_func_85,
    wrapper_func_86,
    wrapper_func_87,
    wrapper_func_88,
    wrapper_func_89,
    wrapper_func_90,
    wrapper_func_91,
    wrapper_func_92,
    wrapper_func_93,
    wrapper_func_94,
    wrapper_func_95,
    wrapper_func_96,
    wrapper_func_97,
    wrapper_func_98,
    wrapper_func_99
};

HRESULT wrap_iface(IUnknown *iface, IUnknown *ref_unk, IUnknown **ret)
{
    iface_wrapper_t *wrapper;

    wrapper = heap_alloc(sizeof(*wrapper));
    if(!wrapper)
        return E_OUTOFMEMORY;

    wrapper->IUnknown_iface.lpVtbl = (const IUnknownVtbl*)wrapper_vtbl;
    wrapper->ref = 1;

    IUnknown_AddRef(iface);
    wrapper->iface = iface;

    IUnknown_AddRef(ref_unk);
    wrapper->ref_unk = ref_unk;

    *ret = &wrapper->IUnknown_iface;
    return S_OK;
}
