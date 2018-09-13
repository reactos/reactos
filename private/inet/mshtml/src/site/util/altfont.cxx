/*
 *  @doc    INTERNAL
 *
 *  @module ALTFONT.CXX -- Alternate Font Name
 *
 *
 *  Owner: <nl>
 *      Chris Thrasher <nl>
 *
 *  History: <nl>
 *      06/29/98     cthrash created
 *
 *  Copyright (c) 1997-1998 Microsoft Corporation. All rights reserved.
 */

#include "headers.hxx"

// The following was generated programmatically.  It is crucial that the
// strings remain sort StrCmpIC-wise, as we use a bsearch to find the
// alternate name of the font.

const TCHAR * g_pszAltFontName000 = L"Ami R";
const TCHAR * g_pszAltFontName001 = L"AR P\x30da\x30f3\x6977\x66f8\x4f53L";
const TCHAR * g_pszAltFontName002 = L"AR P\x30da\x30f3\x884c\x6977\x66f8\x4f53 L";
const TCHAR * g_pszAltFontName003 = L"AR P\x52d8\x4ead\x6d41H";
const TCHAR * g_pszAltFontName004 = L"AR P\x53e4\x5370\x4f53\x0042";
const TCHAR * g_pszAltFontName005 = L"AR P\x6977\x66f8\x4f53\x4f53 M";
const TCHAR * g_pszAltFontName006 = L"AR P\x767d\x4e38\xff30\xff2f\xff30\x4f53H";
const TCHAR * g_pszAltFontName007 = L"AR P\x884c\x6977\x66f8\x4f53H";
const TCHAR * g_pszAltFontName008 = L"AR P\x884c\x6977\x66f8\x4f53L";
const TCHAR * g_pszAltFontName009 = L"AR P\x96b7\x66f8\x4f53 M";
const TCHAR * g_pszAltFontName010 = L"AR P\x9ed2\x4e38\xff30\xff2f\xff30\x4f53H";
const TCHAR * g_pszAltFontName011 = L"Arphic Gyokailenmentai Heavy JIS";
const TCHAR * g_pszAltFontName012 = L"Arphic Gyokailenmentai Light JIS";
const TCHAR * g_pszAltFontName013 = L"Arphic Gyokaisho Heavy JIS";
const TCHAR * g_pszAltFontName014 = L"Arphic Gyokaisho Light JIS";
const TCHAR * g_pszAltFontName015 = L"Arphic Kaisho Medium JIS";
const TCHAR * g_pszAltFontName016 = L"Arphic Kanteiryu Heavy JIS";
const TCHAR * g_pszAltFontName017 = L"Arphic Koin-Tai Bold JIS";
const TCHAR * g_pszAltFontName018 = L"Arphic Kuro-Maru-POP Heavy JIS";
const TCHAR * g_pszAltFontName019 = L"Arphic Pengyokaisho Light JIS";
const TCHAR * g_pszAltFontName020 = L"Arphic Penkaisho Light JIS";
const TCHAR * g_pszAltFontName021 = L"Arphic PGyokaisho Heavy JIS";
const TCHAR * g_pszAltFontName022 = L"Arphic PGyokaisho Light JIS";
const TCHAR * g_pszAltFontName023 = L"Arphic PKaisho Medium JIS";
const TCHAR * g_pszAltFontName024 = L"Arphic PKanteiryu Heavy JIS";
const TCHAR * g_pszAltFontName025 = L"Arphic PKoin-Tai Bold JIS";
const TCHAR * g_pszAltFontName026 = L"Arphic PKuro-Maru-POP Heavy JIS";
const TCHAR * g_pszAltFontName027 = L"Arphic PPengyokaisho Light JIS";
const TCHAR * g_pszAltFontName028 = L"Arphic PPenkaisho Light JIS";
const TCHAR * g_pszAltFontName029 = L"Arphic PReisho Medium JIS";
const TCHAR * g_pszAltFontName030 = L"Arphic PSiro-Maru-POP Heavy JIS";
const TCHAR * g_pszAltFontName031 = L"Arphic Reisho Medium JIS";
const TCHAR * g_pszAltFontName032 = L"Arphic Siro-Maru-POP Heavy JIS";
const TCHAR * g_pszAltFontName033 = L"AR\x30da\x30f3\x6977\x66f8\x4f53L";
const TCHAR * g_pszAltFontName034 = L"AR\x30da\x30f3\x884c\x6977\x66f8\x4f53 L";
const TCHAR * g_pszAltFontName035 = L"AR\x52d8\x4ead\x6d41H";
const TCHAR * g_pszAltFontName036 = L"AR\x53e4\x5370\x4f53\x0042";
const TCHAR * g_pszAltFontName037 = L"AR\x6977\x66f8\x4f53 M";
const TCHAR * g_pszAltFontName038 = L"AR\x767d\x4e38\xff30\xff2f\xff30\x4f53H";
const TCHAR * g_pszAltFontName039 = L"AR\x884c\x6977\x66f8\x4f53H";
const TCHAR * g_pszAltFontName040 = L"AR\x884c\x6977\x66f8\x4f53L";
const TCHAR * g_pszAltFontName041 = L"AR\x884c\x6977\x9023\x7dbf\x4f53H";
const TCHAR * g_pszAltFontName042 = L"AR\x884c\x6977\x9023\x7dbf\x4f53L";
const TCHAR * g_pszAltFontName043 = L"AR\x96b7\x66f8\x4f53 M";
const TCHAR * g_pszAltFontName044 = L"AR\x9ed2\x4e38\xff30\xff2f\xff30\x4f53H";
const TCHAR * g_pszAltFontName045 = L"Batang";
const TCHAR * g_pszAltFontName046 = L"BatangChe";
const TCHAR * g_pszAltFontName047 = L"DFGothic-EB";
const TCHAR * g_pszAltFontName048 = L"DFKai-SB";
const TCHAR * g_pszAltFontName049 = L"DFLiHeiBold";
const TCHAR * g_pszAltFontName050 = L"DFLiHeiBold(P)";
const TCHAR * g_pszAltFontName051 = L"DFPGothic-EB";
const TCHAR * g_pszAltFontName052 = L"DFPOP-SB";
const TCHAR * g_pszAltFontName053 = L"DFPPOP-SB";
const TCHAR * g_pszAltFontName054 = L"Dotum";
const TCHAR * g_pszAltFontName055 = L"DotumChe";
const TCHAR * g_pszAltFontName056 = L"Expo M";
const TCHAR * g_pszAltFontName057 = L"FangSong_GB2312";
const TCHAR * g_pszAltFontName058 = L"FZShuTi";
const TCHAR * g_pszAltFontName059 = L"FZYaoTi";
const TCHAR * g_pszAltFontName060 = L"Gulim";
const TCHAR * g_pszAltFontName061 = L"GulimChe";
const TCHAR * g_pszAltFontName062 = L"Gungsuh";
const TCHAR * g_pszAltFontName063 = L"GungsuhChe";
const TCHAR * g_pszAltFontName064 = L"Headline R";
const TCHAR * g_pszAltFontName065 = L"Headline Sans R";
const TCHAR * g_pszAltFontName066 = L"HGGothicE";
const TCHAR * g_pszAltFontName067 = L"HGGothicM";
const TCHAR * g_pszAltFontName068 = L"HGGyoshotai";
const TCHAR * g_pszAltFontName069 = L"HGKyokashotai";
const TCHAR * g_pszAltFontName070 = L"HGMinchoB";
const TCHAR * g_pszAltFontName071 = L"HGMinchoE";
const TCHAR * g_pszAltFontName072 = L"HGPGothicE";
const TCHAR * g_pszAltFontName073 = L"HGPGothicM";
const TCHAR * g_pszAltFontName074 = L"HGPGyoshotai";
const TCHAR * g_pszAltFontName075 = L"HGPKyokashotai";
const TCHAR * g_pszAltFontName076 = L"HGPMinchoB";
const TCHAR * g_pszAltFontName077 = L"HGPMinchoE";
const TCHAR * g_pszAltFontName078 = L"HGPSoeiKakugothicUB";
const TCHAR * g_pszAltFontName079 = L"HGPSoeiKakupoptai";
const TCHAR * g_pszAltFontName080 = L"HGPSoeiPresenceEB";
const TCHAR * g_pszAltFontName081 = L"HGP\x5275\x82f1\x89d2\xff7a\xff9e\xff7c\xff6f\xff78UB";
const TCHAR * g_pszAltFontName082 = L"HGP\x5275\x82f1\x89d2\xff8e\xff9f\xff6f\xff8c\xff9f\x4f53";
const TCHAR * g_pszAltFontName083 = L"HGP\x5275\x82f1\xff8c\xff9f\xff9a\xff7e\xff9e\xff9d\xff7d\x0045\x0042";
const TCHAR * g_pszAltFontName084 = L"HGP\x6559\x79d1\x66f8\x4f53";
const TCHAR * g_pszAltFontName085 = L"HGP\x660e\x671d\x0042";
const TCHAR * g_pszAltFontName086 = L"HGP\x660e\x671d\x0045";
const TCHAR * g_pszAltFontName087 = L"HGP\x884c\x66f8\x4f53";
const TCHAR * g_pszAltFontName088 = L"HGP\xff7a\xff9e\xff7c\xff6f\xff78\x0045";
const TCHAR * g_pszAltFontName089 = L"HGP\xff7a\xff9e\xff7c\xff6f\xff78M";
const TCHAR * g_pszAltFontName090 = L"HGSGothicE";
const TCHAR * g_pszAltFontName091 = L"HGSGothicM";
const TCHAR * g_pszAltFontName092 = L"HGSGyoshotai";
const TCHAR * g_pszAltFontName093 = L"HGSKyokashotai";
const TCHAR * g_pszAltFontName094 = L"HGSMinchoB";
const TCHAR * g_pszAltFontName095 = L"HGSMinchoE";
const TCHAR * g_pszAltFontName096 = L"HGSoeiKakugothicUB";
const TCHAR * g_pszAltFontName097 = L"HGSoeiKakupoptai";
const TCHAR * g_pszAltFontName098 = L"HGSoeiPresenceEB";
const TCHAR * g_pszAltFontName099 = L"HGSSoeiKakugothicUB";
const TCHAR * g_pszAltFontName100 = L"HGSSoeiKakupoptai";
const TCHAR * g_pszAltFontName101 = L"HGSSoeiPresenceEB";
const TCHAR * g_pszAltFontName102 = L"HGS\x5275\x82f1\x89d2\xff7a\xff9e\xff7c\xff6f\xff78UB";
const TCHAR * g_pszAltFontName103 = L"HGS\x5275\x82f1\x89d2\xff8e\xff9f\xff6f\xff8c\xff9f\x4f53";
const TCHAR * g_pszAltFontName104 = L"HGS\x5275\x82f1\xff8c\xff9f\xff9a\xff7e\xff9e\xff9d\xff7d\x0045\x0042";
const TCHAR * g_pszAltFontName105 = L"HGS\x6559\x79d1\x66f8\x4f53";
const TCHAR * g_pszAltFontName106 = L"HGS\x660e\x671d\x0042";
const TCHAR * g_pszAltFontName107 = L"HGS\x660e\x671d\x0045";
const TCHAR * g_pszAltFontName108 = L"HGS\x884c\x66f8\x4f53";
const TCHAR * g_pszAltFontName109 = L"HGS\xff7a\xff9e\xff7c\xff6f\xff78\x0045";
const TCHAR * g_pszAltFontName110 = L"HGS\xff7a\xff9e\xff7c\xff6f\xff78M";
const TCHAR * g_pszAltFontName111 = L"HG\x5275\x82f1\x89d2\xff7a\xff9e\xff7c\xff6f\xff78UB";
const TCHAR * g_pszAltFontName112 = L"HG\x5275\x82f1\x89d2\xff8e\xff9f\xff6f\xff8c\xff9f\x4f53";
const TCHAR * g_pszAltFontName113 = L"HG\x5275\x82f1\xff8c\xff9f\xff9a\xff7e\xff9e\xff9d\xff7d\x0045\x0042";
const TCHAR * g_pszAltFontName114 = L"HG\x6559\x79d1\x66f8\x4f53";
const TCHAR * g_pszAltFontName115 = L"HG\x660e\x671d\x0042";
const TCHAR * g_pszAltFontName116 = L"HG\x660e\x671d\x0045";
const TCHAR * g_pszAltFontName117 = L"HG\x884c\x66f8\x4f53";
const TCHAR * g_pszAltFontName118 = L"HG\xff7a\xff9e\xff7c\xff6f\xff78\x0045";
const TCHAR * g_pszAltFontName119 = L"HG\xff7a\xff9e\xff7c\xff6f\xff78M";
const TCHAR * g_pszAltFontName120 = L"HYGothic-Extra";
const TCHAR * g_pszAltFontName121 = L"HYMyeongJo-Extra";
const TCHAR * g_pszAltFontName122 = L"HYPMokGak-Bold";
const TCHAR * g_pszAltFontName123 = L"HYPost-Medium";
const TCHAR * g_pszAltFontName124 = L"HYShortSamul-Medium";
const TCHAR * g_pszAltFontName125 = L"HYSinMun-MyeongJo";
const TCHAR * g_pszAltFontName126 = L"HYTaJa-Medium";
const TCHAR * g_pszAltFontName127 = L"HY\xacac\xace0\xb515";
const TCHAR * g_pszAltFontName128 = L"HY\xacac\xba85\xc870";
const TCHAR * g_pszAltFontName129 = L"HY\xbaa9\xac01\xd30c\xc784\x0042";
const TCHAR * g_pszAltFontName130 = L"HY\xc2e0\xbb38\xba85\xc870";
const TCHAR * g_pszAltFontName131 = L"HY\xc595\xc740\xc0d8\xbb3cM";
const TCHAR * g_pszAltFontName132 = L"HY\xc5fd\xc11cM";
const TCHAR * g_pszAltFontName133 = L"HY\xd0c0\xc790M";
const TCHAR * g_pszAltFontName134 = L"KaiTi_GB2312";
const TCHAR * g_pszAltFontName135 = L"LiSu";
const TCHAR * g_pszAltFontName136 = L"MingLiU";
const TCHAR * g_pszAltFontName137 = L"MoeumT R";
const TCHAR * g_pszAltFontName138 = L"MS Gothic";
const TCHAR * g_pszAltFontName139 = L"MS Mincho";
const TCHAR * g_pszAltFontName140 = L"MS PGothic";
const TCHAR * g_pszAltFontName141 = L"MS PMincho";
const TCHAR * g_pszAltFontName142 = L"NSimSun";
const TCHAR * g_pszAltFontName143 = L"PMingLiU";
const TCHAR * g_pszAltFontName144 = L"Pyunji R";
const TCHAR * g_pszAltFontName145 = L"SimHei";
const TCHAR * g_pszAltFontName146 = L"SimSun";
const TCHAR * g_pszAltFontName147 = L"STCaiyun";
const TCHAR * g_pszAltFontName148 = L"STFangsong";
const TCHAR * g_pszAltFontName149 = L"STHupo";
const TCHAR * g_pszAltFontName150 = L"STKaii";
const TCHAR * g_pszAltFontName151 = L"STLiti";
const TCHAR * g_pszAltFontName152 = L"STSong";
const TCHAR * g_pszAltFontName153 = L"STXihei";
const TCHAR * g_pszAltFontName154 = L"STXingkai";
const TCHAR * g_pszAltFontName155 = L"STXinwei";
const TCHAR * g_pszAltFontName156 = L"STZhongsong";
const TCHAR * g_pszAltFontName157 = L"Yet R";
const TCHAR * g_pszAltFontName158 = L"YouYuan";
const TCHAR * g_pszAltFontName159 = L"\x4eff\x5b8b_GBG2312";
const TCHAR * g_pszAltFontName160 = L"\x534e\x6587\x4e2d\x5b8b";
const TCHAR * g_pszAltFontName161 = L"\x534e\x6587\x4eff\x5b8b";
const TCHAR * g_pszAltFontName162 = L"\x534e\x6587\x5b8b\x4f53";
const TCHAR * g_pszAltFontName163 = L"\x534e\x6587\x5f69\x4e91";
const TCHAR * g_pszAltFontName164 = L"\x534e\x6587\x65b0\x9b4f";
const TCHAR * g_pszAltFontName165 = L"\x534e\x6587\x6977\x4f53";
const TCHAR * g_pszAltFontName166 = L"\x534e\x6587\x7425\x73c0";
const TCHAR * g_pszAltFontName167 = L"\x534e\x6587\x7ec6\x9ed1";
const TCHAR * g_pszAltFontName168 = L"\x534e\x6587\x884c\x6977";
const TCHAR * g_pszAltFontName169 = L"\x534e\x6587\x96b6\x4e66";
const TCHAR * g_pszAltFontName170 = L"\x5b8b\x4f53";
const TCHAR * g_pszAltFontName171 = L"\x5e7c\x5706";
const TCHAR * g_pszAltFontName172 = L"\x65b0\x5b8b\x4f53";
const TCHAR * g_pszAltFontName173 = L"\x65b0\x7d30\x660e\x9ad4";
const TCHAR * g_pszAltFontName174 = L"\x65b9\x6b63\x59da\x4f53\x7b80\x4f53";
const TCHAR * g_pszAltFontName175 = L"\x65b9\x6b63\x8212\x4f53\x7b80\x4f53";
const TCHAR * g_pszAltFontName176 = L"\x6977\x4f53_GBG2312";
const TCHAR * g_pszAltFontName177 = L"\x6a19\x6977\x9ad4";
const TCHAR * g_pszAltFontName178 = L"\x7d30\x660e\x9ad4";
const TCHAR * g_pszAltFontName179 = L"\x83ef\x5eb7\x5137\x7c97\x9ed1";
const TCHAR * g_pszAltFontName180 = L"\x83ef\x5eb7\x65b0\x5137\x7c97\x9ed1";
const TCHAR * g_pszAltFontName181 = L"\x96b6\x4e66";
const TCHAR * g_pszAltFontName182 = L"\x9ed1\x4f53";
const TCHAR * g_pszAltFontName183 = L"\xad74\xb9bc";
const TCHAR * g_pszAltFontName184 = L"\xad74\xb9bc\xccb4";
const TCHAR * g_pszAltFontName185 = L"\xad81\xc11c";
const TCHAR * g_pszAltFontName186 = L"\xad81\xc11c\xccb4";
const TCHAR * g_pszAltFontName187 = L"\xb3cb\xc6c0";
const TCHAR * g_pszAltFontName188 = L"\xb3cb\xc6c0\xccb4";
const TCHAR * g_pszAltFontName189 = L"\xbc14\xd0d5";
const TCHAR * g_pszAltFontName190 = L"\xbc14\xd0d5\xccb4";
const TCHAR * g_pszAltFontName191 = L"\xd734\xba3c\xac01\xc9c4\xd5e4\xb4dc\xb77c\xc778";
const TCHAR * g_pszAltFontName192 = L"\xd734\xba3c\xb465\xadfc\xd5e4\xb4dc\xb77c\xc778";
const TCHAR * g_pszAltFontName193 = L"\xd734\xba3c\xbaa8\xc74cT";
const TCHAR * g_pszAltFontName194 = L"\xd734\xba3c\xc544\xbbf8\xccb4";
const TCHAR * g_pszAltFontName195 = L"\xd734\xba3c\xc5d1\xc2a4\xd3ec";
const TCHAR * g_pszAltFontName196 = L"\xd734\xba3c\xc61b\xccb4";
const TCHAR * g_pszAltFontName197 = L"\xd734\xba3c\xd3b8\xc9c0\xccb4";
const TCHAR * g_pszAltFontName198 = L"\xff24\xff26POP\x4f53";
const TCHAR * g_pszAltFontName199 = L"\xff24\xff26\x7279\x592a\x30b4\x30b7\x30c3\x30af\x4f53";
const TCHAR * g_pszAltFontName200 = L"\xff24\xff26\xff30POP\x4f53";
const TCHAR * g_pszAltFontName201 = L"\xff24\xff26\xff30\x7279\x592a\x30b4\x30b7\x30c3\x30af\x4f53";
const TCHAR * g_pszAltFontName202 = L"\xff2d\xff33 \x30b4\x30b7\x30c3\x30af";
const TCHAR * g_pszAltFontName203 = L"\xff2d\xff33 \x660e\x671d";
const TCHAR * g_pszAltFontName204 = L"\xff2d\xff33 \xff30\x30b4\x30b7\x30c3\x30af";
const TCHAR * g_pszAltFontName205 = L"\xff2d\xff33 \xff30\x660e\x671d";

