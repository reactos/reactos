//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       tearoff.hxx
//
//  Contents:   Tearoff Interface Utilities
//
//----------------------------------------------------------------------------


#ifndef _TEAROFF_HXX_
#define _TEAROFF_HXX_

#ifdef __STDC__
#define cat(a,b)        a##b
#else
#define cat(a,b)        a/**/b
#endif

#if defined(UNIX) && defined(_HPUX_SOURCE)
#define THUNK_ARRAY_3_TO_15(x) \
 THUNK_##x(3)   THUNK_##x(4)   THUNK_##x(5)   THUNK_##x(6)   THUNK_##x(7)   THUNK_##x(8)   THUNK_##x(9)   THUNK_##x(10)  THUNK_##x(11)  THUNK_##x(12)  THUNK_##x(13)  \
 THUNK_##x(14)  THUNK_##x(15)
    
#define THUNK_ARRAY_16_AND_UP(x) \
THUNK_##x(16)  THUNK_##x(17)  THUNK_##x(18)  THUNK_##x(19)  THUNK_##x(20)  THUNK_##x(21)  THUNK_##x(22)  THUNK_##x(23)  THUNK_##x(24)  \
THUNK_##x(25)  THUNK_##x(26)  THUNK_##x(27)  THUNK_##x(28)  THUNK_##x(29)  THUNK_##x(30)  THUNK_##x(31)  THUNK_##x(32)  THUNK_##x(33)  THUNK_##x(34)  THUNK_##x(35)  \
THUNK_##x(36)  THUNK_##x(37)  THUNK_##x(38)  THUNK_##x(39)  THUNK_##x(40)  THUNK_##x(41)  THUNK_##x(42)  THUNK_##x(43)  THUNK_##x(44)  THUNK_##x(45)  THUNK_##x(46)  \
THUNK_##x(47)  THUNK_##x(48)  THUNK_##x(49)  THUNK_##x(50)  THUNK_##x(51)  THUNK_##x(52)  THUNK_##x(53)  THUNK_##x(54)  THUNK_##x(55)  THUNK_##x(56)  THUNK_##x(57)  \
THUNK_##x(58)  THUNK_##x(59)  THUNK_##x(60)  THUNK_##x(61)  THUNK_##x(62)  THUNK_##x(63)  THUNK_##x(64)  THUNK_##x(65)  THUNK_##x(66)  THUNK_##x(67)  THUNK_##x(68)  \
THUNK_##x(69)  THUNK_##x(70)  THUNK_##x(71)  THUNK_##x(72)  THUNK_##x(73)  THUNK_##x(74)  THUNK_##x(75)  THUNK_##x(76)  THUNK_##x(77)  THUNK_##x(78)  THUNK_##x(79)  \
THUNK_##x(80)  THUNK_##x(81)  THUNK_##x(82)  THUNK_##x(83)  THUNK_##x(84)  THUNK_##x(85)  THUNK_##x(86)  THUNK_##x(87)  THUNK_##x(88)  THUNK_##x(89)  THUNK_##x(90)  \
THUNK_##x(91)  THUNK_##x(92)  THUNK_##x(93)  THUNK_##x(94)  THUNK_##x(95)  THUNK_##x(96)  THUNK_##x(97)  THUNK_##x(98)  THUNK_##x(99)  THUNK_##x(100) THUNK_##x(101) \
THUNK_##x(102) THUNK_##x(103) THUNK_##x(104) THUNK_##x(105) THUNK_##x(106) THUNK_##x(107) THUNK_##x(108) THUNK_##x(109) THUNK_##x(110) THUNK_##x(111) THUNK_##x(112) \
THUNK_##x(113) THUNK_##x(114) THUNK_##x(115) THUNK_##x(116) THUNK_##x(117) THUNK_##x(118) THUNK_##x(119) THUNK_##x(120) THUNK_##x(121) THUNK_##x(122) THUNK_##x(123) \
THUNK_##x(124) THUNK_##x(125) THUNK_##x(126) THUNK_##x(127) THUNK_##x(128) THUNK_##x(129) THUNK_##x(130) THUNK_##x(131) THUNK_##x(132) THUNK_##x(133) THUNK_##x(134) \
THUNK_##x(135) THUNK_##x(136) THUNK_##x(137) THUNK_##x(138) THUNK_##x(139) THUNK_##x(140) THUNK_##x(141) THUNK_##x(142) THUNK_##x(143) THUNK_##x(144) THUNK_##x(145) \
THUNK_##x(146) THUNK_##x(147) THUNK_##x(148) THUNK_##x(149) THUNK_##x(150) THUNK_##x(151) THUNK_##x(152) THUNK_##x(153) THUNK_##x(154) THUNK_##x(155) THUNK_##x(156) \
THUNK_##x(157) THUNK_##x(158) THUNK_##x(159) THUNK_##x(160) THUNK_##x(161) THUNK_##x(162) THUNK_##x(163) THUNK_##x(164) THUNK_##x(165) THUNK_##x(166) THUNK_##x(167) \
THUNK_##x(168) THUNK_##x(169) THUNK_##x(170) THUNK_##x(171) THUNK_##x(172) THUNK_##x(173) THUNK_##x(174) THUNK_##x(175) THUNK_##x(176) THUNK_##x(177) THUNK_##x(178) \
THUNK_##x(179) THUNK_##x(180) THUNK_##x(181) THUNK_##x(182) THUNK_##x(183) THUNK_##x(184) THUNK_##x(185) THUNK_##x(186) THUNK_##x(187) THUNK_##x(188) THUNK_##x(189) \
THUNK_##x(190) THUNK_##x(191) THUNK_##x(192) THUNK_##x(193) THUNK_##x(194) THUNK_##x(195) THUNK_##x(196) THUNK_##x(197) THUNK_##x(198) THUNK_##x(199)        
#else
#define THUNK_ARRAY_3_TO_15(x) \
 cat(THUNK_,x)(3)   cat(THUNK_,x)(4)   cat(THUNK_,x)(5)   cat(THUNK_,x)(6)   cat(THUNK_,x)(7)   cat(THUNK_,x)(8)   cat(THUNK_,x)(9)   cat(THUNK_,x)(10)  cat(THUNK_,x)(11)  cat(THUNK_,x)(12)  cat(THUNK_,x)(13)  \
 cat(THUNK_,x)(14)  cat(THUNK_,x)(15)
    
