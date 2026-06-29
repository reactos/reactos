/*
 * vtbl thunks
 *
 * Copyright 2009, 2023 Alexandre Julliard
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

#if 0
#pragma makedep arm64ec_x64
#endif

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "rpcproxy.h"
#include "cpsf.h"
#include "ndrtypes.h"
#include "ndr_stubless.h"
#include "wine/asm.h"

#define ALL_THUNK_ENTRIES \
    T(3) T(4) T(5) T(6) T(7) T(8) T(9) T(10) T(11) T(12) T(13) T(14) T(15) \
    T(16) T(17) T(18) T(19) T(20) T(21) T(22) T(23) T(24) T(25) T(26) T(27) T(28) T(29) T(30) T(31) \
    T(32) T(33) T(34) T(35) T(36) T(37) T(38) T(39) T(40) T(41) T(42) T(43) T(44) T(45) T(46) T(47) \
    T(48) T(49) T(50) T(51) T(52) T(53) T(54) T(55) T(56) T(57) T(58) T(59) T(60) T(61) T(62) T(63) \
    T(64) T(65) T(66) T(67) T(68) T(69) T(70) T(71) T(72) T(73) T(74) T(75) T(76) T(77) T(78) T(79) \
    T(80) T(81) T(82) T(83) T(84) T(85) T(86) T(87) T(88) T(89) T(90) T(91) T(92) T(93) T(94) T(95) \
    T(96) T(97) T(98) T(99) T(100) T(101) T(102) T(103) T(104) T(105) T(106) T(107) T(108) T(109) T(110) T(111) \
    T(112) T(113) T(114) T(115) T(116) T(117) T(118) T(119) T(120) T(121) T(122) T(123) T(124) T(125) T(126) T(127) \
    T(128) T(129) T(130) T(131) T(132) T(133) T(134) T(135) T(136) T(137) T(138) T(139) T(140) T(141) T(142) T(143) \
    T(144) T(145) T(146) T(147) T(148) T(149) T(150) T(151) T(152) T(153) T(154) T(155) T(156) T(157) T(158) T(159) \
    T(160) T(161) T(162) T(163) T(164) T(165) T(166) T(167) T(168) T(169) T(170) T(171) T(172) T(173) T(174) T(175) \
    T(176) T(177) T(178) T(179) T(180) T(181) T(182) T(183) T(184) T(185) T(186) T(187) T(188) T(189) T(190) T(191) \
    T(192) T(193) T(194) T(195) T(196) T(197) T(198) T(199) T(200) T(201) T(202) T(203) T(204) T(205) T(206) T(207) \
    T(208) T(209) T(210) T(211) T(212) T(213) T(214) T(215) T(216) T(217) T(218) T(219) T(220) T(221) T(222) T(223) \
    T(224) T(225) T(226) T(227) T(228) T(229) T(230) T(231) T(232) T(233) T(234) T(235) T(236) T(237) T(238) T(239) \
    T(240) T(241) T(242) T(243) T(244) T(245) T(246) T(247) T(248) T(249) T(250) T(251) T(252) T(253) T(254) T(255) \
    T(256) T(257) T(258) T(259) T(260) T(261) T(262) T(263) T(264) T(265) T(266) T(267) T(268) T(269) T(270) T(271) \
    T(272) T(273) T(274) T(275) T(276) T(277) T(278) T(279) T(280) T(281) T(282) T(283) T(284) T(285) T(286) T(287) \
    T(288) T(289) T(290) T(291) T(292) T(293) T(294) T(295) T(296) T(297) T(298) T(299) T(300) T(301) T(302) T(303) \
    T(304) T(305) T(306) T(307) T(308) T(309) T(310) T(311) T(312) T(313) T(314) T(315) T(316) T(317) T(318) T(319) \
    T(320) T(321) T(322) T(323) T(324) T(325) T(326) T(327) T(328) T(329) T(330) T(331) T(332) T(333) T(334) T(335) \
    T(336) T(337) T(338) T(339) T(340) T(341) T(342) T(343) T(344) T(345) T(346) T(347) T(348) T(349) T(350) T(351) \
    T(352) T(353) T(354) T(355) T(356) T(357) T(358) T(359) T(360) T(361) T(362) T(363) T(364) T(365) T(366) T(367) \
    T(368) T(369) T(370) T(371) T(372) T(373) T(374) T(375) T(376) T(377) T(378) T(379) T(380) T(381) T(382) T(383) \
    T(384) T(385) T(386) T(387) T(388) T(389) T(390) T(391) T(392) T(393) T(394) T(395) T(396) T(397) T(398) T(399) \
    T(400) T(401) T(402) T(403) T(404) T(405) T(406) T(407) T(408) T(409) T(410) T(411) T(412) T(413) T(414) T(415) \
    T(416) T(417) T(418) T(419) T(420) T(421) T(422) T(423) T(424) T(425) T(426) T(427) T(428) T(429) T(430) T(431) \
    T(432) T(433) T(434) T(435) T(436) T(437) T(438) T(439) T(440) T(441) T(442) T(443) T(444) T(445) T(446) T(447) \
    T(448) T(449) T(450) T(451) T(452) T(453) T(454) T(455) T(456) T(457) T(458) T(459) T(460) T(461) T(462) T(463) \
    T(464) T(465) T(466) T(467) T(468) T(469) T(470) T(471) T(472) T(473) T(474) T(475) T(476) T(477) T(478) T(479) \
    T(480) T(481) T(482) T(483) T(484) T(485) T(486) T(487) T(488) T(489) T(490) T(491) T(492) T(493) T(494) T(495) \
    T(496) T(497) T(498) T(499) T(500) T(501) T(502) T(503) T(504) T(505) T(506) T(507) T(508) T(509) T(510) T(511) \
    T(512) T(513) T(514) T(515) T(516) T(517) T(518) T(519) T(520) T(521) T(522) T(523) T(524) T(525) T(526) T(527) \
    T(528) T(529) T(530) T(531) T(532) T(533) T(534) T(535) T(536) T(537) T(538) T(539) T(540) T(541) T(542) T(543) \
    T(544) T(545) T(546) T(547) T(548) T(549) T(550) T(551) T(552) T(553) T(554) T(555) T(556) T(557) T(558) T(559) \
    T(560) T(561) T(562) T(563) T(564) T(565) T(566) T(567) T(568) T(569) T(570) T(571) T(572) T(573) T(574) T(575) \
    T(576) T(577) T(578) T(579) T(580) T(581) T(582) T(583) T(584) T(585) T(586) T(587) T(588) T(589) T(590) T(591) \
    T(592) T(593) T(594) T(595) T(596) T(597) T(598) T(599) T(600) T(601) T(602) T(603) T(604) T(605) T(606) T(607) \
    T(608) T(609) T(610) T(611) T(612) T(613) T(614) T(615) T(616) T(617) T(618) T(619) T(620) T(621) T(622) T(623) \
    T(624) T(625) T(626) T(627) T(628) T(629) T(630) T(631) T(632) T(633) T(634) T(635) T(636) T(637) T(638) T(639) \
    T(640) T(641) T(642) T(643) T(644) T(645) T(646) T(647) T(648) T(649) T(650) T(651) T(652) T(653) T(654) T(655) \
    T(656) T(657) T(658) T(659) T(660) T(661) T(662) T(663) T(664) T(665) T(666) T(667) T(668) T(669) T(670) T(671) \
    T(672) T(673) T(674) T(675) T(676) T(677) T(678) T(679) T(680) T(681) T(682) T(683) T(684) T(685) T(686) T(687) \
    T(688) T(689) T(690) T(691) T(692) T(693) T(694) T(695) T(696) T(697) T(698) T(699) T(700) T(701) T(702) T(703) \
    T(704) T(705) T(706) T(707) T(708) T(709) T(710) T(711) T(712) T(713) T(714) T(715) T(716) T(717) T(718) T(719) \
    T(720) T(721) T(722) T(723) T(724) T(725) T(726) T(727) T(728) T(729) T(730) T(731) T(732) T(733) T(734) T(735) \
    T(736) T(737) T(738) T(739) T(740) T(741) T(742) T(743) T(744) T(745) T(746) T(747) T(748) T(749) T(750) T(751) \
    T(752) T(753) T(754) T(755) T(756) T(757) T(758) T(759) T(760) T(761) T(762) T(763) T(764) T(765) T(766) T(767) \
    T(768) T(769) T(770) T(771) T(772) T(773) T(774) T(775) T(776) T(777) T(778) T(779) T(780) T(781) T(782) T(783) \
    T(784) T(785) T(786) T(787) T(788) T(789) T(790) T(791) T(792) T(793) T(794) T(795) T(796) T(797) T(798) T(799) \
    T(800) T(801) T(802) T(803) T(804) T(805) T(806) T(807) T(808) T(809) T(810) T(811) T(812) T(813) T(814) T(815) \
    T(816) T(817) T(818) T(819) T(820) T(821) T(822) T(823) T(824) T(825) T(826) T(827) T(828) T(829) T(830) T(831) \
    T(832) T(833) T(834) T(835) T(836) T(837) T(838) T(839) T(840) T(841) T(842) T(843) T(844) T(845) T(846) T(847) \
    T(848) T(849) T(850) T(851) T(852) T(853) T(854) T(855) T(856) T(857) T(858) T(859) T(860) T(861) T(862) T(863) \
    T(864) T(865) T(866) T(867) T(868) T(869) T(870) T(871) T(872) T(873) T(874) T(875) T(876) T(877) T(878) T(879) \
    T(880) T(881) T(882) T(883) T(884) T(885) T(886) T(887) T(888) T(889) T(890) T(891) T(892) T(893) T(894) T(895) \
    T(896) T(897) T(898) T(899) T(900) T(901) T(902) T(903) T(904) T(905) T(906) T(907) T(908) T(909) T(910) T(911) \
    T(912) T(913) T(914) T(915) T(916) T(917) T(918) T(919) T(920) T(921) T(922) T(923) T(924) T(925) T(926) T(927) \
    T(928) T(929) T(930) T(931) T(932) T(933) T(934) T(935) T(936) T(937) T(938) T(939) T(940) T(941) T(942) T(943) \
    T(944) T(945) T(946) T(947) T(948) T(949) T(950) T(951) T(952) T(953) T(954) T(955) T(956) T(957) T(958) T(959) \
    T(960) T(961) T(962) T(963) T(964) T(965) T(966) T(967) T(968) T(969) T(970) T(971) T(972) T(973) T(974) T(975) \
    T(976) T(977) T(978) T(979) T(980) T(981) T(982) T(983) T(984) T(985) T(986) T(987) T(988) T(989) T(990) T(991) \
    T(992) T(993) T(994) T(995) T(996) T(997) T(998) T(999) T(1000) T(1001) T(1002) T(1003) T(1004) T(1005) T(1006) T(1007) \
    T(1008) T(1009) T(1010) T(1011) T(1012) T(1013) T(1014) T(1015) T(1016) T(1017) T(1018) T(1019) T(1020) T(1021) T(1022) T(1023)

#ifdef __i386__

__ASM_GLOBAL_FUNC( call_stubless_func,
                   "movl 4(%esp),%ecx\n\t"         /* This pointer */
                   "movl (%ecx),%ecx\n\t"          /* This->lpVtbl */
                   "movl -8(%ecx),%ecx\n\t"        /* MIDL_STUBLESS_PROXY_INFO */
                   "movl 8(%ecx),%edx\n\t"         /* info->FormatStringOffset */
                   "movzwl (%edx,%eax,2),%edx\n\t" /* FormatStringOffset[index] */
                   "addl 4(%ecx),%edx\n\t"         /* info->ProcFormatString + offset */
                   "movzbl 1(%edx),%eax\n\t"       /* Oi_flags */
                   "andl $0x08,%eax\n\t"           /* Oi_HAS_RPCFLAGS */
                   "shrl $1,%eax\n\t"
                   "movzwl 4(%edx,%eax),%eax\n\t"  /* arguments size */
                   "pushl %eax\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl $0\n\t"                  /* fpu_stack */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "leal 12(%esp),%eax\n\t"        /* &This */
                   "pushl %eax\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl %edx\n\t"                /* format string */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "pushl (%ecx)\n\t"              /* info->pStubDesc */
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   "call " __ASM_STDCALL("NdrpClientCall2",16) "\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -16\n\t")
                   "popl %edx\n\t"                 /* arguments size */
                   __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                   "movl (%esp),%ecx\n\t"  /* return address */
                   "addl %edx,%esp\n\t"
                   "jmp *%ecx" )