const TCHAR * pszAltFontNames[] = 
{
   g_pszAltFontName000,
   g_pszAltFontName001,
   g_pszAltFontName002,
   g_pszAltFontName003,
   g_pszAltFontName004,
   g_pszAltFontName005,
   g_pszAltFontName006,
   g_pszAltFontName007,
   g_pszAltFontName008,
   g_pszAltFontName009,
   g_pszAltFontName010,
   g_pszAltFontName011,
   g_pszAltFontName012,
   g_pszAltFontName013,
   g_pszAltFontName014,
   g_pszAltFontName015,
   g_pszAltFontName016,
   g_pszAltFontName017,
   g_pszAltFontName018,
   g_pszAltFontName019,
   g_pszAltFontName020,
   g_pszAltFontName021,
   g_pszAltFontName022,
   g_pszAltFontName023,
   g_pszAltFontName024,
   g_pszAltFontName025,
   g_pszAltFontName026,
   g_pszAltFontName027,
   g_pszAltFontName028,
   g_pszAltFontName029,
   g_pszAltFontName030,
   g_pszAltFontName031,
   g_pszAltFontName032,
   g_pszAltFontName033,
   g_pszAltFontName034,
   g_pszAltFontName035,
   g_pszAltFontName036,
   g_pszAltFontName037,
   g_pszAltFontName038,
   g_pszAltFontName039,
   g_pszAltFontName040,
   g_pszAltFontName041,
   g_pszAltFontName042,
   g_pszAltFontName043,
   g_pszAltFontName044,
   g_pszAltFontName045,
   g_pszAltFontName046,
   g_pszAltFontName047,
   g_pszAltFontName048,
   g_pszAltFontName049,
   g_pszAltFontName050,
   g_pszAltFontName051,
   g_pszAltFontName052,
   g_pszAltFontName053,
   g_pszAltFontName054,
   g_pszAltFontName055,
   g_pszAltFontName056,
   g_pszAltFontName057,
   g_pszAltFontName058,
   g_pszAltFontName059,
   g_pszAltFontName060,
   g_pszAltFontName061,
   g_pszAltFontName062,
   g_pszAltFontName063,
   g_pszAltFontName064,
   g_pszAltFontName065,
   g_pszAltFontName066,
   g_pszAltFontName067,
   g_pszAltFontName068,
   g_pszAltFontName069,
   g_pszAltFontName070,
   g_pszAltFontName071,
   g_pszAltFontName072,
   g_pszAltFontName073,
   g_pszAltFontName074,
   g_pszAltFontName075,
   g_pszAltFontName076,
   g_pszAltFontName077,
   g_pszAltFontName078,
   g_pszAltFontName079,
   g_pszAltFontName080,
   g_pszAltFontName081,
   g_pszAltFontName082,
   g_pszAltFontName083,
   g_pszAltFontName084,
   g_pszAltFontName085,
   g_pszAltFontName086,
   g_pszAltFontName087,
   g_pszAltFontName088,
   g_pszAltFontName089,
   g_pszAltFontName090,
   g_pszAltFontName091,
   g_pszAltFontName092,
   g_pszAltFontName093,
   g_pszAltFontName094,
   g_pszAltFontName095,
   g_pszAltFontName096,
   g_pszAltFontName097,
   g_pszAltFontName098,
   g_pszAltFontName099,
   g_pszAltFontName100,
   g_pszAltFontName101,
   g_pszAltFontName102,
   g_pszAltFontName103,
   g_pszAltFontName104,
   g_pszAltFontName105,
   g_pszAltFontName106,
   g_pszAltFontName107,
   g_pszAltFontName108,
   g_pszAltFontName109,
   g_pszAltFontName110,
   g_pszAltFontName111,
   g_pszAltFontName112,
   g_pszAltFontName113,
   g_pszAltFontName114,
   g_pszAltFontName115,
   g_pszAltFontName116,
   g_pszAltFontName117,
   g_pszAltFontName118,
   g_pszAltFontName119,
   g_pszAltFontName120,
   g_pszAltFontName121,
   g_pszAltFontName122,
   g_pszAltFontName123,
   g_pszAltFontName124,
   g_pszAltFontName125,
   g_pszAltFontName126,
   g_pszAltFontName127,
   g_pszAltFontName128,
   g_pszAltFontName129,
   g_pszAltFontName130,
   g_pszAltFontName131,
   g_pszAltFontName132,
   g_pszAltFontName133,
   g_pszAltFontName134,
   g_pszAltFontName135,
   g_pszAltFontName136,
   g_pszAltFontName137,
   g_pszAltFontName138,
   g_pszAltFontName139,
   g_pszAltFontName140,
   g_pszAltFontName141,
   g_pszAltFontName142,
   g_pszAltFontName143,
   g_pszAltFontName144,
   g_pszAltFontName145,
   g_pszAltFontName146,
   g_pszAltFontName147,
   g_pszAltFontName148,
   g_pszAltFontName149,
   g_pszAltFontName150,
   g_pszAltFontName151,
   g_pszAltFontName152,
   g_pszAltFontName153,
   g_pszAltFontName154,
   g_pszAltFontName155,
   g_pszAltFontName156,
   g_pszAltFontName157,
   g_pszAltFontName158,
   g_pszAltFontName159,
   g_pszAltFontName160,
   g_pszAltFontName161,
   g_pszAltFontName162,
   g_pszAltFontName163,
   g_pszAltFontName164,
   g_pszAltFontName165,
   g_pszAltFontName166,
   g_pszAltFontName167,
   g_pszAltFontName168,
   g_pszAltFontName169,
   g_pszAltFontName170,
   g_pszAltFontName171,
   g_pszAltFontName172,
   g_pszAltFontName173,
   g_pszAltFontName174,
   g_pszAltFontName175,
   g_pszAltFontName176,
   g_pszAltFontName177,
   g_pszAltFontName178,
   g_pszAltFontName179,
   g_pszAltFontName180,
   g_pszAltFontName181,
   g_pszAltFontName182,
   g_pszAltFontName183,
   g_pszAltFontName184,
   g_pszAltFontName185,
   g_pszAltFontName186,
   g_pszAltFontName187,
   g_pszAltFontName188,
   g_pszAltFontName189,
   g_pszAltFontName190,
   g_pszAltFontName191,
   g_pszAltFontName192,
   g_pszAltFontName193,
   g_pszAltFontName194,
   g_pszAltFontName195,
   g_pszAltFontName196,
   g_pszAltFontName197,
   g_pszAltFontName198,
   g_pszAltFontName199,
   g_pszAltFontName200,
   g_pszAltFontName201,
   g_pszAltFontName202,
   g_pszAltFontName203,
   g_pszAltFontName204,
   g_pszAltFontName205,
};

