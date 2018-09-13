
 //----------------------------------------------------------------------------
 //
 // File:     thunks.S
 //
 // Contains: Assembly code for the PARISC - hand tuned from a generated .S file
 //
 //----------------------------------------------------------------------------

//#include <tearoff.hxx>



#define this        %r26
#define sp          %r30

#define offset      %r19
#define vtbl        %r20
#define index       %r21
#define dwMask      %r19         // CAREFULL!!! same as offset....
#define pfn         %r22

 //
 // Here's the layout of the 'this' object
 //
 // offset  value
 //
 //     0   don't care
 //     4   don't care
 //     8   don't care
 //     12  pvObject1's this
 //     16  pvObject1's function table
 //     20  pvObject2's this
 //     24  pvObject2's function table
 //     28  mask to decide whether to use Object 1 or 2
 //     32  index of method into vtbl
 //  Note:   Look in unixtearoff.cxx for latest structure definition
 //

#define off_pvObject1           12
#define off_pvObjectVtbl1       16
#define off_pvObject2           20
#define off_dwMask              28
#define off_dwN			32

#define objvtblDelta           (off_pvObjectVtbl1 - off_pvObject1)

 //----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define COMPLETE_THUNK(n)                                                                           !\
L$CompleteThunk##n                                                                                  !\
    LDH     ((8*(n+1))+0)(vtbl), offset                 /* offset = vtbl[n].offset */               !\
    EXTRS   offset,31,16,offset                                                                     !\
    LDW     ((8*(n+1))+4)(vtbl), pfn                    /* pfn = vtbl[n].pfn */                     !\
    LDH     ((8*(n+1))+2)(vtbl), index                  /* index = vtbl[n].realVtblIndex */         !\
    EXTRS   index,31,16,index                                                                       !\
                                                                                                    !\
    COMB,<= index,0,L$NonVirtual##n                     /* if ( index <= 0 ) goto NonVirtual */     !\
    NOP                                                                                             !\
                                                                                                    !\
/* Virtual */                                                                                       !\
    ADD     this,pfn,this                               /* this += (vtbl_offset)pfn */              !\
    LDW     0(this),vtbl                                /* vtbl = *this */                          !\
    SUB     this,pfn,this                               /* this -= (vtbl_offset)pfn */              !\
                                                                                                    !\
    SH3ADD  index,vtbl,vtbl                             /* vtbl += index * sizeof(VTBLENTRY) */     !\
                                                                                                    !\
    ADD     offset,this,this                            /* this += offset */                        !\
    LDW     4(vtbl), pfn                                /* pfn = vtbl->pfn */                       !\
    LDH     0(vtbl), offset                             /* offset = vtbl->offset */                 !\
                                                                                                    !\
L$NonVirtual##n                                                                                     !\
    ADD     offset,this,this                            /* this += offset; */                       !\
    LDWS    2(pfn), %r19                                                                            !\
    LDWS    -2(pfn),pfn                                                                             !\
    LDSID   (pfn), vtbl                                                                             !\
    MTSP    vtbl, %sr0                                                                              !\
    BE      0(%sr0,pfn)                                 /* func(); */                               !\
    NOP


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define THUNK_IMPLEMENT_SIMPLE(n)                \
! .SPACE $TEXT$                                  \
! .SUBSPA $CODE$                                 \
! .EXPORT TearoffThunk##n                        \
                                                !\
TearoffThunk##n                                 !\
                                                !\
        ADDI    n, %r0, %r28                    !\
        LDO     off_dwN(this), dwMask           !\
        STW     %r28, 0(dwMask)                 !\
        COPY    this, %r28                      !\
                                                !\
        LDW     off_pvObjectVtbl1(this),vtbl    !\
        LDW     off_pvObject1(this),this        !\
                                                !\
        COMPLETE_THUNK(n)                       !\



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define THUNK_IMPLEMENT_COMPARE(n)               \
! .SPACE $TEXT$                                  \
! .SUBSPA $CODE$                                 \
! .EXPORT TearoffThunk##n                        \
                                                !\
