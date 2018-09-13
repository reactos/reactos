//+------------------------------------------------------------------------
//
//  File:       toff16.cxx
//
//  Contents:   Tear off interfaces for win16.
//
//  History:    25-Jan-97 stevepro
//
//-------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_TEAROFF_HXX_
#define X_TEAROFF_HXX_
#include "tearoff.hxx"
#endif

#if 0
struct TEAROFF_THUNK
{
    void *      papfnVtblThis;     // Thunk's vtable
    ULONG       ulRef;             // Reference count for this thunk.
    DWORD       unused;            // BUGBUG delete when can be compiled and tested on ALPHA
    void *      pvObject1;         // Delegate other methods to this object using...
    void *      apfnVtblObject1;   // ...this array of pointers to member functions.
    void *      pvObject2;         // Delegate methods to this object using...
    void *      apfnVtblObject2;   // ...this array of pointers to member functions...
    DWORD       dwMask;            // ...the index of the method is set in the mask.
};
#endif

extern void (*s_apfnPlainTearoffVtable[])();

#ifdef WIN16

EXTERN_C void __declspec(naked) TearoffThunk3()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */

        test word ptr es:[bx+28], 0x08 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+12]   /* 4 byte ptrs, so 3*4 */
	};
}

EXTERN_C void __declspec(naked) TearoffThunk4()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x10 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+16]   /* 4 byte ptrs, so 4*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk5()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x20 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+20]   /* 4 byte ptrs, so 5*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk6()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x40 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+24]   /* 4 byte ptrs, so 6*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk7()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x80 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+28]   /* 4 byte ptrs, so 7*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk8()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x0100 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+32]   /* 4 byte ptrs, so 8*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk9()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x0200 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+36]   /* 4 byte ptrs, so 9*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk10()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x0400 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+40]   /* 4 byte ptrs, so 10*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk11()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x0800 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+44]   /* 4 byte ptrs, so 11*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk12()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x1000 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+48]   /* 4 byte ptrs, so 12*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk13()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x2000 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+52]   /* 4 byte ptrs, so 13*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk14()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x4000 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+56]   /* 4 byte ptrs, so 14*4 */
	};
}
EXTERN_C void __declspec(naked) TearoffThunk15()\
{   __asm   {
		push bp
		mov  bp, sp
		les  bx, ss:[bp+6] /* this = thisArg */
        test word ptr es:[bx+28], 0x8000 /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */
		jz  Object1
		add  bx, 8                  /* increment to pvObject2 */
    Object1:
		add  bx, 12                 /* offset pvObject1 */

        mov  ax, word ptr es:[bx] /* get pvObject */
		mov  dx, word ptr es:[bx+2] 

        les  bx, word ptr es:[bx+4]  /* get apfnVtblObject */

        mov  ss:[bp+6], ax             /* thisArg = pvObject, adjusted for the final jmp */
        mov  ss:[bp+8], dx
		pop  bp

        jmp dword ptr es:[bx+60]   /* 4 byte ptrs, so 15*4 */
	};
}

#if 0
#define THUNK_IMPLEMENT_COMPARE(n)\
EXTERN_C void __declspec(naked) TearoffThunk##n()\
{   __asm   {\
		push bp\
		mov  bp, sp\
		les  bx, ss:[bp+6] /* this = thisArg */\
		mov  ax, 1\
		shl	 ax, n\
		test word ptr [bx+28], ax /* pvObject = (this->_dwMask & 1<<n) ? pvObject2 : pvObject1 */\
		jz  Object1\
		add  bx, 8                  /* increment to pvObject2 */\
    Object1:\
		add  bx, 12                 /* offset pvObject1 */\
		mov  cx, word ptr es:[bx-2] /* get pvObject */\
		mov  bx, word ptr es:[bx] \
		mov  [bp+4], cx             /* thisArg = pvObject */\
		mov  [bp+6], bx\
		mov  cx, word ptr es:[bx+2] /* get apfnVtblObject */\
		mov  bx, word ptr es:[bx+4]\
		pop  bp\
		jmp  dword ptr es:[bx+(4*n)] /* pfn = apfnVtblObject[n] */\
	};\
}
#endif