const TCHAR * pszAltFontNamesAlt[] = 
{
   g_pszAltFontName194,
   g_pszAltFontName028,
   g_pszAltFontName027,
   g_pszAltFontName024,
   g_pszAltFontName025,
   g_pszAltFontName023,
   g_pszAltFontName030,
   g_pszAltFontName021,
   g_pszAltFontName022,
   g_pszAltFontName029,
   g_pszAltFontName026,
   g_pszAltFontName041,
   g_pszAltFontName042,
   g_pszAltFontName039,
   g_pszAltFontName040,
   g_pszAltFontName037,
   g_pszAltFontName035,
   g_pszAltFontName036,
   g_pszAltFontName044,
   g_pszAltFontName034,
   g_pszAltFontName033,
   g_pszAltFontName007,
   g_pszAltFontName008,
   g_pszAltFontName005,
   g_pszAltFontName003,
   g_pszAltFontName004,
   g_pszAltFontName010,
   g_pszAltFontName002,
   g_pszAltFontName001,
   g_pszAltFontName009,
   g_pszAltFontName006,
   g_pszAltFontName043,
   g_pszAltFontName038,
   g_pszAltFontName020,
   g_pszAltFontName019,
   g_pszAltFontName016,
   g_pszAltFontName017,
   g_pszAltFontName015,
   g_pszAltFontName032,
   g_pszAltFontName013,
   g_pszAltFontName014,
   g_pszAltFontName011,
   g_pszAltFontName012,
   g_pszAltFontName031,
   g_pszAltFontName018,
   g_pszAltFontName189,
   g_pszAltFontName190,
   g_pszAltFontName199,
   g_pszAltFontName177,
   g_pszAltFontName179,
   g_pszAltFontName180,
   g_pszAltFontName201,
   g_pszAltFontName198,
   g_pszAltFontName200,
   g_pszAltFontName187,
   g_pszAltFontName188,
   g_pszAltFontName195,
   g_pszAltFontName159,
   g_pszAltFontName175,
   g_pszAltFontName174,
   g_pszAltFontName183,
   g_pszAltFontName184,
   g_pszAltFontName185,
   g_pszAltFontName186,
   g_pszAltFontName192,
   g_pszAltFontName191,
   g_pszAltFontName118,
   g_pszAltFontName119,
   g_pszAltFontName117,
   g_pszAltFontName114,
   g_pszAltFontName115,
   g_pszAltFontName116,
   g_pszAltFontName088,
   g_pszAltFontName089,
   g_pszAltFontName087,
   g_pszAltFontName084,
   g_pszAltFontName085,
   g_pszAltFontName086,
   g_pszAltFontName081,
   g_pszAltFontName082,
   g_pszAltFontName083,
   g_pszAltFontName078,
   g_pszAltFontName079,
   g_pszAltFontName080,
   g_pszAltFontName075,
   g_pszAltFontName076,
   g_pszAltFontName077,
   g_pszAltFontName074,
   g_pszAltFontName072,
   g_pszAltFontName073,
   g_pszAltFontName109,
   g_pszAltFontName110,
   g_pszAltFontName108,
   g_pszAltFontName105,
   g_pszAltFontName106,
   g_pszAltFontName107,
   g_pszAltFontName111,
   g_pszAltFontName112,
   g_pszAltFontName113,
   g_pszAltFontName102,
   g_pszAltFontName103,
   g_pszAltFontName104,
   g_pszAltFontName099,
   g_pszAltFontName100,
   g_pszAltFontName101,
   g_pszAltFontName093,
   g_pszAltFontName094,
   g_pszAltFontName095,
   g_pszAltFontName092,
   g_pszAltFontName090,
   g_pszAltFontName091,
   g_pszAltFontName096,
   g_pszAltFontName097,
   g_pszAltFontName098,
   g_pszAltFontName069,
   g_pszAltFontName070,
   g_pszAltFontName071,
   g_pszAltFontName068,
   g_pszAltFontName066,
   g_pszAltFontName067,
   g_pszAltFontName127,
   g_pszAltFontName128,
   g_pszAltFontName129,
   g_pszAltFontName132,
   g_pszAltFontName131,
   g_pszAltFontName130,
   g_pszAltFontName133,
   g_pszAltFontName120,
   g_pszAltFontName121,
   g_pszAltFontName122,
   g_pszAltFontName125,
   g_pszAltFontName124,
   g_pszAltFontName123,
   g_pszAltFontName126,
   g_pszAltFontName176,
   g_pszAltFontName181,
   g_pszAltFontName178,
   g_pszAltFontName193,
   g_pszAltFontName202,
   g_pszAltFontName203,
   g_pszAltFontName204,
   g_pszAltFontName205,
   g_pszAltFontName172,
   g_pszAltFontName173,
   g_pszAltFontName197,
   g_pszAltFontName182,
   g_pszAltFontName170,
   g_pszAltFontName163,
   g_pszAltFontName161,
   g_pszAltFontName166,
   g_pszAltFontName165,
   g_pszAltFontName169,
   g_pszAltFontName162,
   g_pszAltFontName167,
   g_pszAltFontName168,
   g_pszAltFontName164,
   g_pszAltFontName160,
   g_pszAltFontName196,
   g_pszAltFontName171,
   g_pszAltFontName057,
   g_pszAltFontName156,
   g_pszAltFontName148,
   g_pszAltFontName152,
   g_pszAltFontName147,
   g_pszAltFontName155,
   g_pszAltFontName150,
   g_pszAltFontName149,
   g_pszAltFontName153,
   g_pszAltFontName154,
   g_pszAltFontName151,
   g_pszAltFontName146,
   g_pszAltFontName158,
   g_pszAltFontName142,
   g_pszAltFontName143,
   g_pszAltFontName059,
   g_pszAltFontName058,
   g_pszAltFontName134,
   g_pszAltFontName048,
   g_pszAltFontName136,
   g_pszAltFontName049,
   g_pszAltFontName050,
   g_pszAltFontName135,
   g_pszAltFontName145,
   g_pszAltFontName060,
   g_pszAltFontName061,
   g_pszAltFontName062,
   g_pszAltFontName063,
   g_pszAltFontName054,
   g_pszAltFontName055,
   g_pszAltFontName045,
   g_pszAltFontName046,
   g_pszAltFontName065,
   g_pszAltFontName064,
   g_pszAltFontName137,
   g_pszAltFontName000,
   g_pszAltFontName056,
   g_pszAltFontName157,
   g_pszAltFontName144,
   g_pszAltFontName052,
   g_pszAltFontName047,
   g_pszAltFontName053,
   g_pszAltFontName051,
   g_pszAltFontName138,
   g_pszAltFontName139,
   g_pszAltFontName140,
   g_pszAltFontName141,
};