#define THUNK_ARRAY_16_AND_UP(x) \
cat(THUNK_,x)(16)  cat(THUNK_,x)(17)  cat(THUNK_,x)(18)  cat(THUNK_,x)(19)  cat(THUNK_,x)(20)  cat(THUNK_,x)(21)  cat(THUNK_,x)(22)  cat(THUNK_,x)(23)  cat(THUNK_,x)(24)  \
cat(THUNK_,x)(25)  cat(THUNK_,x)(26)  cat(THUNK_,x)(27)  cat(THUNK_,x)(28)  cat(THUNK_,x)(29)  cat(THUNK_,x)(30)  cat(THUNK_,x)(31)  cat(THUNK_,x)(32)  cat(THUNK_,x)(33)  cat(THUNK_,x)(34)  cat(THUNK_,x)(35)  \
cat(THUNK_,x)(36)  cat(THUNK_,x)(37)  cat(THUNK_,x)(38)  cat(THUNK_,x)(39)  cat(THUNK_,x)(40)  cat(THUNK_,x)(41)  cat(THUNK_,x)(42)  cat(THUNK_,x)(43)  cat(THUNK_,x)(44)  cat(THUNK_,x)(45)  cat(THUNK_,x)(46)  \
cat(THUNK_,x)(47)  cat(THUNK_,x)(48)  cat(THUNK_,x)(49)  cat(THUNK_,x)(50)  cat(THUNK_,x)(51)  cat(THUNK_,x)(52)  cat(THUNK_,x)(53)  cat(THUNK_,x)(54)  cat(THUNK_,x)(55)  cat(THUNK_,x)(56)  cat(THUNK_,x)(57)  \
cat(THUNK_,x)(58)  cat(THUNK_,x)(59)  cat(THUNK_,x)(60)  cat(THUNK_,x)(61)  cat(THUNK_,x)(62)  cat(THUNK_,x)(63)  cat(THUNK_,x)(64)  cat(THUNK_,x)(65)  cat(THUNK_,x)(66)  cat(THUNK_,x)(67)  cat(THUNK_,x)(68)  \
cat(THUNK_,x)(69)  cat(THUNK_,x)(70)  cat(THUNK_,x)(71)  cat(THUNK_,x)(72)  cat(THUNK_,x)(73)  cat(THUNK_,x)(74)  cat(THUNK_,x)(75)  cat(THUNK_,x)(76)  cat(THUNK_,x)(77)  cat(THUNK_,x)(78)  cat(THUNK_,x)(79)  \
cat(THUNK_,x)(80)  cat(THUNK_,x)(81)  cat(THUNK_,x)(82)  cat(THUNK_,x)(83)  cat(THUNK_,x)(84)  cat(THUNK_,x)(85)  cat(THUNK_,x)(86)  cat(THUNK_,x)(87)  cat(THUNK_,x)(88)  cat(THUNK_,x)(89)  cat(THUNK_,x)(90)  \
cat(THUNK_,x)(91)  cat(THUNK_,x)(92)  cat(THUNK_,x)(93)  cat(THUNK_,x)(94)  cat(THUNK_,x)(95)  cat(THUNK_,x)(96)  cat(THUNK_,x)(97)  cat(THUNK_,x)(98)  cat(THUNK_,x)(99)  cat(THUNK_,x)(100) cat(THUNK_,x)(101) \
cat(THUNK_,x)(102) cat(THUNK_,x)(103) cat(THUNK_,x)(104) cat(THUNK_,x)(105) cat(THUNK_,x)(106) cat(THUNK_,x)(107) cat(THUNK_,x)(108) cat(THUNK_,x)(109) cat(THUNK_,x)(110) cat(THUNK_,x)(111) cat(THUNK_,x)(112) \
cat(THUNK_,x)(113) cat(THUNK_,x)(114) cat(THUNK_,x)(115) cat(THUNK_,x)(116) cat(THUNK_,x)(117) cat(THUNK_,x)(118) cat(THUNK_,x)(119) cat(THUNK_,x)(120) cat(THUNK_,x)(121) cat(THUNK_,x)(122) cat(THUNK_,x)(123) \
cat(THUNK_,x)(124) cat(THUNK_,x)(125) cat(THUNK_,x)(126) cat(THUNK_,x)(127) cat(THUNK_,x)(128) cat(THUNK_,x)(129) cat(THUNK_,x)(130) cat(THUNK_,x)(131) cat(THUNK_,x)(132) cat(THUNK_,x)(133) cat(THUNK_,x)(134) \
cat(THUNK_,x)(135) cat(THUNK_,x)(136) cat(THUNK_,x)(137) cat(THUNK_,x)(138) cat(THUNK_,x)(139) cat(THUNK_,x)(140) cat(THUNK_,x)(141) cat(THUNK_,x)(142) cat(THUNK_,x)(143) cat(THUNK_,x)(144) cat(THUNK_,x)(145) \
cat(THUNK_,x)(146) cat(THUNK_,x)(147) cat(THUNK_,x)(148) cat(THUNK_,x)(149) cat(THUNK_,x)(150) cat(THUNK_,x)(151) cat(THUNK_,x)(152) cat(THUNK_,x)(153) cat(THUNK_,x)(154) cat(THUNK_,x)(155) cat(THUNK_,x)(156) \
cat(THUNK_,x)(157) cat(THUNK_,x)(158) cat(THUNK_,x)(159) cat(THUNK_,x)(160) cat(THUNK_,x)(161) cat(THUNK_,x)(162) cat(THUNK_,x)(163) cat(THUNK_,x)(164) cat(THUNK_,x)(165) cat(THUNK_,x)(166) cat(THUNK_,x)(167) \
cat(THUNK_,x)(168) cat(THUNK_,x)(169) cat(THUNK_,x)(170) cat(THUNK_,x)(171) cat(THUNK_,x)(172) cat(THUNK_,x)(173) cat(THUNK_,x)(174) cat(THUNK_,x)(175) cat(THUNK_,x)(176) cat(THUNK_,x)(177) cat(THUNK_,x)(178) \
cat(THUNK_,x)(179) cat(THUNK_,x)(180) cat(THUNK_,x)(181) cat(THUNK_,x)(182) cat(THUNK_,x)(183) cat(THUNK_,x)(184) cat(THUNK_,x)(185) cat(THUNK_,x)(186) cat(THUNK_,x)(187) cat(THUNK_,x)(188) cat(THUNK_,x)(189) \
cat(THUNK_,x)(190) cat(THUNK_,x)(191) cat(THUNK_,x)(192) cat(THUNK_,x)(193) cat(THUNK_,x)(194) cat(THUNK_,x)(195) cat(THUNK_,x)(196) cat(THUNK_,x)(197) cat(THUNK_,x)(198) cat(THUNK_,x)(199)        

#endif // UNIX
#endif //_TEAROFF_HXX_