EXTERN_C void __declspec(naked) TearoffThunk22()
{                                           
    __asm push  bp 
    __asm mov	bp, sp 
    __asm les   bx, ss:[bp+6]           /* this = thisArg */ 

    __asm mov   ax, word ptr es:[bx+12] /* get pvObject */ 
    __asm mov   dx, word ptr es:[bx+14] 

    __asm les   bx, word ptr es:[bx+16] /* get apfnVtblObject */ 

	__asm mov	cx, 22
	__asm shl	cx, 2
	__asm add	bx, cx

    __asm mov   ss:[bp+6], ax              /* thisArg = pvObject, adjusted for the final jmp */ 
    __asm mov   ss:[bp+8], dx 
    __asm pop 	bp 
    
    __asm jmp dword ptr es:[bx]
}


EXTERN_C void __declspec(naked) TearoffThunk29()
{                                           
    __asm push  bp 
    __asm mov	bp, sp 
    __asm les   bx, ss:[bp+6]           /* this = thisArg */ 

    __asm mov   ax, word ptr es:[bx+12] /* get pvObject */ 
    __asm mov   dx, word ptr es:[bx+14] 

    __asm les   bx, word ptr es:[bx+16] /* get apfnVtblObject */ 

	__asm mov	cx, 29
	__asm shl	cx, 2
	__asm add	bx, cx

    __asm mov   ss:[bp+6], ax              /* thisArg = pvObject, adjusted for the final jmp */ 
    __asm mov   ss:[bp+8], dx 
    __asm pop 	bp 
    
    __asm jmp dword ptr es:[bx]
}

#define THUNK_IMPLEMENT_SIMPLE(n)\
EXTERN_C void __declspec(naked) TearoffThunk##n()\
{                                           \
    __asm push  bp \
    __asm mov	bp, sp \
    __asm les   bx, ss:[bp+6]           /* this = thisArg */ \
\
    __asm mov   ax, word ptr es:[bx+12] /* get pvObject */ \
    __asm mov   dx, word ptr es:[bx+14] \
\
    __asm les   bx, word ptr es:[bx+16] /* get apfnVtblObject */ \
\
	__asm mov	cx, n\
	__asm shl	cx, 2\
	__asm add	bx, cx\
\
    __asm mov   ss:[bp+6], ax              /* thisArg = pvObject, adjusted for the final jmp */ \
    __asm mov   ss:[bp+8], dx \
    __asm pop 	bp \
\
    __asm jmp dword ptr es:[bx]\
}
// Single step a few times for the function you are calling.

//THUNK_ARRAY_3_TO_15(IMPLEMENT_COMPARE)
//THUNK_ARRAY_16_AND_UP(IMPLEMENT_SIMPLE)