//+----------------------------------------------------------------------------
//
//  Function:   CompareStringFunction
//
//  Synopsis:   Comparison function used by the bsearch call in the function
//              AlternateFontName.  Note that we call StrCmpIC, so the only
//              variation we recognize is case in the ASCII range.  We *do
//              not*, for example, treat narrow and wide variations or kana
//              variations as equivalent.
//
//-----------------------------------------------------------------------------

int __cdecl
CompareStringFunction( const void * v0, const void * v1)
{
    return StrCmpIC( *(WCHAR **)v0, *(WCHAR **)v1 );
}

//+----------------------------------------------------------------------------
//
//  Function:   AlternateFontName
//
//  Synopsis:   Fonts often have two names associated with them, an ASCII
//              name and a Native name.  This is most often true on Asian
//              systems.  GDI, however, is not smart enough to recognize
//              alternate names as equivalent.  We therefore have a hard-
//              coded table (courtesy of the Office group) to help GDI out.
//
//  Returns:    Alternate font name, or NULL, if none is known.
//
//-----------------------------------------------------------------------------

const WCHAR *
AlternateFontName( const WCHAR * pszName )
{
    WCHAR **pszNameT = (WCHAR **)bsearch( (const void *)&pszName,
                                          (const void *)&pszAltFontNames,
                                          sizeof(pszAltFontNames) / sizeof(WCHAR *),
                                          sizeof(WCHAR *),
                                          CompareStringFunction );

    return pszNameT
            ? pszAltFontNamesAlt[pszNameT - pszAltFontNames]
            : NULL;
}

const WCHAR *
AlternateFontNameIfAvailable( const WCHAR * pszName )
{
    WCHAR **pszNameT = (WCHAR **)bsearch( (const void *)&pszName,
                                          (const void *)&pszAltFontNames,
                                          sizeof(pszAltFontNames) / sizeof(WCHAR *),
                                          sizeof(WCHAR *),
                                          CompareStringFunction );

    return pszNameT
            ? pszAltFontNamesAlt[pszNameT - pszAltFontNames]
            : pszName;
}