TearoffThunk##n                                 !\
        ADDI    n, %r0, %r28                    !\
        LDO     off_dwN(this), dwMask           !\
        STW     %r28, 0(dwMask)                 !\
        COPY    this, %r28                      !\
                                                !\
        LDW          28(this), dwMask           !\
        EXTRS,>=   dwMask,31-n,1,%r0            !\
        LDO          8(this),this               !\
                                                !\
        LDW     off_pvObjectVtbl1(this),vtbl    !\
        LDW     off_pvObject1(this),this        !\
                                                !\
        COMPLETE_THUNK(n)                       !\

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define THUNK_IMPLEMENT_CRASH(n)                 \
! .SPACE $TEXT$                                  \
! .SUBSPA $CODE$                                 \
! .EXPORT TearoffThunk##n                        \
                                                !\
TearoffThunk##n                                 !\
        ADDI    0, %r0, %r28                    !\
        STW     %r28, 0(%r28)                   

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        .SPACE  $TEXT$
        .SUBSPA $CODE$,QUAD=0,ALIGN=4,ACCESS=0x2c,CODE_ONLY,SORT=24

        .EXPORT doThunk__12CMethodThunkFv,ENTRY,PRIV_LEV=3 //,NO_RELOCATION,LONG_RETURN
doThunk__12CMethodThunkFv

        .EXPORT doThunk__12CMethodThunkFPve,ENTRY,PRIV_LEV=3,ARGW0=GR,ARGW1=GR,ARGW2=GR,ARGW3=GR,RTNVAL=GR
doThunk__12CMethodThunkFPve

        .EXPORT doThunk__12CMethodThunkFie,ENTRY,PRIV_LEV=3,ARGW0=GR,ARGW1=GR,ARGW2=GR,ARGW3=GR,RTNVAL=GR
doThunk__12CMethodThunkFie

        .EXPORT doThunk__12CMethodThunkFR16TextContextEvent,ENTRY,PRIV_LEV=3 //,NO_RELOCATION,LONG_RETURN
doThunk__12CMethodThunkFR16TextContextEvent

        .EXPORT doThunk__12CMethodThunkFPFP6HWND__UiT2l_le,ENTRY,PRIV_LEV=3 //,NO_RELOCATION,LONG_RETURN
doThunk__12CMethodThunkFPFP6HWND__UiT2l_le

        .EXPORT doThunk__12CMethodThunkF5_GUIDe,ENTRY,PRIV_LEV=3 //,NO_RELOCATION,LONG_RETURN
doThunk__12CMethodThunkF5_GUIDe

        STW     %r2,-0x18(%r30)                             /* store the return address */
        LDW     4(this),vtbl                 
        LDW     0(this),this                
        LDO     -8(vtbl),vtbl
        BL      L$CompleteThunk0,%r0
        NOP


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
        .SPACE  $TEXT$
        .SUBSPA $CODE$
        .EXPORT _GetTearoff,ENTRY,PRIV_LEV=3 //,NO_RELOCATION,LONG_RETURN
_GetTearoff
        BV      %r0(%r2)
        NOP


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
//      Define IUnknown thunks (0 - 2) only used by unixtearoff.cxx (simple)
//      Define the thunks from 3 to 15 (compare thunks)
//      Define the thunks from 16 onwards (simple thunks)
//


THUNK_IMPLEMENT_COMPARE(0)
THUNK_IMPLEMENT_SIMPLE(1)
THUNK_IMPLEMENT_SIMPLE(2)