#define T(num) \
    ".balign 4\n\t" \
    ".globl " __ASM_NAME("ObjectStublessClient" #num) "\n" \
    __ASM_NAME("ObjectStublessClient" #num) ":\n\t" \
    "movl $"#num",%eax\n\t" \
    ".byte 0xe9\n\t" /* jmp */ \
    ".long " __ASM_NAME("call_stubless_func") "-1f\n" \
    "1:\n\t"

#elif defined __x86_64__

__ASM_GLOBAL_FUNC( call_stubless_func,
                   "subq $0x38,%rsp\n\t"
                   __ASM_SEH(".seh_stackalloc 0x38\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 0x38\n\t")
                   "movq %rcx,0x40(%rsp)\n\t"
                   "movq %rdx,0x48(%rsp)\n\t"
                   "movq %r8,0x50(%rsp)\n\t"
                   "movq %r9,0x58(%rsp)\n\t"
                   "leaq 0x40(%rsp),%rdx\n\t"    /* args */
                   "movq %xmm1,0x20(%rsp)\n\t"
                   "movq %xmm2,0x28(%rsp)\n\t"
                   "movq %xmm3,0x30(%rsp)\n\t"
                   "leaq 0x18(%rsp),%r8\n\t"     /* fpu_regs */
                   "movl %r10d,%ecx\n\t"         /* index */
                   "call " __ASM_NAME("ndr_stubless_client_call") "\n\t"
                   "addq $0x38,%rsp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -0x38\n\t")
                   "ret" )

#define T(num) \
    ".balign 4\n\t" \
    ".globl " __ASM_NAME("ObjectStublessClient" #num) "\n" \
    __ASM_NAME("ObjectStublessClient" #num) ":\n\t" \
    "movl $"#num",%r10d\n\t" \
    ".byte 0xe9\n\t" /* jmp */ \
    ".long " __ASM_NAME("call_stubless_func") "-1f\n" \
    "1:\n\t"

#elif defined __aarch64__

__ASM_GLOBAL_FUNC( call_stubless_func,
                   "stp x29, x30, [sp, #-0x90]!\n\t"
                   ".seh_save_fplr_x 0x90\n\t"
                   "mov x29, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "stp d0, d1, [sp, #0x10]\n\t"
                   "stp d2, d3, [sp, #0x20]\n\t"
                   "stp d4, d5, [sp, #0x30]\n\t"
                   "stp d6, d7, [sp, #0x40]\n\t"
                   "stp x0, x1, [sp, #0x50]\n\t"
                   "stp x2, x3, [sp, #0x60]\n\t"
                   "stp x4, x5, [sp, #0x70]\n\t"
                   "stp x6, x7, [sp, #0x80]\n\t"
                   "mov w0, w16\n\t"            /* index */
                   "add x1, sp, #0x50\n\t"      /* args */
                   "add x2, sp, #0x10\n\t"      /* fpu_regs */
                   "bl ndr_stubless_client_call\n\t"
                   "ldp x29, x30, [sp], #0x90\n\t"
                   "ret" )

#define T(num) \
    ".globl ObjectStublessClient" #num "\n" \
    "ObjectStublessClient" #num ":\n\t" \
    "mov w16,#"#num"\n\t" \
    "b call_stubless_func\n\t"

#elif defined __arm__

__ASM_GLOBAL_FUNC( call_stubless_func,
                   "push {r0-r3}\n\t"
                   ".seh_save_regs {r0-r3}\n\t"
                   "push {fp,lr}\n\t"
                   ".seh_save_regs_w {fp,lr}\n\t"
                   "mov fp, sp\n\t"
                   ".seh_save_sp fp\n\t"
                   ".seh_endprologue\n\t"
                   "mov r0, ip\n\t"              /* index */
                   "add r1, sp, #8\n\t"          /* args */
                   "vpush {s0-s15}\n\t"          /* store the s0-s15/d0-d7 arguments */
                   "mov r2, sp\n\t"              /* fpu_regs */
                   "bl ndr_stubless_client_call\n\t"
                   "mov sp, fp\n\t"
                   "pop {fp,lr}\n\t"
                   "add sp, #16\n\t"
                   "bx lr" )

#define T(num) \
    ".globl ObjectStublessClient" #num "\n" \
    "ObjectStublessClient" #num ":\n\t" \
    "ldr ip,1f\n\t" \
    "b.w call_stubless_func\n" \
    "1:\t.long "#num"\n\t"

#endif  /* __i386__ */

__ASM_GLOBAL_FUNC( stubless_thunks, ALL_THUNK_ENTRIES )

#undef T


/* The idea here is to replace the first param on the stack
   ie. This (which will point to cstdstubbuffer_delegating_t)
   with This->stub_buffer.pvServerObject and then jump to the
   relevant offset in This->stub_buffer.pvServerObject's vtbl.
*/
#ifdef __i386__

#define T(num) \
    ".balign 4\n\t" \
    ".globl " __ASM_NAME("NdrProxyForwardingFunction" #num) "\n" \
    __ASM_NAME("NdrProxyForwardingFunction" #num) ":\n\t" \
    "mov 4(%esp),%eax\n\t" \
    "mov 0x10(%eax),%eax\n\t" \
    "mov %eax,4(%esp)\n\t" \
    "mov (%eax),%eax\n\t" \
    ".byte 0xff,0xa0\n\t" /* jmp *offset(%eax) */ \
    ".long 4*"#num"\n\t"

#elif defined __x86_64__

#define T(num) \
    ".balign 4\n\t" \
    ".globl " __ASM_NAME("NdrProxyForwardingFunction" #num) "\n" \
    __ASM_NAME("NdrProxyForwardingFunction" #num) ":\n\t" \
    "movq 0x20(%rcx),%rcx\n\t" \
    "movq (%rcx),%rax\n\t" \
    ".byte 0xff,0xa0\n\t" /* jmp *offset(%rax) */ \
    ".long 8*"#num"\n\t"

#elif defined __aarch64__

#define T(num) \
    ".globl NdrProxyForwardingFunction" #num "\n" \
    "NdrProxyForwardingFunction" #num ":\n\t" \
    "ldr x0, [x0, #0x20]\n\t" \
    "ldr x16, [x0]\n\t" \
    "mov x17, #"#num"\n\t" \
    "ldr x16, [x16, x17, lsl #3]\n\t" \
    "br x16\n\t"

#elif defined __arm__

#define T(num) \
    ".balign 4\n\t" \
    ".globl NdrProxyForwardingFunction" #num "\n" \
    "NdrProxyForwardingFunction" #num ":\n\t" \
    "ldr r0, [r0, #0x10]\n\t" \
    "ldr ip, [r0]\n\t" \
    "ldr pc, [ip, #(4*"#num")]\n\t"

#endif  /* __i386__ */

__ASM_GLOBAL_FUNC( vtbl_thunks, ALL_THUNK_ENTRIES )

#undef T

static HRESULT WINAPI delegating_QueryInterface(IUnknown *pUnk, REFIID iid, void **ppv)
{
    *ppv = pUnk;
    return S_OK;
}

static ULONG WINAPI delegating_AddRef(IUnknown *pUnk)
{
    return 1;
}

static ULONG WINAPI delegating_Release(IUnknown *pUnk)
{
    return 1;
}

#define T(num) extern void NdrProxyForwardingFunction##num(void);
ALL_THUNK_ENTRIES
#undef T

const struct delegating_vtbl delegating_vtbl =
{
    { delegating_QueryInterface, delegating_AddRef, delegating_Release },
    {
#define T(num) (void *)NdrProxyForwardingFunction##num,
        ALL_THUNK_ENTRIES
#undef T
    }
};


#if defined(__aarch64__) || defined(__arm__)
static void __attribute__((used)) args_stack_to_regs( void **args, void **regs, void **stack,
                                                      const NDR_PROC_PARTIAL_OIF_HEADER *header )
{
    const NDR_PROC_HEADER_EXTS *ext = (const NDR_PROC_HEADER_EXTS *)(header + 1);
    unsigned int i, size, count, pos;
    unsigned char *data;

#ifdef __arm__
    const NDR_PARAM_OIF *params = (const NDR_PARAM_OIF *)((const char *)ext + ext->Size);

    for (i = 0; i < header->number_of_params; i++)
        if (params[i].attr.IsIn && params[i].attr.IsBasetype)
        {
            int *arg = (int *)((char *)args + params[i].stack_offset);

            switch (params[i].u.type_format_char)
            {
            case FC_BYTE:
            case FC_USMALL:
                *arg = (unsigned char)*arg;
                break;
            case FC_CHAR:
            case FC_SMALL:
                *arg = (signed char)*arg;
                break;
            case FC_WCHAR:
            case FC_USHORT:
                *arg = (unsigned short)*arg;
                break;
            case FC_SHORT:
                *arg = (short)*arg;
                break;
            }
        }
#endif

    if (ext->Size < sizeof(*ext) + 3) return;
    data = (unsigned char *)(ext + 1);
    size = min( ext->Size - sizeof(*ext) - 3, data[2] );
    data += 3;
    for (i = pos = 0; i < size; i++, pos++)
    {
        if (data[i] < 0x80) continue;
        else if (data[i] < 0x94) regs[data[i] - 0x80] = args[pos];
        else if (data[i] == 0x9d) /* repeat */
        {
            if (i + 3 >= size) break;
            count = data[i + 2] + (data[i + 3] << 8);
            memcpy( &stack[pos + (signed char)data[i + 1]], &args[pos], count * sizeof(*args) );
            pos += count - 1;
            i += 3;
        }
        else if (data[i] < 0xa0) continue;
        else stack[pos + (signed char)data[i]] = args[pos];
    }
}
#endif


/* Call a function with the specified arguments, restoring the stack
 * properly afterwards as we don't know the calling convention of the
 * function */
#if defined __i386__ && defined _MSC_VER && defined __REACTOS__ // Wine has removed this :(
__declspec(naked) LONG_PTR __cdecl call_server_func(SERVER_ROUTINE func, unsigned char * args, unsigned int stack_size,
                                                    const NDR_PROC_PARTIAL_OIF_HEADER *header )
{
    __asm
    {
        push ebp
        mov ebp, esp
        push edi            ; Save registers
        push esi
        mov eax, [ebp+16]   ; Get stack size
        sub esp, eax        ; Make room in stack for arguments
        and esp, 0xFFFFFFF0
        mov edi, esp
        mov ecx, eax
        mov esi, [ebp+12]
        shr ecx, 2
        cld
        rep movsd           ; Copy dword blocks
        call [ebp+8]        ; Call function
        lea esp, [ebp-8]    ; Restore stack
        pop esi             ; Restore registers
        pop edi
        pop ebp
        ret
    }
}
#elif defined __i386__ // remove above, replace here with #ifdef __i386__
__ASM_GLOBAL_FUNC( call_server_func,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"            /* Save registers */
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "movl 16(%ebp), %eax\n\t"   /* Get stack size */
                   "subl %eax, %esp\n\t"       /* Make room in stack for arguments */
                   "andl $~15, %esp\n\t"	/* Make sure stack has 16-byte alignment for Mac OS X */
                   "movl %esp, %edi\n\t"
                   "movl %eax, %ecx\n\t"
                   "movl 12(%ebp), %esi\n\t"
                   "shrl $2, %ecx\n\t"         /* divide by 4 */
                   "cld\n\t"
                   "rep; movsl\n\t"            /* Copy dword blocks */
                   "call *8(%ebp)\n\t"         /* Call function */
                   "leal -8(%ebp), %esp\n\t"   /* Restore stack */
                   "popl %esi\n\t"             /* Restore registers */
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
#elif defined __x86_64__
__ASM_GLOBAL_FUNC( call_server_func,
                   "pushq %rbp\n\t"
                   __ASM_SEH(".seh_pushreg %rbp\n\t")
                   __ASM_CFI(".cfi_adjust_cfa_offset 8\n\t")
                   __ASM_CFI(".cfi_rel_offset %rbp,0\n\t")
                   "movq %rsp,%rbp\n\t"
                   __ASM_SEH(".seh_setframe %rbp,0\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rbp\n\t")
                   "pushq %rsi\n\t"
                   __ASM_SEH(".seh_pushreg %rsi\n\t")
                   __ASM_CFI(".cfi_rel_offset %rsi,-8\n\t")
                   "pushq %rdi\n\t"
                   __ASM_SEH(".seh_pushreg %rdi\n\t")
                   __ASM_SEH(".seh_endprologue\n\t")
                   __ASM_CFI(".cfi_rel_offset %rdi,-16\n\t")
                   "movq %rcx,%rax\n\t"   /* function to call */
                   "movq $32,%rcx\n\t"    /* allocate max(32,stack_size) bytes of stack space */
                   "cmpq %rcx,%r8\n\t"
                   "cmovgq %r8,%rcx\n\t"
                   "subq %rcx,%rsp\n\t"
                   "andq $~15,%rsp\n\t"
                   "movq %r8,%rcx\n\t"
                   "shrq $3,%rcx\n\t"
                   "movq %rsp,%rdi\n\t"
                   "movq %rdx,%rsi\n\t"
                   "rep; movsq\n\t"       /* copy arguments */
                   "movq 0(%rsp),%rcx\n\t"
                   "movq 8(%rsp),%rdx\n\t"
                   "movq 16(%rsp),%r8\n\t"
                   "movq 24(%rsp),%r9\n\t"
                   "movq 0(%rsp),%xmm0\n\t"
                   "movq 8(%rsp),%xmm1\n\t"
                   "movq 16(%rsp),%xmm2\n\t"
                   "movq 24(%rsp),%xmm3\n\t"
                   "callq *%rax\n\t"
                   "leaq -16(%rbp),%rsp\n\t"  /* restore stack */
                   "popq %rdi\n\t"
                   __ASM_CFI(".cfi_same_value %rdi\n\t")
                   "popq %rsi\n\t"
                   __ASM_CFI(".cfi_same_value %rsi\n\t")
                   __ASM_CFI(".cfi_def_cfa_register %rsp\n\t")
                   "popq %rbp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                   __ASM_CFI(".cfi_same_value %rbp\n\t")
                   "ret" )
#elif defined __arm__
__ASM_GLOBAL_FUNC( call_server_func,
                   "push {r4,r5,fp,lr}\n\t"
                   ".seh_save_regs_w {r4,r5,fp,lr}\n\t"
                   "mov fp, sp\n\t"
                   ".seh_save_sp fp\n\t"
                   ".seh_endprologue\n\t"
                   "add r2, r2, #20*4+8+4\n\t"
                   "and r2, r2, #~7\n\t"
                   "sub sp, sp, r2\n\t"
                   "mov r4, r0\n\t"        /* func */
                   "mov r0, r1\n\t"        /* args */
                   "add r1, sp, #8\n\t"    /* regs */
                   "add r2, r1, #20*4\n\t" /* stack */
                   "bl args_stack_to_regs\n\t"
                   "add sp, sp, #8\n\t"
                   "pop {r0-r3}\n\t"
                   "vpop {s0-s15}\n\t"
                   "blx r4\n\t"
                   "mov sp, fp\n\t"
                   "pop {r4,r5,fp,pc}" )
#elif defined __aarch64__
__ASM_GLOBAL_FUNC( call_server_func,
                   "stp x29, x30, [sp, #-0x20]!\n\t"
                   ".seh_save_fplr_x 0x20\n\t"
                   "stp x19, x20, [sp, #0x10]\n\t"
                   ".seh_save_regp x19, 0x10\n\t"
                   "mov x29, sp\n\t"
                   ".seh_set_fp\n\t"
                   ".seh_endprologue\n\t"
                   "add x9, x2, #16*8+15\n\t"
                   "lsr x9, x9, #4\n\t"
                   "sub sp, sp, x9, lsl #4\n\t"
                   "mov x19, x0\n\t"       /* func */
                   "mov x0, x1\n\t"        /* args */
                   "mov x1, sp\n\t"        /* regs */
                   "add x2, sp, #16*8\n\t" /* stack */
                   "bl args_stack_to_regs\n\t"
                   "ldp x2, x3, [sp, #0x10]\n\t"
                   "ldp x4, x5, [sp, #0x20]\n\t"
                   "ldp x6, x7, [sp, #0x30]\n\t"
                   "ldp d0, d1, [sp, #0x40]\n\t"
                   "ldp d2, d3, [sp, #0x50]\n\t"
                   "ldp d4, d5, [sp, #0x60]\n\t"
                   "ldp d6, d7, [sp, #0x70]\n\t"
                   "ldp x0, x1, [sp], #0x80\n\t"
                   "blr x19\n\t"
                   "mov sp, x29\n\t"
                   "ldp x19, x20, [sp, #0x10]\n\t"
                   "ldp x29, x30, [sp], #0x20\n\t"
                   "ret" )
#endif
