/*
Copyright (c) 2006-2007 dogbert <dogber1@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define IDC_STATIC                              -1

// icons
#define IDI_APP_ICON                            150

// main dialog
#define IDD_MAIN                                100
#define IDB_CLOSE                               101
#define IDB_APPLY                               102
#define IDC_TAB                                 103
#define IDT_SWAPJACKS                           104

#define IDD_TAB1                                200
#define IDC_EN_PCMDAC                           201
#define IDC_EXCH_FB                             202
#define IDC_EN_REAR2LINE                        203
#define IDC_EN_BASS2LINE                        204
#define IDC_EN_CENTER2LINE                      205
#define IDC_NOROUTE_LINE                        206
#define IDC_EN_CENTER2MIC                       207
#define IDC_NOROUTE_MIC                         208
#define IDCB_CHANNELCONFIG                      210
#define IDC_LEFT                                211
#define IDC_CLEFT                               212
#define IDC_CENTER                              213
#define IDC_CRIGHT                              214
#define IDC_RIGHT                               215
#define IDC_BLEFT                               216
#define IDC_BRIGHT                              217
#define IDC_SUB                                 218
#define IDB_STARTSTOP                           219

#define IDD_TAB2                                300
#define IDC_EN_SPDO                             301
#define IDC_EN_SPDO5V                           302
#define IDC_EN_SPDCOPYRHT                       303
#define IDC_EN_DAC2SPDO                         304
#define IDC_SEL_SPDIFI                          310
#define IDC_INV_SPDIFI                          311
#define IDC_POLVALID                            312
#define IDC_LOOP_SPDF                           313
#define IDC_EN_SPDI                             314

#define IDD_TAB3                                400
#define IDC_FMT_441_PCM                         401
#define IDC_FMT_480_PCM                         402
#define IDC_FMT_882_PCM                         403
#define IDC_FMT_960_PCM                         404
#define IDC_FMT_441_MULTI_PCM                   405
#define IDC_FMT_480_MULTI_PCM                   406
#define IDC_FMT_882_MULTI_PCM                   407
#define IDC_FMT_960_MULTI_PCM                   408
#define IDC_FMT_441_DOLBY                       409
#define IDC_FMT_480_DOLBY                       410
#define IDC_FMT_882_DOLBY                       411
#define IDC_FMT_960_DOLBY                       412

#define IDD_TAB4                                500
#define IDC_VERSION                             501
#define IDC_HWREV                               502
#define IDC_MAXCHAN                             503
#define IDC_BASEADR                             504
#define IDC_MPUADR                              505
#define IDC_URL1                                506
#define IDC_URL2                                507

char* tabsName[]     = { "Analog", "Digital", "Formats", "About" };
int   tabsResource[] = { IDD_TAB1, IDD_TAB2, IDD_TAB3, IDD_TAB4 };
#define NUM_TABS   4