//THUNK_IMPLEMENT_CRASH(3)
THUNK_IMPLEMENT_COMPARE(3)
THUNK_IMPLEMENT_COMPARE(4)
THUNK_IMPLEMENT_COMPARE(5)
THUNK_IMPLEMENT_COMPARE(6)
THUNK_IMPLEMENT_COMPARE(7)
THUNK_IMPLEMENT_COMPARE(8)
THUNK_IMPLEMENT_COMPARE(9)
THUNK_IMPLEMENT_COMPARE(10)
THUNK_IMPLEMENT_COMPARE(11)
THUNK_IMPLEMENT_COMPARE(12)
THUNK_IMPLEMENT_COMPARE(13)
THUNK_IMPLEMENT_COMPARE(14)
THUNK_IMPLEMENT_COMPARE(15)

THUNK_IMPLEMENT_SIMPLE(16)
THUNK_IMPLEMENT_SIMPLE(17)
THUNK_IMPLEMENT_SIMPLE(18)
THUNK_IMPLEMENT_SIMPLE(19)
THUNK_IMPLEMENT_SIMPLE(20)
THUNK_IMPLEMENT_SIMPLE(21)
THUNK_IMPLEMENT_SIMPLE(22)
THUNK_IMPLEMENT_SIMPLE(23)
THUNK_IMPLEMENT_SIMPLE(24)
THUNK_IMPLEMENT_SIMPLE(25)
THUNK_IMPLEMENT_SIMPLE(26)
THUNK_IMPLEMENT_SIMPLE(27)
THUNK_IMPLEMENT_SIMPLE(28)
THUNK_IMPLEMENT_SIMPLE(29)
THUNK_IMPLEMENT_SIMPLE(30)
THUNK_IMPLEMENT_SIMPLE(31)
THUNK_IMPLEMENT_SIMPLE(32)
THUNK_IMPLEMENT_SIMPLE(33)
THUNK_IMPLEMENT_SIMPLE(34)
THUNK_IMPLEMENT_SIMPLE(35)
THUNK_IMPLEMENT_SIMPLE(36)
THUNK_IMPLEMENT_SIMPLE(37)
THUNK_IMPLEMENT_SIMPLE(38)
THUNK_IMPLEMENT_SIMPLE(39)
THUNK_IMPLEMENT_SIMPLE(40)
THUNK_IMPLEMENT_SIMPLE(41)
THUNK_IMPLEMENT_SIMPLE(42)
THUNK_IMPLEMENT_SIMPLE(43)
THUNK_IMPLEMENT_SIMPLE(44)
THUNK_IMPLEMENT_SIMPLE(45)
THUNK_IMPLEMENT_SIMPLE(46)
THUNK_IMPLEMENT_SIMPLE(47)
THUNK_IMPLEMENT_SIMPLE(48)
THUNK_IMPLEMENT_SIMPLE(49)
THUNK_IMPLEMENT_SIMPLE(50)
THUNK_IMPLEMENT_SIMPLE(51)
THUNK_IMPLEMENT_SIMPLE(52)
THUNK_IMPLEMENT_SIMPLE(53)
THUNK_IMPLEMENT_SIMPLE(54)
THUNK_IMPLEMENT_SIMPLE(55)
THUNK_IMPLEMENT_SIMPLE(56)
THUNK_IMPLEMENT_SIMPLE(57)
THUNK_IMPLEMENT_SIMPLE(58)
THUNK_IMPLEMENT_SIMPLE(59)
THUNK_IMPLEMENT_SIMPLE(60)
THUNK_IMPLEMENT_SIMPLE(61)
THUNK_IMPLEMENT_SIMPLE(62)
THUNK_IMPLEMENT_SIMPLE(63)
THUNK_IMPLEMENT_SIMPLE(64)
THUNK_IMPLEMENT_SIMPLE(65)
THUNK_IMPLEMENT_SIMPLE(66)
THUNK_IMPLEMENT_SIMPLE(67)
THUNK_IMPLEMENT_SIMPLE(68)
THUNK_IMPLEMENT_SIMPLE(69)
THUNK_IMPLEMENT_SIMPLE(70)
THUNK_IMPLEMENT_SIMPLE(71)
THUNK_IMPLEMENT_SIMPLE(72)
THUNK_IMPLEMENT_SIMPLE(73)
THUNK_IMPLEMENT_SIMPLE(74)
THUNK_IMPLEMENT_SIMPLE(75)
THUNK_IMPLEMENT_SIMPLE(76)
THUNK_IMPLEMENT_SIMPLE(77)
THUNK_IMPLEMENT_SIMPLE(78)
THUNK_IMPLEMENT_SIMPLE(79)
THUNK_IMPLEMENT_SIMPLE(80)
THUNK_IMPLEMENT_SIMPLE(81)
THUNK_IMPLEMENT_SIMPLE(82)
THUNK_IMPLEMENT_SIMPLE(83)
THUNK_IMPLEMENT_SIMPLE(84)
THUNK_IMPLEMENT_SIMPLE(85)
THUNK_IMPLEMENT_SIMPLE(86)
THUNK_IMPLEMENT_SIMPLE(87)
THUNK_IMPLEMENT_SIMPLE(88)
THUNK_IMPLEMENT_SIMPLE(89)
THUNK_IMPLEMENT_SIMPLE(90)
THUNK_IMPLEMENT_SIMPLE(91)
THUNK_IMPLEMENT_SIMPLE(92)
THUNK_IMPLEMENT_SIMPLE(93)
THUNK_IMPLEMENT_SIMPLE(94)
THUNK_IMPLEMENT_SIMPLE(95)
THUNK_IMPLEMENT_SIMPLE(96)
THUNK_IMPLEMENT_SIMPLE(97)
THUNK_IMPLEMENT_SIMPLE(98)
THUNK_IMPLEMENT_SIMPLE(99)
THUNK_IMPLEMENT_SIMPLE(100)
THUNK_IMPLEMENT_SIMPLE(101)
THUNK_IMPLEMENT_SIMPLE(102)
THUNK_IMPLEMENT_SIMPLE(103)
THUNK_IMPLEMENT_SIMPLE(104)
THUNK_IMPLEMENT_SIMPLE(105)
THUNK_IMPLEMENT_SIMPLE(106)
THUNK_IMPLEMENT_SIMPLE(107)
THUNK_IMPLEMENT_SIMPLE(108)
THUNK_IMPLEMENT_SIMPLE(109)
THUNK_IMPLEMENT_SIMPLE(110)
THUNK_IMPLEMENT_SIMPLE(111)
THUNK_IMPLEMENT_SIMPLE(112)
THUNK_IMPLEMENT_SIMPLE(113)
THUNK_IMPLEMENT_SIMPLE(114)
THUNK_IMPLEMENT_SIMPLE(115)
THUNK_IMPLEMENT_SIMPLE(116)
THUNK_IMPLEMENT_SIMPLE(117)
THUNK_IMPLEMENT_SIMPLE(118)
THUNK_IMPLEMENT_SIMPLE(119)
THUNK_IMPLEMENT_SIMPLE(120)
THUNK_IMPLEMENT_SIMPLE(121)
THUNK_IMPLEMENT_SIMPLE(122)
THUNK_IMPLEMENT_SIMPLE(123)
THUNK_IMPLEMENT_SIMPLE(124)
THUNK_IMPLEMENT_SIMPLE(125)
THUNK_IMPLEMENT_SIMPLE(126)
THUNK_IMPLEMENT_SIMPLE(127)
THUNK_IMPLEMENT_SIMPLE(128)
THUNK_IMPLEMENT_SIMPLE(129)
THUNK_IMPLEMENT_SIMPLE(130)
THUNK_IMPLEMENT_SIMPLE(131)
THUNK_IMPLEMENT_SIMPLE(132)
THUNK_IMPLEMENT_SIMPLE(133)
THUNK_IMPLEMENT_SIMPLE(134)
THUNK_IMPLEMENT_SIMPLE(135)
THUNK_IMPLEMENT_SIMPLE(136)
THUNK_IMPLEMENT_SIMPLE(137)
THUNK_IMPLEMENT_SIMPLE(138)
THUNK_IMPLEMENT_SIMPLE(139)
THUNK_IMPLEMENT_SIMPLE(140)
THUNK_IMPLEMENT_SIMPLE(141)
THUNK_IMPLEMENT_SIMPLE(142)
THUNK_IMPLEMENT_SIMPLE(143)
THUNK_IMPLEMENT_SIMPLE(144)
THUNK_IMPLEMENT_SIMPLE(145)
THUNK_IMPLEMENT_SIMPLE(146)
THUNK_IMPLEMENT_SIMPLE(147)
THUNK_IMPLEMENT_SIMPLE(148)
THUNK_IMPLEMENT_SIMPLE(149)
THUNK_IMPLEMENT_SIMPLE(150)
THUNK_IMPLEMENT_SIMPLE(151)
THUNK_IMPLEMENT_SIMPLE(152)
THUNK_IMPLEMENT_SIMPLE(153)
THUNK_IMPLEMENT_SIMPLE(154)
THUNK_IMPLEMENT_SIMPLE(155)
THUNK_IMPLEMENT_SIMPLE(156)
THUNK_IMPLEMENT_SIMPLE(157)
THUNK_IMPLEMENT_SIMPLE(158)
THUNK_IMPLEMENT_SIMPLE(159)
THUNK_IMPLEMENT_SIMPLE(160)
THUNK_IMPLEMENT_SIMPLE(161)
THUNK_IMPLEMENT_SIMPLE(162)
THUNK_IMPLEMENT_SIMPLE(163)
THUNK_IMPLEMENT_SIMPLE(164)
THUNK_IMPLEMENT_SIMPLE(165)
THUNK_IMPLEMENT_SIMPLE(166)
THUNK_IMPLEMENT_SIMPLE(167)
THUNK_IMPLEMENT_SIMPLE(168)
THUNK_IMPLEMENT_SIMPLE(169)
THUNK_IMPLEMENT_SIMPLE(170)
THUNK_IMPLEMENT_SIMPLE(171)
THUNK_IMPLEMENT_SIMPLE(172)
THUNK_IMPLEMENT_SIMPLE(173)
THUNK_IMPLEMENT_SIMPLE(174)
THUNK_IMPLEMENT_SIMPLE(175)
THUNK_IMPLEMENT_SIMPLE(176)
THUNK_IMPLEMENT_SIMPLE(177)
THUNK_IMPLEMENT_SIMPLE(178)
THUNK_IMPLEMENT_SIMPLE(179)
THUNK_IMPLEMENT_SIMPLE(180)
THUNK_IMPLEMENT_SIMPLE(181)
THUNK_IMPLEMENT_SIMPLE(182)
THUNK_IMPLEMENT_SIMPLE(183)
THUNK_IMPLEMENT_SIMPLE(184)
THUNK_IMPLEMENT_SIMPLE(185)
THUNK_IMPLEMENT_SIMPLE(186)
THUNK_IMPLEMENT_SIMPLE(187)
THUNK_IMPLEMENT_SIMPLE(188)
THUNK_IMPLEMENT_SIMPLE(189)
THUNK_IMPLEMENT_SIMPLE(190)
THUNK_IMPLEMENT_SIMPLE(191)
THUNK_IMPLEMENT_SIMPLE(192)
THUNK_IMPLEMENT_SIMPLE(193)
THUNK_IMPLEMENT_SIMPLE(194)
THUNK_IMPLEMENT_SIMPLE(195)
THUNK_IMPLEMENT_SIMPLE(196)
THUNK_IMPLEMENT_SIMPLE(197)
THUNK_IMPLEMENT_SIMPLE(198)
THUNK_IMPLEMENT_SIMPLE(199)

.END