THUNK_IMPLEMENT_SIMPLE(16)  THUNK_IMPLEMENT_SIMPLE(17)  THUNK_IMPLEMENT_SIMPLE(18)  THUNK_IMPLEMENT_SIMPLE(19)  THUNK_IMPLEMENT_SIMPLE(20)  THUNK_IMPLEMENT_SIMPLE(21)  
/*THUNK_IMPLEMENT_SIMPLE(22)*/  THUNK_IMPLEMENT_SIMPLE(23)  THUNK_IMPLEMENT_SIMPLE(24)  \
THUNK_IMPLEMENT_SIMPLE(25)  THUNK_IMPLEMENT_SIMPLE(26)  THUNK_IMPLEMENT_SIMPLE(27)  THUNK_IMPLEMENT_SIMPLE(28)  /* THUNK_IMPLEMENT_SIMPLE(29) */  THUNK_IMPLEMENT_SIMPLE(30)  
THUNK_IMPLEMENT_SIMPLE(31)  THUNK_IMPLEMENT_SIMPLE(32)  THUNK_IMPLEMENT_SIMPLE(33)  THUNK_IMPLEMENT_SIMPLE(34)  THUNK_IMPLEMENT_SIMPLE(35)  \
THUNK_IMPLEMENT_SIMPLE(36)  THUNK_IMPLEMENT_SIMPLE(37)  THUNK_IMPLEMENT_SIMPLE(38)  THUNK_IMPLEMENT_SIMPLE(39)  THUNK_IMPLEMENT_SIMPLE(40)  
THUNK_IMPLEMENT_SIMPLE(41)  THUNK_IMPLEMENT_SIMPLE(42)  THUNK_IMPLEMENT_SIMPLE(43)  THUNK_IMPLEMENT_SIMPLE(44)  THUNK_IMPLEMENT_SIMPLE(45)  THUNK_IMPLEMENT_SIMPLE(46)  \
THUNK_IMPLEMENT_SIMPLE(47)  THUNK_IMPLEMENT_SIMPLE(48)  THUNK_IMPLEMENT_SIMPLE(49)  THUNK_IMPLEMENT_SIMPLE(50)  THUNK_IMPLEMENT_SIMPLE(51)  THUNK_IMPLEMENT_SIMPLE(52)  
THUNK_IMPLEMENT_SIMPLE(53)  THUNK_IMPLEMENT_SIMPLE(54)  THUNK_IMPLEMENT_SIMPLE(55)  THUNK_IMPLEMENT_SIMPLE(56)  THUNK_IMPLEMENT_SIMPLE(57)  \
THUNK_IMPLEMENT_SIMPLE(58)  THUNK_IMPLEMENT_SIMPLE(59)  THUNK_IMPLEMENT_SIMPLE(60)  THUNK_IMPLEMENT_SIMPLE(61)  THUNK_IMPLEMENT_SIMPLE(62)  
THUNK_IMPLEMENT_SIMPLE(63)  THUNK_IMPLEMENT_SIMPLE(64)  THUNK_IMPLEMENT_SIMPLE(65)  THUNK_IMPLEMENT_SIMPLE(66)  THUNK_IMPLEMENT_SIMPLE(67)  THUNK_IMPLEMENT_SIMPLE(68)  \
THUNK_IMPLEMENT_SIMPLE(69)  THUNK_IMPLEMENT_SIMPLE(70)  THUNK_IMPLEMENT_SIMPLE(71)  THUNK_IMPLEMENT_SIMPLE(72)  THUNK_IMPLEMENT_SIMPLE(73)  THUNK_IMPLEMENT_SIMPLE(74)  
THUNK_IMPLEMENT_SIMPLE(75)  THUNK_IMPLEMENT_SIMPLE(76)  THUNK_IMPLEMENT_SIMPLE(77)  THUNK_IMPLEMENT_SIMPLE(78)  THUNK_IMPLEMENT_SIMPLE(79)  \
THUNK_IMPLEMENT_SIMPLE(80)  THUNK_IMPLEMENT_SIMPLE(81)  THUNK_IMPLEMENT_SIMPLE(82)  THUNK_IMPLEMENT_SIMPLE(83)  THUNK_IMPLEMENT_SIMPLE(84)  
THUNK_IMPLEMENT_SIMPLE(85)  THUNK_IMPLEMENT_SIMPLE(86)  THUNK_IMPLEMENT_SIMPLE(87)  THUNK_IMPLEMENT_SIMPLE(88)  THUNK_IMPLEMENT_SIMPLE(89)  THUNK_IMPLEMENT_SIMPLE(90)  \
THUNK_IMPLEMENT_SIMPLE(91)  THUNK_IMPLEMENT_SIMPLE(92)  THUNK_IMPLEMENT_SIMPLE(93)  THUNK_IMPLEMENT_SIMPLE(94)  THUNK_IMPLEMENT_SIMPLE(95)  THUNK_IMPLEMENT_SIMPLE(96)  
THUNK_IMPLEMENT_SIMPLE(97)  THUNK_IMPLEMENT_SIMPLE(98)  THUNK_IMPLEMENT_SIMPLE(99)  THUNK_IMPLEMENT_SIMPLE(100) THUNK_IMPLEMENT_SIMPLE(101) \
THUNK_IMPLEMENT_SIMPLE(102) THUNK_IMPLEMENT_SIMPLE(103) THUNK_IMPLEMENT_SIMPLE(104) THUNK_IMPLEMENT_SIMPLE(105) THUNK_IMPLEMENT_SIMPLE(106) 
THUNK_IMPLEMENT_SIMPLE(107) THUNK_IMPLEMENT_SIMPLE(108) THUNK_IMPLEMENT_SIMPLE(109) THUNK_IMPLEMENT_SIMPLE(110) THUNK_IMPLEMENT_SIMPLE(111) THUNK_IMPLEMENT_SIMPLE(112) \
THUNK_IMPLEMENT_SIMPLE(113) THUNK_IMPLEMENT_SIMPLE(114) THUNK_IMPLEMENT_SIMPLE(115) THUNK_IMPLEMENT_SIMPLE(116) THUNK_IMPLEMENT_SIMPLE(117) THUNK_IMPLEMENT_SIMPLE(118) 
THUNK_IMPLEMENT_SIMPLE(119) THUNK_IMPLEMENT_SIMPLE(120) THUNK_IMPLEMENT_SIMPLE(121) THUNK_IMPLEMENT_SIMPLE(122) THUNK_IMPLEMENT_SIMPLE(123) \
THUNK_IMPLEMENT_SIMPLE(124) THUNK_IMPLEMENT_SIMPLE(125) THUNK_IMPLEMENT_SIMPLE(126) THUNK_IMPLEMENT_SIMPLE(127) THUNK_IMPLEMENT_SIMPLE(128) 
THUNK_IMPLEMENT_SIMPLE(129) THUNK_IMPLEMENT_SIMPLE(130) THUNK_IMPLEMENT_SIMPLE(131) THUNK_IMPLEMENT_SIMPLE(132) THUNK_IMPLEMENT_SIMPLE(133) THUNK_IMPLEMENT_SIMPLE(134) \
THUNK_IMPLEMENT_SIMPLE(135) THUNK_IMPLEMENT_SIMPLE(136) THUNK_IMPLEMENT_SIMPLE(137) THUNK_IMPLEMENT_SIMPLE(138) THUNK_IMPLEMENT_SIMPLE(139) THUNK_IMPLEMENT_SIMPLE(140) 
THUNK_IMPLEMENT_SIMPLE(141) THUNK_IMPLEMENT_SIMPLE(142) THUNK_IMPLEMENT_SIMPLE(143) THUNK_IMPLEMENT_SIMPLE(144) THUNK_IMPLEMENT_SIMPLE(145) \
THUNK_IMPLEMENT_SIMPLE(146) THUNK_IMPLEMENT_SIMPLE(147) THUNK_IMPLEMENT_SIMPLE(148) THUNK_IMPLEMENT_SIMPLE(149) THUNK_IMPLEMENT_SIMPLE(150) 
THUNK_IMPLEMENT_SIMPLE(151) THUNK_IMPLEMENT_SIMPLE(152) THUNK_IMPLEMENT_SIMPLE(153) THUNK_IMPLEMENT_SIMPLE(154) THUNK_IMPLEMENT_SIMPLE(155) THUNK_IMPLEMENT_SIMPLE(156) \
THUNK_IMPLEMENT_SIMPLE(157) THUNK_IMPLEMENT_SIMPLE(158) THUNK_IMPLEMENT_SIMPLE(159) THUNK_IMPLEMENT_SIMPLE(160) THUNK_IMPLEMENT_SIMPLE(161) THUNK_IMPLEMENT_SIMPLE(162) 
THUNK_IMPLEMENT_SIMPLE(163) THUNK_IMPLEMENT_SIMPLE(164) THUNK_IMPLEMENT_SIMPLE(165) THUNK_IMPLEMENT_SIMPLE(166) THUNK_IMPLEMENT_SIMPLE(167) \
THUNK_IMPLEMENT_SIMPLE(168) THUNK_IMPLEMENT_SIMPLE(169) THUNK_IMPLEMENT_SIMPLE(170) THUNK_IMPLEMENT_SIMPLE(171) THUNK_IMPLEMENT_SIMPLE(172) 
THUNK_IMPLEMENT_SIMPLE(173) THUNK_IMPLEMENT_SIMPLE(174) THUNK_IMPLEMENT_SIMPLE(175) THUNK_IMPLEMENT_SIMPLE(176) THUNK_IMPLEMENT_SIMPLE(177) THUNK_IMPLEMENT_SIMPLE(178) \
THUNK_IMPLEMENT_SIMPLE(179) THUNK_IMPLEMENT_SIMPLE(180) THUNK_IMPLEMENT_SIMPLE(181) THUNK_IMPLEMENT_SIMPLE(182) THUNK_IMPLEMENT_SIMPLE(183) THUNK_IMPLEMENT_SIMPLE(184) 
THUNK_IMPLEMENT_SIMPLE(185) THUNK_IMPLEMENT_SIMPLE(186) THUNK_IMPLEMENT_SIMPLE(187) THUNK_IMPLEMENT_SIMPLE(188) THUNK_IMPLEMENT_SIMPLE(189) \
THUNK_IMPLEMENT_SIMPLE(190) THUNK_IMPLEMENT_SIMPLE(191) THUNK_IMPLEMENT_SIMPLE(192) THUNK_IMPLEMENT_SIMPLE(193) THUNK_IMPLEMENT_SIMPLE(194) 
THUNK_IMPLEMENT_SIMPLE(195) THUNK_IMPLEMENT_SIMPLE(196) THUNK_IMPLEMENT_SIMPLE(197) THUNK_IMPLEMENT_SIMPLE(198) THUNK_IMPLEMENT_SIMPLE(199)

#endif  //WIN16

