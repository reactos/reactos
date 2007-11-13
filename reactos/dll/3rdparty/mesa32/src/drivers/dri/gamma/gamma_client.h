/* Automaticallly generated -- do not edit */
#ifndef _GLINT_CLIENT_H_
#define _GLINT_CLIENT_H_
/* **********************************************************************/
/* START OF glint_extra.h INCLUSION                                     */
/* **********************************************************************/

/* glint_extra.h
 * Created: Fri Apr  2 23:32:05 1999 by faith@precisioninsight.com
 * Revised: Fri Apr  2 23:33:00 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_client.h,v 1.3 2002/02/22 21:33:00 dawes Exp $
 * 
 */

#define AreaStippleEnable          0x00001
#define LineStippleEnable          0x00002
#define GResetLineStipple          0x00004
#define FastFillEnable             0x00008
#define PrimitiveLine              0x00000
#define PrimitiveTrapezoid         0x00040
#define PrimitivePoint             0x00080
#define PrimitiveRectangle         0x000C0
#define AntialiasEnable            0x00100
#define AntialiasingQuality        0x00200
#define UsePointTable              0x00400
#define SyncOnBitMask              0x00800
#define SyncOnHostData             0x01000
#define TextureEnable              0x02000
#define FogEnable                  0x04000
#define CoverageEnable             0x08000
#define SubPixelCorrectionEnable   0x10000
#define SpanOperation              0x40000


/* **********************************************************************/
/* END OF glint_extra.h INCLUSION                                       */
/* **********************************************************************/


#define GlintResetStatus                              0x0000
#define GlintResetStatusReg                                0
#define GlintResetStatusOff                           0x0000
#define GlintResetStatusSec                           0x0000
#define GlintResetStatusSecReg                        2
#define GlintResetStatusSecOff                        0x0000

#define GlintIntEnable                                0x0008
#define GlintIntEnableReg                                  0
#define GlintIntEnableOff                             0x0008
#define GlintIntEnableSec                             0x0008
#define GlintIntEnableSecReg                          2
#define GlintIntEnableSecOff                          0x0008

#define GlintIntFlags                                 0x0010
#define GlintIntFlagsReg                                   0
#define GlintIntFlagsOff                              0x0010
#define GlintIntFlagsSec                              0x0010
#define GlintIntFlagsSecReg                           2
#define GlintIntFlagsSecOff                           0x0010

#define GlintInFIFOSpace                              0x0018
#define GlintInFIFOSpaceReg                                0
#define GlintInFIFOSpaceOff                           0x0018
#define GlintInFIFOSpaceSec                           0x0018
#define GlintInFIFOSpaceSecReg                        2
#define GlintInFIFOSpaceSecOff                        0x0018

#define GlintOutFIFOWords                             0x0020
#define GlintOutFIFOWordsReg                               0
#define GlintOutFIFOWordsOff                          0x0020
#define GlintOutFIFOWordsSec                          0x0020
#define GlintOutFIFOWordsSecReg                       2
#define GlintOutFIFOWordsSecOff                       0x0020

#define GlintDMAAddress                               0x0028
#define GlintDMAAddressReg                                 0
#define GlintDMAAddressOff                            0x0028
#define GlintDMAAddressSec                            0x0028
#define GlintDMAAddressSecReg                         2
#define GlintDMAAddressSecOff                         0x0028

#define GlintDMACount                                 0x0030
#define GlintDMACountReg                                   0
#define GlintDMACountOff                              0x0030
#define GlintDMACountSec                              0x0030
#define GlintDMACountSecReg                           2
#define GlintDMACountSecOff                           0x0030

#define GlintErrorFlags                               0x0038
#define GlintErrorFlagsReg                                 0
#define GlintErrorFlagsOff                            0x0038
#define GlintErrorFlagsSec                            0x0038
#define GlintErrorFlagsSecReg                         2
#define GlintErrorFlagsSecOff                         0x0038

#define GlintVClkCtl                                  0x0040
#define GlintVClkCtlReg                                    0
#define GlintVClkCtlOff                               0x0040
#define GlintVClkCtlSec                               0x0040
#define GlintVClkCtlSecReg                            2
#define GlintVClkCtlSecOff                            0x0040

#define GlintTestRegister                             0x0048
#define GlintTestRegisterReg                               0
#define GlintTestRegisterOff                          0x0048
#define GlintTestRegisterSec                          0x0048
#define GlintTestRegisterSecReg                       2
#define GlintTestRegisterSecOff                       0x0048

#define GlintAperture0                                0x0050
#define GlintAperture0Reg                                  0
#define GlintAperture0Off                             0x0050
#define GlintAperture0Sec                             0x0050
#define GlintAperture0SecReg                          2
#define GlintAperture0SecOff                          0x0050

#define GlintAperture1                                0x0058
#define GlintAperture1Reg                                  0
#define GlintAperture1Off                             0x0058
#define GlintAperture1Sec                             0x0058
#define GlintAperture1SecReg                          2
#define GlintAperture1SecOff                          0x0058

#define GlintDMAControl                               0x0060
#define GlintDMAControlReg                                 0
#define GlintDMAControlOff                            0x0060
#define GlintDMAControlSec                            0x0060
#define GlintDMAControlSecReg                         2
#define GlintDMAControlSecOff                         0x0060

#define GlintFIFODis                                  0x0068
#define GlintFIFODisReg                                    0
#define GlintFIFODisOff                               0x0068
#define GlintFIFODisSec                               0x0068
#define GlintFIFODisSecReg                            2
#define GlintFIFODisSecOff                            0x0068

#define GlintLBMemoryCtl                              0x1000
#define GlintLBMemoryCtlReg                                1
#define GlintLBMemoryCtlOff                           0x0000
#define GlintLBMemoryCtlSec                           0x1000
#define GlintLBMemoryCtlSecReg                        3
#define GlintLBMemoryCtlSecOff                        0x0000

#define GlintLBMemoryEDO                              0x1008
#define GlintLBMemoryEDOReg                                1
#define GlintLBMemoryEDOOff                           0x0008
#define GlintLBMemoryEDOSec                           0x1008
#define GlintLBMemoryEDOSecReg                        3
#define GlintLBMemoryEDOSecOff                        0x0008

#define GlintFBMemoryCtl                              0x1800
#define GlintFBMemoryCtlReg                                1
#define GlintFBMemoryCtlOff                           0x0800
#define GlintFBMemoryCtlSec                           0x1800
#define GlintFBMemoryCtlSecReg                        3
#define GlintFBMemoryCtlSecOff                        0x0800

#define GlintFBModeSel                                0x1808
#define GlintFBModeSelReg                                  1
#define GlintFBModeSelOff                             0x0808
#define GlintFBModeSelSec                             0x1808
#define GlintFBModeSelSecReg                          3
#define GlintFBModeSelSecOff                          0x0808

#define GlintFBGCWrMask                               0x1810
#define GlintFBGCWrMaskReg                                 1
#define GlintFBGCWrMaskOff                            0x0810
#define GlintFBGCWrMaskSec                            0x1810
#define GlintFBGCWrMaskSecReg                         3
#define GlintFBGCWrMaskSecOff                         0x0810

#define GlintFBGCColorLower                           0x1818
#define GlintFBGCColorLowerReg                             1
#define GlintFBGCColorLowerOff                        0x0818
#define GlintFBGCColorLowerSec                        0x1818
#define GlintFBGCColorLowerSecReg                     3
#define GlintFBGCColorLowerSecOff                     0x0818

#define GlintFBTXMemCtl                               0x1820
#define GlintFBTXMemCtlReg                                 1
#define GlintFBTXMemCtlOff                            0x0820
#define GlintFBTXMemCtlSec                            0x1820
#define GlintFBTXMemCtlSecReg                         3
#define GlintFBTXMemCtlSecOff                         0x0820

#define GlintFBWrMask                                 0x1830
#define GlintFBWrMaskReg                                   1
#define GlintFBWrMaskOff                              0x0830
#define GlintFBWrMaskSec                              0x1830
#define GlintFBWrMaskSecReg                           3
#define GlintFBWrMaskSecOff                           0x0830

#define GlintFBGCColorUpper                           0x1838
#define GlintFBGCColorUpperReg                             1
#define GlintFBGCColorUpperOff                        0x0838
#define GlintFBGCColorUpperSec                        0x1838
#define GlintFBGCColorUpperSecReg                     3
#define GlintFBGCColorUpperSecOff                     0x0838

#define GlintVTGHLimit                                0x3000
#define GlintVTGHLimitReg                                  1
#define GlintVTGHLimitOff                             0x2000
#define GlintVTGHLimitSec                             0x3000
#define GlintVTGHLimitSecReg                          3
#define GlintVTGHLimitSecOff                          0x2000

#define GlintVTGHSyncStart                            0x3008
#define GlintVTGHSyncStartReg                              1
#define GlintVTGHSyncStartOff                         0x2008
#define GlintVTGHSyncStartSec                         0x3008
#define GlintVTGHSyncStartSecReg                      3
#define GlintVTGHSyncStartSecOff                      0x2008

#define GlintVTGHSyncEnd                              0x3010
#define GlintVTGHSyncEndReg                                1
#define GlintVTGHSyncEndOff                           0x2010
#define GlintVTGHSyncEndSec                           0x3010
#define GlintVTGHSyncEndSecReg                        3
#define GlintVTGHSyncEndSecOff                        0x2010

#define GlintVTGHBlankEnd                             0x3018
#define GlintVTGHBlankEndReg                               1
#define GlintVTGHBlankEndOff                          0x2018
#define GlintVTGHBlankEndSec                          0x3018
#define GlintVTGHBlankEndSecReg                       3
#define GlintVTGHBlankEndSecOff                       0x2018

#define GlintVTGVLimit                                0x3020
#define GlintVTGVLimitReg                                  1
#define GlintVTGVLimitOff                             0x2020
#define GlintVTGVLimitSec                             0x3020
#define GlintVTGVLimitSecReg                          3
#define GlintVTGVLimitSecOff                          0x2020

#define GlintVTGVSyncStart                            0x3028
#define GlintVTGVSyncStartReg                              1
#define GlintVTGVSyncStartOff                         0x2028
#define GlintVTGVSyncStartSec                         0x3028
#define GlintVTGVSyncStartSecReg                      3
#define GlintVTGVSyncStartSecOff                      0x2028

#define GlintVTGVSyncEnd                              0x3030
#define GlintVTGVSyncEndReg                                1
#define GlintVTGVSyncEndOff                           0x2030
#define GlintVTGVSyncEndSec                           0x3030
#define GlintVTGVSyncEndSecReg                        3
#define GlintVTGVSyncEndSecOff                        0x2030

#define GlintVTGVBlankEnd                             0x3038
#define GlintVTGVBlankEndReg                               1
#define GlintVTGVBlankEndOff                          0x2038
#define GlintVTGVBlankEndSec                          0x3038
#define GlintVTGVBlankEndSecReg                       3
#define GlintVTGVBlankEndSecOff                       0x2038

#define GlintVTGHGateStart                            0x3040
#define GlintVTGHGateStartReg                              1
#define GlintVTGHGateStartOff                         0x2040
#define GlintVTGHGateStartSec                         0x3040
#define GlintVTGHGateStartSecReg                      3
#define GlintVTGHGateStartSecOff                      0x2040

#define GlintVTGHGateEnd                              0x3048
#define GlintVTGHGateEndReg                                1
#define GlintVTGHGateEndOff                           0x2048
#define GlintVTGHGateEndSec                           0x3048
#define GlintVTGHGateEndSecReg                        3
#define GlintVTGHGateEndSecOff                        0x2048

#define GlintVTGVGateStart                            0x3050
#define GlintVTGVGateStartReg                              1
#define GlintVTGVGateStartOff                         0x2050
#define GlintVTGVGateStartSec                         0x3050
#define GlintVTGVGateStartSecReg                      3
#define GlintVTGVGateStartSecOff                      0x2050

#define GlintVTGVGateEnd                              0x3058
#define GlintVTGVGateEndReg                                1
#define GlintVTGVGateEndOff                           0x2058
#define GlintVTGVGateEndSec                           0x3058
#define GlintVTGVGateEndSecReg                        3
#define GlintVTGVGateEndSecOff                        0x2058

#define GlintVTGPolarity                              0x3060
#define GlintVTGPolarityReg                                1
#define GlintVTGPolarityOff                           0x2060
#define GlintVTGPolaritySec                           0x3060
#define GlintVTGPolaritySecReg                        3
#define GlintVTGPolaritySecOff                        0x2060

#define GlintVTGFrameRowAddr                          0x3068
#define GlintVTGFrameRowAddrReg                            1
#define GlintVTGFrameRowAddrOff                       0x2068
#define GlintVTGFrameRowAddrSec                       0x3068
#define GlintVTGFrameRowAddrSecReg                    3
#define GlintVTGFrameRowAddrSecOff                    0x2068

#define GlintVTGVLineNumber                           0x3070
#define GlintVTGVLineNumberReg                             1
#define GlintVTGVLineNumberOff                        0x2070
#define GlintVTGVLineNumberSec                        0x3070
#define GlintVTGVLineNumberSecReg                     3
#define GlintVTGVLineNumberSecOff                     0x2070

#define GlintVTGSerialClk                             0x3078
#define GlintVTGSerialClkReg                               1
#define GlintVTGSerialClkOff                          0x2078
#define GlintVTGSerialClkSec                          0x3078
#define GlintVTGSerialClkSecReg                       3
#define GlintVTGSerialClkSecOff                       0x2078

#define GlintVTGModeCtl                               0x3080
#define GlintVTGModeCtlReg                                 1
#define GlintVTGModeCtlOff                            0x2080
#define GlintVTGModeCtlSec                            0x3080
#define GlintVTGModeCtlSecReg                         3
#define GlintVTGModeCtlSecOff                         0x2080

#define GlintOutputFIFO                               0x2000
#define GlintOutputFIFOReg                                 1
#define GlintOutputFIFOOff                            0x1000
#define GlintOutputFIFOSec                            0x2000
#define GlintOutputFIFOSecReg                         3
#define GlintOutputFIFOSecOff                         0x1000

#define GlintGInFIFOSpace                             0x0018
#define GlintGInFIFOSpaceReg                               0
#define GlintGInFIFOSpaceOff                          0x0018

#define GlintGDMAAddress                              0x0028
#define GlintGDMAAddressReg                                0
#define GlintGDMAAddressOff                           0x0028

#define GlintGDMACount                                0x0030
#define GlintGDMACountReg                                  0
#define GlintGDMACountOff                             0x0030

#define GlintGDMAControl                              0x0060
#define GlintGDMAControlReg                                0
#define GlintGDMAControlOff                           0x0060

#define GlintGOutDMA                                  0x0080
#define GlintGOutDMAReg                                    0
#define GlintGOutDMAOff                               0x0080

#define GlintGOutDMACount                             0x0088
#define GlintGOutDMACountReg                               0
#define GlintGOutDMACountOff                          0x0088

#define GlintGResetStatus                             0x0800
#define GlintGResetStatusReg                               0
#define GlintGResetStatusOff                          0x0800

#define GlintGIntEnable                               0x0808
#define GlintGIntEnableReg                                 0
#define GlintGIntEnableOff                            0x0808

#define GlintGIntFlags                                0x0810
#define GlintGIntFlagsReg                                  0
#define GlintGIntFlagsOff                             0x0810

#define GlintGErrorFlags                              0x0838
#define GlintGErrorFlagsReg                                0
#define GlintGErrorFlagsOff                           0x0838

#define GlintGTestRegister                            0x0848
#define GlintGTestRegisterReg                              0
#define GlintGTestRegisterOff                         0x0848

#define GlintGFIFODis                                 0x0868
#define GlintGFIFODisReg                                   0
#define GlintGFIFODisOff                              0x0868

#define GlintGChipConfig                              0x0870
#define GlintGChipConfigReg                                0
#define GlintGChipConfigOff                           0x0870

#define GlintGCSRAperture                             0x0878
#define GlintGCSRApertureReg                               0
#define GlintGCSRApertureOff                          0x0878

#define GlintGPageTableAddr                           0x0c00
#define GlintGPageTableAddrReg                             0
#define GlintGPageTableAddrOff                        0x0c00

#define GlintGPageTableLength                         0x0c08
#define GlintGPageTableLengthReg                           0
#define GlintGPageTableLengthOff                      0x0c08

#define GlintGDelayTimer                              0x0c38
#define GlintGDelayTimerReg                                0
#define GlintGDelayTimerOff                           0x0c38

#define GlintGCommandMode                             0x0c40
#define GlintGCommandModeReg                               0
#define GlintGCommandModeOff                          0x0c40

#define GlintGCommandIntEnable                        0x0c48
#define GlintGCommandIntEnableReg                          0
#define GlintGCommandIntEnableOff                     0x0c48

#define GlintGCommandIntFlags                         0x0c50
#define GlintGCommandIntFlagsReg                           0
#define GlintGCommandIntFlagsOff                      0x0c50

#define GlintGCommandErrorFlags                       0x0c58
#define GlintGCommandErrorFlagsReg                         0
#define GlintGCommandErrorFlagsOff                    0x0c58

#define GlintGCommandStatus                           0x0c60
#define GlintGCommandStatusReg                             0
#define GlintGCommandStatusOff                        0x0c60

#define GlintGCommandFaultingAddr                     0x0c68
#define GlintGCommandFaultingAddrReg                       0
#define GlintGCommandFaultingAddrOff                  0x0c68

#define GlintGVertexFaultingAddr                      0x0c70
#define GlintGVertexFaultingAddrReg                        0
#define GlintGVertexFaultingAddrOff                   0x0c70

#define GlintGWriteFaultingAddr                       0x0c88
#define GlintGWriteFaultingAddrReg                         0
#define GlintGWriteFaultingAddrOff                    0x0c88

#define GlintGFeedbackSelectCount                     0x0c98
#define GlintGFeedbackSelectCountReg                       0
#define GlintGFeedbackSelectCountOff                  0x0c98

#define GlintGGammaProcessorMode                      0x0cb8
#define GlintGGammaProcessorModeReg                        0
#define GlintGGammaProcessorModeOff                   0x0cb8

#define GlintGVGAShadow                               0x0d00
#define GlintGVGAShadowReg                                 0
#define GlintGVGAShadowOff                            0x0d00

#define GlintGMultGLINTAperture                       0x0d08
#define GlintGMultGLINTApertureReg                         0
#define GlintGMultGLINTApertureOff                    0x0d08

#define GlintGMultGLINT1                              0x0d10
#define GlintGMultGLINT1Reg                                0
#define GlintGMultGLINT1Off                           0x0d10

#define GlintGMultGLINT2                              0x0d18
#define GlintGMultGLINT2Reg                                0
#define GlintGMultGLINT2Off                           0x0d18

#define GlintStartXDom                                0x8000
#define GlintStartXDomTag                             0x0000
#define GlintStartXDomReg                                  1
#define GlintStartXDomOff                             0x7000
#define GlintStartXDomSec                             0x8000
#define GlintStartXDomSecReg                          3
#define GlintStartXDomSecOff                          0x7000

#define GlintdXDom                                    0x8008
#define GlintdXDomTag                                 0x0001
#define GlintdXDomReg                                      1
#define GlintdXDomOff                                 0x7008
#define GlintdXDomSec                                 0x8008
#define GlintdXDomSecReg                              3
#define GlintdXDomSecOff                              0x7008

#define GlintStartXSub                                0x8010
#define GlintStartXSubTag                             0x0002
#define GlintStartXSubReg                                  1
#define GlintStartXSubOff                             0x7010
#define GlintStartXSubSec                             0x8010
#define GlintStartXSubSecReg                          3
#define GlintStartXSubSecOff                          0x7010

#define GlintdXSub                                    0x8018
#define GlintdXSubTag                                 0x0003
#define GlintdXSubReg                                      1
#define GlintdXSubOff                                 0x7018
#define GlintdXSubSec                                 0x8018
#define GlintdXSubSecReg                              3
#define GlintdXSubSecOff                              0x7018

#define GlintStartY                                   0x8020
#define GlintStartYTag                                0x0004
#define GlintStartYReg                                     1
#define GlintStartYOff                                0x7020
#define GlintStartYSec                                0x8020
#define GlintStartYSecReg                             3
#define GlintStartYSecOff                             0x7020

#define GlintdY                                       0x8028
#define GlintdYTag                                    0x0005
#define GlintdYReg                                         1
#define GlintdYOff                                    0x7028
#define GlintdYSec                                    0x8028
#define GlintdYSecReg                                 3
#define GlintdYSecOff                                 0x7028

#define GlintGLINTCount                               0x8030
#define GlintGLINTCountTag                            0x0006
#define GlintGLINTCountReg                                 1
#define GlintGLINTCountOff                            0x7030
#define GlintGLINTCountSec                            0x8030
#define GlintGLINTCountSecReg                         3
#define GlintGLINTCountSecOff                         0x7030

#define GlintRender                                   0x8038
#define GlintRenderTag                                0x0007
#define GlintRenderReg                                     1
#define GlintRenderOff                                0x7038
#define GlintRenderSec                                0x8038
#define GlintRenderSecReg                             3
#define GlintRenderSecOff                             0x7038

#define GlintContinueNewLine                          0x8040
#define GlintContinueNewLineTag                       0x0008
#define GlintContinueNewLineReg                            1
#define GlintContinueNewLineOff                       0x7040
#define GlintContinueNewLineSec                       0x8040
#define GlintContinueNewLineSecReg                    3
#define GlintContinueNewLineSecOff                    0x7040

#define GlintContinueNewDom                           0x8048
#define GlintContinueNewDomTag                        0x0009
#define GlintContinueNewDomReg                             1
#define GlintContinueNewDomOff                        0x7048
#define GlintContinueNewDomSec                        0x8048
#define GlintContinueNewDomSecReg                     3
#define GlintContinueNewDomSecOff                     0x7048

#define GlintContinueNewSub                           0x8050
#define GlintContinueNewSubTag                        0x000a
#define GlintContinueNewSubReg                             1
#define GlintContinueNewSubOff                        0x7050
#define GlintContinueNewSubSec                        0x8050
#define GlintContinueNewSubSecReg                     3
#define GlintContinueNewSubSecOff                     0x7050

#define GlintContinue                                 0x8058
#define GlintContinueTag                              0x000b
#define GlintContinueReg                                   1
#define GlintContinueOff                              0x7058
#define GlintContinueSec                              0x8058
#define GlintContinueSecReg                           3
#define GlintContinueSecOff                           0x7058

#define GlintFlushSpan                                0x8060
#define GlintFlushSpanTag                             0x000c
#define GlintFlushSpanReg                                  1
#define GlintFlushSpanOff                             0x7060
#define GlintFlushSpanSec                             0x8060
#define GlintFlushSpanSecReg                          3
#define GlintFlushSpanSecOff                          0x7060

#define GlintBitMaskPattern                           0x8068
#define GlintBitMaskPatternTag                        0x000d
#define GlintBitMaskPatternReg                             1
#define GlintBitMaskPatternOff                        0x7068
#define GlintBitMaskPatternSec                        0x8068
#define GlintBitMaskPatternSecReg                     3
#define GlintBitMaskPatternSecOff                     0x7068

#define GlintPointTable0                              0x8080
#define GlintPointTable0Tag                           0x0010
#define GlintPointTable0Reg                                1
#define GlintPointTable0Off                           0x7080
#define GlintPointTable0Sec                           0x8080
#define GlintPointTable0SecReg                        3
#define GlintPointTable0SecOff                        0x7080

#define GlintPointTable1                              0x8088
#define GlintPointTable1Tag                           0x0011
#define GlintPointTable1Reg                                1
#define GlintPointTable1Off                           0x7088
#define GlintPointTable1Sec                           0x8088
#define GlintPointTable1SecReg                        3
#define GlintPointTable1SecOff                        0x7088

#define GlintPointTable2                              0x8090
#define GlintPointTable2Tag                           0x0012
#define GlintPointTable2Reg                                1
#define GlintPointTable2Off                           0x7090
#define GlintPointTable2Sec                           0x8090
#define GlintPointTable2SecReg                        3
#define GlintPointTable2SecOff                        0x7090

#define GlintPointTable3                              0x8098
#define GlintPointTable3Tag                           0x0013
#define GlintPointTable3Reg                                1
#define GlintPointTable3Off                           0x7098
#define GlintPointTable3Sec                           0x8098
#define GlintPointTable3SecReg                        3
#define GlintPointTable3SecOff                        0x7098

#define GlintRasterizerMode                           0x80a0
#define GlintRasterizerModeTag                        0x0014
#define GlintRasterizerModeReg                             1
#define GlintRasterizerModeOff                        0x70a0
#define GlintRasterizerModeSec                        0x80a0
#define GlintRasterizerModeSecReg                     3
#define GlintRasterizerModeSecOff                     0x70a0

#define GlintYLimits                                  0x80a8
#define GlintYLimitsTag                               0x0015
#define GlintYLimitsReg                                    1
#define GlintYLimitsOff                               0x70a8
#define GlintYLimitsSec                               0x80a8
#define GlintYLimitsSecReg                            3
#define GlintYLimitsSecOff                            0x70a8

#define GlintScanLineOwnership                        0x80b0
#define GlintScanLineOwnershipTag                     0x0016
#define GlintScanLineOwnershipReg                          1
#define GlintScanLineOwnershipOff                     0x70b0
#define GlintScanLineOwnershipSec                     0x80b0
#define GlintScanLineOwnershipSecReg                  3
#define GlintScanLineOwnershipSecOff                  0x70b0

#define GlintWaitForCompletion                        0x80b8
#define GlintWaitForCompletionTag                     0x0017
#define GlintWaitForCompletionReg                          1
#define GlintWaitForCompletionOff                     0x70b8
#define GlintWaitForCompletionSec                     0x80b8
#define GlintWaitForCompletionSecReg                  3
#define GlintWaitForCompletionSecOff                  0x70b8

#define GlintPixelSize                                0x80c0
#define GlintPixelSizeTag                             0x0018
#define GlintPixelSizeReg                                  1
#define GlintPixelSizeOff                             0x70c0
#define GlintPixelSizeSec                             0x80c0
#define GlintPixelSizeSecReg                          3
#define GlintPixelSizeSecOff                          0x70c0

#define GlintScissorMode                              0x8180
#define GlintScissorModeTag                           0x0030
#define GlintScissorModeReg                                1
#define GlintScissorModeOff                           0x7180
#define GlintScissorModeSec                           0x8180
#define GlintScissorModeSecReg                        3
#define GlintScissorModeSecOff                        0x7180

#define GlintScissorMinXY                             0x8188
#define GlintScissorMinXYTag                          0x0031
#define GlintScissorMinXYReg                               1
#define GlintScissorMinXYOff                          0x7188
#define GlintScissorMinXYSec                          0x8188
#define GlintScissorMinXYSecReg                       3
#define GlintScissorMinXYSecOff                       0x7188

#define GlintScissorMaxXY                             0x8190
#define GlintScissorMaxXYTag                          0x0032
#define GlintScissorMaxXYReg                               1
#define GlintScissorMaxXYOff                          0x7190
#define GlintScissorMaxXYSec                          0x8190
#define GlintScissorMaxXYSecReg                       3
#define GlintScissorMaxXYSecOff                       0x7190

#define GlintScreenSize                               0x8198
#define GlintScreenSizeTag                            0x0033
#define GlintScreenSizeReg                                 1
#define GlintScreenSizeOff                            0x7198
#define GlintScreenSizeSec                            0x8198
#define GlintScreenSizeSecReg                         3
#define GlintScreenSizeSecOff                         0x7198

#define GlintAreaStippleMode                          0x81a0
#define GlintAreaStippleModeTag                       0x0034
#define GlintAreaStippleModeReg                            1
#define GlintAreaStippleModeOff                       0x71a0
#define GlintAreaStippleModeSec                       0x81a0
#define GlintAreaStippleModeSecReg                    3
#define GlintAreaStippleModeSecOff                    0x71a0

#define GlintLineStippleMode                          0x81a8
#define GlintLineStippleModeTag                       0x0035
#define GlintLineStippleModeReg                            1
#define GlintLineStippleModeOff                       0x71a8
#define GlintLineStippleModeSec                       0x81a8
#define GlintLineStippleModeSecReg                    3
#define GlintLineStippleModeSecOff                    0x71a8

#define GlintLoadLineStippleCounters                  0x81b0
#define GlintLoadLineStippleCountersTag               0x0036
#define GlintLoadLineStippleCountersReg                    1
#define GlintLoadLineStippleCountersOff               0x71b0
#define GlintLoadLineStippleCountersSec               0x81b0
#define GlintLoadLineStippleCountersSecReg            3
#define GlintLoadLineStippleCountersSecOff            0x71b0

#define GlintUpdateLineStippleCounters                0x81b8
#define GlintUpdateLineStippleCountersTag             0x0037
#define GlintUpdateLineStippleCountersReg                  1
#define GlintUpdateLineStippleCountersOff             0x71b8
#define GlintUpdateLineStippleCountersSec             0x81b8
#define GlintUpdateLineStippleCountersSecReg          3
#define GlintUpdateLineStippleCountersSecOff          0x71b8

#define GlintSaveLineStippleState                     0x81c0
#define GlintSaveLineStippleStateTag                  0x0038
#define GlintSaveLineStippleStateReg                       1
#define GlintSaveLineStippleStateOff                  0x71c0
#define GlintSaveLineStippleStateSec                  0x81c0
#define GlintSaveLineStippleStateSecReg               3
#define GlintSaveLineStippleStateSecOff               0x71c0

#define GlintWindowOrigin                             0x81c8
#define GlintWindowOriginTag                          0x0039
#define GlintWindowOriginReg                               1
#define GlintWindowOriginOff                          0x71c8
#define GlintWindowOriginSec                          0x81c8
#define GlintWindowOriginSecReg                       3
#define GlintWindowOriginSecOff                       0x71c8

#define GlintAreaStipplePattern0                      0x8200
#define GlintAreaStipplePattern0Tag                   0x0040
#define GlintAreaStipplePattern0Reg                        1
#define GlintAreaStipplePattern0Off                   0x7200
#define GlintAreaStipplePattern0Sec                   0x8200
#define GlintAreaStipplePattern0SecReg                3
#define GlintAreaStipplePattern0SecOff                0x7200

#define GlintAreaStipplePattern1                      0x8208
#define GlintAreaStipplePattern1Tag                   0x0041
#define GlintAreaStipplePattern1Reg                        1
#define GlintAreaStipplePattern1Off                   0x7208
#define GlintAreaStipplePattern1Sec                   0x8208
#define GlintAreaStipplePattern1SecReg                3
#define GlintAreaStipplePattern1SecOff                0x7208

#define GlintAreaStipplePattern2                      0x8210
#define GlintAreaStipplePattern2Tag                   0x0042
#define GlintAreaStipplePattern2Reg                        1
#define GlintAreaStipplePattern2Off                   0x7210
#define GlintAreaStipplePattern2Sec                   0x8210
#define GlintAreaStipplePattern2SecReg                3
#define GlintAreaStipplePattern2SecOff                0x7210

#define GlintAreaStipplePattern3                      0x8218
#define GlintAreaStipplePattern3Tag                   0x0043
#define GlintAreaStipplePattern3Reg                        1
#define GlintAreaStipplePattern3Off                   0x7218
#define GlintAreaStipplePattern3Sec                   0x8218
#define GlintAreaStipplePattern3SecReg                3
#define GlintAreaStipplePattern3SecOff                0x7218

#define GlintAreaStipplePattern4                      0x8220
#define GlintAreaStipplePattern4Tag                   0x0044
#define GlintAreaStipplePattern4Reg                        1
#define GlintAreaStipplePattern4Off                   0x7220
#define GlintAreaStipplePattern4Sec                   0x8220
#define GlintAreaStipplePattern4SecReg                3
#define GlintAreaStipplePattern4SecOff                0x7220

#define GlintAreaStipplePattern5                      0x8228
#define GlintAreaStipplePattern5Tag                   0x0045
#define GlintAreaStipplePattern5Reg                        1
#define GlintAreaStipplePattern5Off                   0x7228
#define GlintAreaStipplePattern5Sec                   0x8228
#define GlintAreaStipplePattern5SecReg                3
#define GlintAreaStipplePattern5SecOff                0x7228

#define GlintAreaStipplePattern6                      0x8230
#define GlintAreaStipplePattern6Tag                   0x0046
#define GlintAreaStipplePattern6Reg                        1
#define GlintAreaStipplePattern6Off                   0x7230
#define GlintAreaStipplePattern6Sec                   0x8230
#define GlintAreaStipplePattern6SecReg                3
#define GlintAreaStipplePattern6SecOff                0x7230

#define GlintAreaStipplePattern7                      0x8238
#define GlintAreaStipplePattern7Tag                   0x0047
#define GlintAreaStipplePattern7Reg                        1
#define GlintAreaStipplePattern7Off                   0x7238
#define GlintAreaStipplePattern7Sec                   0x8238
#define GlintAreaStipplePattern7SecReg                3
#define GlintAreaStipplePattern7SecOff                0x7238

#define GlintAreaStipplePattern8                      0x8240
#define GlintAreaStipplePattern8Tag                   0x0048
#define GlintAreaStipplePattern8Reg                        1
#define GlintAreaStipplePattern8Off                   0x7240
#define GlintAreaStipplePattern8Sec                   0x8240
#define GlintAreaStipplePattern8SecReg                3
#define GlintAreaStipplePattern8SecOff                0x7240

#define GlintAreaStipplePattern9                      0x8248
#define GlintAreaStipplePattern9Tag                   0x0049
#define GlintAreaStipplePattern9Reg                        1
#define GlintAreaStipplePattern9Off                   0x7248
#define GlintAreaStipplePattern9Sec                   0x8248
#define GlintAreaStipplePattern9SecReg                3
#define GlintAreaStipplePattern9SecOff                0x7248

#define GlintAreaStipplePattern10                     0x8250
#define GlintAreaStipplePattern10Tag                  0x004a
#define GlintAreaStipplePattern10Reg                       1
#define GlintAreaStipplePattern10Off                  0x7250
#define GlintAreaStipplePattern10Sec                  0x8250
#define GlintAreaStipplePattern10SecReg               3
#define GlintAreaStipplePattern10SecOff               0x7250

#define GlintAreaStipplePattern11                     0x8258
#define GlintAreaStipplePattern11Tag                  0x004b
#define GlintAreaStipplePattern11Reg                       1
#define GlintAreaStipplePattern11Off                  0x7258
#define GlintAreaStipplePattern11Sec                  0x8258
#define GlintAreaStipplePattern11SecReg               3
#define GlintAreaStipplePattern11SecOff               0x7258

#define GlintAreaStipplePattern12                     0x8260
#define GlintAreaStipplePattern12Tag                  0x004c
#define GlintAreaStipplePattern12Reg                       1
#define GlintAreaStipplePattern12Off                  0x7260
#define GlintAreaStipplePattern12Sec                  0x8260
#define GlintAreaStipplePattern12SecReg               3
#define GlintAreaStipplePattern12SecOff               0x7260

#define GlintAreaStipplePattern13                     0x8268
#define GlintAreaStipplePattern13Tag                  0x004d
#define GlintAreaStipplePattern13Reg                       1
#define GlintAreaStipplePattern13Off                  0x7268
#define GlintAreaStipplePattern13Sec                  0x8268
#define GlintAreaStipplePattern13SecReg               3
#define GlintAreaStipplePattern13SecOff               0x7268

#define GlintAreaStipplePattern14                     0x8270
#define GlintAreaStipplePattern14Tag                  0x004e
#define GlintAreaStipplePattern14Reg                       1
#define GlintAreaStipplePattern14Off                  0x7270
#define GlintAreaStipplePattern14Sec                  0x8270
#define GlintAreaStipplePattern14SecReg               3
#define GlintAreaStipplePattern14SecOff               0x7270

#define GlintAreaStipplePattern15                     0x8278
#define GlintAreaStipplePattern15Tag                  0x004f
#define GlintAreaStipplePattern15Reg                       1
#define GlintAreaStipplePattern15Off                  0x7278
#define GlintAreaStipplePattern15Sec                  0x8278
#define GlintAreaStipplePattern15SecReg               3
#define GlintAreaStipplePattern15SecOff               0x7278

#define GlintAreaStipplePattern16                     0x8280
#define GlintAreaStipplePattern16Tag                  0x0050
#define GlintAreaStipplePattern16Reg                       1
#define GlintAreaStipplePattern16Off                  0x7280
#define GlintAreaStipplePattern16Sec                  0x8280
#define GlintAreaStipplePattern16SecReg               3
#define GlintAreaStipplePattern16SecOff               0x7280

#define GlintAreaStipplePattern17                     0x8288
#define GlintAreaStipplePattern17Tag                  0x0051
#define GlintAreaStipplePattern17Reg                       1
#define GlintAreaStipplePattern17Off                  0x7288
#define GlintAreaStipplePattern17Sec                  0x8288
#define GlintAreaStipplePattern17SecReg               3
#define GlintAreaStipplePattern17SecOff               0x7288

#define GlintAreaStipplePattern18                     0x8290
#define GlintAreaStipplePattern18Tag                  0x0052
#define GlintAreaStipplePattern18Reg                       1
#define GlintAreaStipplePattern18Off                  0x7290
#define GlintAreaStipplePattern18Sec                  0x8290
#define GlintAreaStipplePattern18SecReg               3
#define GlintAreaStipplePattern18SecOff               0x7290

#define GlintAreaStipplePattern19                     0x8298
#define GlintAreaStipplePattern19Tag                  0x0053
#define GlintAreaStipplePattern19Reg                       1
#define GlintAreaStipplePattern19Off                  0x7298
#define GlintAreaStipplePattern19Sec                  0x8298
#define GlintAreaStipplePattern19SecReg               3
#define GlintAreaStipplePattern19SecOff               0x7298

#define GlintAreaStipplePattern20                     0x82a0
#define GlintAreaStipplePattern20Tag                  0x0054
#define GlintAreaStipplePattern20Reg                       1
#define GlintAreaStipplePattern20Off                  0x72a0
#define GlintAreaStipplePattern20Sec                  0x82a0
#define GlintAreaStipplePattern20SecReg               3
#define GlintAreaStipplePattern20SecOff               0x72a0

#define GlintAreaStipplePattern21                     0x82a8
#define GlintAreaStipplePattern21Tag                  0x0055
#define GlintAreaStipplePattern21Reg                       1
#define GlintAreaStipplePattern21Off                  0x72a8
#define GlintAreaStipplePattern21Sec                  0x82a8
#define GlintAreaStipplePattern21SecReg               3
#define GlintAreaStipplePattern21SecOff               0x72a8

#define GlintAreaStipplePattern22                     0x82b0
#define GlintAreaStipplePattern22Tag                  0x0056
#define GlintAreaStipplePattern22Reg                       1
#define GlintAreaStipplePattern22Off                  0x72b0
#define GlintAreaStipplePattern22Sec                  0x82b0
#define GlintAreaStipplePattern22SecReg               3
#define GlintAreaStipplePattern22SecOff               0x72b0

#define GlintAreaStipplePattern23                     0x82b8
#define GlintAreaStipplePattern23Tag                  0x0057
#define GlintAreaStipplePattern23Reg                       1
#define GlintAreaStipplePattern23Off                  0x72b8
#define GlintAreaStipplePattern23Sec                  0x82b8
#define GlintAreaStipplePattern23SecReg               3
#define GlintAreaStipplePattern23SecOff               0x72b8

#define GlintAreaStipplePattern24                     0x82c0
#define GlintAreaStipplePattern24Tag                  0x0058
#define GlintAreaStipplePattern24Reg                       1
#define GlintAreaStipplePattern24Off                  0x72c0
#define GlintAreaStipplePattern24Sec                  0x82c0
#define GlintAreaStipplePattern24SecReg               3
#define GlintAreaStipplePattern24SecOff               0x72c0

#define GlintAreaStipplePattern25                     0x82c8
#define GlintAreaStipplePattern25Tag                  0x0059
#define GlintAreaStipplePattern25Reg                       1
#define GlintAreaStipplePattern25Off                  0x72c8
#define GlintAreaStipplePattern25Sec                  0x82c8
#define GlintAreaStipplePattern25SecReg               3
#define GlintAreaStipplePattern25SecOff               0x72c8

#define GlintAreaStipplePattern26                     0x82d0
#define GlintAreaStipplePattern26Tag                  0x005a
#define GlintAreaStipplePattern26Reg                       1
#define GlintAreaStipplePattern26Off                  0x72d0
#define GlintAreaStipplePattern26Sec                  0x82d0
#define GlintAreaStipplePattern26SecReg               3
#define GlintAreaStipplePattern26SecOff               0x72d0

#define GlintAreaStipplePattern27                     0x82d8
#define GlintAreaStipplePattern27Tag                  0x005b
#define GlintAreaStipplePattern27Reg                       1
#define GlintAreaStipplePattern27Off                  0x72d8
#define GlintAreaStipplePattern27Sec                  0x82d8
#define GlintAreaStipplePattern27SecReg               3
#define GlintAreaStipplePattern27SecOff               0x72d8

#define GlintAreaStipplePattern28                     0x82e0
#define GlintAreaStipplePattern28Tag                  0x005c
#define GlintAreaStipplePattern28Reg                       1
#define GlintAreaStipplePattern28Off                  0x72e0
#define GlintAreaStipplePattern28Sec                  0x82e0
#define GlintAreaStipplePattern28SecReg               3
#define GlintAreaStipplePattern28SecOff               0x72e0

#define GlintAreaStipplePattern29                     0x82e8
#define GlintAreaStipplePattern29Tag                  0x005d
#define GlintAreaStipplePattern29Reg                       1
#define GlintAreaStipplePattern29Off                  0x72e8
#define GlintAreaStipplePattern29Sec                  0x82e8
#define GlintAreaStipplePattern29SecReg               3
#define GlintAreaStipplePattern29SecOff               0x72e8

#define GlintAreaStipplePattern30                     0x82f0
#define GlintAreaStipplePattern30Tag                  0x005e
#define GlintAreaStipplePattern30Reg                       1
#define GlintAreaStipplePattern30Off                  0x72f0
#define GlintAreaStipplePattern30Sec                  0x82f0
#define GlintAreaStipplePattern30SecReg               3
#define GlintAreaStipplePattern30SecOff               0x72f0

#define GlintAreaStipplePattern31                     0x82f8
#define GlintAreaStipplePattern31Tag                  0x005f
#define GlintAreaStipplePattern31Reg                       1
#define GlintAreaStipplePattern31Off                  0x72f8
#define GlintAreaStipplePattern31Sec                  0x82f8
#define GlintAreaStipplePattern31SecReg               3
#define GlintAreaStipplePattern31SecOff               0x72f8

#define GlintRouterMode                               0x8840
#define GlintRouterModeTag                            0x0108
#define GlintRouterModeReg                                 1
#define GlintRouterModeOff                            0x7840
#define GlintRouterModeSec                            0x8840
#define GlintRouterModeSecReg                         3
#define GlintRouterModeSecOff                         0x7840

#define GlintTextureAddressMode                       0x8380
#define GlintTextureAddressModeTag                    0x0070
#define GlintTextureAddressModeReg                         1
#define GlintTextureAddressModeOff                    0x7380
#define GlintTextureAddressModeSec                    0x8380
#define GlintTextureAddressModeSecReg                 3
#define GlintTextureAddressModeSecOff                 0x7380

#define GlintSStart                                   0x8388
#define GlintSStartTag                                0x0071
#define GlintSStartReg                                     1
#define GlintSStartOff                                0x7388
#define GlintSStartSec                                0x8388
#define GlintSStartSecReg                             3
#define GlintSStartSecOff                             0x7388

#define GlintdSdx                                     0x8390
#define GlintdSdxTag                                  0x0072
#define GlintdSdxReg                                       1
#define GlintdSdxOff                                  0x7390
#define GlintdSdxSec                                  0x8390
#define GlintdSdxSecReg                               3
#define GlintdSdxSecOff                               0x7390

#define GlintdSdyDom                                  0x8398
#define GlintdSdyDomTag                               0x0073
#define GlintdSdyDomReg                                    1
#define GlintdSdyDomOff                               0x7398
#define GlintdSdyDomSec                               0x8398
#define GlintdSdyDomSecReg                            3
#define GlintdSdyDomSecOff                            0x7398

#define GlintTStart                                   0x83a0
#define GlintTStartTag                                0x0074
#define GlintTStartReg                                     1
#define GlintTStartOff                                0x73a0
#define GlintTStartSec                                0x83a0
#define GlintTStartSecReg                             3
#define GlintTStartSecOff                             0x73a0

#define GlintdTdx                                     0x83a8
#define GlintdTdxTag                                  0x0075
#define GlintdTdxReg                                       1
#define GlintdTdxOff                                  0x73a8
#define GlintdTdxSec                                  0x83a8
#define GlintdTdxSecReg                               3
#define GlintdTdxSecOff                               0x73a8

#define GlintdTdyDom                                  0x83b0
#define GlintdTdyDomTag                               0x0076
#define GlintdTdyDomReg                                    1
#define GlintdTdyDomOff                               0x73b0
#define GlintdTdyDomSec                               0x83b0
#define GlintdTdyDomSecReg                            3
#define GlintdTdyDomSecOff                            0x73b0

#define GlintQStart                                   0x83b8
#define GlintQStartTag                                0x0077
#define GlintQStartReg                                     1
#define GlintQStartOff                                0x73b8
#define GlintQStartSec                                0x83b8
#define GlintQStartSecReg                             3
#define GlintQStartSecOff                             0x73b8

#define GlintdQdx                                     0x83c0
#define GlintdQdxTag                                  0x0078
#define GlintdQdxReg                                       1
#define GlintdQdxOff                                  0x73c0
#define GlintdQdxSec                                  0x83c0
#define GlintdQdxSecReg                               3
#define GlintdQdxSecOff                               0x73c0

#define GlintdQdyDom                                  0x83c8
#define GlintdQdyDomTag                               0x0079
#define GlintdQdyDomReg                                    1
#define GlintdQdyDomOff                               0x73c8
#define GlintdQdyDomSec                               0x83c8
#define GlintdQdyDomSecReg                            3
#define GlintdQdyDomSecOff                            0x73c8

#define GlintLOD                                      0x83d0
#define GlintLODTag                                   0x007a
#define GlintLODReg                                        1
#define GlintLODOff                                   0x73d0
#define GlintLODSec                                   0x83d0
#define GlintLODSecReg                                3
#define GlintLODSecOff                                0x73d0

#define GlintdSdy                                     0x83d8
#define GlintdSdyTag                                  0x007b
#define GlintdSdyReg                                       1
#define GlintdSdyOff                                  0x73d8
#define GlintdSdySec                                  0x83d8
#define GlintdSdySecReg                               3
#define GlintdSdySecOff                               0x73d8

#define GlintdTdy                                     0x83e0
#define GlintdTdyTag                                  0x007c
#define GlintdTdyReg                                       1
#define GlintdTdyOff                                  0x73e0
#define GlintdTdySec                                  0x83e0
#define GlintdTdySecReg                               3
#define GlintdTdySecOff                               0x73e0

#define GlintdQdy                                     0x83e8
#define GlintdQdyTag                                  0x007d
#define GlintdQdyReg                                       1
#define GlintdQdyOff                                  0x73e8
#define GlintdQdySec                                  0x83e8
#define GlintdQdySecReg                               3
#define GlintdQdySecOff                               0x73e8

#define GlintTextureReadMode                          0x8480
#define GlintTextureReadModeTag                       0x0090
#define GlintTextureReadModeReg                            1
#define GlintTextureReadModeOff                       0x7480
#define GlintTextureReadModeSec                       0x8480
#define GlintTextureReadModeSecReg                    3
#define GlintTextureReadModeSecOff                    0x7480

#define GlintTextureFormat                            0x8488
#define GlintTextureFormatTag                         0x0091
#define GlintTextureFormatReg                              1
#define GlintTextureFormatOff                         0x7488
#define GlintTextureFormatSec                         0x8488
#define GlintTextureFormatSecReg                      3
#define GlintTextureFormatSecOff                      0x7488

#define GlintTextureCacheControl                      0x8490
#define GlintTextureCacheControlTag                   0x0092
#define GlintTextureCacheControlReg                        1
#define GlintTextureCacheControlOff                   0x7490
#define GlintTextureCacheControlSec                   0x8490
#define GlintTextureCacheControlSecReg                3
#define GlintTextureCacheControlSecOff                0x7490

#define GlintGLINTBorderColor                         0x84a8
#define GlintGLINTBorderColorTag                      0x0095
#define GlintGLINTBorderColorReg                           1
#define GlintGLINTBorderColorOff                      0x74a8
#define GlintGLINTBorderColorSec                      0x84a8
#define GlintGLINTBorderColorSecReg                   3
#define GlintGLINTBorderColorSecOff                   0x74a8

#define GlintTexelLUTIndex                            0x84c0
#define GlintTexelLUTIndexTag                         0x0098
#define GlintTexelLUTIndexReg                              1
#define GlintTexelLUTIndexOff                         0x74c0
#define GlintTexelLUTIndexSec                         0x84c0
#define GlintTexelLUTIndexSecReg                      3
#define GlintTexelLUTIndexSecOff                      0x74c0

#define GlintTexelLUTData                             0x84c8
#define GlintTexelLUTDataTag                          0x0099
#define GlintTexelLUTDataReg                               1
#define GlintTexelLUTDataOff                          0x74c8
#define GlintTexelLUTDataSec                          0x84c8
#define GlintTexelLUTDataSecReg                       3
#define GlintTexelLUTDataSecOff                       0x74c8

#define GlintTexelLUTAddress                          0x84d0
#define GlintTexelLUTAddressTag                       0x009a
#define GlintTexelLUTAddressReg                            1
#define GlintTexelLUTAddressOff                       0x74d0
#define GlintTexelLUTAddressSec                       0x84d0
#define GlintTexelLUTAddressSecReg                    3
#define GlintTexelLUTAddressSecOff                    0x74d0

#define GlintTexelLUTTransfer                         0x84d8
#define GlintTexelLUTTransferTag                      0x009b
#define GlintTexelLUTTransferReg                           1
#define GlintTexelLUTTransferOff                      0x74d8
#define GlintTexelLUTTransferSec                      0x84d8
#define GlintTexelLUTTransferSecReg                   3
#define GlintTexelLUTTransferSecOff                   0x74d8

#define GlintTextureFilterMode                        0x84e0
#define GlintTextureFilterModeTag                     0x009c
#define GlintTextureFilterModeReg                          1
#define GlintTextureFilterModeOff                     0x74e0
#define GlintTextureFilterModeSec                     0x84e0
#define GlintTextureFilterModeSecReg                  3
#define GlintTextureFilterModeSecOff                  0x74e0

#define GlintTextureChromaUpper                       0x84e8
#define GlintTextureChromaUpperTag                    0x009d
#define GlintTextureChromaUpperReg                         1
#define GlintTextureChromaUpperOff                    0x74e8
#define GlintTextureChromaUpperSec                    0x84e8
#define GlintTextureChromaUpperSecReg                 3
#define GlintTextureChromaUpperSecOff                 0x74e8

#define GlintTextureChromaLower                       0x84f0
#define GlintTextureChromaLowerTag                    0x009e
#define GlintTextureChromaLowerReg                         1
#define GlintTextureChromaLowerOff                    0x74f0
#define GlintTextureChromaLowerSec                    0x84f0
#define GlintTextureChromaLowerSecReg                 3
#define GlintTextureChromaLowerSecOff                 0x74f0

#define GlintTxBaseAddr0                              0x8500
#define GlintTxBaseAddr0Tag                           0x00a0
#define GlintTxBaseAddr0Reg                                1
#define GlintTxBaseAddr0Off                           0x7500
#define GlintTxBaseAddr0Sec                           0x8500
#define GlintTxBaseAddr0SecReg                        3
#define GlintTxBaseAddr0SecOff                        0x7500

#define GlintTxBaseAddr1                              0x8508
#define GlintTxBaseAddr1Tag                           0x00a1
#define GlintTxBaseAddr1Reg                                1
#define GlintTxBaseAddr1Off                           0x7508
#define GlintTxBaseAddr1Sec                           0x8508
#define GlintTxBaseAddr1SecReg                        3
#define GlintTxBaseAddr1SecOff                        0x7508

#define GlintTxBaseAddr2                              0x8510
#define GlintTxBaseAddr2Tag                           0x00a2
#define GlintTxBaseAddr2Reg                                1
#define GlintTxBaseAddr2Off                           0x7510
#define GlintTxBaseAddr2Sec                           0x8510
#define GlintTxBaseAddr2SecReg                        3
#define GlintTxBaseAddr2SecOff                        0x7510

#define GlintTxBaseAddr3                              0x8518
#define GlintTxBaseAddr3Tag                           0x00a3
#define GlintTxBaseAddr3Reg                                1
#define GlintTxBaseAddr3Off                           0x7518
#define GlintTxBaseAddr3Sec                           0x8518
#define GlintTxBaseAddr3SecReg                        3
#define GlintTxBaseAddr3SecOff                        0x7518

#define GlintTxBaseAddr4                              0x8520
#define GlintTxBaseAddr4Tag                           0x00a4
#define GlintTxBaseAddr4Reg                                1
#define GlintTxBaseAddr4Off                           0x7520
#define GlintTxBaseAddr4Sec                           0x8520
#define GlintTxBaseAddr4SecReg                        3
#define GlintTxBaseAddr4SecOff                        0x7520

#define GlintTxBaseAddr5                              0x8528
#define GlintTxBaseAddr5Tag                           0x00a5
#define GlintTxBaseAddr5Reg                                1
#define GlintTxBaseAddr5Off                           0x7528
#define GlintTxBaseAddr5Sec                           0x8528
#define GlintTxBaseAddr5SecReg                        3
#define GlintTxBaseAddr5SecOff                        0x7528

#define GlintTxBaseAddr6                              0x8530
#define GlintTxBaseAddr6Tag                           0x00a6
#define GlintTxBaseAddr6Reg                                1
#define GlintTxBaseAddr6Off                           0x7530
#define GlintTxBaseAddr6Sec                           0x8530
#define GlintTxBaseAddr6SecReg                        3
#define GlintTxBaseAddr6SecOff                        0x7530

#define GlintTxBaseAddr7                              0x8538
#define GlintTxBaseAddr7Tag                           0x00a7
#define GlintTxBaseAddr7Reg                                1
#define GlintTxBaseAddr7Off                           0x7538
#define GlintTxBaseAddr7Sec                           0x8538
#define GlintTxBaseAddr7SecReg                        3
#define GlintTxBaseAddr7SecOff                        0x7538

#define GlintTxBaseAddr8                              0x8540
#define GlintTxBaseAddr8Tag                           0x00a8
#define GlintTxBaseAddr8Reg                                1
#define GlintTxBaseAddr8Off                           0x7540
#define GlintTxBaseAddr8Sec                           0x8540
#define GlintTxBaseAddr8SecReg                        3
#define GlintTxBaseAddr8SecOff                        0x7540

#define GlintTxBaseAddr9                              0x8548
#define GlintTxBaseAddr9Tag                           0x00a9
#define GlintTxBaseAddr9Reg                                1
#define GlintTxBaseAddr9Off                           0x7548
#define GlintTxBaseAddr9Sec                           0x8548
#define GlintTxBaseAddr9SecReg                        3
#define GlintTxBaseAddr9SecOff                        0x7548

#define GlintTxBaseAddr10                             0x8550
#define GlintTxBaseAddr10Tag                          0x00aa
#define GlintTxBaseAddr10Reg                               1
#define GlintTxBaseAddr10Off                          0x7550
#define GlintTxBaseAddr10Sec                          0x8550
#define GlintTxBaseAddr10SecReg                       3
#define GlintTxBaseAddr10SecOff                       0x7550

#define GlintTxBaseAddr11                             0x8558
#define GlintTxBaseAddr11Tag                          0x00ab
#define GlintTxBaseAddr11Reg                               1
#define GlintTxBaseAddr11Off                          0x7558
#define GlintTxBaseAddr11Sec                          0x8558
#define GlintTxBaseAddr11SecReg                       3
#define GlintTxBaseAddr11SecOff                       0x7558

#define GlintTxBaseAddr12                             0x8560
#define GlintTxBaseAddr12Tag                          0x00ac
#define GlintTxBaseAddr12Reg                               1
#define GlintTxBaseAddr12Off                          0x7560
#define GlintTxBaseAddr12Sec                          0x8560
#define GlintTxBaseAddr12SecReg                       3
#define GlintTxBaseAddr12SecOff                       0x7560

#define GlintTexelLUT0                                0x8e80
#define GlintTexelLUT0Tag                             0x01d0
#define GlintTexelLUT0Reg                                  1
#define GlintTexelLUT0Off                             0x7e80
#define GlintTexelLUT0Sec                             0x8e80
#define GlintTexelLUT0SecReg                          3
#define GlintTexelLUT0SecOff                          0x7e80

#define GlintTexelLUT1                                0x8e88
#define GlintTexelLUT1Tag                             0x01d1
#define GlintTexelLUT1Reg                                  1
#define GlintTexelLUT1Off                             0x7e88
#define GlintTexelLUT1Sec                             0x8e88
#define GlintTexelLUT1SecReg                          3
#define GlintTexelLUT1SecOff                          0x7e88

#define GlintTexelLUT2                                0x8e90
#define GlintTexelLUT2Tag                             0x01d2
#define GlintTexelLUT2Reg                                  1
#define GlintTexelLUT2Off                             0x7e90
#define GlintTexelLUT2Sec                             0x8e90
#define GlintTexelLUT2SecReg                          3
#define GlintTexelLUT2SecOff                          0x7e90

#define GlintTexelLUT3                                0x8e98
#define GlintTexelLUT3Tag                             0x01d3
#define GlintTexelLUT3Reg                                  1
#define GlintTexelLUT3Off                             0x7e98
#define GlintTexelLUT3Sec                             0x8e98
#define GlintTexelLUT3SecReg                          3
#define GlintTexelLUT3SecOff                          0x7e98

#define GlintTexelLUT4                                0x8ea0
#define GlintTexelLUT4Tag                             0x01d4
#define GlintTexelLUT4Reg                                  1
#define GlintTexelLUT4Off                             0x7ea0
#define GlintTexelLUT4Sec                             0x8ea0
#define GlintTexelLUT4SecReg                          3
#define GlintTexelLUT4SecOff                          0x7ea0

#define GlintTexelLUT5                                0x8ea8
#define GlintTexelLUT5Tag                             0x01d5
#define GlintTexelLUT5Reg                                  1
#define GlintTexelLUT5Off                             0x7ea8
#define GlintTexelLUT5Sec                             0x8ea8
#define GlintTexelLUT5SecReg                          3
#define GlintTexelLUT5SecOff                          0x7ea8

#define GlintTexelLUT6                                0x8eb0
#define GlintTexelLUT6Tag                             0x01d6
#define GlintTexelLUT6Reg                                  1
#define GlintTexelLUT6Off                             0x7eb0
#define GlintTexelLUT6Sec                             0x8eb0
#define GlintTexelLUT6SecReg                          3
#define GlintTexelLUT6SecOff                          0x7eb0

#define GlintTexelLUT7                                0x8eb8
#define GlintTexelLUT7Tag                             0x01d7
#define GlintTexelLUT7Reg                                  1
#define GlintTexelLUT7Off                             0x7eb8
#define GlintTexelLUT7Sec                             0x8eb8
#define GlintTexelLUT7SecReg                          3
#define GlintTexelLUT7SecOff                          0x7eb8

#define GlintTexelLUT8                                0x8ec0
#define GlintTexelLUT8Tag                             0x01d8
#define GlintTexelLUT8Reg                                  1
#define GlintTexelLUT8Off                             0x7ec0
#define GlintTexelLUT8Sec                             0x8ec0
#define GlintTexelLUT8SecReg                          3
#define GlintTexelLUT8SecOff                          0x7ec0

#define GlintTexelLUT9                                0x8ec8
#define GlintTexelLUT9Tag                             0x01d9
#define GlintTexelLUT9Reg                                  1
#define GlintTexelLUT9Off                             0x7ec8
#define GlintTexelLUT9Sec                             0x8ec8
#define GlintTexelLUT9SecReg                          3
#define GlintTexelLUT9SecOff                          0x7ec8

#define GlintTexelLUT10                               0x8ed0
#define GlintTexelLUT10Tag                            0x01da
#define GlintTexelLUT10Reg                                 1
#define GlintTexelLUT10Off                            0x7ed0
#define GlintTexelLUT10Sec                            0x8ed0
#define GlintTexelLUT10SecReg                         3
#define GlintTexelLUT10SecOff                         0x7ed0

#define GlintTexelLUT11                               0x8ed8
#define GlintTexelLUT11Tag                            0x01db
#define GlintTexelLUT11Reg                                 1
#define GlintTexelLUT11Off                            0x7ed8
#define GlintTexelLUT11Sec                            0x8ed8
#define GlintTexelLUT11SecReg                         3
#define GlintTexelLUT11SecOff                         0x7ed8

#define GlintTexelLUT12                               0x8ee0
#define GlintTexelLUT12Tag                            0x01dc
#define GlintTexelLUT12Reg                                 1
#define GlintTexelLUT12Off                            0x7ee0
#define GlintTexelLUT12Sec                            0x8ee0
#define GlintTexelLUT12SecReg                         3
#define GlintTexelLUT12SecOff                         0x7ee0

#define GlintTexelLUT13                               0x8ee8
#define GlintTexelLUT13Tag                            0x01dd
#define GlintTexelLUT13Reg                                 1
#define GlintTexelLUT13Off                            0x7ee8
#define GlintTexelLUT13Sec                            0x8ee8
#define GlintTexelLUT13SecReg                         3
#define GlintTexelLUT13SecOff                         0x7ee8

#define GlintTexelLUT14                               0x8ef0
#define GlintTexelLUT14Tag                            0x01de
#define GlintTexelLUT14Reg                                 1
#define GlintTexelLUT14Off                            0x7ef0
#define GlintTexelLUT14Sec                            0x8ef0
#define GlintTexelLUT14SecReg                         3
#define GlintTexelLUT14SecOff                         0x7ef0

#define GlintTexelLUT15                               0x8ef8
#define GlintTexelLUT15Tag                            0x01df
#define GlintTexelLUT15Reg                                 1
#define GlintTexelLUT15Off                            0x7ef8
#define GlintTexelLUT15Sec                            0x8ef8
#define GlintTexelLUT15SecReg                         3
#define GlintTexelLUT15SecOff                         0x7ef8

#define GlintTexel0                                   0x8600
#define GlintTexel0Tag                                0x00c0
#define GlintTexel0Reg                                     1
#define GlintTexel0Off                                0x7600
#define GlintTexel0Sec                                0x8600
#define GlintTexel0SecReg                             3
#define GlintTexel0SecOff                             0x7600

#define GlintTexel1                                   0x8608
#define GlintTexel1Tag                                0x00c1
#define GlintTexel1Reg                                     1
#define GlintTexel1Off                                0x7608
#define GlintTexel1Sec                                0x8608
#define GlintTexel1SecReg                             3
#define GlintTexel1SecOff                             0x7608

#define GlintTexel2                                   0x8610
#define GlintTexel2Tag                                0x00c2
#define GlintTexel2Reg                                     1
#define GlintTexel2Off                                0x7610
#define GlintTexel2Sec                                0x8610
#define GlintTexel2SecReg                             3
#define GlintTexel2SecOff                             0x7610

#define GlintTexel3                                   0x8618
#define GlintTexel3Tag                                0x00c3
#define GlintTexel3Reg                                     1
#define GlintTexel3Off                                0x7618
#define GlintTexel3Sec                                0x8618
#define GlintTexel3SecReg                             3
#define GlintTexel3SecOff                             0x7618

#define GlintTexel4                                   0x8620
#define GlintTexel4Tag                                0x00c4
#define GlintTexel4Reg                                     1
#define GlintTexel4Off                                0x7620
#define GlintTexel4Sec                                0x8620
#define GlintTexel4SecReg                             3
#define GlintTexel4SecOff                             0x7620

#define GlintTexel5                                   0x8628
#define GlintTexel5Tag                                0x00c5
#define GlintTexel5Reg                                     1
#define GlintTexel5Off                                0x7628
#define GlintTexel5Sec                                0x8628
#define GlintTexel5SecReg                             3
#define GlintTexel5SecOff                             0x7628

#define GlintTexel6                                   0x8630
#define GlintTexel6Tag                                0x00c6
#define GlintTexel6Reg                                     1
#define GlintTexel6Off                                0x7630
#define GlintTexel6Sec                                0x8630
#define GlintTexel6SecReg                             3
#define GlintTexel6SecOff                             0x7630

#define GlintTexel7                                   0x8638
#define GlintTexel7Tag                                0x00c7
#define GlintTexel7Reg                                     1
#define GlintTexel7Off                                0x7638
#define GlintTexel7Sec                                0x8638
#define GlintTexel7SecReg                             3
#define GlintTexel7SecOff                             0x7638

#define GlintInterp0                                  0x8640
#define GlintInterp0Tag                               0x00c8
#define GlintInterp0Reg                                    1
#define GlintInterp0Off                               0x7640
#define GlintInterp0Sec                               0x8640
#define GlintInterp0SecReg                            3
#define GlintInterp0SecOff                            0x7640

#define GlintInterp1                                  0x8648
#define GlintInterp1Tag                               0x00c9
#define GlintInterp1Reg                                    1
#define GlintInterp1Off                               0x7648
#define GlintInterp1Sec                               0x8648
#define GlintInterp1SecReg                            3
#define GlintInterp1SecOff                            0x7648

#define GlintInterp2                                  0x8650
#define GlintInterp2Tag                               0x00ca
#define GlintInterp2Reg                                    1
#define GlintInterp2Off                               0x7650
#define GlintInterp2Sec                               0x8650
#define GlintInterp2SecReg                            3
#define GlintInterp2SecOff                            0x7650

#define GlintInterp3                                  0x8658
#define GlintInterp3Tag                               0x00cb
#define GlintInterp3Reg                                    1
#define GlintInterp3Off                               0x7658
#define GlintInterp3Sec                               0x8658
#define GlintInterp3SecReg                            3
#define GlintInterp3SecOff                            0x7658

#define GlintInterp4                                  0x8660
#define GlintInterp4Tag                               0x00cc
#define GlintInterp4Reg                                    1
#define GlintInterp4Off                               0x7660
#define GlintInterp4Sec                               0x8660
#define GlintInterp4SecReg                            3
#define GlintInterp4SecOff                            0x7660

#define GlintTextureFilter                            0x8668
#define GlintTextureFilterTag                         0x00cd
#define GlintTextureFilterReg                              1
#define GlintTextureFilterOff                         0x7668
#define GlintTextureFilterSec                         0x8668
#define GlintTextureFilterSecReg                      3
#define GlintTextureFilterSecOff                      0x7668

#define GlintTextureColorMode                         0x8680
#define GlintTextureColorModeTag                      0x00d0
#define GlintTextureColorModeReg                           1
#define GlintTextureColorModeOff                      0x7680
#define GlintTextureColorModeSec                      0x8680
#define GlintTextureColorModeSecReg                   3
#define GlintTextureColorModeSecOff                   0x7680

#define GlintTextureEnvColor                          0x8688
#define GlintTextureEnvColorTag                       0x00d1
#define GlintTextureEnvColorReg                            1
#define GlintTextureEnvColorOff                       0x7688
#define GlintTextureEnvColorSec                       0x8688
#define GlintTextureEnvColorSecReg                    3
#define GlintTextureEnvColorSecOff                    0x7688

#define GlintFogMode                                  0x8690
#define GlintFogModeTag                               0x00d2
#define GlintFogModeReg                                    1
#define GlintFogModeOff                               0x7690
#define GlintFogModeSec                               0x8690
#define GlintFogModeSecReg                            3
#define GlintFogModeSecOff                            0x7690

#define GlintFogColor                                 0x8698
#define GlintFogColorTag                              0x00d3
#define GlintFogColorReg                                   1
#define GlintFogColorOff                              0x7698
#define GlintFogColorSec                              0x8698
#define GlintFogColorSecReg                           3
#define GlintFogColorSecOff                           0x7698

#define GlintFStart                                   0x86a0
#define GlintFStartTag                                0x00d4
#define GlintFStartReg                                     1
#define GlintFStartOff                                0x76a0
#define GlintFStartSec                                0x86a0
#define GlintFStartSecReg                             3
#define GlintFStartSecOff                             0x76a0

#define GlintdFdx                                     0x86a8
#define GlintdFdxTag                                  0x00d5
#define GlintdFdxReg                                       1
#define GlintdFdxOff                                  0x76a8
#define GlintdFdxSec                                  0x86a8
#define GlintdFdxSecReg                               3
#define GlintdFdxSecOff                               0x76a8

#define GlintdFdyDom                                  0x86b0
#define GlintdFdyDomTag                               0x00d6
#define GlintdFdyDomReg                                    1
#define GlintdFdyDomOff                               0x76b0
#define GlintdFdyDomSec                               0x86b0
#define GlintdFdyDomSecReg                            3
#define GlintdFdyDomSecOff                            0x76b0

#define GlintKsStart                                  0x86c8
#define GlintKsStartTag                               0x00d9
#define GlintKsStartReg                                    1
#define GlintKsStartOff                               0x76c8
#define GlintKsStartSec                               0x86c8
#define GlintKsStartSecReg                            3
#define GlintKsStartSecOff                            0x76c8

#define GlintdKsdx                                    0x86d0
#define GlintdKsdxTag                                 0x00da
#define GlintdKsdxReg                                      1
#define GlintdKsdxOff                                 0x76d0
#define GlintdKsdxSec                                 0x86d0
#define GlintdKsdxSecReg                              3
#define GlintdKsdxSecOff                              0x76d0

#define GlintdKsdyDom                                 0x86d8
#define GlintdKsdyDomTag                              0x00db
#define GlintdKsdyDomReg                                   1
#define GlintdKsdyDomOff                              0x76d8
#define GlintdKsdyDomSec                              0x86d8
#define GlintdKsdyDomSecReg                           3
#define GlintdKsdyDomSecOff                           0x76d8

#define GlintKdStart                                  0x86e0
#define GlintKdStartTag                               0x00dc
#define GlintKdStartReg                                    1
#define GlintKdStartOff                               0x76e0
#define GlintKdStartSec                               0x86e0
#define GlintKdStartSecReg                            3
#define GlintKdStartSecOff                            0x76e0

#define GlintdKdStart                                 0x86e8
#define GlintdKdStartTag                              0x00dd
#define GlintdKdStartReg                                   1
#define GlintdKdStartOff                              0x76e8
#define GlintdKdStartSec                              0x86e8
#define GlintdKdStartSecReg                           3
#define GlintdKdStartSecOff                           0x76e8

#define GlintdKddyDom                                 0x86f0
#define GlintdKddyDomTag                              0x00de
#define GlintdKddyDomReg                                   1
#define GlintdKddyDomOff                              0x76f0
#define GlintdKddyDomSec                              0x86f0
#define GlintdKddyDomSecReg                           3
#define GlintdKddyDomSecOff                           0x76f0

#define GlintRStart                                   0x8780
#define GlintRStartTag                                0x00f0
#define GlintRStartReg                                     1
#define GlintRStartOff                                0x7780
#define GlintRStartSec                                0x8780
#define GlintRStartSecReg                             3
#define GlintRStartSecOff                             0x7780

#define GlintdRdx                                     0x8788
#define GlintdRdxTag                                  0x00f1
#define GlintdRdxReg                                       1
#define GlintdRdxOff                                  0x7788
#define GlintdRdxSec                                  0x8788
#define GlintdRdxSecReg                               3
#define GlintdRdxSecOff                               0x7788

#define GlintdRdyDom                                  0x8790
#define GlintdRdyDomTag                               0x00f2
#define GlintdRdyDomReg                                    1
#define GlintdRdyDomOff                               0x7790
#define GlintdRdyDomSec                               0x8790
#define GlintdRdyDomSecReg                            3
#define GlintdRdyDomSecOff                            0x7790

#define GlintGStart                                   0x8798
#define GlintGStartTag                                0x00f3
#define GlintGStartReg                                     1
#define GlintGStartOff                                0x7798
#define GlintGStartSec                                0x8798
#define GlintGStartSecReg                             3
#define GlintGStartSecOff                             0x7798

#define GlintdGdx                                     0x87a0
#define GlintdGdxTag                                  0x00f4
#define GlintdGdxReg                                       1
#define GlintdGdxOff                                  0x77a0
#define GlintdGdxSec                                  0x87a0
#define GlintdGdxSecReg                               3
#define GlintdGdxSecOff                               0x77a0

#define GlintdGdyDom                                  0x87a8
#define GlintdGdyDomTag                               0x00f5
#define GlintdGdyDomReg                                    1
#define GlintdGdyDomOff                               0x77a8
#define GlintdGdyDomSec                               0x87a8
#define GlintdGdyDomSecReg                            3
#define GlintdGdyDomSecOff                            0x77a8

#define GlintBStart                                   0x87b0
#define GlintBStartTag                                0x00f6
#define GlintBStartReg                                     1
#define GlintBStartOff                                0x77b0
#define GlintBStartSec                                0x87b0
#define GlintBStartSecReg                             3
#define GlintBStartSecOff                             0x77b0

#define GlintdBdx                                     0x87b8
#define GlintdBdxTag                                  0x00f7
#define GlintdBdxReg                                       1
#define GlintdBdxOff                                  0x77b8
#define GlintdBdxSec                                  0x87b8
#define GlintdBdxSecReg                               3
#define GlintdBdxSecOff                               0x77b8

#define GlintdBdyDom                                  0x87c0
#define GlintdBdyDomTag                               0x00f8
#define GlintdBdyDomReg                                    1
#define GlintdBdyDomOff                               0x77c0
#define GlintdBdyDomSec                               0x87c0
#define GlintdBdyDomSecReg                            3
#define GlintdBdyDomSecOff                            0x77c0

#define GlintAStart                                   0x87c8
#define GlintAStartTag                                0x00f9
#define GlintAStartReg                                     1
#define GlintAStartOff                                0x77c8
#define GlintAStartSec                                0x87c8
#define GlintAStartSecReg                             3
#define GlintAStartSecOff                             0x77c8

#define GlintdAdx                                     0x87d0
#define GlintdAdxTag                                  0x00fa
#define GlintdAdxReg                                       1
#define GlintdAdxOff                                  0x77d0
#define GlintdAdxSec                                  0x87d0
#define GlintdAdxSecReg                               3
#define GlintdAdxSecOff                               0x77d0

#define GlintdAdyDom                                  0x87d8
#define GlintdAdyDomTag                               0x00fb
#define GlintdAdyDomReg                                    1
#define GlintdAdyDomOff                               0x77d8
#define GlintdAdyDomSec                               0x87d8
#define GlintdAdyDomSecReg                            3
#define GlintdAdyDomSecOff                            0x77d8

#define GlintColorDDAMode                             0x87e0
#define GlintColorDDAModeTag                          0x00fc
#define GlintColorDDAModeReg                               1
#define GlintColorDDAModeOff                          0x77e0
#define GlintColorDDAModeSec                          0x87e0
#define GlintColorDDAModeSecReg                       3
#define GlintColorDDAModeSecOff                       0x77e0

#define GlintConstantColor                            0x87e8
#define GlintConstantColorTag                         0x00fd
#define GlintConstantColorReg                              1
#define GlintConstantColorOff                         0x77e8
#define GlintConstantColorSec                         0x87e8
#define GlintConstantColorSecReg                      3
#define GlintConstantColorSecOff                      0x77e8

#define GlintGLINTColor                               0x87f0
#define GlintGLINTColorTag                            0x00fe
#define GlintGLINTColorReg                                 1
#define GlintGLINTColorOff                            0x77f0
#define GlintGLINTColorSec                            0x87f0
#define GlintGLINTColorSecReg                         3
#define GlintGLINTColorSecOff                         0x77f0

#define GlintAlphaTestMode                            0x8800
#define GlintAlphaTestModeTag                         0x0100
#define GlintAlphaTestModeReg                              1
#define GlintAlphaTestModeOff                         0x7800
#define GlintAlphaTestModeSec                         0x8800
#define GlintAlphaTestModeSecReg                      3
#define GlintAlphaTestModeSecOff                      0x7800

#define GlintAntialiasMode                            0x8808
#define GlintAntialiasModeTag                         0x0101
#define GlintAntialiasModeReg                              1
#define GlintAntialiasModeOff                         0x7808
#define GlintAntialiasModeSec                         0x8808
#define GlintAntialiasModeSecReg                      3
#define GlintAntialiasModeSecOff                      0x7808

#define GlintAlphaBlendMode                           0x8810
#define GlintAlphaBlendModeTag                        0x0102
#define GlintAlphaBlendModeReg                             1
#define GlintAlphaBlendModeOff                        0x7810
#define GlintAlphaBlendModeSec                        0x8810
#define GlintAlphaBlendModeSecReg                     3
#define GlintAlphaBlendModeSecOff                     0x7810

#define GlintChromaUpper                              0x8f08
#define GlintChromaUpperTag                           0x01e1
#define GlintChromaUpperReg                                1
#define GlintChromaUpperOff                           0x7f08
#define GlintChromaUpperSec                           0x8f08
#define GlintChromaUpperSecReg                        3
#define GlintChromaUpperSecOff                        0x7f08

#define GlintChromaLower                              0x8f10
#define GlintChromaLowerTag                           0x01e2
#define GlintChromaLowerReg                                1
#define GlintChromaLowerOff                           0x7f10
#define GlintChromaLowerSec                           0x8f10
#define GlintChromaLowerSecReg                        3
#define GlintChromaLowerSecOff                        0x7f10

#define GlintChromaTestMode                           0x8f18
#define GlintChromaTestModeTag                        0x01e3
#define GlintChromaTestModeReg                             1
#define GlintChromaTestModeOff                        0x7f18
#define GlintChromaTestModeSec                        0x8f18
#define GlintChromaTestModeSecReg                     3
#define GlintChromaTestModeSecOff                     0x7f18

#define GlintDitherMode                               0x8818
#define GlintDitherModeTag                            0x0103
#define GlintDitherModeReg                                 1
#define GlintDitherModeOff                            0x7818
#define GlintDitherModeSec                            0x8818
#define GlintDitherModeSecReg                         3
#define GlintDitherModeSecOff                         0x7818

#define GlintFBSoftwareWriteMask                      0x8820
#define GlintFBSoftwareWriteMaskTag                   0x0104
#define GlintFBSoftwareWriteMaskReg                        1
#define GlintFBSoftwareWriteMaskOff                   0x7820
#define GlintFBSoftwareWriteMaskSec                   0x8820
#define GlintFBSoftwareWriteMaskSecReg                3
#define GlintFBSoftwareWriteMaskSecOff                0x7820

#define GlintLogicalOpMode                            0x8828
#define GlintLogicalOpModeTag                         0x0105
#define GlintLogicalOpModeReg                              1
#define GlintLogicalOpModeOff                         0x7828
#define GlintLogicalOpModeSec                         0x8828
#define GlintLogicalOpModeSecReg                      3
#define GlintLogicalOpModeSecOff                      0x7828

#define GlintFBWriteData                              0x8830
#define GlintFBWriteDataTag                           0x0106
#define GlintFBWriteDataReg                                1
#define GlintFBWriteDataOff                           0x7830
#define GlintFBWriteDataSec                           0x8830
#define GlintFBWriteDataSecReg                        3
#define GlintFBWriteDataSecOff                        0x7830

#define GlintLBReadMode                               0x8880
#define GlintLBReadModeTag                            0x0110
#define GlintLBReadModeReg                                 1
#define GlintLBReadModeOff                            0x7880
#define GlintLBReadModeSec                            0x8880
#define GlintLBReadModeSecReg                         3
#define GlintLBReadModeSecOff                         0x7880

#define GlintLBReadFormat                             0x8888
#define GlintLBReadFormatTag                          0x0111
#define GlintLBReadFormatReg                               1
#define GlintLBReadFormatOff                          0x7888
#define GlintLBReadFormatSec                          0x8888
#define GlintLBReadFormatSecReg                       3
#define GlintLBReadFormatSecOff                       0x7888

#define GlintLBSourceOffset                           0x8890
#define GlintLBSourceOffsetTag                        0x0112
#define GlintLBSourceOffsetReg                             1
#define GlintLBSourceOffsetOff                        0x7890
#define GlintLBSourceOffsetSec                        0x8890
#define GlintLBSourceOffsetSecReg                     3
#define GlintLBSourceOffsetSecOff                     0x7890

#define GlintLBStencil                                0x88a8
#define GlintLBStencilTag                             0x0115
#define GlintLBStencilReg                                  1
#define GlintLBStencilOff                             0x78a8
#define GlintLBStencilSec                             0x88a8
#define GlintLBStencilSecReg                          3
#define GlintLBStencilSecOff                          0x78a8

#define GlintLBDepth                                  0x88b0
#define GlintLBDepthTag                               0x0116
#define GlintLBDepthReg                                    1
#define GlintLBDepthOff                               0x78b0
#define GlintLBDepthSec                               0x88b0
#define GlintLBDepthSecReg                            3
#define GlintLBDepthSecOff                            0x78b0

#define GlintLBWindowBase                             0x88b8
#define GlintLBWindowBaseTag                          0x0117
#define GlintLBWindowBaseReg                               1
#define GlintLBWindowBaseOff                          0x78b8
#define GlintLBWindowBaseSec                          0x88b8
#define GlintLBWindowBaseSecReg                       3
#define GlintLBWindowBaseSecOff                       0x78b8

#define GlintLBWriteMode                              0x88c0
#define GlintLBWriteModeTag                           0x0118
#define GlintLBWriteModeReg                                1
#define GlintLBWriteModeOff                           0x78c0
#define GlintLBWriteModeSec                           0x88c0
#define GlintLBWriteModeSecReg                        3
#define GlintLBWriteModeSecOff                        0x78c0

#define GlintLBWriteFormat                            0x88c8
#define GlintLBWriteFormatTag                         0x0119
#define GlintLBWriteFormatReg                              1
#define GlintLBWriteFormatOff                         0x78c8
#define GlintLBWriteFormatSec                         0x88c8
#define GlintLBWriteFormatSecReg                      3
#define GlintLBWriteFormatSecOff                      0x78c8

#define GlintTextureData                              0x88e8
#define GlintTextureDataTag                           0x011d
#define GlintTextureDataReg                                1
#define GlintTextureDataOff                           0x78e8
#define GlintTextureDataSec                           0x88e8
#define GlintTextureDataSecReg                        3
#define GlintTextureDataSecOff                        0x78e8

#define GlintTextureDownloadOffset                    0x88f0
#define GlintTextureDownloadOffsetTag                 0x011e
#define GlintTextureDownloadOffsetReg                      1
#define GlintTextureDownloadOffsetOff                 0x78f0
#define GlintTextureDownloadOffsetSec                 0x88f0
#define GlintTextureDownloadOffsetSecReg              3
#define GlintTextureDownloadOffsetSecOff              0x78f0

#define GlintLBWindowOffset                           0x88f8
#define GlintLBWindowOffsetTag                        0x011f
#define GlintLBWindowOffsetReg                             1
#define GlintLBWindowOffsetOff                        0x78f8
#define GlintLBWindowOffsetSec                        0x88f8
#define GlintLBWindowOffsetSecReg                     3
#define GlintLBWindowOffsetSecOff                     0x78f8

#define GlintGLINTWindow                              0x8980
#define GlintGLINTWindowTag                           0x0130
#define GlintGLINTWindowReg                                1
#define GlintGLINTWindowOff                           0x7980
#define GlintGLINTWindowSec                           0x8980
#define GlintGLINTWindowSecReg                        3
#define GlintGLINTWindowSecOff                        0x7980

#define GlintStencilMode                              0x8988
#define GlintStencilModeTag                           0x0131
#define GlintStencilModeReg                                1
#define GlintStencilModeOff                           0x7988
#define GlintStencilModeSec                           0x8988
#define GlintStencilModeSecReg                        3
#define GlintStencilModeSecOff                        0x7988

#define GlintStencilData                              0x8990
#define GlintStencilDataTag                           0x0132
#define GlintStencilDataReg                                1
#define GlintStencilDataOff                           0x7990
#define GlintStencilDataSec                           0x8990
#define GlintStencilDataSecReg                        3
#define GlintStencilDataSecOff                        0x7990

#define GlintGLINTStencil                             0x8998
#define GlintGLINTStencilTag                          0x0133
#define GlintGLINTStencilReg                               1
#define GlintGLINTStencilOff                          0x7998
#define GlintGLINTStencilSec                          0x8998
#define GlintGLINTStencilSecReg                       3
#define GlintGLINTStencilSecOff                       0x7998

#define GlintDepthMode                                0x89a0
#define GlintDepthModeTag                             0x0134
#define GlintDepthModeReg                                  1
#define GlintDepthModeOff                             0x79a0
#define GlintDepthModeSec                             0x89a0
#define GlintDepthModeSecReg                          3
#define GlintDepthModeSecOff                          0x79a0

#define GlintGLINTDepth                               0x89a8
#define GlintGLINTDepthTag                            0x0135
#define GlintGLINTDepthReg                                 1
#define GlintGLINTDepthOff                            0x79a8
#define GlintGLINTDepthSec                            0x89a8
#define GlintGLINTDepthSecReg                         3
#define GlintGLINTDepthSecOff                         0x79a8

#define GlintZStartU                                  0x89b0
#define GlintZStartUTag                               0x0136
#define GlintZStartUReg                                    1
#define GlintZStartUOff                               0x79b0
#define GlintZStartUSec                               0x89b0
#define GlintZStartUSecReg                            3
#define GlintZStartUSecOff                            0x79b0

#define GlintZStartL                                  0x89b8
#define GlintZStartLTag                               0x0137
#define GlintZStartLReg                                    1
#define GlintZStartLOff                               0x79b8
#define GlintZStartLSec                               0x89b8
#define GlintZStartLSecReg                            3
#define GlintZStartLSecOff                            0x79b8

#define GlintdZdxU                                    0x89c0
#define GlintdZdxUTag                                 0x0138
#define GlintdZdxUReg                                      1
#define GlintdZdxUOff                                 0x79c0
#define GlintdZdxUSec                                 0x89c0
#define GlintdZdxUSecReg                              3
#define GlintdZdxUSecOff                              0x79c0

#define GlintdZdxL                                    0x89c8
#define GlintdZdxLTag                                 0x0139
#define GlintdZdxLReg                                      1
#define GlintdZdxLOff                                 0x79c8
#define GlintdZdxLSec                                 0x89c8
#define GlintdZdxLSecReg                              3
#define GlintdZdxLSecOff                              0x79c8

#define GlintdZdyDomU                                 0x89d0
#define GlintdZdyDomUTag                              0x013a
#define GlintdZdyDomUReg                                   1
#define GlintdZdyDomUOff                              0x79d0
#define GlintdZdyDomUSec                              0x89d0
#define GlintdZdyDomUSecReg                           3
#define GlintdZdyDomUSecOff                           0x79d0

#define GlintdZdyDomL                                 0x89d8
#define GlintdZdyDomLTag                              0x013b
#define GlintdZdyDomLReg                                   1
#define GlintdZdyDomLOff                              0x79d8
#define GlintdZdyDomLSec                              0x89d8
#define GlintdZdyDomLSecReg                           3
#define GlintdZdyDomLSecOff                           0x79d8

#define GlintFastClearDepth                           0x89e0
#define GlintFastClearDepthTag                        0x013c
#define GlintFastClearDepthReg                             1
#define GlintFastClearDepthOff                        0x79e0
#define GlintFastClearDepthSec                        0x89e0
#define GlintFastClearDepthSecReg                     3
#define GlintFastClearDepthSecOff                     0x79e0

#define GlintFBReadMode                               0x8a80
#define GlintFBReadModeTag                            0x0150
#define GlintFBReadModeReg                                 1
#define GlintFBReadModeOff                            0x7a80
#define GlintFBReadModeSec                            0x8a80
#define GlintFBReadModeSecReg                         3
#define GlintFBReadModeSecOff                         0x7a80

#define GlintFBSourceOffset                           0x8a88
#define GlintFBSourceOffsetTag                        0x0151
#define GlintFBSourceOffsetReg                             1
#define GlintFBSourceOffsetOff                        0x7a88
#define GlintFBSourceOffsetSec                        0x8a88
#define GlintFBSourceOffsetSecReg                     3
#define GlintFBSourceOffsetSecOff                     0x7a88

#define GlintFBPixelOffset                            0x8a90
#define GlintFBPixelOffsetTag                         0x0152
#define GlintFBPixelOffsetReg                              1
#define GlintFBPixelOffsetOff                         0x7a90
#define GlintFBPixelOffsetSec                         0x8a90
#define GlintFBPixelOffsetSecReg                      3
#define GlintFBPixelOffsetSecOff                      0x7a90

#define GlintFBColor                                  0x8a98
#define GlintFBColorTag                               0x0153
#define GlintFBColorReg                                    1
#define GlintFBColorOff                               0x7a98
#define GlintFBColorSec                               0x8a98
#define GlintFBColorSecReg                            3
#define GlintFBColorSecOff                            0x7a98

#define GlintFBData                                   0x8aa0
#define GlintFBDataTag                                0x0154
#define GlintFBDataReg                                     1
#define GlintFBDataOff                                0x7aa0
#define GlintFBDataSec                                0x8aa0
#define GlintFBDataSecReg                             3
#define GlintFBDataSecOff                             0x7aa0

#define GlintFBSourceData                             0x8aa8
#define GlintFBSourceDataTag                          0x0155
#define GlintFBSourceDataReg                               1
#define GlintFBSourceDataOff                          0x7aa8
#define GlintFBSourceDataSec                          0x8aa8
#define GlintFBSourceDataSecReg                       3
#define GlintFBSourceDataSecOff                       0x7aa8

#define GlintFBWindowBase                             0x8ab0
#define GlintFBWindowBaseTag                          0x0156
#define GlintFBWindowBaseReg                               1
#define GlintFBWindowBaseOff                          0x7ab0
#define GlintFBWindowBaseSec                          0x8ab0
#define GlintFBWindowBaseSecReg                       3
#define GlintFBWindowBaseSecOff                       0x7ab0

#define GlintFBWriteMode                              0x8ab8
#define GlintFBWriteModeTag                           0x0157
#define GlintFBWriteModeReg                                1
#define GlintFBWriteModeOff                           0x7ab8
#define GlintFBWriteModeSec                           0x8ab8
#define GlintFBWriteModeSecReg                        3
#define GlintFBWriteModeSecOff                        0x7ab8

#define GlintFBHardwareWriteMask                      0x8ac0
#define GlintFBHardwareWriteMaskTag                   0x0158
#define GlintFBHardwareWriteMaskReg                        1
#define GlintFBHardwareWriteMaskOff                   0x7ac0
#define GlintFBHardwareWriteMaskSec                   0x8ac0
#define GlintFBHardwareWriteMaskSecReg                3
#define GlintFBHardwareWriteMaskSecOff                0x7ac0

#define GlintFBBlockColor                             0x8ac8
#define GlintFBBlockColorTag                          0x0159
#define GlintFBBlockColorReg                               1
#define GlintFBBlockColorOff                          0x7ac8
#define GlintFBBlockColorSec                          0x8ac8
#define GlintFBBlockColorSecReg                       3
#define GlintFBBlockColorSecOff                       0x7ac8

#define GlintPatternRamMode                           0x8af8
#define GlintPatternRamModeTag                        0x015f
#define GlintPatternRamModeReg                             1
#define GlintPatternRamModeOff                        0x7af8
#define GlintPatternRamModeSec                        0x8af8
#define GlintPatternRamModeSecReg                     3
#define GlintPatternRamModeSecOff                     0x7af8

#define GlintPatternRamData0                          0x8b00
#define GlintPatternRamData0Tag                       0x0160
#define GlintPatternRamData0Reg                            1
#define GlintPatternRamData0Off                       0x7b00
#define GlintPatternRamData0Sec                       0x8b00
#define GlintPatternRamData0SecReg                    3
#define GlintPatternRamData0SecOff                    0x7b00

#define GlintPatternRamData1                          0x8b08
#define GlintPatternRamData1Tag                       0x0161
#define GlintPatternRamData1Reg                            1
#define GlintPatternRamData1Off                       0x7b08
#define GlintPatternRamData1Sec                       0x8b08
#define GlintPatternRamData1SecReg                    3
#define GlintPatternRamData1SecOff                    0x7b08

#define GlintPatternRamData2                          0x8b10
#define GlintPatternRamData2Tag                       0x0162
#define GlintPatternRamData2Reg                            1
#define GlintPatternRamData2Off                       0x7b10
#define GlintPatternRamData2Sec                       0x8b10
#define GlintPatternRamData2SecReg                    3
#define GlintPatternRamData2SecOff                    0x7b10

#define GlintPatternRamData3                          0x8b18
#define GlintPatternRamData3Tag                       0x0163
#define GlintPatternRamData3Reg                            1
#define GlintPatternRamData3Off                       0x7b18
#define GlintPatternRamData3Sec                       0x8b18
#define GlintPatternRamData3SecReg                    3
#define GlintPatternRamData3SecOff                    0x7b18

#define GlintPatternRamData4                          0x8b20
#define GlintPatternRamData4Tag                       0x0164
#define GlintPatternRamData4Reg                            1
#define GlintPatternRamData4Off                       0x7b20
#define GlintPatternRamData4Sec                       0x8b20
#define GlintPatternRamData4SecReg                    3
#define GlintPatternRamData4SecOff                    0x7b20

#define GlintPatternRamData5                          0x8b28
#define GlintPatternRamData5Tag                       0x0165
#define GlintPatternRamData5Reg                            1
#define GlintPatternRamData5Off                       0x7b28
#define GlintPatternRamData5Sec                       0x8b28
#define GlintPatternRamData5SecReg                    3
#define GlintPatternRamData5SecOff                    0x7b28

#define GlintPatternRamData6                          0x8b30
#define GlintPatternRamData6Tag                       0x0166
#define GlintPatternRamData6Reg                            1
#define GlintPatternRamData6Off                       0x7b30
#define GlintPatternRamData6Sec                       0x8b30
#define GlintPatternRamData6SecReg                    3
#define GlintPatternRamData6SecOff                    0x7b30

#define GlintPatternRamData7                          0x8b38
#define GlintPatternRamData7Tag                       0x0167
#define GlintPatternRamData7Reg                            1
#define GlintPatternRamData7Off                       0x7b38
#define GlintPatternRamData7Sec                       0x8b38
#define GlintPatternRamData7SecReg                    3
#define GlintPatternRamData7SecOff                    0x7b38

#define GlintPatternRamData8                          0x8b40
#define GlintPatternRamData8Tag                       0x0168
#define GlintPatternRamData8Reg                            1
#define GlintPatternRamData8Off                       0x7b40
#define GlintPatternRamData8Sec                       0x8b40
#define GlintPatternRamData8SecReg                    3
#define GlintPatternRamData8SecOff                    0x7b40

#define GlintPatternRamData9                          0x8b48
#define GlintPatternRamData9Tag                       0x0169
#define GlintPatternRamData9Reg                            1
#define GlintPatternRamData9Off                       0x7b48
#define GlintPatternRamData9Sec                       0x8b48
#define GlintPatternRamData9SecReg                    3
#define GlintPatternRamData9SecOff                    0x7b48

#define GlintPatternRamData10                         0x8b50
#define GlintPatternRamData10Tag                      0x016a
#define GlintPatternRamData10Reg                           1
#define GlintPatternRamData10Off                      0x7b50
#define GlintPatternRamData10Sec                      0x8b50
#define GlintPatternRamData10SecReg                   3
#define GlintPatternRamData10SecOff                   0x7b50

#define GlintPatternRamData11                         0x8b58
#define GlintPatternRamData11Tag                      0x016b
#define GlintPatternRamData11Reg                           1
#define GlintPatternRamData11Off                      0x7b58
#define GlintPatternRamData11Sec                      0x8b58
#define GlintPatternRamData11SecReg                   3
#define GlintPatternRamData11SecOff                   0x7b58

#define GlintPatternRamData12                         0x8b60
#define GlintPatternRamData12Tag                      0x016c
#define GlintPatternRamData12Reg                           1
#define GlintPatternRamData12Off                      0x7b60
#define GlintPatternRamData12Sec                      0x8b60
#define GlintPatternRamData12SecReg                   3
#define GlintPatternRamData12SecOff                   0x7b60

#define GlintPatternRamData13                         0x8b68
#define GlintPatternRamData13Tag                      0x016d
#define GlintPatternRamData13Reg                           1
#define GlintPatternRamData13Off                      0x7b68
#define GlintPatternRamData13Sec                      0x8b68
#define GlintPatternRamData13SecReg                   3
#define GlintPatternRamData13SecOff                   0x7b68

#define GlintPatternRamData14                         0x8b70
#define GlintPatternRamData14Tag                      0x016e
#define GlintPatternRamData14Reg                           1
#define GlintPatternRamData14Off                      0x7b70
#define GlintPatternRamData14Sec                      0x8b70
#define GlintPatternRamData14SecReg                   3
#define GlintPatternRamData14SecOff                   0x7b70

#define GlintPatternRamData15                         0x8b78
#define GlintPatternRamData15Tag                      0x016f
#define GlintPatternRamData15Reg                           1
#define GlintPatternRamData15Off                      0x7b78
#define GlintPatternRamData15Sec                      0x8b78
#define GlintPatternRamData15SecReg                   3
#define GlintPatternRamData15SecOff                   0x7b78

#define GlintPatternRamData16                         0x8b80
#define GlintPatternRamData16Tag                      0x0170
#define GlintPatternRamData16Reg                           1
#define GlintPatternRamData16Off                      0x7b80
#define GlintPatternRamData16Sec                      0x8b80
#define GlintPatternRamData16SecReg                   3
#define GlintPatternRamData16SecOff                   0x7b80

#define GlintPatternRamData17                         0x8b88
#define GlintPatternRamData17Tag                      0x0171
#define GlintPatternRamData17Reg                           1
#define GlintPatternRamData17Off                      0x7b88
#define GlintPatternRamData17Sec                      0x8b88
#define GlintPatternRamData17SecReg                   3
#define GlintPatternRamData17SecOff                   0x7b88

#define GlintPatternRamData18                         0x8b90
#define GlintPatternRamData18Tag                      0x0172
#define GlintPatternRamData18Reg                           1
#define GlintPatternRamData18Off                      0x7b90
#define GlintPatternRamData18Sec                      0x8b90
#define GlintPatternRamData18SecReg                   3
#define GlintPatternRamData18SecOff                   0x7b90

#define GlintPatternRamData19                         0x8b98
#define GlintPatternRamData19Tag                      0x0173
#define GlintPatternRamData19Reg                           1
#define GlintPatternRamData19Off                      0x7b98
#define GlintPatternRamData19Sec                      0x8b98
#define GlintPatternRamData19SecReg                   3
#define GlintPatternRamData19SecOff                   0x7b98

#define GlintPatternRamData20                         0x8ba0
#define GlintPatternRamData20Tag                      0x0174
#define GlintPatternRamData20Reg                           1
#define GlintPatternRamData20Off                      0x7ba0
#define GlintPatternRamData20Sec                      0x8ba0
#define GlintPatternRamData20SecReg                   3
#define GlintPatternRamData20SecOff                   0x7ba0

#define GlintPatternRamData21                         0x8ba8
#define GlintPatternRamData21Tag                      0x0175
#define GlintPatternRamData21Reg                           1
#define GlintPatternRamData21Off                      0x7ba8
#define GlintPatternRamData21Sec                      0x8ba8
#define GlintPatternRamData21SecReg                   3
#define GlintPatternRamData21SecOff                   0x7ba8

#define GlintPatternRamData22                         0x8bb0
#define GlintPatternRamData22Tag                      0x0176
#define GlintPatternRamData22Reg                           1
#define GlintPatternRamData22Off                      0x7bb0
#define GlintPatternRamData22Sec                      0x8bb0
#define GlintPatternRamData22SecReg                   3
#define GlintPatternRamData22SecOff                   0x7bb0

#define GlintPatternRamData23                         0x8bb8
#define GlintPatternRamData23Tag                      0x0177
#define GlintPatternRamData23Reg                           1
#define GlintPatternRamData23Off                      0x7bb8
#define GlintPatternRamData23Sec                      0x8bb8
#define GlintPatternRamData23SecReg                   3
#define GlintPatternRamData23SecOff                   0x7bb8

#define GlintPatternRamData24                         0x8bc0
#define GlintPatternRamData24Tag                      0x0178
#define GlintPatternRamData24Reg                           1
#define GlintPatternRamData24Off                      0x7bc0
#define GlintPatternRamData24Sec                      0x8bc0
#define GlintPatternRamData24SecReg                   3
#define GlintPatternRamData24SecOff                   0x7bc0

#define GlintPatternRamData25                         0x8bc8
#define GlintPatternRamData25Tag                      0x0179
#define GlintPatternRamData25Reg                           1
#define GlintPatternRamData25Off                      0x7bc8
#define GlintPatternRamData25Sec                      0x8bc8
#define GlintPatternRamData25SecReg                   3
#define GlintPatternRamData25SecOff                   0x7bc8

#define GlintPatternRamData26                         0x8bd0
#define GlintPatternRamData26Tag                      0x017a
#define GlintPatternRamData26Reg                           1
#define GlintPatternRamData26Off                      0x7bd0
#define GlintPatternRamData26Sec                      0x8bd0
#define GlintPatternRamData26SecReg                   3
#define GlintPatternRamData26SecOff                   0x7bd0

#define GlintPatternRamData27                         0x8bd8
#define GlintPatternRamData27Tag                      0x017b
#define GlintPatternRamData27Reg                           1
#define GlintPatternRamData27Off                      0x7bd8
#define GlintPatternRamData27Sec                      0x8bd8
#define GlintPatternRamData27SecReg                   3
#define GlintPatternRamData27SecOff                   0x7bd8

#define GlintPatternRamData28                         0x8be0
#define GlintPatternRamData28Tag                      0x017c
#define GlintPatternRamData28Reg                           1
#define GlintPatternRamData28Off                      0x7be0
#define GlintPatternRamData28Sec                      0x8be0
#define GlintPatternRamData28SecReg                   3
#define GlintPatternRamData28SecOff                   0x7be0

#define GlintPatternRamData29                         0x8be8
#define GlintPatternRamData29Tag                      0x017d
#define GlintPatternRamData29Reg                           1
#define GlintPatternRamData29Off                      0x7be8
#define GlintPatternRamData29Sec                      0x8be8
#define GlintPatternRamData29SecReg                   3
#define GlintPatternRamData29SecOff                   0x7be8

#define GlintPatternRamData30                         0x8bf0
#define GlintPatternRamData30Tag                      0x017e
#define GlintPatternRamData30Reg                           1
#define GlintPatternRamData30Off                      0x7bf0
#define GlintPatternRamData30Sec                      0x8bf0
#define GlintPatternRamData30SecReg                   3
#define GlintPatternRamData30SecOff                   0x7bf0

#define GlintPatternRamData31                         0x8bf8
#define GlintPatternRamData31Tag                      0x017f
#define GlintPatternRamData31Reg                           1
#define GlintPatternRamData31Off                      0x7bf8
#define GlintPatternRamData31Sec                      0x8bf8
#define GlintPatternRamData31SecReg                   3
#define GlintPatternRamData31SecOff                   0x7bf8

#define GlintFBBlockColorU                            0x8c68
#define GlintFBBlockColorUTag                         0x018d
#define GlintFBBlockColorUReg                              1
#define GlintFBBlockColorUOff                         0x7c68
#define GlintFBBlockColorUSec                         0x8c68
#define GlintFBBlockColorUSecReg                      3
#define GlintFBBlockColorUSecOff                      0x7c68

#define GlintFBBlockColorL                            0x8c70
#define GlintFBBlockColorLTag                         0x018e
#define GlintFBBlockColorLReg                              1
#define GlintFBBlockColorLOff                         0x7c70
#define GlintFBBlockColorLSec                         0x8c70
#define GlintFBBlockColorLSecReg                      3
#define GlintFBBlockColorLSecOff                      0x7c70

#define GlintSuspendUntilFrameBlank                   0x8c78
#define GlintSuspendUntilFrameBlankTag                0x018f
#define GlintSuspendUntilFrameBlankReg                     1
#define GlintSuspendUntilFrameBlankOff                0x7c78
#define GlintSuspendUntilFrameBlankSec                0x8c78
#define GlintSuspendUntilFrameBlankSecReg             3
#define GlintSuspendUntilFrameBlankSecOff             0x7c78

#define GlintFilterMode                               0x8c00
#define GlintFilterModeTag                            0x0180
#define GlintFilterModeReg                                 1
#define GlintFilterModeOff                            0x7c00
#define GlintFilterModeSec                            0x8c00
#define GlintFilterModeSecReg                         3
#define GlintFilterModeSecOff                         0x7c00

#define GlintStatisticMode                            0x8c08
#define GlintStatisticModeTag                         0x0181
#define GlintStatisticModeReg                              1
#define GlintStatisticModeOff                         0x7c08
#define GlintStatisticModeSec                         0x8c08
#define GlintStatisticModeSecReg                      3
#define GlintStatisticModeSecOff                      0x7c08

#define GlintMinRegion                                0x8c10
#define GlintMinRegionTag                             0x0182
#define GlintMinRegionReg                                  1
#define GlintMinRegionOff                             0x7c10
#define GlintMinRegionSec                             0x8c10
#define GlintMinRegionSecReg                          3
#define GlintMinRegionSecOff                          0x7c10

#define GlintMaxRegion                                0x8c18
#define GlintMaxRegionTag                             0x0183
#define GlintMaxRegionReg                                  1
#define GlintMaxRegionOff                             0x7c18
#define GlintMaxRegionSec                             0x8c18
#define GlintMaxRegionSecReg                          3
#define GlintMaxRegionSecOff                          0x7c18

#define GlintResetPickResult                          0x8c20
#define GlintResetPickResultTag                       0x0184
#define GlintResetPickResultReg                            1
#define GlintResetPickResultOff                       0x7c20
#define GlintResetPickResultSec                       0x8c20
#define GlintResetPickResultSecReg                    3
#define GlintResetPickResultSecOff                    0x7c20

#define GlintMitHitRegion                             0x8c28
#define GlintMitHitRegionTag                          0x0185
#define GlintMitHitRegionReg                               1
#define GlintMitHitRegionOff                          0x7c28
#define GlintMitHitRegionSec                          0x8c28
#define GlintMitHitRegionSecReg                       3
#define GlintMitHitRegionSecOff                       0x7c28

#define GlintMaxHitRegion                             0x8c30
#define GlintMaxHitRegionTag                          0x0186
#define GlintMaxHitRegionReg                               1
#define GlintMaxHitRegionOff                          0x7c30
#define GlintMaxHitRegionSec                          0x8c30
#define GlintMaxHitRegionSecReg                       3
#define GlintMaxHitRegionSecOff                       0x7c30

#define GlintPickResult                               0x8c38
#define GlintPickResultTag                            0x0187
#define GlintPickResultReg                                 1
#define GlintPickResultOff                            0x7c38
#define GlintPickResultSec                            0x8c38
#define GlintPickResultSecReg                         3
#define GlintPickResultSecOff                         0x7c38

#define GlintGLINTSync                                0x8c40
#define GlintGLINTSyncTag                             0x0188
#define GlintGLINTSyncReg                                  1
#define GlintGLINTSyncOff                             0x7c40
#define GlintGLINTSyncSec                             0x8c40
#define GlintGLINTSyncSecReg                          3
#define GlintGLINTSyncSecOff                          0x7c40

#define GlintKsRStart                                 0x8c80
#define GlintKsRStartTag                              0x0190
#define GlintKsRStartReg                                   1
#define GlintKsRStartOff                              0x7c80
#define GlintKsRStartSec                              0x8c80
#define GlintKsRStartSecReg                           3
#define GlintKsRStartSecOff                           0x7c80

#define GlintdKsRdx                                   0x8c88
#define GlintdKsRdxTag                                0x0191
#define GlintdKsRdxReg                                     1
#define GlintdKsRdxOff                                0x7c88
#define GlintdKsRdxSec                                0x8c88
#define GlintdKsRdxSecReg                             3
#define GlintdKsRdxSecOff                             0x7c88

#define GlintdKsRdyDom                                0x8c90
#define GlintdKsRdyDomTag                             0x0192
#define GlintdKsRdyDomReg                                  1
#define GlintdKsRdyDomOff                             0x7c90
#define GlintdKsRdyDomSec                             0x8c90
#define GlintdKsRdyDomSecReg                          3
#define GlintdKsRdyDomSecOff                          0x7c90

#define GlintKsGStart                                 0x8c98
#define GlintKsGStartTag                              0x0193
#define GlintKsGStartReg                                   1
#define GlintKsGStartOff                              0x7c98
#define GlintKsGStartSec                              0x8c98
#define GlintKsGStartSecReg                           3
#define GlintKsGStartSecOff                           0x7c98

#define GlintdKsGdx                                   0x8ca0
#define GlintdKsGdxTag                                0x0194
#define GlintdKsGdxReg                                     1
#define GlintdKsGdxOff                                0x7ca0
#define GlintdKsGdxSec                                0x8ca0
#define GlintdKsGdxSecReg                             3
#define GlintdKsGdxSecOff                             0x7ca0

#define GlintdKsGdyDom                                0x8ca8
#define GlintdKsGdyDomTag                             0x0195
#define GlintdKsGdyDomReg                                  1
#define GlintdKsGdyDomOff                             0x7ca8
#define GlintdKsGdyDomSec                             0x8ca8
#define GlintdKsGdyDomSecReg                          3
#define GlintdKsGdyDomSecOff                          0x7ca8

#define GlintKsBStart                                 0x8cb0
#define GlintKsBStartTag                              0x0196
#define GlintKsBStartReg                                   1
#define GlintKsBStartOff                              0x7cb0
#define GlintKsBStartSec                              0x8cb0
#define GlintKsBStartSecReg                           3
#define GlintKsBStartSecOff                           0x7cb0

#define GlintdKsBdx                                   0x8cb8
#define GlintdKsBdxTag                                0x0197
#define GlintdKsBdxReg                                     1
#define GlintdKsBdxOff                                0x7cb8
#define GlintdKsBdxSec                                0x8cb8
#define GlintdKsBdxSecReg                             3
#define GlintdKsBdxSecOff                             0x7cb8

#define GlintdKsBdyDom                                0x8cc0
#define GlintdKsBdyDomTag                             0x0198
#define GlintdKsBdyDomReg                                  1
#define GlintdKsBdyDomOff                             0x7cc0
#define GlintdKsBdyDomSec                             0x8cc0
#define GlintdKsBdyDomSecReg                          3
#define GlintdKsBdyDomSecOff                          0x7cc0

#define GlintKdRStart                                 0x8d00
#define GlintKdRStartTag                              0x01a0
#define GlintKdRStartReg                                   1
#define GlintKdRStartOff                              0x7d00
#define GlintKdRStartSec                              0x8d00
#define GlintKdRStartSecReg                           3
#define GlintKdRStartSecOff                           0x7d00

#define GlintdKdRdx                                   0x8d08
#define GlintdKdRdxTag                                0x01a1
#define GlintdKdRdxReg                                     1
#define GlintdKdRdxOff                                0x7d08
#define GlintdKdRdxSec                                0x8d08
#define GlintdKdRdxSecReg                             3
#define GlintdKdRdxSecOff                             0x7d08

#define GlintdKdRdyDom                                0x8d10
#define GlintdKdRdyDomTag                             0x01a2
#define GlintdKdRdyDomReg                                  1
#define GlintdKdRdyDomOff                             0x7d10
#define GlintdKdRdyDomSec                             0x8d10
#define GlintdKdRdyDomSecReg                          3
#define GlintdKdRdyDomSecOff                          0x7d10

#define GlintKdGStart                                 0x8d18
#define GlintKdGStartTag                              0x01a3
#define GlintKdGStartReg                                   1
#define GlintKdGStartOff                              0x7d18
#define GlintKdGStartSec                              0x8d18
#define GlintKdGStartSecReg                           3
#define GlintKdGStartSecOff                           0x7d18

#define GlintdKdGdx                                   0x8d20
#define GlintdKdGdxTag                                0x01a4
#define GlintdKdGdxReg                                     1
#define GlintdKdGdxOff                                0x7d20
#define GlintdKdGdxSec                                0x8d20
#define GlintdKdGdxSecReg                             3
#define GlintdKdGdxSecOff                             0x7d20

#define GlintdKdGdyDom                                0x8d28
#define GlintdKdGdyDomTag                             0x01a5
#define GlintdKdGdyDomReg                                  1
#define GlintdKdGdyDomOff                             0x7d28
#define GlintdKdGdyDomSec                             0x8d28
#define GlintdKdGdyDomSecReg                          3
#define GlintdKdGdyDomSecOff                          0x7d28

#define GlintKdBStart                                 0x8d30
#define GlintKdBStartTag                              0x01a6
#define GlintKdBStartReg                                   1
#define GlintKdBStartOff                              0x7d30
#define GlintKdBStartSec                              0x8d30
#define GlintKdBStartSecReg                           3
#define GlintKdBStartSecOff                           0x7d30

#define GlintdKdBdx                                   0x8d38
#define GlintdKdBdxTag                                0x01a7
#define GlintdKdBdxReg                                     1
#define GlintdKdBdxOff                                0x7d38
#define GlintdKdBdxSec                                0x8d38
#define GlintdKdBdxSecReg                             3
#define GlintdKdBdxSecOff                             0x7d38

#define GlintdKdBdyDom                                0x8d40
#define GlintdKdBdyDomTag                             0x01a8
#define GlintdKdBdyDomReg                                  1
#define GlintdKdBdyDomOff                             0x7d40
#define GlintdKdBdyDomSec                             0x8d40
#define GlintdKdBdyDomSecReg                          3
#define GlintdKdBdyDomSecOff                          0x7d40

#define GlintContextDump                              0x8dc0
#define GlintContextDumpTag                           0x01b8
#define GlintContextDumpReg                                1
#define GlintContextDumpOff                           0x7dc0

#define GlintContextRestore                           0x8dc8
#define GlintContextRestoreTag                        0x01b9
#define GlintContextRestoreReg                             1
#define GlintContextRestoreOff                        0x7dc8

#define GlintContextData                              0x8dd0
#define GlintContextDataTag                           0x01ba
#define GlintContextDataReg                                1
#define GlintContextDataOff                           0x7dd0

#define GlintFeedbackToken                            0x8f80
#define GlintFeedbackTokenTag                         0x01f0
#define GlintFeedbackTokenReg                              1
#define GlintFeedbackTokenOff                         0x7f80

#define GlintFeedbackX                                0x8f88
#define GlintFeedbackXTag                             0x01f1
#define GlintFeedbackXReg                                  1
#define GlintFeedbackXOff                             0x7f88

#define GlintFeedbackY                                0x8f90
#define GlintFeedbackYTag                             0x01f2
#define GlintFeedbackYReg                                  1
#define GlintFeedbackYOff                             0x7f90

#define GlintFeedbackZ                                0x8f98
#define GlintFeedbackZTag                             0x01f3
#define GlintFeedbackZReg                                  1
#define GlintFeedbackZOff                             0x7f98

#define GlintFeedbackW                                0x8fa0
#define GlintFeedbackWTag                             0x01f4
#define GlintFeedbackWReg                                  1
#define GlintFeedbackWOff                             0x7fa0

#define GlintFeedbackRed                              0x8fa8
#define GlintFeedbackRedTag                           0x01f5
#define GlintFeedbackRedReg                                1
#define GlintFeedbackRedOff                           0x7fa8

#define GlintFeedbackGreen                            0x8fb0
#define GlintFeedbackGreenTag                         0x01f6
#define GlintFeedbackGreenReg                              1
#define GlintFeedbackGreenOff                         0x7fb0

#define GlintFeedbackBlue                             0x8fb8
#define GlintFeedbackBlueTag                          0x01f7
#define GlintFeedbackBlueReg                               1
#define GlintFeedbackBlueOff                          0x7fb8

#define GlintFeedbackAlpha                            0x8fc0
#define GlintFeedbackAlphaTag                         0x01f8
#define GlintFeedbackAlphaReg                              1
#define GlintFeedbackAlphaOff                         0x7fc0

#define GlintFeedbackS                                0x8fc8
#define GlintFeedbackSTag                             0x01f9
#define GlintFeedbackSReg                                  1
#define GlintFeedbackSOff                             0x7fc8

#define GlintFeedbackT                                0x8fd0
#define GlintFeedbackTTag                             0x01fa
#define GlintFeedbackTReg                                  1
#define GlintFeedbackTOff                             0x7fd0

#define GlintFeedbackR                                0x8fd8
#define GlintFeedbackRTag                             0x01fb
#define GlintFeedbackRReg                                  1
#define GlintFeedbackROff                             0x7fd8

#define GlintFeedbackQ                                0x8fe0
#define GlintFeedbackQTag                             0x01fc
#define GlintFeedbackQReg                                  1
#define GlintFeedbackQOff                             0x7fe0

#define GlintSelectRecord                             0x8fe8
#define GlintSelectRecordTag                          0x01fd
#define GlintSelectRecordReg                               1
#define GlintSelectRecordOff                          0x7fe8

#define GlintPassThrough                              0x8ff0
#define GlintPassThroughTag                           0x01fe
#define GlintPassThroughReg                                1
#define GlintPassThroughOff                           0x7ff0

#define GlintEndOfFeedback                            0x8ff8
#define GlintEndOfFeedbackTag                         0x01ff
#define GlintEndOfFeedbackReg                              1
#define GlintEndOfFeedbackOff                         0x7ff8

#define GlintV0FixedS                                 0x9000
#define GlintV0FixedSTag                              0x0200
#define GlintV0FixedSReg                                   1
#define GlintV0FixedSOff                              0x8000

#define GlintV0FixedT                                 0x9008
#define GlintV0FixedTTag                              0x0201
#define GlintV0FixedTReg                                   1
#define GlintV0FixedTOff                              0x8008

#define GlintV0FixedQ                                 0x9010
#define GlintV0FixedQTag                              0x0202
#define GlintV0FixedQReg                                   1
#define GlintV0FixedQOff                              0x8010

#define GlintV0FixedKs                                0x9018
#define GlintV0FixedKsTag                             0x0203
#define GlintV0FixedKsReg                                  1
#define GlintV0FixedKsOff                             0x8018

#define GlintV0FixedKd                                0x9020
#define GlintV0FixedKdTag                             0x0204
#define GlintV0FixedKdReg                                  1
#define GlintV0FixedKdOff                             0x8020

#define GlintV0FixedR                                 0x9028
#define GlintV0FixedRTag                              0x0205
#define GlintV0FixedRReg                                   1
#define GlintV0FixedROff                              0x8028

#define GlintV0FixedG                                 0x9030
#define GlintV0FixedGTag                              0x0206
#define GlintV0FixedGReg                                   1
#define GlintV0FixedGOff                              0x8030

#define GlintV0FixedB                                 0x9038
#define GlintV0FixedBTag                              0x0207
#define GlintV0FixedBReg                                   1
#define GlintV0FixedBOff                              0x8038

#define GlintV0FixedA                                 0x9040
#define GlintV0FixedATag                              0x0208
#define GlintV0FixedAReg                                   1
#define GlintV0FixedAOff                              0x8040

#define GlintV0FixedF                                 0x9048
#define GlintV0FixedFTag                              0x0209
#define GlintV0FixedFReg                                   1
#define GlintV0FixedFOff                              0x8048

#define GlintV0FixedX                                 0x9050
#define GlintV0FixedXTag                              0x020a
#define GlintV0FixedXReg                                   1
#define GlintV0FixedXOff                              0x8050

#define GlintV0FixedY                                 0x9058
#define GlintV0FixedYTag                              0x020b
#define GlintV0FixedYReg                                   1
#define GlintV0FixedYOff                              0x8058

#define GlintV0FixedZ                                 0x9060
#define GlintV0FixedZTag                              0x020c
#define GlintV0FixedZReg                                   1
#define GlintV0FixedZOff                              0x8060

#define GlintV1FixedS                                 0x9080
#define GlintV1FixedSTag                              0x0210
#define GlintV1FixedSReg                                   1
#define GlintV1FixedSOff                              0x8080

#define GlintV1FixedT                                 0x9088
#define GlintV1FixedTTag                              0x0211
#define GlintV1FixedTReg                                   1
#define GlintV1FixedTOff                              0x8088

#define GlintV1FixedQ                                 0x9090
#define GlintV1FixedQTag                              0x0212
#define GlintV1FixedQReg                                   1
#define GlintV1FixedQOff                              0x8090

#define GlintV1FixedKs                                0x9098
#define GlintV1FixedKsTag                             0x0213
#define GlintV1FixedKsReg                                  1
#define GlintV1FixedKsOff                             0x8098

#define GlintV1FixedKd                                0x90a0
#define GlintV1FixedKdTag                             0x0214
#define GlintV1FixedKdReg                                  1
#define GlintV1FixedKdOff                             0x80a0

#define GlintV1FixedR                                 0x90a8
#define GlintV1FixedRTag                              0x0215
#define GlintV1FixedRReg                                   1
#define GlintV1FixedROff                              0x80a8

#define GlintV1FixedG                                 0x90b0
#define GlintV1FixedGTag                              0x0216
#define GlintV1FixedGReg                                   1
#define GlintV1FixedGOff                              0x80b0

#define GlintV1FixedB                                 0x90b8
#define GlintV1FixedBTag                              0x0217
#define GlintV1FixedBReg                                   1
#define GlintV1FixedBOff                              0x80b8

#define GlintV1FixedA                                 0x90c0
#define GlintV1FixedATag                              0x0218
#define GlintV1FixedAReg                                   1
#define GlintV1FixedAOff                              0x80c0

#define GlintV1FixedF                                 0x90c8
#define GlintV1FixedFTag                              0x0219
#define GlintV1FixedFReg                                   1
#define GlintV1FixedFOff                              0x80c8

#define GlintV1FixedX                                 0x90d0
#define GlintV1FixedXTag                              0x021a
#define GlintV1FixedXReg                                   1
#define GlintV1FixedXOff                              0x80d0

#define GlintV1FixedY                                 0x90d8
#define GlintV1FixedYTag                              0x021b
#define GlintV1FixedYReg                                   1
#define GlintV1FixedYOff                              0x80d8

#define GlintV1FixedZ                                 0x90e0
#define GlintV1FixedZTag                              0x021c
#define GlintV1FixedZReg                                   1
#define GlintV1FixedZOff                              0x80e0

#define GlintV2FixedS                                 0x9100
#define GlintV2FixedSTag                              0x0220
#define GlintV2FixedSReg                                   1
#define GlintV2FixedSOff                              0x8100

#define GlintV2FixedT                                 0x9108
#define GlintV2FixedTTag                              0x0221
#define GlintV2FixedTReg                                   1
#define GlintV2FixedTOff                              0x8108

#define GlintV2FixedQ                                 0x9110
#define GlintV2FixedQTag                              0x0222
#define GlintV2FixedQReg                                   1
#define GlintV2FixedQOff                              0x8110

#define GlintV2FixedKs                                0x9118
#define GlintV2FixedKsTag                             0x0223
#define GlintV2FixedKsReg                                  1
#define GlintV2FixedKsOff                             0x8118

#define GlintV2FixedKd                                0x9120
#define GlintV2FixedKdTag                             0x0224
#define GlintV2FixedKdReg                                  1
#define GlintV2FixedKdOff                             0x8120

#define GlintV2FixedR                                 0x9128
#define GlintV2FixedRTag                              0x0225
#define GlintV2FixedRReg                                   1
#define GlintV2FixedROff                              0x8128

#define GlintV2FixedG                                 0x9130
#define GlintV2FixedGTag                              0x0226
#define GlintV2FixedGReg                                   1
#define GlintV2FixedGOff                              0x8130

#define GlintV2FixedB                                 0x9138
#define GlintV2FixedBTag                              0x0227
#define GlintV2FixedBReg                                   1
#define GlintV2FixedBOff                              0x8138

#define GlintV2FixedA                                 0x9140
#define GlintV2FixedATag                              0x0228
#define GlintV2FixedAReg                                   1
#define GlintV2FixedAOff                              0x8140

#define GlintV2FixedF                                 0x9148
#define GlintV2FixedFTag                              0x0229
#define GlintV2FixedFReg                                   1
#define GlintV2FixedFOff                              0x8148

#define GlintV2FixedX                                 0x9150
#define GlintV2FixedXTag                              0x022a
#define GlintV2FixedXReg                                   1
#define GlintV2FixedXOff                              0x8150

#define GlintV2FixedY                                 0x9158
#define GlintV2FixedYTag                              0x022b
#define GlintV2FixedYReg                                   1
#define GlintV2FixedYOff                              0x8158

#define GlintV2FixedZ                                 0x9160
#define GlintV2FixedZTag                              0x022c
#define GlintV2FixedZReg                                   1
#define GlintV2FixedZOff                              0x8160

#define GlintV0FloatS                                 0x9180
#define GlintV0FloatSTag                              0x0230
#define GlintV0FloatSReg                                   1
#define GlintV0FloatSOff                              0x8180

#define GlintV0FloatT                                 0x9188
#define GlintV0FloatTTag                              0x0231
#define GlintV0FloatTReg                                   1
#define GlintV0FloatTOff                              0x8188

#define GlintV0FloatQ                                 0x9190
#define GlintV0FloatQTag                              0x0232
#define GlintV0FloatQReg                                   1
#define GlintV0FloatQOff                              0x8190

#define GlintV0FloatKs                                0x9198
#define GlintV0FloatKsTag                             0x0233
#define GlintV0FloatKsReg                                  1
#define GlintV0FloatKsOff                             0x8198

#define GlintV0FloatKd                                0x91a0
#define GlintV0FloatKdTag                             0x0234
#define GlintV0FloatKdReg                                  1
#define GlintV0FloatKdOff                             0x81a0

#define GlintV0FloatR                                 0x91a8
#define GlintV0FloatRTag                              0x0235
#define GlintV0FloatRReg                                   1
#define GlintV0FloatROff                              0x81a8

#define GlintV0FloatG                                 0x91b0
#define GlintV0FloatGTag                              0x0236
#define GlintV0FloatGReg                                   1
#define GlintV0FloatGOff                              0x81b0

#define GlintV0FloatB                                 0x91b8
#define GlintV0FloatBTag                              0x0237
#define GlintV0FloatBReg                                   1
#define GlintV0FloatBOff                              0x81b8

#define GlintV0FloatA                                 0x91c0
#define GlintV0FloatATag                              0x0238
#define GlintV0FloatAReg                                   1
#define GlintV0FloatAOff                              0x81c0

#define GlintV0FloatF                                 0x91c8
#define GlintV0FloatFTag                              0x0239
#define GlintV0FloatFReg                                   1
#define GlintV0FloatFOff                              0x81c8

#define GlintV0FloatX                                 0x91d0
#define GlintV0FloatXTag                              0x023a
#define GlintV0FloatXReg                                   1
#define GlintV0FloatXOff                              0x81d0

#define GlintV0FloatY                                 0x91d8
#define GlintV0FloatYTag                              0x023b
#define GlintV0FloatYReg                                   1
#define GlintV0FloatYOff                              0x81d8

#define GlintV0FloatZ                                 0x91e0
#define GlintV0FloatZTag                              0x023c
#define GlintV0FloatZReg                                   1
#define GlintV0FloatZOff                              0x81e0

#define GlintV1FloatS                                 0x9200
#define GlintV1FloatSTag                              0x0240
#define GlintV1FloatSReg                                   1
#define GlintV1FloatSOff                              0x8200

#define GlintV1FloatT                                 0x9208
#define GlintV1FloatTTag                              0x0241
#define GlintV1FloatTReg                                   1
#define GlintV1FloatTOff                              0x8208

#define GlintV1FloatQ                                 0x9210
#define GlintV1FloatQTag                              0x0242
#define GlintV1FloatQReg                                   1
#define GlintV1FloatQOff                              0x8210

#define GlintV1FloatKs                                0x9218
#define GlintV1FloatKsTag                             0x0243
#define GlintV1FloatKsReg                                  1
#define GlintV1FloatKsOff                             0x8218

#define GlintV1FloatKd                                0x9220
#define GlintV1FloatKdTag                             0x0244
#define GlintV1FloatKdReg                                  1
#define GlintV1FloatKdOff                             0x8220

#define GlintV1FloatR                                 0x9228
#define GlintV1FloatRTag                              0x0245
#define GlintV1FloatRReg                                   1
#define GlintV1FloatROff                              0x8228

#define GlintV1FloatG                                 0x9230
#define GlintV1FloatGTag                              0x0246
#define GlintV1FloatGReg                                   1
#define GlintV1FloatGOff                              0x8230

#define GlintV1FloatB                                 0x9238
#define GlintV1FloatBTag                              0x0247
#define GlintV1FloatBReg                                   1
#define GlintV1FloatBOff                              0x8238

#define GlintV1FloatA                                 0x9240
#define GlintV1FloatATag                              0x0248
#define GlintV1FloatAReg                                   1
#define GlintV1FloatAOff                              0x8240

#define GlintV1FloatF                                 0x9248
#define GlintV1FloatFTag                              0x0249
#define GlintV1FloatFReg                                   1
#define GlintV1FloatFOff                              0x8248

#define GlintV1FloatX                                 0x9250
#define GlintV1FloatXTag                              0x024a
#define GlintV1FloatXReg                                   1
#define GlintV1FloatXOff                              0x8250

#define GlintV1FloatY                                 0x9258
#define GlintV1FloatYTag                              0x024b
#define GlintV1FloatYReg                                   1
#define GlintV1FloatYOff                              0x8258

#define GlintV1FloatZ                                 0x9260
#define GlintV1FloatZTag                              0x024c
#define GlintV1FloatZReg                                   1
#define GlintV1FloatZOff                              0x8260

#define GlintV2FloatS                                 0x9280
#define GlintV2FloatSTag                              0x0250
#define GlintV2FloatSReg                                   1
#define GlintV2FloatSOff                              0x8280

#define GlintV2FloatT                                 0x9288
#define GlintV2FloatTTag                              0x0251
#define GlintV2FloatTReg                                   1
#define GlintV2FloatTOff                              0x8288

#define GlintV2FloatQ                                 0x9290
#define GlintV2FloatQTag                              0x0252
#define GlintV2FloatQReg                                   1
#define GlintV2FloatQOff                              0x8290

#define GlintV2FloatKs                                0x9298
#define GlintV2FloatKsTag                             0x0253
#define GlintV2FloatKsReg                                  1
#define GlintV2FloatKsOff                             0x8298

#define GlintV2FloatKd                                0x92a0
#define GlintV2FloatKdTag                             0x0254
#define GlintV2FloatKdReg                                  1
#define GlintV2FloatKdOff                             0x82a0

#define GlintV2FloatR                                 0x92a8
#define GlintV2FloatRTag                              0x0255
#define GlintV2FloatRReg                                   1
#define GlintV2FloatROff                              0x82a8

#define GlintV2FloatG                                 0x92b0
#define GlintV2FloatGTag                              0x0256
#define GlintV2FloatGReg                                   1
#define GlintV2FloatGOff                              0x82b0

#define GlintV2FloatB                                 0x92b8
#define GlintV2FloatBTag                              0x0257
#define GlintV2FloatBReg                                   1
#define GlintV2FloatBOff                              0x82b8

#define GlintV2FloatA                                 0x92c0
#define GlintV2FloatATag                              0x0258
#define GlintV2FloatAReg                                   1
#define GlintV2FloatAOff                              0x82c0

#define GlintV2FloatF                                 0x92c8
#define GlintV2FloatFTag                              0x0259
#define GlintV2FloatFReg                                   1
#define GlintV2FloatFOff                              0x82c8

#define GlintV2FloatX                                 0x92d0
#define GlintV2FloatXTag                              0x025a
#define GlintV2FloatXReg                                   1
#define GlintV2FloatXOff                              0x82d0

#define GlintV2FloatY                                 0x92d8
#define GlintV2FloatYTag                              0x025b
#define GlintV2FloatYReg                                   1
#define GlintV2FloatYOff                              0x82d8

#define GlintV2FloatZ                                 0x92e0
#define GlintV2FloatZTag                              0x025c
#define GlintV2FloatZReg                                   1
#define GlintV2FloatZOff                              0x82e0

#define GlintDeltaMode                                0x9300
#define GlintDeltaModeTag                             0x0260
#define GlintDeltaModeReg                                  1
#define GlintDeltaModeOff                             0x8300

#define GlintDrawTriangle                             0x9308
#define GlintDrawTriangleTag                          0x0261
#define GlintDrawTriangleReg                               1
#define GlintDrawTriangleOff                          0x8308

#define GlintRepeatTriangle                           0x9310
#define GlintRepeatTriangleTag                        0x0262
#define GlintRepeatTriangleReg                             1
#define GlintRepeatTriangleOff                        0x8310

#define GlintDrawLine01                               0x9318
#define GlintDrawLine01Tag                            0x0263
#define GlintDrawLine01Reg                                 1
#define GlintDrawLine01Off                            0x8318

#define GlintDrawLine10                               0x9320
#define GlintDrawLine10Tag                            0x0264
#define GlintDrawLine10Reg                                 1
#define GlintDrawLine10Off                            0x8320

#define GlintRepeatLine                               0x9328
#define GlintRepeatLineTag                            0x0265
#define GlintRepeatLineReg                                 1
#define GlintRepeatLineOff                            0x8328

#define GlintEpilogueTag                              0x9368
#define GlintEpilogueTagTag                           0x026d
#define GlintEpilogueTagReg                                1
#define GlintEpilogueTagOff                           0x8368

#define GlintEpilogueData                             0x9370
#define GlintEpilogueDataTag                          0x026e
#define GlintEpilogueDataReg                               1
#define GlintEpilogueDataOff                          0x8370

#define GlintBroadcastMask                            0x9378
#define GlintBroadcastMaskTag                         0x026f
#define GlintBroadcastMaskReg                              1
#define GlintBroadcastMaskOff                         0x8378

#define GlintXBias                                    0x9480
#define GlintXBiasTag                                 0x0290
#define GlintXBiasReg                                      1
#define GlintXBiasOff                                 0x8480

#define GlintYBias                                    0x9488
#define GlintYBiasTag                                 0x0291
#define GlintYBiasReg                                      1
#define GlintYBiasOff                                 0x8488

#define GlintPointMode                                0x9490
#define GlintPointModeTag                             0x0292
#define GlintPointModeReg                                  1
#define GlintPointModeOff                             0x8490

#define GlintPointSize                                0x9498
#define GlintPointSizeTag                             0x0293
#define GlintPointSizeReg                                  1
#define GlintPointSizeOff                             0x8498

#define GlintAApointSize                              0x94a0
#define GlintAApointSizeTag                           0x0294
#define GlintAApointSizeReg                                1
#define GlintAApointSizeOff                           0x84a0

#define GlintLineMode                                 0x94a8
#define GlintLineModeTag                              0x0295
#define GlintLineModeReg                                   1
#define GlintLineModeOff                              0x84a8

#define GlintLineWidth                                0x94b0
#define GlintLineWidthTag                             0x0296
#define GlintLineWidthReg                                  1
#define GlintLineWidthOff                             0x84b0

#define GlintLineWidthOffset                          0x94b8
#define GlintLineWidthOffsetTag                       0x0297
#define GlintLineWidthOffsetReg                            1
#define GlintLineWidthOffsetOff                       0x84b8

#define GlintAAlineWidth                              0x94c0
#define GlintAAlineWidthTag                           0x0298
#define GlintAAlineWidthReg                                1
#define GlintAAlineWidthOff                           0x84c0

#define GlintTriangleMode                             0x94c8
#define GlintTriangleModeTag                          0x0299
#define GlintTriangleModeReg                               1
#define GlintTriangleModeOff                          0x84c8

#define GlintRectangleMode                            0x94d0
#define GlintRectangleModeTag                         0x029a
#define GlintRectangleModeReg                              1
#define GlintRectangleModeOff                         0x84d0

#define GlintRectangleWidth                           0x94d8
#define GlintRectangleWidthTag                        0x029b
#define GlintRectangleWidthReg                             1
#define GlintRectangleWidthOff                        0x84d8

#define GlintRectangleHeight                          0x94e0
#define GlintRectangleHeightTag                       0x029c
#define GlintRectangleHeightReg                            1
#define GlintRectangleHeightOff                       0x84e0

#define GlintRectangle2DMode                          0x94e8
#define GlintRectangle2DModeTag                       0x029d
#define GlintRectangle2DModeReg                            1
#define GlintRectangle2DModeOff                       0x84e8

#define GlintRectangle2DControl                       0x94f0
#define GlintRectangle2DControlTag                    0x029e
#define GlintRectangle2DControlReg                         1
#define GlintRectangle2DControlOff                    0x84f0

#define GlintTransformMode                            0x9508
#define GlintTransformModeTag                         0x02a1
#define GlintTransformModeReg                              1
#define GlintTransformModeOff                         0x8508

#define GlintGeometryMode                             0x9510
#define GlintGeometryModeTag                          0x02a2
#define GlintGeometryModeReg                               1
#define GlintGeometryModeOff                          0x8510

#define GlintNormalizeMode                            0x9518
#define GlintNormalizeModeTag                         0x02a3
#define GlintNormalizeModeReg                              1
#define GlintNormalizeModeOff                         0x8518

#define GlintLightingMode                             0x9520
#define GlintLightingModeTag                          0x02a4
#define GlintLightingModeReg                               1
#define GlintLightingModeOff                          0x8520

#define GlintColorMaterialMode                        0x9528
#define GlintColorMaterialModeTag                     0x02a5
#define GlintColorMaterialModeReg                          1
#define GlintColorMaterialModeOff                     0x8528

#define GlintMaterialMode                             0x9530
#define GlintMaterialModeTag                          0x02a6
#define GlintMaterialModeReg                               1
#define GlintMaterialModeOff                          0x8530

#define GlintSelectResult                             0x9580
#define GlintSelectResultTag                          0x02b0
#define GlintSelectResultReg                               1
#define GlintSelectResultOff                          0x8580

#define GlintBegin                                    0x9590
#define GlintBeginTag                                 0x02b2
#define GlintBeginReg                                      1
#define GlintBeginOff                                 0x8590

#define GlintEnd                                      0x9598
#define GlintEndTag                                   0x02b3
#define GlintEndReg                                        1
#define GlintEndOff                                   0x8598

#define GlintEdgeFlag                                 0x95a0
#define GlintEdgeFlagTag                              0x02b4
#define GlintEdgeFlagReg                                   1
#define GlintEdgeFlagOff                              0x85a0

#define GlintObjectIDvalue                            0x95a8
#define GlintObjectIDvalueTag                         0x02b5
#define GlintObjectIDvalueReg                              1
#define GlintObjectIDvalueOff                         0x85a8

#define GlintIncrementObjectID                        0x95b0
#define GlintIncrementObjectIDTag                     0x02b6
#define GlintIncrementObjectIDReg                          1
#define GlintIncrementObjectIDOff                     0x85b0

#define GlintTransformCurrent                         0x95b8
#define GlintTransformCurrentTag                      0x02b7
#define GlintTransformCurrentReg                           1
#define GlintTransformCurrentOff                      0x85b8

#define GlintSaveCurrent                              0x95c8
#define GlintSaveCurrentTag                           0x02b9
#define GlintSaveCurrentReg                                1
#define GlintSaveCurrentOff                           0x85c8

#define GlintRestoreCurrent                           0x95d0
#define GlintRestoreCurrentTag                        0x02ba
#define GlintRestoreCurrentReg                             1
#define GlintRestoreCurrentOff                        0x85d0

#define GlintInitNames                                0x95d8
#define GlintInitNamesTag                             0x02bb
#define GlintInitNamesReg                                  1
#define GlintInitNamesOff                             0x85d8

#define GlintPushName                                 0x95e0
#define GlintPushNameTag                              0x02bc
#define GlintPushNameReg                                   1
#define GlintPushNameOff                              0x85e0

#define GlintPopName                                  0x95e8
#define GlintPopNameTag                               0x02bd
#define GlintPopNameReg                                    1
#define GlintPopNameOff                               0x85e8

#define GlintLoadName                                 0x95f0
#define GlintLoadNameTag                              0x02be
#define GlintLoadNameReg                                   1
#define GlintLoadNameOff                              0x85f0

#define GlintGeomRectangle                            0x96a0
#define GlintGeomRectangleTag                         0x02d4
#define GlintGeomRectangleReg                              1
#define GlintGeomRectangleOff                         0x86a0

#define GlintDrawRectangle2D                          0x97a0
#define GlintDrawRectangle2DTag                       0x02f4
#define GlintDrawRectangle2DReg                            1
#define GlintDrawRectangle2DOff                       0x87a0

#define GlintNz                                       0x9800
#define GlintNzTag                                    0x0300
#define GlintNzReg                                         1
#define GlintNzOff                                    0x8800

#define GlintNy                                       0x9808
#define GlintNyTag                                    0x0301
#define GlintNyReg                                         1
#define GlintNyOff                                    0x8808

#define GlintNx                                       0x9810
#define GlintNxTag                                    0x0302
#define GlintNxReg                                         1
#define GlintNxOff                                    0x8810

#define GlintCa                                       0x9818
#define GlintCaTag                                    0x0303
#define GlintCaReg                                         1
#define GlintCaOff                                    0x8818

#define GlintCb                                       0x9820
#define GlintCbTag                                    0x0304
#define GlintCbReg                                         1
#define GlintCbOff                                    0x8820

#define GlintCg                                       0x9828
#define GlintCgTag                                    0x0305
#define GlintCgReg                                         1
#define GlintCgOff                                    0x8828

#define GlintCr3                                      0x9830
#define GlintCr3Tag                                   0x0306
#define GlintCr3Reg                                        1
#define GlintCr3Off                                   0x8830

#define GlintCr4                                      0x9838
#define GlintCr4Tag                                   0x0307
#define GlintCr4Reg                                        1
#define GlintCr4Off                                   0x8838

#define GlintTt2                                      0x9840
#define GlintTt2Tag                                   0x0308
#define GlintTt2Reg                                        1
#define GlintTt2Off                                   0x8840

#define GlintTs2                                      0x9848
#define GlintTs2Tag                                   0x0309
#define GlintTs2Reg                                        1
#define GlintTs2Off                                   0x8848

#define GlintVw                                       0x9850
#define GlintVwTag                                    0x030a
#define GlintVwReg                                         1
#define GlintVwOff                                    0x8850

#define GlintVz                                       0x9858
#define GlintVzTag                                    0x030b
#define GlintVzReg                                         1
#define GlintVzOff                                    0x8858

#define GlintVy                                       0x9860
#define GlintVyTag                                    0x030c
#define GlintVyReg                                         1
#define GlintVyOff                                    0x8860

#define GlintVx2                                      0x9868
#define GlintVx2Tag                                   0x030d
#define GlintVx2Reg                                        1
#define GlintVx2Off                                   0x8868

#define GlintVx3                                      0x9870
#define GlintVx3Tag                                   0x030e
#define GlintVx3Reg                                        1
#define GlintVx3Off                                   0x8870

#define GlintVx4                                      0x9878
#define GlintVx4Tag                                   0x030f
#define GlintVx4Reg                                        1
#define GlintVx4Off                                   0x8878

#define GlintFNz                                      0x9880
#define GlintFNzTag                                   0x0310
#define GlintFNzReg                                        1
#define GlintFNzOff                                   0x8880

#define GlintFNy                                      0x9888
#define GlintFNyTag                                   0x0311
#define GlintFNyReg                                        1
#define GlintFNyOff                                   0x8888

#define GlintFNx                                      0x9890
#define GlintFNxTag                                   0x0312
#define GlintFNxReg                                        1
#define GlintFNxOff                                   0x8890

#define GlintPackedColor3                             0x9898
#define GlintPackedColor3Tag                          0x0313
#define GlintPackedColor3Reg                               1
#define GlintPackedColor3Off                          0x8898

#define GlintPackedColor4                             0x98a0
#define GlintPackedColor4Tag                          0x0314
#define GlintPackedColor4Reg                               1
#define GlintPackedColor4Off                          0x88a0

#define GlintTq4                                      0x98a8
#define GlintTq4Tag                                   0x0315
#define GlintTq4Reg                                        1
#define GlintTq4Off                                   0x88a8

#define GlintTr4                                      0x98b0
#define GlintTr4Tag                                   0x0316
#define GlintTr4Reg                                        1
#define GlintTr4Off                                   0x88b0

#define GlintTt4                                      0x98b8
#define GlintTt4Tag                                   0x0317
#define GlintTt4Reg                                        1
#define GlintTt4Off                                   0x88b8

#define GlintTs4                                      0x98c0
#define GlintTs4Tag                                   0x0318
#define GlintTs4Reg                                        1
#define GlintTs4Off                                   0x88c0

#define GlintRPw                                      0x98c8
#define GlintRPwTag                                   0x0319
#define GlintRPwReg                                        1
#define GlintRPwOff                                   0x88c8

#define GlintRPz                                      0x98d0
#define GlintRPzTag                                   0x031a
#define GlintRPzReg                                        1
#define GlintRPzOff                                   0x88d0

#define GlintRPy                                      0x98d8
#define GlintRPyTag                                   0x031b
#define GlintRPyReg                                        1
#define GlintRPyOff                                   0x88d8

#define GlintRPx2                                     0x98e0
#define GlintRPx2Tag                                  0x031c
#define GlintRPx2Reg                                       1
#define GlintRPx2Off                                  0x88e0

#define GlintRPx3                                     0x98e8
#define GlintRPx3Tag                                  0x031d
#define GlintRPx3Reg                                       1
#define GlintRPx3Off                                  0x88e8

#define GlintRPx4                                     0x98f0
#define GlintRPx4Tag                                  0x031e
#define GlintRPx4Reg                                       1
#define GlintRPx4Off                                  0x88f0

#define GlintTs1                                      0x98f8
#define GlintTs1Tag                                   0x031f
#define GlintTs1Reg                                        1
#define GlintTs1Off                                   0x88f8

#define GlintModelViewMatrix0                         0x9900
#define GlintModelViewMatrix0Tag                      0x0320
#define GlintModelViewMatrix0Reg                           1
#define GlintModelViewMatrix0Off                      0x8900

#define GlintModelViewMatrix1                         0x9908
#define GlintModelViewMatrix1Tag                      0x0321
#define GlintModelViewMatrix1Reg                           1
#define GlintModelViewMatrix1Off                      0x8908

#define GlintModelViewMatrix2                         0x9910
#define GlintModelViewMatrix2Tag                      0x0322
#define GlintModelViewMatrix2Reg                           1
#define GlintModelViewMatrix2Off                      0x8910

#define GlintModelViewMatrix3                         0x9918
#define GlintModelViewMatrix3Tag                      0x0323
#define GlintModelViewMatrix3Reg                           1
#define GlintModelViewMatrix3Off                      0x8918

#define GlintModelViewMatrix4                         0x9920
#define GlintModelViewMatrix4Tag                      0x0324
#define GlintModelViewMatrix4Reg                           1
#define GlintModelViewMatrix4Off                      0x8920

#define GlintModelViewMatrix5                         0x9928
#define GlintModelViewMatrix5Tag                      0x0325
#define GlintModelViewMatrix5Reg                           1
#define GlintModelViewMatrix5Off                      0x8928

#define GlintModelViewMatrix6                         0x9930
#define GlintModelViewMatrix6Tag                      0x0326
#define GlintModelViewMatrix6Reg                           1
#define GlintModelViewMatrix6Off                      0x8930

#define GlintModelViewMatrix7                         0x9938
#define GlintModelViewMatrix7Tag                      0x0327
#define GlintModelViewMatrix7Reg                           1
#define GlintModelViewMatrix7Off                      0x8938

#define GlintModelViewMatrix8                         0x9940
#define GlintModelViewMatrix8Tag                      0x0328
#define GlintModelViewMatrix8Reg                           1
#define GlintModelViewMatrix8Off                      0x8940

#define GlintModelViewMatrix9                         0x9948
#define GlintModelViewMatrix9Tag                      0x0329
#define GlintModelViewMatrix9Reg                           1
#define GlintModelViewMatrix9Off                      0x8948

#define GlintModelViewMatrix10                        0x9950
#define GlintModelViewMatrix10Tag                     0x032a
#define GlintModelViewMatrix10Reg                          1
#define GlintModelViewMatrix10Off                     0x8950

#define GlintModelViewMatrix11                        0x9958
#define GlintModelViewMatrix11Tag                     0x032b
#define GlintModelViewMatrix11Reg                          1
#define GlintModelViewMatrix11Off                     0x8958

#define GlintModelViewMatrix12                        0x9960
#define GlintModelViewMatrix12Tag                     0x032c
#define GlintModelViewMatrix12Reg                          1
#define GlintModelViewMatrix12Off                     0x8960

#define GlintModelViewMatrix13                        0x9968
#define GlintModelViewMatrix13Tag                     0x032d
#define GlintModelViewMatrix13Reg                          1
#define GlintModelViewMatrix13Off                     0x8968

#define GlintModelViewMatrix14                        0x9970
#define GlintModelViewMatrix14Tag                     0x032e
#define GlintModelViewMatrix14Reg                          1
#define GlintModelViewMatrix14Off                     0x8970

#define GlintModelViewMatrix15                        0x9978
#define GlintModelViewMatrix15Tag                     0x032f
#define GlintModelViewMatrix15Reg                          1
#define GlintModelViewMatrix15Off                     0x8978

#define GlintModelViewProjectionMatrix0               0x9980
#define GlintModelViewProjectionMatrix0Tag            0x0330
#define GlintModelViewProjectionMatrix0Reg                 1
#define GlintModelViewProjectionMatrix0Off            0x8980

#define GlintModelViewProjectionMatrix1               0x9988
#define GlintModelViewProjectionMatrix1Tag            0x0331
#define GlintModelViewProjectionMatrix1Reg                 1
#define GlintModelViewProjectionMatrix1Off            0x8988

#define GlintModelViewProjectionMatrix2               0x9990
#define GlintModelViewProjectionMatrix2Tag            0x0332
#define GlintModelViewProjectionMatrix2Reg                 1
#define GlintModelViewProjectionMatrix2Off            0x8990

#define GlintModelViewProjectionMatrix3               0x9998
#define GlintModelViewProjectionMatrix3Tag            0x0333
#define GlintModelViewProjectionMatrix3Reg                 1
#define GlintModelViewProjectionMatrix3Off            0x8998

#define GlintModelViewProjectionMatrix4               0x99a0
#define GlintModelViewProjectionMatrix4Tag            0x0334
#define GlintModelViewProjectionMatrix4Reg                 1
#define GlintModelViewProjectionMatrix4Off            0x89a0

#define GlintModelViewProjectionMatrix5               0x99a8
#define GlintModelViewProjectionMatrix5Tag            0x0335
#define GlintModelViewProjectionMatrix5Reg                 1
#define GlintModelViewProjectionMatrix5Off            0x89a8

#define GlintModelViewProjectionMatrix6               0x99b0
#define GlintModelViewProjectionMatrix6Tag            0x0336
#define GlintModelViewProjectionMatrix6Reg                 1
#define GlintModelViewProjectionMatrix6Off            0x89b0

#define GlintModelViewProjectionMatrix7               0x99b8
#define GlintModelViewProjectionMatrix7Tag            0x0337
#define GlintModelViewProjectionMatrix7Reg                 1
#define GlintModelViewProjectionMatrix7Off            0x89b8

#define GlintModelViewProjectionMatrix8               0x99c0
#define GlintModelViewProjectionMatrix8Tag            0x0338
#define GlintModelViewProjectionMatrix8Reg                 1
#define GlintModelViewProjectionMatrix8Off            0x89c0

#define GlintModelViewProjectionMatrix9               0x99c8
#define GlintModelViewProjectionMatrix9Tag            0x0339
#define GlintModelViewProjectionMatrix9Reg                 1
#define GlintModelViewProjectionMatrix9Off            0x89c8

#define GlintModelViewProjectionMatrix10              0x99d0
#define GlintModelViewProjectionMatrix10Tag           0x033a
#define GlintModelViewProjectionMatrix10Reg                1
#define GlintModelViewProjectionMatrix10Off           0x89d0

#define GlintModelViewProjectionMatrix11              0x99d8
#define GlintModelViewProjectionMatrix11Tag           0x033b
#define GlintModelViewProjectionMatrix11Reg                1
#define GlintModelViewProjectionMatrix11Off           0x89d8

#define GlintModelViewProjectionMatrix12              0x99e0
#define GlintModelViewProjectionMatrix12Tag           0x033c
#define GlintModelViewProjectionMatrix12Reg                1
#define GlintModelViewProjectionMatrix12Off           0x89e0

#define GlintModelViewProjectionMatrix13              0x99e8
#define GlintModelViewProjectionMatrix13Tag           0x033d
#define GlintModelViewProjectionMatrix13Reg                1
#define GlintModelViewProjectionMatrix13Off           0x89e8

#define GlintModelViewProjectionMatrix14              0x99f0
#define GlintModelViewProjectionMatrix14Tag           0x033e
#define GlintModelViewProjectionMatrix14Reg                1
#define GlintModelViewProjectionMatrix14Off           0x89f0

#define GlintModelViewProjectionMatrix15              0x99f8
#define GlintModelViewProjectionMatrix15Tag           0x033f
#define GlintModelViewProjectionMatrix15Reg                1
#define GlintModelViewProjectionMatrix15Off           0x89f8

#define GlintNormalMatrix0                            0x9a00
#define GlintNormalMatrix0Tag                         0x0340
#define GlintNormalMatrix0Reg                              1
#define GlintNormalMatrix0Off                         0x8a00

#define GlintNormalMatrix1                            0x9a08
#define GlintNormalMatrix1Tag                         0x0341
#define GlintNormalMatrix1Reg                              1
#define GlintNormalMatrix1Off                         0x8a08

#define GlintNormalMatrix2                            0x9a10
#define GlintNormalMatrix2Tag                         0x0342
#define GlintNormalMatrix2Reg                              1
#define GlintNormalMatrix2Off                         0x8a10

#define GlintNormalMatrix3                            0x9a18
#define GlintNormalMatrix3Tag                         0x0343
#define GlintNormalMatrix3Reg                              1
#define GlintNormalMatrix3Off                         0x8a18

#define GlintNormalMatrix4                            0x9a20
#define GlintNormalMatrix4Tag                         0x0344
#define GlintNormalMatrix4Reg                              1
#define GlintNormalMatrix4Off                         0x8a20

#define GlintNormalMatrix5                            0x9a28
#define GlintNormalMatrix5Tag                         0x0345
#define GlintNormalMatrix5Reg                              1
#define GlintNormalMatrix5Off                         0x8a28

#define GlintNormalMatrix6                            0x9a30
#define GlintNormalMatrix6Tag                         0x0346
#define GlintNormalMatrix6Reg                              1
#define GlintNormalMatrix6Off                         0x8a30

#define GlintNormalMatrix7                            0x9a38
#define GlintNormalMatrix7Tag                         0x0347
#define GlintNormalMatrix7Reg                              1
#define GlintNormalMatrix7Off                         0x8a38

#define GlintNormalMatrix8                            0x9a40
#define GlintNormalMatrix8Tag                         0x0348
#define GlintNormalMatrix8Reg                              1
#define GlintNormalMatrix8Off                         0x8a40

#define GlintTextureMatrix0                           0x9a80
#define GlintTextureMatrix0Tag                        0x0350
#define GlintTextureMatrix0Reg                             1
#define GlintTextureMatrix0Off                        0x8a80

#define GlintTextureMatrix1                           0x9a88
#define GlintTextureMatrix1Tag                        0x0351
#define GlintTextureMatrix1Reg                             1
#define GlintTextureMatrix1Off                        0x8a88

#define GlintTextureMatrix2                           0x9a90
#define GlintTextureMatrix2Tag                        0x0352
#define GlintTextureMatrix2Reg                             1
#define GlintTextureMatrix2Off                        0x8a90

#define GlintTextureMatrix3                           0x9a98
#define GlintTextureMatrix3Tag                        0x0353
#define GlintTextureMatrix3Reg                             1
#define GlintTextureMatrix3Off                        0x8a98

#define GlintTextureMatrix4                           0x9aa0
#define GlintTextureMatrix4Tag                        0x0354
#define GlintTextureMatrix4Reg                             1
#define GlintTextureMatrix4Off                        0x8aa0

#define GlintTextureMatrix5                           0x9aa8
#define GlintTextureMatrix5Tag                        0x0355
#define GlintTextureMatrix5Reg                             1
#define GlintTextureMatrix5Off                        0x8aa8

#define GlintTextureMatrix6                           0x9ab0
#define GlintTextureMatrix6Tag                        0x0356
#define GlintTextureMatrix6Reg                             1
#define GlintTextureMatrix6Off                        0x8ab0

#define GlintTextureMatrix7                           0x9ab8
#define GlintTextureMatrix7Tag                        0x0357
#define GlintTextureMatrix7Reg                             1
#define GlintTextureMatrix7Off                        0x8ab8

#define GlintTextureMatrix8                           0x9ac0
#define GlintTextureMatrix8Tag                        0x0358
#define GlintTextureMatrix8Reg                             1
#define GlintTextureMatrix8Off                        0x8ac0

#define GlintTextureMatrix9                           0x9ac8
#define GlintTextureMatrix9Tag                        0x0359
#define GlintTextureMatrix9Reg                             1
#define GlintTextureMatrix9Off                        0x8ac8

#define GlintTextureMatrix10                          0x9ad0
#define GlintTextureMatrix10Tag                       0x035a
#define GlintTextureMatrix10Reg                            1
#define GlintTextureMatrix10Off                       0x8ad0

#define GlintTextureMatrix11                          0x9ad8
#define GlintTextureMatrix11Tag                       0x035b
#define GlintTextureMatrix11Reg                            1
#define GlintTextureMatrix11Off                       0x8ad8

#define GlintTextureMatrix12                          0x9ae0
#define GlintTextureMatrix12Tag                       0x035c
#define GlintTextureMatrix12Reg                            1
#define GlintTextureMatrix12Off                       0x8ae0

#define GlintTextureMatrix13                          0x9ae8
#define GlintTextureMatrix13Tag                       0x035d
#define GlintTextureMatrix13Reg                            1
#define GlintTextureMatrix13Off                       0x8ae8

#define GlintTextureMatrix14                          0x9af0
#define GlintTextureMatrix14Tag                       0x035e
#define GlintTextureMatrix14Reg                            1
#define GlintTextureMatrix14Off                       0x8af0

#define GlintTextureMatrix15                          0x9af8
#define GlintTextureMatrix15Tag                       0x035f
#define GlintTextureMatrix15Reg                            1
#define GlintTextureMatrix15Off                       0x8af8

#define GlintTexGen0                                  0x9b00
#define GlintTexGen0Tag                               0x0360
#define GlintTexGen0Reg                                    1
#define GlintTexGen0Off                               0x8b00

#define GlintTexGen1                                  0x9b08
#define GlintTexGen1Tag                               0x0361
#define GlintTexGen1Reg                                    1
#define GlintTexGen1Off                               0x8b08

#define GlintTexGen2                                  0x9b10
#define GlintTexGen2Tag                               0x0362
#define GlintTexGen2Reg                                    1
#define GlintTexGen2Off                               0x8b10

#define GlintTexGen3                                  0x9b18
#define GlintTexGen3Tag                               0x0363
#define GlintTexGen3Reg                                    1
#define GlintTexGen3Off                               0x8b18

#define GlintTexGen4                                  0x9b20
#define GlintTexGen4Tag                               0x0364
#define GlintTexGen4Reg                                    1
#define GlintTexGen4Off                               0x8b20

#define GlintTexGen5                                  0x9b28
#define GlintTexGen5Tag                               0x0365
#define GlintTexGen5Reg                                    1
#define GlintTexGen5Off                               0x8b28

#define GlintTexGen6                                  0x9b30
#define GlintTexGen6Tag                               0x0366
#define GlintTexGen6Reg                                    1
#define GlintTexGen6Off                               0x8b30

#define GlintTexGen7                                  0x9b38
#define GlintTexGen7Tag                               0x0367
#define GlintTexGen7Reg                                    1
#define GlintTexGen7Off                               0x8b38

#define GlintTexGen8                                  0x9b40
#define GlintTexGen8Tag                               0x0368
#define GlintTexGen8Reg                                    1
#define GlintTexGen8Off                               0x8b40

#define GlintTexGen9                                  0x9b48
#define GlintTexGen9Tag                               0x0369
#define GlintTexGen9Reg                                    1
#define GlintTexGen9Off                               0x8b48

#define GlintTexGen10                                 0x9b50
#define GlintTexGen10Tag                              0x036a
#define GlintTexGen10Reg                                   1
#define GlintTexGen10Off                              0x8b50

#define GlintTexGen11                                 0x9b58
#define GlintTexGen11Tag                              0x036b
#define GlintTexGen11Reg                                   1
#define GlintTexGen11Off                              0x8b58

#define GlintTexGen12                                 0x9b60
#define GlintTexGen12Tag                              0x036c
#define GlintTexGen12Reg                                   1
#define GlintTexGen12Off                              0x8b60

#define GlintTexGen13                                 0x9b68
#define GlintTexGen13Tag                              0x036d
#define GlintTexGen13Reg                                   1
#define GlintTexGen13Off                              0x8b68

#define GlintTexGen14                                 0x9b70
#define GlintTexGen14Tag                              0x036e
#define GlintTexGen14Reg                                   1
#define GlintTexGen14Off                              0x8b70

#define GlintTexGen15                                 0x9b78
#define GlintTexGen15Tag                              0x036f
#define GlintTexGen15Reg                                   1
#define GlintTexGen15Off                              0x8b78

#define GlintViewPortScaleX                           0x9b80
#define GlintViewPortScaleXTag                        0x0370
#define GlintViewPortScaleXReg                             1
#define GlintViewPortScaleXOff                        0x8b80

#define GlintViewPortScaleY                           0x9b88
#define GlintViewPortScaleYTag                        0x0371
#define GlintViewPortScaleYReg                             1
#define GlintViewPortScaleYOff                        0x8b88

#define GlintViewPortScaleZ                           0x9b90
#define GlintViewPortScaleZTag                        0x0372
#define GlintViewPortScaleZReg                             1
#define GlintViewPortScaleZOff                        0x8b90

#define GlintViewPortOffsetX                          0x9b98
#define GlintViewPortOffsetXTag                       0x0373
#define GlintViewPortOffsetXReg                            1
#define GlintViewPortOffsetXOff                       0x8b98

#define GlintViewPortOffsetY                          0x9ba0
#define GlintViewPortOffsetYTag                       0x0374
#define GlintViewPortOffsetYReg                            1
#define GlintViewPortOffsetYOff                       0x8ba0

#define GlintViewPortOffsetZ                          0x9ba8
#define GlintViewPortOffsetZTag                       0x0375
#define GlintViewPortOffsetZReg                            1
#define GlintViewPortOffsetZOff                       0x8ba8

#define GlintFogDensity                               0x9bb0
#define GlintFogDensityTag                            0x0376
#define GlintFogDensityReg                                 1
#define GlintFogDensityOff                            0x8bb0

#define GlintFogScale                                 0x9bb8
#define GlintFogScaleTag                              0x0377
#define GlintFogScaleReg                                   1
#define GlintFogScaleOff                              0x8bb8

#define GlintFogEnd                                   0x9bc0
#define GlintFogEndTag                                0x0378
#define GlintFogEndReg                                     1
#define GlintFogEndOff                                0x8bc0

#define GlintPolygonOffsetFactor                      0x9bc8
#define GlintPolygonOffsetFactorTag                   0x0379
#define GlintPolygonOffsetFactorReg                        1
#define GlintPolygonOffsetFactorOff                   0x8bc8

#define GlintPolygonOffsetBias                        0x9bd0
#define GlintPolygonOffsetBiasTag                     0x037a
#define GlintPolygonOffsetBiasReg                          1
#define GlintPolygonOffsetBiasOff                     0x8bd0

#define GlintLineClipLengthThreshold                  0x9bd8
#define GlintLineClipLengthThresholdTag               0x037b
#define GlintLineClipLengthThresholdReg                    1
#define GlintLineClipLengthThresholdOff               0x8bd8

#define GlintTriangleClipAreaThreshold                0x9be0
#define GlintTriangleClipAreaThresholdTag             0x037c
#define GlintTriangleClipAreaThresholdReg                  1
#define GlintTriangleClipAreaThresholdOff             0x8be0

#define GlintRasterPosXIncrement                      0x9be8
#define GlintRasterPosXIncrementTag                   0x037d
#define GlintRasterPosXIncrementReg                        1
#define GlintRasterPosXIncrementOff                   0x8be8

#define GlintRasterPosYIncrement                      0x9bf0
#define GlintRasterPosYIncrementTag                   0x037e
#define GlintRasterPosYIncrementReg                        1
#define GlintRasterPosYIncrementOff                   0x8bf0

#define GlintUserClip0X                               0x9c00
#define GlintUserClip0XTag                            0x0380
#define GlintUserClip0XReg                                 1
#define GlintUserClip0XOff                            0x8c00

#define GlintUserClip0Y                               0x9c08
#define GlintUserClip0YTag                            0x0381
#define GlintUserClip0YReg                                 1
#define GlintUserClip0YOff                            0x8c08

#define GlintUserClip0Z                               0x9c10
#define GlintUserClip0ZTag                            0x0382
#define GlintUserClip0ZReg                                 1
#define GlintUserClip0ZOff                            0x8c10

#define GlintUserClip0W                               0x9c18
#define GlintUserClip0WTag                            0x0383
#define GlintUserClip0WReg                                 1
#define GlintUserClip0WOff                            0x8c18

#define GlintUserClip1X                               0x9c20
#define GlintUserClip1XTag                            0x0384
#define GlintUserClip1XReg                                 1
#define GlintUserClip1XOff                            0x8c20

#define GlintUserClip1Y                               0x9c28
#define GlintUserClip1YTag                            0x0385
#define GlintUserClip1YReg                                 1
#define GlintUserClip1YOff                            0x8c28

#define GlintUserClip1Z                               0x9c30
#define GlintUserClip1ZTag                            0x0386
#define GlintUserClip1ZReg                                 1
#define GlintUserClip1ZOff                            0x8c30

#define GlintUserClip1W                               0x9c38
#define GlintUserClip1WTag                            0x0387
#define GlintUserClip1WReg                                 1
#define GlintUserClip1WOff                            0x8c38

#define GlintUserClip2X                               0x9c40
#define GlintUserClip2XTag                            0x0388
#define GlintUserClip2XReg                                 1
#define GlintUserClip2XOff                            0x8c40

#define GlintUserClip2Y                               0x9c48
#define GlintUserClip2YTag                            0x0389
#define GlintUserClip2YReg                                 1
#define GlintUserClip2YOff                            0x8c48

#define GlintUserClip2Z                               0x9c50
#define GlintUserClip2ZTag                            0x038a
#define GlintUserClip2ZReg                                 1
#define GlintUserClip2ZOff                            0x8c50

#define GlintUserClip2W                               0x9c58
#define GlintUserClip2WTag                            0x038b
#define GlintUserClip2WReg                                 1
#define GlintUserClip2WOff                            0x8c58

#define GlintUserClip3X                               0x9c60
#define GlintUserClip3XTag                            0x038c
#define GlintUserClip3XReg                                 1
#define GlintUserClip3XOff                            0x8c60

#define GlintUserClip3Y                               0x9c68
#define GlintUserClip3YTag                            0x038d
#define GlintUserClip3YReg                                 1
#define GlintUserClip3YOff                            0x8c68

#define GlintUserClip3Z                               0x9c70
#define GlintUserClip3ZTag                            0x038e
#define GlintUserClip3ZReg                                 1
#define GlintUserClip3ZOff                            0x8c70

#define GlintUserClip3W                               0x9c78
#define GlintUserClip3WTag                            0x038f
#define GlintUserClip3WReg                                 1
#define GlintUserClip3WOff                            0x8c78

#define GlintUserClip4X                               0x9c80
#define GlintUserClip4XTag                            0x0390
#define GlintUserClip4XReg                                 1
#define GlintUserClip4XOff                            0x8c80

#define GlintUserClip4Y                               0x9c88
#define GlintUserClip4YTag                            0x0391
#define GlintUserClip4YReg                                 1
#define GlintUserClip4YOff                            0x8c88

#define GlintUserClip4Z                               0x9c90
#define GlintUserClip4ZTag                            0x0392
#define GlintUserClip4ZReg                                 1
#define GlintUserClip4ZOff                            0x8c90

#define GlintUserClip4W                               0x9c98
#define GlintUserClip4WTag                            0x0393
#define GlintUserClip4WReg                                 1
#define GlintUserClip4WOff                            0x8c98

#define GlintUserClip5X                               0x9ca0
#define GlintUserClip5XTag                            0x0394
#define GlintUserClip5XReg                                 1
#define GlintUserClip5XOff                            0x8ca0

#define GlintUserClip5Y                               0x9ca8
#define GlintUserClip5YTag                            0x0395
#define GlintUserClip5YReg                                 1
#define GlintUserClip5YOff                            0x8ca8

#define GlintUserClip5Z                               0x9cb0
#define GlintUserClip5ZTag                            0x0396
#define GlintUserClip5ZReg                                 1
#define GlintUserClip5ZOff                            0x8cb0

#define GlintUserClip5W                               0x9cb8
#define GlintUserClip5WTag                            0x0397
#define GlintUserClip5WReg                                 1
#define GlintUserClip5WOff                            0x8cb8

#define GlintRasterPosXOffset                         0x9ce8
#define GlintRasterPosXOffsetTag                      0x039d
#define GlintRasterPosXOffsetReg                           1
#define GlintRasterPosXOffsetOff                      0x8ce8

#define GlintRasterPosYOffset                         0x9cf0
#define GlintRasterPosYOffsetTag                      0x039e
#define GlintRasterPosYOffsetReg                           1
#define GlintRasterPosYOffsetOff                      0x8cf0

#define GlintAttenuationCutOff                        0x9cf8
#define GlintAttenuationCutOffTag                     0x039f
#define GlintAttenuationCutOffReg                          1
#define GlintAttenuationCutOffOff                     0x8cf8

#define GlintLight0Mode                               0x9d00
#define GlintLight0ModeTag                            0x03a0
#define GlintLight0ModeReg                                 1
#define GlintLight0ModeOff                            0x8d00

#define GlintLight0AmbientIntensityRed                0x9d08
#define GlintLight0AmbientIntensityRedTag             0x03a1
#define GlintLight0AmbientIntensityRedReg                  1
#define GlintLight0AmbientIntensityRedOff             0x8d08

#define GlintLight0AmbientIntensityGreen              0x9d10
#define GlintLight0AmbientIntensityGreenTag           0x03a2
#define GlintLight0AmbientIntensityGreenReg                1
#define GlintLight0AmbientIntensityGreenOff           0x8d10

#define GlintLight0AmbientIntensityBlue               0x9d18
#define GlintLight0AmbientIntensityBlueTag            0x03a3
#define GlintLight0AmbientIntensityBlueReg                 1
#define GlintLight0AmbientIntensityBlueOff            0x8d18

#define GlintLight0DiffuseIntensityRed                0x9d20
#define GlintLight0DiffuseIntensityRedTag             0x03a4
#define GlintLight0DiffuseIntensityRedReg                  1
#define GlintLight0DiffuseIntensityRedOff             0x8d20

#define GlintLight0DiffuseIntensityGreen              0x9d28
#define GlintLight0DiffuseIntensityGreenTag           0x03a5
#define GlintLight0DiffuseIntensityGreenReg                1
#define GlintLight0DiffuseIntensityGreenOff           0x8d28

#define GlintLight0DiffuseIntensityBlue               0x9d30
#define GlintLight0DiffuseIntensityBlueTag            0x03a6
#define GlintLight0DiffuseIntensityBlueReg                 1
#define GlintLight0DiffuseIntensityBlueOff            0x8d30

#define GlintLight0SpecularIntensityRed               0x9d38
#define GlintLight0SpecularIntensityRedTag            0x03a7
#define GlintLight0SpecularIntensityRedReg                 1
#define GlintLight0SpecularIntensityRedOff            0x8d38

#define GlintLight0SpecularIntensityGreen             0x9d40
#define GlintLight0SpecularIntensityGreenTag          0x03a8
#define GlintLight0SpecularIntensityGreenReg               1
#define GlintLight0SpecularIntensityGreenOff          0x8d40

#define GlintLight0SpecularIntensityBlue              0x9d48
#define GlintLight0SpecularIntensityBlueTag           0x03a9
#define GlintLight0SpecularIntensityBlueReg                1
#define GlintLight0SpecularIntensityBlueOff           0x8d48

#define GlintLight0PositionX                          0x9d50
#define GlintLight0PositionXTag                       0x03aa
#define GlintLight0PositionXReg                            1
#define GlintLight0PositionXOff                       0x8d50

#define GlintLight0PositionY                          0x9d58
#define GlintLight0PositionYTag                       0x03ab
#define GlintLight0PositionYReg                            1
#define GlintLight0PositionYOff                       0x8d58

#define GlintLight0PositionZ                          0x9d60
#define GlintLight0PositionZTag                       0x03ac
#define GlintLight0PositionZReg                            1
#define GlintLight0PositionZOff                       0x8d60

#define GlintLight0PositionW                          0x9d68
#define GlintLight0PositionWTag                       0x03ad
#define GlintLight0PositionWReg                            1
#define GlintLight0PositionWOff                       0x8d68

#define GlintLight0SpotlightDirectionX                0x9d70
#define GlintLight0SpotlightDirectionXTag             0x03ae
#define GlintLight0SpotlightDirectionXReg                  1
#define GlintLight0SpotlightDirectionXOff             0x8d70

#define GlintLight0SpotlightDirectionY                0x9d78
#define GlintLight0SpotlightDirectionYTag             0x03af
#define GlintLight0SpotlightDirectionYReg                  1
#define GlintLight0SpotlightDirectionYOff             0x8d78

#define GlintLight0SpotlightDirectionZ                0x9d80
#define GlintLight0SpotlightDirectionZTag             0x03b0
#define GlintLight0SpotlightDirectionZReg                  1
#define GlintLight0SpotlightDirectionZOff             0x8d80

#define GlintLight0SpotlightExponent                  0x9d88
#define GlintLight0SpotlightExponentTag               0x03b1
#define GlintLight0SpotlightExponentReg                    1
#define GlintLight0SpotlightExponentOff               0x8d88

#define GlintLight0CosSpotlightCutoffAngle            0x9d90
#define GlintLight0CosSpotlightCutoffAngleTag         0x03b2
#define GlintLight0CosSpotlightCutoffAngleReg              1
#define GlintLight0CosSpotlightCutoffAngleOff         0x8d90

#define GlintLight0ConstantAttenuation                0x9d98
#define GlintLight0ConstantAttenuationTag             0x03b3
#define GlintLight0ConstantAttenuationReg                  1
#define GlintLight0ConstantAttenuationOff             0x8d98

#define GlintLight0LinearAttenuation                  0x9da0
#define GlintLight0LinearAttenuationTag               0x03b4
#define GlintLight0LinearAttenuationReg                    1
#define GlintLight0LinearAttenuationOff               0x8da0

#define GlintLight0QuadraticAttenuation               0x9da8
#define GlintLight0QuadraticAttenuationTag            0x03b5
#define GlintLight0QuadraticAttenuationReg                 1
#define GlintLight0QuadraticAttenuationOff            0x8da8

#define GlintLight1Mode                               0x9db0
#define GlintLight1ModeTag                            0x03b6
#define GlintLight1ModeReg                                 1
#define GlintLight1ModeOff                            0x8db0

#define GlintLight1AmbientIntensityRed                0x9db8
#define GlintLight1AmbientIntensityRedTag             0x03b7
#define GlintLight1AmbientIntensityRedReg                  1
#define GlintLight1AmbientIntensityRedOff             0x8db8

#define GlintLight1AmbientIntensityGreen              0x9dc0
#define GlintLight1AmbientIntensityGreenTag           0x03b8
#define GlintLight1AmbientIntensityGreenReg                1
#define GlintLight1AmbientIntensityGreenOff           0x8dc0

#define GlintLight1AmbientIntensityBlue               0x9dc8
#define GlintLight1AmbientIntensityBlueTag            0x03b9
#define GlintLight1AmbientIntensityBlueReg                 1
#define GlintLight1AmbientIntensityBlueOff            0x8dc8

#define GlintLight1DiffuseIntensityRed                0x9dd0
#define GlintLight1DiffuseIntensityRedTag             0x03ba
#define GlintLight1DiffuseIntensityRedReg                  1
#define GlintLight1DiffuseIntensityRedOff             0x8dd0

#define GlintLight1DiffuseIntensityGreen              0x9dd8
#define GlintLight1DiffuseIntensityGreenTag           0x03bb
#define GlintLight1DiffuseIntensityGreenReg                1
#define GlintLight1DiffuseIntensityGreenOff           0x8dd8

#define GlintLight1DiffuseIntensityBlue               0x9de0
#define GlintLight1DiffuseIntensityBlueTag            0x03bc
#define GlintLight1DiffuseIntensityBlueReg                 1
#define GlintLight1DiffuseIntensityBlueOff            0x8de0

#define GlintLight1SpecularIntensityRed               0x9de8
#define GlintLight1SpecularIntensityRedTag            0x03bd
#define GlintLight1SpecularIntensityRedReg                 1
#define GlintLight1SpecularIntensityRedOff            0x8de8

#define GlintLight1SpecularIntensityGreen             0x9df0
#define GlintLight1SpecularIntensityGreenTag          0x03be
#define GlintLight1SpecularIntensityGreenReg               1
#define GlintLight1SpecularIntensityGreenOff          0x8df0

#define GlintLight1SpecularIntensityBlue              0x9df8
#define GlintLight1SpecularIntensityBlueTag           0x03bf
#define GlintLight1SpecularIntensityBlueReg                1
#define GlintLight1SpecularIntensityBlueOff           0x8df8

#define GlintLight1PositionX                          0x9e00
#define GlintLight1PositionXTag                       0x03c0
#define GlintLight1PositionXReg                            1
#define GlintLight1PositionXOff                       0x8e00

#define GlintLight1PositionY                          0x9e08
#define GlintLight1PositionYTag                       0x03c1
#define GlintLight1PositionYReg                            1
#define GlintLight1PositionYOff                       0x8e08

#define GlintLight1PositionZ                          0x9e10
#define GlintLight1PositionZTag                       0x03c2
#define GlintLight1PositionZReg                            1
#define GlintLight1PositionZOff                       0x8e10

#define GlintLight1PositionW                          0x9e18
#define GlintLight1PositionWTag                       0x03c3
#define GlintLight1PositionWReg                            1
#define GlintLight1PositionWOff                       0x8e18

#define GlintLight1SpotlightDirectionX                0x9e20
#define GlintLight1SpotlightDirectionXTag             0x03c4
#define GlintLight1SpotlightDirectionXReg                  1
#define GlintLight1SpotlightDirectionXOff             0x8e20

#define GlintLight1SpotlightDirectionY                0x9e28
#define GlintLight1SpotlightDirectionYTag             0x03c5
#define GlintLight1SpotlightDirectionYReg                  1
#define GlintLight1SpotlightDirectionYOff             0x8e28

#define GlintLight1SpotlightDirectionZ                0x9e30
#define GlintLight1SpotlightDirectionZTag             0x03c6
#define GlintLight1SpotlightDirectionZReg                  1
#define GlintLight1SpotlightDirectionZOff             0x8e30

#define GlintLight1SpotlightExponent                  0x9e38
#define GlintLight1SpotlightExponentTag               0x03c7
#define GlintLight1SpotlightExponentReg                    1
#define GlintLight1SpotlightExponentOff               0x8e38

#define GlintLight1CosSpotlightCutoffAngle            0x9e40
#define GlintLight1CosSpotlightCutoffAngleTag         0x03c8
#define GlintLight1CosSpotlightCutoffAngleReg              1
#define GlintLight1CosSpotlightCutoffAngleOff         0x8e40

#define GlintLight1ConstantAttenuation                0x9e48
#define GlintLight1ConstantAttenuationTag             0x03c9
#define GlintLight1ConstantAttenuationReg                  1
#define GlintLight1ConstantAttenuationOff             0x8e48

#define GlintLight1LinearAttenuation                  0x9e50
#define GlintLight1LinearAttenuationTag               0x03ca
#define GlintLight1LinearAttenuationReg                    1
#define GlintLight1LinearAttenuationOff               0x8e50

#define GlintLight1QuadraticAttenuation               0x9e58
#define GlintLight1QuadraticAttenuationTag            0x03cb
#define GlintLight1QuadraticAttenuationReg                 1
#define GlintLight1QuadraticAttenuationOff            0x8e58

#define GlintLight2Mode                               0x9e60
#define GlintLight2ModeTag                            0x03cc
#define GlintLight2ModeReg                                 1
#define GlintLight2ModeOff                            0x8e60

#define GlintLight2AmbientIntensityRed                0x9e68
#define GlintLight2AmbientIntensityRedTag             0x03cd
#define GlintLight2AmbientIntensityRedReg                  1
#define GlintLight2AmbientIntensityRedOff             0x8e68

#define GlintLight2AmbientIntensityGreen              0x9e70
#define GlintLight2AmbientIntensityGreenTag           0x03ce
#define GlintLight2AmbientIntensityGreenReg                1
#define GlintLight2AmbientIntensityGreenOff           0x8e70

#define GlintLight2AmbientIntensityBlue               0x9e78
#define GlintLight2AmbientIntensityBlueTag            0x03cf
#define GlintLight2AmbientIntensityBlueReg                 1
#define GlintLight2AmbientIntensityBlueOff            0x8e78

#define GlintLight2DiffuseIntensityRed                0x9e80
#define GlintLight2DiffuseIntensityRedTag             0x03d0
#define GlintLight2DiffuseIntensityRedReg                  1
#define GlintLight2DiffuseIntensityRedOff             0x8e80

#define GlintLight2DiffuseIntensityGreen              0x9e88
#define GlintLight2DiffuseIntensityGreenTag           0x03d1
#define GlintLight2DiffuseIntensityGreenReg                1
#define GlintLight2DiffuseIntensityGreenOff           0x8e88

#define GlintLight2DiffuseIntensityBlue               0x9e90
#define GlintLight2DiffuseIntensityBlueTag            0x03d2
#define GlintLight2DiffuseIntensityBlueReg                 1
#define GlintLight2DiffuseIntensityBlueOff            0x8e90

#define GlintLight2SpecularIntensityRed               0x9e98
#define GlintLight2SpecularIntensityRedTag            0x03d3
#define GlintLight2SpecularIntensityRedReg                 1
#define GlintLight2SpecularIntensityRedOff            0x8e98

#define GlintLight2SpecularIntensityGreen             0x9ea0
#define GlintLight2SpecularIntensityGreenTag          0x03d4
#define GlintLight2SpecularIntensityGreenReg               1
#define GlintLight2SpecularIntensityGreenOff          0x8ea0

#define GlintLight2SpecularIntensityBlue              0x9ea8
#define GlintLight2SpecularIntensityBlueTag           0x03d5
#define GlintLight2SpecularIntensityBlueReg                1
#define GlintLight2SpecularIntensityBlueOff           0x8ea8

#define GlintLight2PositionX                          0x9eb0
#define GlintLight2PositionXTag                       0x03d6
#define GlintLight2PositionXReg                            1
#define GlintLight2PositionXOff                       0x8eb0

#define GlintLight2PositionY                          0x9eb8
#define GlintLight2PositionYTag                       0x03d7
#define GlintLight2PositionYReg                            1
#define GlintLight2PositionYOff                       0x8eb8

#define GlintLight2PositionZ                          0x9ec0
#define GlintLight2PositionZTag                       0x03d8
#define GlintLight2PositionZReg                            1
#define GlintLight2PositionZOff                       0x8ec0

#define GlintLight2PositionW                          0x9ec8
#define GlintLight2PositionWTag                       0x03d9
#define GlintLight2PositionWReg                            1
#define GlintLight2PositionWOff                       0x8ec8

#define GlintLight2SpotlightDirectionX                0x9ed0
#define GlintLight2SpotlightDirectionXTag             0x03da
#define GlintLight2SpotlightDirectionXReg                  1
#define GlintLight2SpotlightDirectionXOff             0x8ed0

#define GlintLight2SpotlightDirectionY                0x9ed8
#define GlintLight2SpotlightDirectionYTag             0x03db
#define GlintLight2SpotlightDirectionYReg                  1
#define GlintLight2SpotlightDirectionYOff             0x8ed8

#define GlintLight2SpotlightDirectionZ                0x9ee0
#define GlintLight2SpotlightDirectionZTag             0x03dc
#define GlintLight2SpotlightDirectionZReg                  1
#define GlintLight2SpotlightDirectionZOff             0x8ee0

#define GlintLight2SpotlightExponent                  0x9ee8
#define GlintLight2SpotlightExponentTag               0x03dd
#define GlintLight2SpotlightExponentReg                    1
#define GlintLight2SpotlightExponentOff               0x8ee8

#define GlintLight2CosSpotlightCutoffAngle            0x9ef0
#define GlintLight2CosSpotlightCutoffAngleTag         0x03de
#define GlintLight2CosSpotlightCutoffAngleReg              1
#define GlintLight2CosSpotlightCutoffAngleOff         0x8ef0

#define GlintLight2ConstantAttenuation                0x9ef8
#define GlintLight2ConstantAttenuationTag             0x03df
#define GlintLight2ConstantAttenuationReg                  1
#define GlintLight2ConstantAttenuationOff             0x8ef8

#define GlintLight2LinearAttenuation                  0x9f00
#define GlintLight2LinearAttenuationTag               0x03e0
#define GlintLight2LinearAttenuationReg                    1
#define GlintLight2LinearAttenuationOff               0x8f00

#define GlintLight2QuadraticAttenuation               0x9f08
#define GlintLight2QuadraticAttenuationTag            0x03e1
#define GlintLight2QuadraticAttenuationReg                 1
#define GlintLight2QuadraticAttenuationOff            0x8f08

#define GlintLight3Mode                               0x9f10
#define GlintLight3ModeTag                            0x03e2
#define GlintLight3ModeReg                                 1
#define GlintLight3ModeOff                            0x8f10

#define GlintLight3AmbientIntensityRed                0x9f18
#define GlintLight3AmbientIntensityRedTag             0x03e3
#define GlintLight3AmbientIntensityRedReg                  1
#define GlintLight3AmbientIntensityRedOff             0x8f18

#define GlintLight3AmbientIntensityGreen              0x9f20
#define GlintLight3AmbientIntensityGreenTag           0x03e4
#define GlintLight3AmbientIntensityGreenReg                1
#define GlintLight3AmbientIntensityGreenOff           0x8f20

#define GlintLight3AmbientIntensityBlue               0x9f28
#define GlintLight3AmbientIntensityBlueTag            0x03e5
#define GlintLight3AmbientIntensityBlueReg                 1
#define GlintLight3AmbientIntensityBlueOff            0x8f28

#define GlintLight3DiffuseIntensityRed                0x9f30
#define GlintLight3DiffuseIntensityRedTag             0x03e6
#define GlintLight3DiffuseIntensityRedReg                  1
#define GlintLight3DiffuseIntensityRedOff             0x8f30

#define GlintLight3DiffuseIntensityGreen              0x9f38
#define GlintLight3DiffuseIntensityGreenTag           0x03e7
#define GlintLight3DiffuseIntensityGreenReg                1
#define GlintLight3DiffuseIntensityGreenOff           0x8f38

#define GlintLight3DiffuseIntensityBlue               0x9f40
#define GlintLight3DiffuseIntensityBlueTag            0x03e8
#define GlintLight3DiffuseIntensityBlueReg                 1
#define GlintLight3DiffuseIntensityBlueOff            0x8f40

#define GlintLight3SpecularIntensityRed               0x9f48
#define GlintLight3SpecularIntensityRedTag            0x03e9
#define GlintLight3SpecularIntensityRedReg                 1
#define GlintLight3SpecularIntensityRedOff            0x8f48

#define GlintLight3SpecularIntensityGreen             0x9f50
#define GlintLight3SpecularIntensityGreenTag          0x03ea
#define GlintLight3SpecularIntensityGreenReg               1
#define GlintLight3SpecularIntensityGreenOff          0x8f50

#define GlintLight3SpecularIntensityBlue              0x9f58
#define GlintLight3SpecularIntensityBlueTag           0x03eb
#define GlintLight3SpecularIntensityBlueReg                1
#define GlintLight3SpecularIntensityBlueOff           0x8f58

#define GlintLight3PositionX                          0x9f60
#define GlintLight3PositionXTag                       0x03ec
#define GlintLight3PositionXReg                            1
#define GlintLight3PositionXOff                       0x8f60

#define GlintLight3PositionY                          0x9f68
#define GlintLight3PositionYTag                       0x03ed
#define GlintLight3PositionYReg                            1
#define GlintLight3PositionYOff                       0x8f68

#define GlintLight3PositionZ                          0x9f70
#define GlintLight3PositionZTag                       0x03ee
#define GlintLight3PositionZReg                            1
#define GlintLight3PositionZOff                       0x8f70

#define GlintLight3PositionW                          0x9f78
#define GlintLight3PositionWTag                       0x03ef
#define GlintLight3PositionWReg                            1
#define GlintLight3PositionWOff                       0x8f78

#define GlintLight3SpotlightDirectionX                0x9f80
#define GlintLight3SpotlightDirectionXTag             0x03f0
#define GlintLight3SpotlightDirectionXReg                  1
#define GlintLight3SpotlightDirectionXOff             0x8f80

#define GlintLight3SpotlightDirectionY                0x9f88
#define GlintLight3SpotlightDirectionYTag             0x03f1
#define GlintLight3SpotlightDirectionYReg                  1
#define GlintLight3SpotlightDirectionYOff             0x8f88

#define GlintLight3SpotlightDirectionZ                0x9f90
#define GlintLight3SpotlightDirectionZTag             0x03f2
#define GlintLight3SpotlightDirectionZReg                  1
#define GlintLight3SpotlightDirectionZOff             0x8f90

#define GlintLight3SpotlightExponent                  0x9f98
#define GlintLight3SpotlightExponentTag               0x03f3
#define GlintLight3SpotlightExponentReg                    1
#define GlintLight3SpotlightExponentOff               0x8f98

#define GlintLight3CosSpotlightCutoffAngle            0x9fa0
#define GlintLight3CosSpotlightCutoffAngleTag         0x03f4
#define GlintLight3CosSpotlightCutoffAngleReg              1
#define GlintLight3CosSpotlightCutoffAngleOff         0x8fa0

#define GlintLight3ConstantAttenuation                0x9fa8
#define GlintLight3ConstantAttenuationTag             0x03f5
#define GlintLight3ConstantAttenuationReg                  1
#define GlintLight3ConstantAttenuationOff             0x8fa8

#define GlintLight3LinearAttenuation                  0x9fb0
#define GlintLight3LinearAttenuationTag               0x03f6
#define GlintLight3LinearAttenuationReg                    1
#define GlintLight3LinearAttenuationOff               0x8fb0

#define GlintLight3QuadraticAttenuation               0x9fb8
#define GlintLight3QuadraticAttenuationTag            0x03f7
#define GlintLight3QuadraticAttenuationReg                 1
#define GlintLight3QuadraticAttenuationOff            0x8fb8

#define GlintLight4Mode                               0x9fc0
#define GlintLight4ModeTag                            0x03f8
#define GlintLight4ModeReg                                 1
#define GlintLight4ModeOff                            0x8fc0

#define GlintLight4AmbientIntensityRed                0x9fc8
#define GlintLight4AmbientIntensityRedTag             0x03f9
#define GlintLight4AmbientIntensityRedReg                  1
#define GlintLight4AmbientIntensityRedOff             0x8fc8

#define GlintLight4AmbientIntensityGreen              0x9fd0
#define GlintLight4AmbientIntensityGreenTag           0x03fa
#define GlintLight4AmbientIntensityGreenReg                1
#define GlintLight4AmbientIntensityGreenOff           0x8fd0

#define GlintLight4AmbientIntensityBlue               0x9fd8
#define GlintLight4AmbientIntensityBlueTag            0x03fb
#define GlintLight4AmbientIntensityBlueReg                 1
#define GlintLight4AmbientIntensityBlueOff            0x8fd8

#define GlintLight4DiffuseIntensityRed                0x9fe0
#define GlintLight4DiffuseIntensityRedTag             0x03fc
#define GlintLight4DiffuseIntensityRedReg                  1
#define GlintLight4DiffuseIntensityRedOff             0x8fe0

#define GlintLight4DiffuseIntensityGreen              0x9fe8
#define GlintLight4DiffuseIntensityGreenTag           0x03fd
#define GlintLight4DiffuseIntensityGreenReg                1
#define GlintLight4DiffuseIntensityGreenOff           0x8fe8

#define GlintLight4DiffuseIntensityBlue               0x9ff0
#define GlintLight4DiffuseIntensityBlueTag            0x03fe
#define GlintLight4DiffuseIntensityBlueReg                 1
#define GlintLight4DiffuseIntensityBlueOff            0x8ff0

#define GlintLight4SpecularIntensityRed               0x9ff8
#define GlintLight4SpecularIntensityRedTag            0x03ff
#define GlintLight4SpecularIntensityRedReg                 1
#define GlintLight4SpecularIntensityRedOff            0x8ff8

#define GlintLight4SpecularIntensityGreen             0xa000
#define GlintLight4SpecularIntensityGreenTag          0x0400
#define GlintLight4SpecularIntensityGreenReg               1
#define GlintLight4SpecularIntensityGreenOff          0x9000

#define GlintLight4SpecularIntensityBlue              0xa008
#define GlintLight4SpecularIntensityBlueTag           0x0401
#define GlintLight4SpecularIntensityBlueReg                1
#define GlintLight4SpecularIntensityBlueOff           0x9008

#define GlintLight4PositionX                          0xa010
#define GlintLight4PositionXTag                       0x0402
#define GlintLight4PositionXReg                            1
#define GlintLight4PositionXOff                       0x9010

#define GlintLight4PositionY                          0xa018
#define GlintLight4PositionYTag                       0x0403
#define GlintLight4PositionYReg                            1
#define GlintLight4PositionYOff                       0x9018

#define GlintLight4PositionZ                          0xa020
#define GlintLight4PositionZTag                       0x0404
#define GlintLight4PositionZReg                            1
#define GlintLight4PositionZOff                       0x9020

#define GlintLight4PositionW                          0xa028
#define GlintLight4PositionWTag                       0x0405
#define GlintLight4PositionWReg                            1
#define GlintLight4PositionWOff                       0x9028

#define GlintLight4SpotlightDirectionX                0xa030
#define GlintLight4SpotlightDirectionXTag             0x0406
#define GlintLight4SpotlightDirectionXReg                  1
#define GlintLight4SpotlightDirectionXOff             0x9030

#define GlintLight4SpotlightDirectionY                0xa038
#define GlintLight4SpotlightDirectionYTag             0x0407
#define GlintLight4SpotlightDirectionYReg                  1
#define GlintLight4SpotlightDirectionYOff             0x9038

#define GlintLight4SpotlightDirectionZ                0xa040
#define GlintLight4SpotlightDirectionZTag             0x0408
#define GlintLight4SpotlightDirectionZReg                  1
#define GlintLight4SpotlightDirectionZOff             0x9040

#define GlintLight4SpotlightExponent                  0xa048
#define GlintLight4SpotlightExponentTag               0x0409
#define GlintLight4SpotlightExponentReg                    1
#define GlintLight4SpotlightExponentOff               0x9048

#define GlintLight4CosSpotlightCutoffAngle            0xa050
#define GlintLight4CosSpotlightCutoffAngleTag         0x040a
#define GlintLight4CosSpotlightCutoffAngleReg              1
#define GlintLight4CosSpotlightCutoffAngleOff         0x9050

#define GlintLight4ConstantAttenuation                0xa058
#define GlintLight4ConstantAttenuationTag             0x040b
#define GlintLight4ConstantAttenuationReg                  1
#define GlintLight4ConstantAttenuationOff             0x9058

#define GlintLight4LinearAttenuation                  0xa060
#define GlintLight4LinearAttenuationTag               0x040c
#define GlintLight4LinearAttenuationReg                    1
#define GlintLight4LinearAttenuationOff               0x9060

#define GlintLight4QuadraticAttenuation               0xa068
#define GlintLight4QuadraticAttenuationTag            0x040d
#define GlintLight4QuadraticAttenuationReg                 1
#define GlintLight4QuadraticAttenuationOff            0x9068

#define GlintLight5Mode                               0xa070
#define GlintLight5ModeTag                            0x040e
#define GlintLight5ModeReg                                 1
#define GlintLight5ModeOff                            0x9070

#define GlintLight5AmbientIntensityRed                0xa078
#define GlintLight5AmbientIntensityRedTag             0x040f
#define GlintLight5AmbientIntensityRedReg                  1
#define GlintLight5AmbientIntensityRedOff             0x9078

#define GlintLight5AmbientIntensityGreen              0xa080
#define GlintLight5AmbientIntensityGreenTag           0x0410
#define GlintLight5AmbientIntensityGreenReg                1
#define GlintLight5AmbientIntensityGreenOff           0x9080

#define GlintLight5AmbientIntensityBlue               0xa088
#define GlintLight5AmbientIntensityBlueTag            0x0411
#define GlintLight5AmbientIntensityBlueReg                 1
#define GlintLight5AmbientIntensityBlueOff            0x9088

#define GlintLight5DiffuseIntensityRed                0xa090
#define GlintLight5DiffuseIntensityRedTag             0x0412
#define GlintLight5DiffuseIntensityRedReg                  1
#define GlintLight5DiffuseIntensityRedOff             0x9090

#define GlintLight5DiffuseIntensityGreen              0xa098
#define GlintLight5DiffuseIntensityGreenTag           0x0413
#define GlintLight5DiffuseIntensityGreenReg                1
#define GlintLight5DiffuseIntensityGreenOff           0x9098

#define GlintLight5DiffuseIntensityBlue               0xa0a0
#define GlintLight5DiffuseIntensityBlueTag            0x0414
#define GlintLight5DiffuseIntensityBlueReg                 1
#define GlintLight5DiffuseIntensityBlueOff            0x90a0

#define GlintLight5SpecularIntensityRed               0xa0a8
#define GlintLight5SpecularIntensityRedTag            0x0415
#define GlintLight5SpecularIntensityRedReg                 1
#define GlintLight5SpecularIntensityRedOff            0x90a8

#define GlintLight5SpecularIntensityGreen             0xa0b0
#define GlintLight5SpecularIntensityGreenTag          0x0416
#define GlintLight5SpecularIntensityGreenReg               1
#define GlintLight5SpecularIntensityGreenOff          0x90b0

#define GlintLight5SpecularIntensityBlue              0xa0b8
#define GlintLight5SpecularIntensityBlueTag           0x0417
#define GlintLight5SpecularIntensityBlueReg                1
#define GlintLight5SpecularIntensityBlueOff           0x90b8

#define GlintLight5PositionX                          0xa0c0
#define GlintLight5PositionXTag                       0x0418
#define GlintLight5PositionXReg                            1
#define GlintLight5PositionXOff                       0x90c0

#define GlintLight5PositionY                          0xa0c8
#define GlintLight5PositionYTag                       0x0419
#define GlintLight5PositionYReg                            1
#define GlintLight5PositionYOff                       0x90c8

#define GlintLight5PositionZ                          0xa0d0
#define GlintLight5PositionZTag                       0x041a
#define GlintLight5PositionZReg                            1
#define GlintLight5PositionZOff                       0x90d0

#define GlintLight5PositionW                          0xa0d8
#define GlintLight5PositionWTag                       0x041b
#define GlintLight5PositionWReg                            1
#define GlintLight5PositionWOff                       0x90d8

#define GlintLight5SpotlightDirectionX                0xa0e0
#define GlintLight5SpotlightDirectionXTag             0x041c
#define GlintLight5SpotlightDirectionXReg                  1
#define GlintLight5SpotlightDirectionXOff             0x90e0

#define GlintLight5SpotlightDirectionY                0xa0e8
#define GlintLight5SpotlightDirectionYTag             0x041d
#define GlintLight5SpotlightDirectionYReg                  1
#define GlintLight5SpotlightDirectionYOff             0x90e8

#define GlintLight5SpotlightDirectionZ                0xa0f0
#define GlintLight5SpotlightDirectionZTag             0x041e
#define GlintLight5SpotlightDirectionZReg                  1
#define GlintLight5SpotlightDirectionZOff             0x90f0

#define GlintLight5SpotlightExponent                  0xa0f8
#define GlintLight5SpotlightExponentTag               0x041f
#define GlintLight5SpotlightExponentReg                    1
#define GlintLight5SpotlightExponentOff               0x90f8

#define GlintLight5CosSpotlightCutoffAngle            0xa100
#define GlintLight5CosSpotlightCutoffAngleTag         0x0420
#define GlintLight5CosSpotlightCutoffAngleReg              1
#define GlintLight5CosSpotlightCutoffAngleOff         0x9100

#define GlintLight5ConstantAttenuation                0xa108
#define GlintLight5ConstantAttenuationTag             0x0421
#define GlintLight5ConstantAttenuationReg                  1
#define GlintLight5ConstantAttenuationOff             0x9108

#define GlintLight5LinearAttenuation                  0xa110
#define GlintLight5LinearAttenuationTag               0x0422
#define GlintLight5LinearAttenuationReg                    1
#define GlintLight5LinearAttenuationOff               0x9110

#define GlintLight5QuadraticAttenuation               0xa118
#define GlintLight5QuadraticAttenuationTag            0x0423
#define GlintLight5QuadraticAttenuationReg                 1
#define GlintLight5QuadraticAttenuationOff            0x9118

#define GlintLight6Mode                               0xa120
#define GlintLight6ModeTag                            0x0424
#define GlintLight6ModeReg                                 1
#define GlintLight6ModeOff                            0x9120

#define GlintLight6AmbientIntensityRed                0xa128
#define GlintLight6AmbientIntensityRedTag             0x0425
#define GlintLight6AmbientIntensityRedReg                  1
#define GlintLight6AmbientIntensityRedOff             0x9128

#define GlintLight6AmbientIntensityGreen              0xa130
#define GlintLight6AmbientIntensityGreenTag           0x0426
#define GlintLight6AmbientIntensityGreenReg                1
#define GlintLight6AmbientIntensityGreenOff           0x9130

#define GlintLight6AmbientIntensityBlue               0xa138
#define GlintLight6AmbientIntensityBlueTag            0x0427
#define GlintLight6AmbientIntensityBlueReg                 1
#define GlintLight6AmbientIntensityBlueOff            0x9138

#define GlintLight6DiffuseIntensityRed                0xa140
#define GlintLight6DiffuseIntensityRedTag             0x0428
#define GlintLight6DiffuseIntensityRedReg                  1
#define GlintLight6DiffuseIntensityRedOff             0x9140

#define GlintLight6DiffuseIntensityGreen              0xa148
#define GlintLight6DiffuseIntensityGreenTag           0x0429
#define GlintLight6DiffuseIntensityGreenReg                1
#define GlintLight6DiffuseIntensityGreenOff           0x9148

#define GlintLight6DiffuseIntensityBlue               0xa150
#define GlintLight6DiffuseIntensityBlueTag            0x042a
#define GlintLight6DiffuseIntensityBlueReg                 1
#define GlintLight6DiffuseIntensityBlueOff            0x9150

#define GlintLight6SpecularIntensityRed               0xa158
#define GlintLight6SpecularIntensityRedTag            0x042b
#define GlintLight6SpecularIntensityRedReg                 1
#define GlintLight6SpecularIntensityRedOff            0x9158

#define GlintLight6SpecularIntensityGreen             0xa160
#define GlintLight6SpecularIntensityGreenTag          0x042c
#define GlintLight6SpecularIntensityGreenReg               1
#define GlintLight6SpecularIntensityGreenOff          0x9160

#define GlintLight6SpecularIntensityBlue              0xa168
#define GlintLight6SpecularIntensityBlueTag           0x042d
#define GlintLight6SpecularIntensityBlueReg                1
#define GlintLight6SpecularIntensityBlueOff           0x9168

#define GlintLight6PositionX                          0xa170
#define GlintLight6PositionXTag                       0x042e
#define GlintLight6PositionXReg                            1
#define GlintLight6PositionXOff                       0x9170

#define GlintLight6PositionY                          0xa178
#define GlintLight6PositionYTag                       0x042f
#define GlintLight6PositionYReg                            1
#define GlintLight6PositionYOff                       0x9178

#define GlintLight6PositionZ                          0xa180
#define GlintLight6PositionZTag                       0x0430
#define GlintLight6PositionZReg                            1
#define GlintLight6PositionZOff                       0x9180

#define GlintLight6PositionW                          0xa188
#define GlintLight6PositionWTag                       0x0431
#define GlintLight6PositionWReg                            1
#define GlintLight6PositionWOff                       0x9188

#define GlintLight6SpotlightDirectionX                0xa190
#define GlintLight6SpotlightDirectionXTag             0x0432
#define GlintLight6SpotlightDirectionXReg                  1
#define GlintLight6SpotlightDirectionXOff             0x9190

#define GlintLight6SpotlightDirectionY                0xa198
#define GlintLight6SpotlightDirectionYTag             0x0433
#define GlintLight6SpotlightDirectionYReg                  1
#define GlintLight6SpotlightDirectionYOff             0x9198

#define GlintLight6SpotlightDirectionZ                0xa1a0
#define GlintLight6SpotlightDirectionZTag             0x0434
#define GlintLight6SpotlightDirectionZReg                  1
#define GlintLight6SpotlightDirectionZOff             0x91a0

#define GlintLight6SpotlightExponent                  0xa1a8
#define GlintLight6SpotlightExponentTag               0x0435
#define GlintLight6SpotlightExponentReg                    1
#define GlintLight6SpotlightExponentOff               0x91a8

#define GlintLight6CosSpotlightCutoffAngle            0xa1b0
#define GlintLight6CosSpotlightCutoffAngleTag         0x0436
#define GlintLight6CosSpotlightCutoffAngleReg              1
#define GlintLight6CosSpotlightCutoffAngleOff         0x91b0

#define GlintLight6ConstantAttenuation                0xa1b8
#define GlintLight6ConstantAttenuationTag             0x0437
#define GlintLight6ConstantAttenuationReg                  1
#define GlintLight6ConstantAttenuationOff             0x91b8

#define GlintLight6LinearAttenuation                  0xa1c0
#define GlintLight6LinearAttenuationTag               0x0438
#define GlintLight6LinearAttenuationReg                    1
#define GlintLight6LinearAttenuationOff               0x91c0

#define GlintLight6QuadraticAttenuation               0xa1c8
#define GlintLight6QuadraticAttenuationTag            0x0439
#define GlintLight6QuadraticAttenuationReg                 1
#define GlintLight6QuadraticAttenuationOff            0x91c8

#define GlintLight7Mode                               0xa1d0
#define GlintLight7ModeTag                            0x043a
#define GlintLight7ModeReg                                 1
#define GlintLight7ModeOff                            0x91d0

#define GlintLight7AmbientIntensityRed                0xa1d8
#define GlintLight7AmbientIntensityRedTag             0x043b
#define GlintLight7AmbientIntensityRedReg                  1
#define GlintLight7AmbientIntensityRedOff             0x91d8

#define GlintLight7AmbientIntensityGreen              0xa1e0
#define GlintLight7AmbientIntensityGreenTag           0x043c
#define GlintLight7AmbientIntensityGreenReg                1
#define GlintLight7AmbientIntensityGreenOff           0x91e0

#define GlintLight7AmbientIntensityBlue               0xa1e8
#define GlintLight7AmbientIntensityBlueTag            0x043d
#define GlintLight7AmbientIntensityBlueReg                 1
#define GlintLight7AmbientIntensityBlueOff            0x91e8

#define GlintLight7DiffuseIntensityRed                0xa1f0
#define GlintLight7DiffuseIntensityRedTag             0x043e
#define GlintLight7DiffuseIntensityRedReg                  1
#define GlintLight7DiffuseIntensityRedOff             0x91f0

#define GlintLight7DiffuseIntensityGreen              0xa1f8
#define GlintLight7DiffuseIntensityGreenTag           0x043f
#define GlintLight7DiffuseIntensityGreenReg                1
#define GlintLight7DiffuseIntensityGreenOff           0x91f8

#define GlintLight7DiffuseIntensityBlue               0xa200
#define GlintLight7DiffuseIntensityBlueTag            0x0440
#define GlintLight7DiffuseIntensityBlueReg                 1
#define GlintLight7DiffuseIntensityBlueOff            0x9200

#define GlintLight7SpecularIntensityRed               0xa208
#define GlintLight7SpecularIntensityRedTag            0x0441
#define GlintLight7SpecularIntensityRedReg                 1
#define GlintLight7SpecularIntensityRedOff            0x9208

#define GlintLight7SpecularIntensityGreen             0xa210
#define GlintLight7SpecularIntensityGreenTag          0x0442
#define GlintLight7SpecularIntensityGreenReg               1
#define GlintLight7SpecularIntensityGreenOff          0x9210

#define GlintLight7SpecularIntensityBlue              0xa218
#define GlintLight7SpecularIntensityBlueTag           0x0443
#define GlintLight7SpecularIntensityBlueReg                1
#define GlintLight7SpecularIntensityBlueOff           0x9218

#define GlintLight7PositionX                          0xa220
#define GlintLight7PositionXTag                       0x0444
#define GlintLight7PositionXReg                            1
#define GlintLight7PositionXOff                       0x9220

#define GlintLight7PositionY                          0xa228
#define GlintLight7PositionYTag                       0x0445
#define GlintLight7PositionYReg                            1
#define GlintLight7PositionYOff                       0x9228

#define GlintLight7PositionZ                          0xa230
#define GlintLight7PositionZTag                       0x0446
#define GlintLight7PositionZReg                            1
#define GlintLight7PositionZOff                       0x9230

#define GlintLight7PositionW                          0xa238
#define GlintLight7PositionWTag                       0x0447
#define GlintLight7PositionWReg                            1
#define GlintLight7PositionWOff                       0x9238

#define GlintLight7SpotlightDirectionX                0xa240
#define GlintLight7SpotlightDirectionXTag             0x0448
#define GlintLight7SpotlightDirectionXReg                  1
#define GlintLight7SpotlightDirectionXOff             0x9240

#define GlintLight7SpotlightDirectionY                0xa248
#define GlintLight7SpotlightDirectionYTag             0x0449
#define GlintLight7SpotlightDirectionYReg                  1
#define GlintLight7SpotlightDirectionYOff             0x9248

#define GlintLight7SpotlightDirectionZ                0xa250
#define GlintLight7SpotlightDirectionZTag             0x044a
#define GlintLight7SpotlightDirectionZReg                  1
#define GlintLight7SpotlightDirectionZOff             0x9250

#define GlintLight7SpotlightExponent                  0xa258
#define GlintLight7SpotlightExponentTag               0x044b
#define GlintLight7SpotlightExponentReg                    1
#define GlintLight7SpotlightExponentOff               0x9258

#define GlintLight7CosSpotlightCutoffAngle            0xa260
#define GlintLight7CosSpotlightCutoffAngleTag         0x044c
#define GlintLight7CosSpotlightCutoffAngleReg              1
#define GlintLight7CosSpotlightCutoffAngleOff         0x9260

#define GlintLight7ConstantAttenuation                0xa268
#define GlintLight7ConstantAttenuationTag             0x044d
#define GlintLight7ConstantAttenuationReg                  1
#define GlintLight7ConstantAttenuationOff             0x9268

#define GlintLight7LinearAttenuation                  0xa270
#define GlintLight7LinearAttenuationTag               0x044e
#define GlintLight7LinearAttenuationReg                    1
#define GlintLight7LinearAttenuationOff               0x9270

#define GlintLight7QuadraticAttenuation               0xa278
#define GlintLight7QuadraticAttenuationTag            0x044f
#define GlintLight7QuadraticAttenuationReg                 1
#define GlintLight7QuadraticAttenuationOff            0x9278

#define GlintLight8Mode                               0xa280
#define GlintLight8ModeTag                            0x0450
#define GlintLight8ModeReg                                 1
#define GlintLight8ModeOff                            0x9280

#define GlintLight8AmbientIntensityRed                0xa288
#define GlintLight8AmbientIntensityRedTag             0x0451
#define GlintLight8AmbientIntensityRedReg                  1
#define GlintLight8AmbientIntensityRedOff             0x9288

#define GlintLight8AmbientIntensityGreen              0xa290
#define GlintLight8AmbientIntensityGreenTag           0x0452
#define GlintLight8AmbientIntensityGreenReg                1
#define GlintLight8AmbientIntensityGreenOff           0x9290

#define GlintLight8AmbientIntensityBlue               0xa298
#define GlintLight8AmbientIntensityBlueTag            0x0453
#define GlintLight8AmbientIntensityBlueReg                 1
#define GlintLight8AmbientIntensityBlueOff            0x9298

#define GlintLight8DiffuseIntensityRed                0xa2a0
#define GlintLight8DiffuseIntensityRedTag             0x0454
#define GlintLight8DiffuseIntensityRedReg                  1
#define GlintLight8DiffuseIntensityRedOff             0x92a0

#define GlintLight8DiffuseIntensityGreen              0xa2a8
#define GlintLight8DiffuseIntensityGreenTag           0x0455
#define GlintLight8DiffuseIntensityGreenReg                1
#define GlintLight8DiffuseIntensityGreenOff           0x92a8

#define GlintLight8DiffuseIntensityBlue               0xa2b0
#define GlintLight8DiffuseIntensityBlueTag            0x0456
#define GlintLight8DiffuseIntensityBlueReg                 1
#define GlintLight8DiffuseIntensityBlueOff            0x92b0

#define GlintLight8SpecularIntensityRed               0xa2b8
#define GlintLight8SpecularIntensityRedTag            0x0457
#define GlintLight8SpecularIntensityRedReg                 1
#define GlintLight8SpecularIntensityRedOff            0x92b8

#define GlintLight8SpecularIntensityGreen             0xa2c0
#define GlintLight8SpecularIntensityGreenTag          0x0458
#define GlintLight8SpecularIntensityGreenReg               1
#define GlintLight8SpecularIntensityGreenOff          0x92c0

#define GlintLight8SpecularIntensityBlue              0xa2c8
#define GlintLight8SpecularIntensityBlueTag           0x0459
#define GlintLight8SpecularIntensityBlueReg                1
#define GlintLight8SpecularIntensityBlueOff           0x92c8

#define GlintLight8PositionX                          0xa2d0
#define GlintLight8PositionXTag                       0x045a
#define GlintLight8PositionXReg                            1
#define GlintLight8PositionXOff                       0x92d0

#define GlintLight8PositionY                          0xa2d8
#define GlintLight8PositionYTag                       0x045b
#define GlintLight8PositionYReg                            1
#define GlintLight8PositionYOff                       0x92d8

#define GlintLight8PositionZ                          0xa2e0
#define GlintLight8PositionZTag                       0x045c
#define GlintLight8PositionZReg                            1
#define GlintLight8PositionZOff                       0x92e0

#define GlintLight8PositionW                          0xa2e8
#define GlintLight8PositionWTag                       0x045d
#define GlintLight8PositionWReg                            1
#define GlintLight8PositionWOff                       0x92e8

#define GlintLight8SpotlightDirectionX                0xa2f0
#define GlintLight8SpotlightDirectionXTag             0x045e
#define GlintLight8SpotlightDirectionXReg                  1
#define GlintLight8SpotlightDirectionXOff             0x92f0

#define GlintLight8SpotlightDirectionY                0xa2f8
#define GlintLight8SpotlightDirectionYTag             0x045f
#define GlintLight8SpotlightDirectionYReg                  1
#define GlintLight8SpotlightDirectionYOff             0x92f8

#define GlintLight8SpotlightDirectionZ                0xa300
#define GlintLight8SpotlightDirectionZTag             0x0460
#define GlintLight8SpotlightDirectionZReg                  1
#define GlintLight8SpotlightDirectionZOff             0x9300

#define GlintLight8SpotlightExponent                  0xa308
#define GlintLight8SpotlightExponentTag               0x0461
#define GlintLight8SpotlightExponentReg                    1
#define GlintLight8SpotlightExponentOff               0x9308

#define GlintLight8CosSpotlightCutoffAngle            0xa310
#define GlintLight8CosSpotlightCutoffAngleTag         0x0462
#define GlintLight8CosSpotlightCutoffAngleReg              1
#define GlintLight8CosSpotlightCutoffAngleOff         0x9310

#define GlintLight8ConstantAttenuation                0xa318
#define GlintLight8ConstantAttenuationTag             0x0463
#define GlintLight8ConstantAttenuationReg                  1
#define GlintLight8ConstantAttenuationOff             0x9318

#define GlintLight8LinearAttenuation                  0xa320
#define GlintLight8LinearAttenuationTag               0x0464
#define GlintLight8LinearAttenuationReg                    1
#define GlintLight8LinearAttenuationOff               0x9320

#define GlintLight8QuadraticAttenuation               0xa328
#define GlintLight8QuadraticAttenuationTag            0x0465
#define GlintLight8QuadraticAttenuationReg                 1
#define GlintLight8QuadraticAttenuationOff            0x9328

#define GlintLight9Mode                               0xa330
#define GlintLight9ModeTag                            0x0466
#define GlintLight9ModeReg                                 1
#define GlintLight9ModeOff                            0x9330

#define GlintLight9AmbientIntensityRed                0xa338
#define GlintLight9AmbientIntensityRedTag             0x0467
#define GlintLight9AmbientIntensityRedReg                  1
#define GlintLight9AmbientIntensityRedOff             0x9338

#define GlintLight9AmbientIntensityGreen              0xa340
#define GlintLight9AmbientIntensityGreenTag           0x0468
#define GlintLight9AmbientIntensityGreenReg                1
#define GlintLight9AmbientIntensityGreenOff           0x9340

#define GlintLight9AmbientIntensityBlue               0xa348
#define GlintLight9AmbientIntensityBlueTag            0x0469
#define GlintLight9AmbientIntensityBlueReg                 1
#define GlintLight9AmbientIntensityBlueOff            0x9348

#define GlintLight9DiffuseIntensityRed                0xa350
#define GlintLight9DiffuseIntensityRedTag             0x046a
#define GlintLight9DiffuseIntensityRedReg                  1
#define GlintLight9DiffuseIntensityRedOff             0x9350

#define GlintLight9DiffuseIntensityGreen              0xa358
#define GlintLight9DiffuseIntensityGreenTag           0x046b
#define GlintLight9DiffuseIntensityGreenReg                1
#define GlintLight9DiffuseIntensityGreenOff           0x9358

#define GlintLight9DiffuseIntensityBlue               0xa360
#define GlintLight9DiffuseIntensityBlueTag            0x046c
#define GlintLight9DiffuseIntensityBlueReg                 1
#define GlintLight9DiffuseIntensityBlueOff            0x9360

#define GlintLight9SpecularIntensityRed               0xa368
#define GlintLight9SpecularIntensityRedTag            0x046d
#define GlintLight9SpecularIntensityRedReg                 1
#define GlintLight9SpecularIntensityRedOff            0x9368

#define GlintLight9SpecularIntensityGreen             0xa370
#define GlintLight9SpecularIntensityGreenTag          0x046e
#define GlintLight9SpecularIntensityGreenReg               1
#define GlintLight9SpecularIntensityGreenOff          0x9370

#define GlintLight9SpecularIntensityBlue              0xa378
#define GlintLight9SpecularIntensityBlueTag           0x046f
#define GlintLight9SpecularIntensityBlueReg                1
#define GlintLight9SpecularIntensityBlueOff           0x9378

#define GlintLight9PositionX                          0xa380
#define GlintLight9PositionXTag                       0x0470
#define GlintLight9PositionXReg                            1
#define GlintLight9PositionXOff                       0x9380

#define GlintLight9PositionY                          0xa388
#define GlintLight9PositionYTag                       0x0471
#define GlintLight9PositionYReg                            1
#define GlintLight9PositionYOff                       0x9388

#define GlintLight9PositionZ                          0xa390
#define GlintLight9PositionZTag                       0x0472
#define GlintLight9PositionZReg                            1
#define GlintLight9PositionZOff                       0x9390

#define GlintLight9PositionW                          0xa398
#define GlintLight9PositionWTag                       0x0473
#define GlintLight9PositionWReg                            1
#define GlintLight9PositionWOff                       0x9398

#define GlintLight9SpotlightDirectionX                0xa3a0
#define GlintLight9SpotlightDirectionXTag             0x0474
#define GlintLight9SpotlightDirectionXReg                  1
#define GlintLight9SpotlightDirectionXOff             0x93a0

#define GlintLight9SpotlightDirectionY                0xa3a8
#define GlintLight9SpotlightDirectionYTag             0x0475
#define GlintLight9SpotlightDirectionYReg                  1
#define GlintLight9SpotlightDirectionYOff             0x93a8

#define GlintLight9SpotlightDirectionZ                0xa3b0
#define GlintLight9SpotlightDirectionZTag             0x0476
#define GlintLight9SpotlightDirectionZReg                  1
#define GlintLight9SpotlightDirectionZOff             0x93b0

#define GlintLight9SpotlightExponent                  0xa3b8
#define GlintLight9SpotlightExponentTag               0x0477
#define GlintLight9SpotlightExponentReg                    1
#define GlintLight9SpotlightExponentOff               0x93b8

#define GlintLight9CosSpotlightCutoffAngle            0xa3c0
#define GlintLight9CosSpotlightCutoffAngleTag         0x0478
#define GlintLight9CosSpotlightCutoffAngleReg              1
#define GlintLight9CosSpotlightCutoffAngleOff         0x93c0

#define GlintLight9ConstantAttenuation                0xa3c8
#define GlintLight9ConstantAttenuationTag             0x0479
#define GlintLight9ConstantAttenuationReg                  1
#define GlintLight9ConstantAttenuationOff             0x93c8

#define GlintLight9LinearAttenuation                  0xa3d0
#define GlintLight9LinearAttenuationTag               0x047a
#define GlintLight9LinearAttenuationReg                    1
#define GlintLight9LinearAttenuationOff               0x93d0

#define GlintLight9QuadraticAttenuation               0xa3d8
#define GlintLight9QuadraticAttenuationTag            0x047b
#define GlintLight9QuadraticAttenuationReg                 1
#define GlintLight9QuadraticAttenuationOff            0x93d8

#define GlintLight10Mode                              0xa3e0
#define GlintLight10ModeTag                           0x047c
#define GlintLight10ModeReg                                1
#define GlintLight10ModeOff                           0x93e0

#define GlintLight10AmbientIntensityRed               0xa3e8
#define GlintLight10AmbientIntensityRedTag            0x047d
#define GlintLight10AmbientIntensityRedReg                 1
#define GlintLight10AmbientIntensityRedOff            0x93e8

#define GlintLight10AmbientIntensityGreen             0xa3f0
#define GlintLight10AmbientIntensityGreenTag          0x047e
#define GlintLight10AmbientIntensityGreenReg               1
#define GlintLight10AmbientIntensityGreenOff          0x93f0

#define GlintLight10AmbientIntensityBlue              0xa3f8
#define GlintLight10AmbientIntensityBlueTag           0x047f
#define GlintLight10AmbientIntensityBlueReg                1
#define GlintLight10AmbientIntensityBlueOff           0x93f8

#define GlintLight10DiffuseIntensityRed               0xa400
#define GlintLight10DiffuseIntensityRedTag            0x0480
#define GlintLight10DiffuseIntensityRedReg                 1
#define GlintLight10DiffuseIntensityRedOff            0x9400

#define GlintLight10DiffuseIntensityGreen             0xa408
#define GlintLight10DiffuseIntensityGreenTag          0x0481
#define GlintLight10DiffuseIntensityGreenReg               1
#define GlintLight10DiffuseIntensityGreenOff          0x9408

#define GlintLight10DiffuseIntensityBlue              0xa410
#define GlintLight10DiffuseIntensityBlueTag           0x0482
#define GlintLight10DiffuseIntensityBlueReg                1
#define GlintLight10DiffuseIntensityBlueOff           0x9410

#define GlintLight10SpecularIntensityRed              0xa418
#define GlintLight10SpecularIntensityRedTag           0x0483
#define GlintLight10SpecularIntensityRedReg                1
#define GlintLight10SpecularIntensityRedOff           0x9418

#define GlintLight10SpecularIntensityGreen            0xa420
#define GlintLight10SpecularIntensityGreenTag         0x0484
#define GlintLight10SpecularIntensityGreenReg              1
#define GlintLight10SpecularIntensityGreenOff         0x9420

#define GlintLight10SpecularIntensityBlue             0xa428
#define GlintLight10SpecularIntensityBlueTag          0x0485
#define GlintLight10SpecularIntensityBlueReg               1
#define GlintLight10SpecularIntensityBlueOff          0x9428

#define GlintLight10PositionX                         0xa430
#define GlintLight10PositionXTag                      0x0486
#define GlintLight10PositionXReg                           1
#define GlintLight10PositionXOff                      0x9430

#define GlintLight10PositionY                         0xa438
#define GlintLight10PositionYTag                      0x0487
#define GlintLight10PositionYReg                           1
#define GlintLight10PositionYOff                      0x9438

#define GlintLight10PositionZ                         0xa440
#define GlintLight10PositionZTag                      0x0488
#define GlintLight10PositionZReg                           1
#define GlintLight10PositionZOff                      0x9440

#define GlintLight10PositionW                         0xa448
#define GlintLight10PositionWTag                      0x0489
#define GlintLight10PositionWReg                           1
#define GlintLight10PositionWOff                      0x9448

#define GlintLight10SpotlightDirectionX               0xa450
#define GlintLight10SpotlightDirectionXTag            0x048a
#define GlintLight10SpotlightDirectionXReg                 1
#define GlintLight10SpotlightDirectionXOff            0x9450

#define GlintLight10SpotlightDirectionY               0xa458
#define GlintLight10SpotlightDirectionYTag            0x048b
#define GlintLight10SpotlightDirectionYReg                 1
#define GlintLight10SpotlightDirectionYOff            0x9458

#define GlintLight10SpotlightDirectionZ               0xa460
#define GlintLight10SpotlightDirectionZTag            0x048c
#define GlintLight10SpotlightDirectionZReg                 1
#define GlintLight10SpotlightDirectionZOff            0x9460

#define GlintLight10SpotlightExponent                 0xa468
#define GlintLight10SpotlightExponentTag              0x048d
#define GlintLight10SpotlightExponentReg                   1
#define GlintLight10SpotlightExponentOff              0x9468

#define GlintLight10CosSpotlightCutoffAngle           0xa470
#define GlintLight10CosSpotlightCutoffAngleTag        0x048e
#define GlintLight10CosSpotlightCutoffAngleReg             1
#define GlintLight10CosSpotlightCutoffAngleOff        0x9470

#define GlintLight10ConstantAttenuation               0xa478
#define GlintLight10ConstantAttenuationTag            0x048f
#define GlintLight10ConstantAttenuationReg                 1
#define GlintLight10ConstantAttenuationOff            0x9478

#define GlintLight10LinearAttenuation                 0xa480
#define GlintLight10LinearAttenuationTag              0x0490
#define GlintLight10LinearAttenuationReg                   1
#define GlintLight10LinearAttenuationOff              0x9480

#define GlintLight10QuadraticAttenuation              0xa488
#define GlintLight10QuadraticAttenuationTag           0x0491
#define GlintLight10QuadraticAttenuationReg                1
#define GlintLight10QuadraticAttenuationOff           0x9488

#define GlintLight11Mode                              0xa490
#define GlintLight11ModeTag                           0x0492
#define GlintLight11ModeReg                                1
#define GlintLight11ModeOff                           0x9490

#define GlintLight11AmbientIntensiveRed               0xa498
#define GlintLight11AmbientIntensiveRedTag            0x0493
#define GlintLight11AmbientIntensiveRedReg                 1
#define GlintLight11AmbientIntensiveRedOff            0x9498

#define GlintLight11AmbientIntensityGreen             0xa4a0
#define GlintLight11AmbientIntensityGreenTag          0x0494
#define GlintLight11AmbientIntensityGreenReg               1
#define GlintLight11AmbientIntensityGreenOff          0x94a0

#define GlintLight11AmbientIntensityBlue              0xa4a8
#define GlintLight11AmbientIntensityBlueTag           0x0495
#define GlintLight11AmbientIntensityBlueReg                1
#define GlintLight11AmbientIntensityBlueOff           0x94a8

#define GlintLight11DiffuseIntensityRed               0xa4b0
#define GlintLight11DiffuseIntensityRedTag            0x0496
#define GlintLight11DiffuseIntensityRedReg                 1
#define GlintLight11DiffuseIntensityRedOff            0x94b0

#define GlintLight11DiffuseIntensityGreen             0xa4b8
#define GlintLight11DiffuseIntensityGreenTag          0x0497
#define GlintLight11DiffuseIntensityGreenReg               1
#define GlintLight11DiffuseIntensityGreenOff          0x94b8

#define GlintLight11DiffuseIntensityBlue              0xa4c0
#define GlintLight11DiffuseIntensityBlueTag           0x0498
#define GlintLight11DiffuseIntensityBlueReg                1
#define GlintLight11DiffuseIntensityBlueOff           0x94c0

#define GlintLight11SpecularIntensityRed              0xa4c8
#define GlintLight11SpecularIntensityRedTag           0x0499
#define GlintLight11SpecularIntensityRedReg                1
#define GlintLight11SpecularIntensityRedOff           0x94c8

#define GlintLight11SpecularIntensityGreen            0xa4d0
#define GlintLight11SpecularIntensityGreenTag         0x049a
#define GlintLight11SpecularIntensityGreenReg              1
#define GlintLight11SpecularIntensityGreenOff         0x94d0

#define GlintLight11SpecularIntensityBlue             0xa4d8
#define GlintLight11SpecularIntensityBlueTag          0x049b
#define GlintLight11SpecularIntensityBlueReg               1
#define GlintLight11SpecularIntensityBlueOff          0x94d8

#define GlintLight11PositionX                         0xa4e0
#define GlintLight11PositionXTag                      0x049c
#define GlintLight11PositionXReg                           1
#define GlintLight11PositionXOff                      0x94e0

#define GlintLight11PositionY                         0xa4e8
#define GlintLight11PositionYTag                      0x049d
#define GlintLight11PositionYReg                           1
#define GlintLight11PositionYOff                      0x94e8

#define GlintLight11PositionZ                         0xa4f0
#define GlintLight11PositionZTag                      0x049e
#define GlintLight11PositionZReg                           1
#define GlintLight11PositionZOff                      0x94f0

#define GlintLight11PositionW                         0xa4f8
#define GlintLight11PositionWTag                      0x049f
#define GlintLight11PositionWReg                           1
#define GlintLight11PositionWOff                      0x94f8

#define GlintLight11SpotlightDirectionX               0xa500
#define GlintLight11SpotlightDirectionXTag            0x04a0
#define GlintLight11SpotlightDirectionXReg                 1
#define GlintLight11SpotlightDirectionXOff            0x9500

#define GlintLight11SpotlightDirectionY               0xa508
#define GlintLight11SpotlightDirectionYTag            0x04a1
#define GlintLight11SpotlightDirectionYReg                 1
#define GlintLight11SpotlightDirectionYOff            0x9508

#define GlintLight11SpotlightDirectionZ               0xa510
#define GlintLight11SpotlightDirectionZTag            0x04a2
#define GlintLight11SpotlightDirectionZReg                 1
#define GlintLight11SpotlightDirectionZOff            0x9510

#define GlintLight11SpotlightExponent                 0xa518
#define GlintLight11SpotlightExponentTag              0x04a3
#define GlintLight11SpotlightExponentReg                   1
#define GlintLight11SpotlightExponentOff              0x9518

#define GlintLight11CosSpotlightCutoffAngle           0xa520
#define GlintLight11CosSpotlightCutoffAngleTag        0x04a4
#define GlintLight11CosSpotlightCutoffAngleReg             1
#define GlintLight11CosSpotlightCutoffAngleOff        0x9520

#define GlintLight11ConstantAttenuation               0xa528
#define GlintLight11ConstantAttenuationTag            0x04a5
#define GlintLight11ConstantAttenuationReg                 1
#define GlintLight11ConstantAttenuationOff            0x9528

#define GlintLight11LinearAttenuation                 0xa530
#define GlintLight11LinearAttenuationTag              0x04a6
#define GlintLight11LinearAttenuationReg                   1
#define GlintLight11LinearAttenuationOff              0x9530

#define GlintLight11QuadraticAttenuation              0xa538
#define GlintLight11QuadraticAttenuationTag           0x04a7
#define GlintLight11QuadraticAttenuationReg                1
#define GlintLight11QuadraticAttenuationOff           0x9538

#define GlintLight12Mode                              0xa540
#define GlintLight12ModeTag                           0x04a8
#define GlintLight12ModeReg                                1
#define GlintLight12ModeOff                           0x9540

#define GlintLight12AmbientIntensityRed               0xa548
#define GlintLight12AmbientIntensityRedTag            0x04a9
#define GlintLight12AmbientIntensityRedReg                 1
#define GlintLight12AmbientIntensityRedOff            0x9548

#define GlintLight12AmbientIntensityGreen             0xa550
#define GlintLight12AmbientIntensityGreenTag          0x04aa
#define GlintLight12AmbientIntensityGreenReg               1
#define GlintLight12AmbientIntensityGreenOff          0x9550

#define GlintLight12AmbientIntensityBlue              0xa558
#define GlintLight12AmbientIntensityBlueTag           0x04ab
#define GlintLight12AmbientIntensityBlueReg                1
#define GlintLight12AmbientIntensityBlueOff           0x9558

#define GlintLight12DiffuseIntensityRed               0xa560
#define GlintLight12DiffuseIntensityRedTag            0x04ac
#define GlintLight12DiffuseIntensityRedReg                 1
#define GlintLight12DiffuseIntensityRedOff            0x9560

#define GlintLight12DiffuseIntensityGreen             0xa568
#define GlintLight12DiffuseIntensityGreenTag          0x04ad
#define GlintLight12DiffuseIntensityGreenReg               1
#define GlintLight12DiffuseIntensityGreenOff          0x9568

#define GlintLight12DiffuseIntensityBlue              0xa570
#define GlintLight12DiffuseIntensityBlueTag           0x04ae
#define GlintLight12DiffuseIntensityBlueReg                1
#define GlintLight12DiffuseIntensityBlueOff           0x9570

#define GlintLight12SpecularIntensityRed              0xa578
#define GlintLight12SpecularIntensityRedTag           0x04af
#define GlintLight12SpecularIntensityRedReg                1
#define GlintLight12SpecularIntensityRedOff           0x9578

#define GlintLight12SpecularIntensityGreen            0xa580
#define GlintLight12SpecularIntensityGreenTag         0x04b0
#define GlintLight12SpecularIntensityGreenReg              1
#define GlintLight12SpecularIntensityGreenOff         0x9580

#define GlintLight12SpecularIntensityBlue             0xa588
#define GlintLight12SpecularIntensityBlueTag          0x04b1
#define GlintLight12SpecularIntensityBlueReg               1
#define GlintLight12SpecularIntensityBlueOff          0x9588

#define GlintLight12PositionX                         0xa590
#define GlintLight12PositionXTag                      0x04b2
#define GlintLight12PositionXReg                           1
#define GlintLight12PositionXOff                      0x9590

#define GlintLight12PositionY                         0xa598
#define GlintLight12PositionYTag                      0x04b3
#define GlintLight12PositionYReg                           1
#define GlintLight12PositionYOff                      0x9598

#define GlintLight12PositionZ                         0xa5a0
#define GlintLight12PositionZTag                      0x04b4
#define GlintLight12PositionZReg                           1
#define GlintLight12PositionZOff                      0x95a0

#define GlintLight12PositionW                         0xa5a8
#define GlintLight12PositionWTag                      0x04b5
#define GlintLight12PositionWReg                           1
#define GlintLight12PositionWOff                      0x95a8

#define GlintLight12SpotlightDirectionX               0xa5b0
#define GlintLight12SpotlightDirectionXTag            0x04b6
#define GlintLight12SpotlightDirectionXReg                 1
#define GlintLight12SpotlightDirectionXOff            0x95b0

#define GlintLight12SpotlightDirectionY               0xa5b8
#define GlintLight12SpotlightDirectionYTag            0x04b7
#define GlintLight12SpotlightDirectionYReg                 1
#define GlintLight12SpotlightDirectionYOff            0x95b8

#define GlintLight12SpotlightDirectionZ               0xa5c0
#define GlintLight12SpotlightDirectionZTag            0x04b8
#define GlintLight12SpotlightDirectionZReg                 1
#define GlintLight12SpotlightDirectionZOff            0x95c0

#define GlintLight12SpotlightExponent                 0xa5c8
#define GlintLight12SpotlightExponentTag              0x04b9
#define GlintLight12SpotlightExponentReg                   1
#define GlintLight12SpotlightExponentOff              0x95c8

#define GlintLight12CosSpotlightCutoffAngle           0xa5d0
#define GlintLight12CosSpotlightCutoffAngleTag        0x04ba
#define GlintLight12CosSpotlightCutoffAngleReg             1
#define GlintLight12CosSpotlightCutoffAngleOff        0x95d0

#define GlintLight12ConstantAttenuation               0xa5d8
#define GlintLight12ConstantAttenuationTag            0x04bb
#define GlintLight12ConstantAttenuationReg                 1
#define GlintLight12ConstantAttenuationOff            0x95d8

#define GlintLight12LinearAttenuation                 0xa5e0
#define GlintLight12LinearAttenuationTag              0x04bc
#define GlintLight12LinearAttenuationReg                   1
#define GlintLight12LinearAttenuationOff              0x95e0

#define GlintLight12QuadraticAttenuation              0xa5e8
#define GlintLight12QuadraticAttenuationTag           0x04bd
#define GlintLight12QuadraticAttenuationReg                1
#define GlintLight12QuadraticAttenuationOff           0x95e8

#define GlintLight13Mode                              0xa5f0
#define GlintLight13ModeTag                           0x04be
#define GlintLight13ModeReg                                1
#define GlintLight13ModeOff                           0x95f0

#define GlintLight13AmbientIntensityRed               0xa5f8
#define GlintLight13AmbientIntensityRedTag            0x04bf
#define GlintLight13AmbientIntensityRedReg                 1
#define GlintLight13AmbientIntensityRedOff            0x95f8

#define GlintLight13AmbientIntensityGreen             0xa600
#define GlintLight13AmbientIntensityGreenTag          0x04c0
#define GlintLight13AmbientIntensityGreenReg               1
#define GlintLight13AmbientIntensityGreenOff          0x9600

#define GlintLight13AmbientIntensityBlue              0xa608
#define GlintLight13AmbientIntensityBlueTag           0x04c1
#define GlintLight13AmbientIntensityBlueReg                1
#define GlintLight13AmbientIntensityBlueOff           0x9608

#define GlintLight13DiffuseIntensityRed               0xa610
#define GlintLight13DiffuseIntensityRedTag            0x04c2
#define GlintLight13DiffuseIntensityRedReg                 1
#define GlintLight13DiffuseIntensityRedOff            0x9610

#define GlintLight13DiffuseIntensityGreen             0xa618
#define GlintLight13DiffuseIntensityGreenTag          0x04c3
#define GlintLight13DiffuseIntensityGreenReg               1
#define GlintLight13DiffuseIntensityGreenOff          0x9618

#define GlintLight13DiffuseIntensityBlue              0xa620
#define GlintLight13DiffuseIntensityBlueTag           0x04c4
#define GlintLight13DiffuseIntensityBlueReg                1
#define GlintLight13DiffuseIntensityBlueOff           0x9620

#define GlintLight13SpecularIntensityRed              0xa628
#define GlintLight13SpecularIntensityRedTag           0x04c5
#define GlintLight13SpecularIntensityRedReg                1
#define GlintLight13SpecularIntensityRedOff           0x9628

#define GlintLight13SpecularIntensityGreen            0xa630
#define GlintLight13SpecularIntensityGreenTag         0x04c6
#define GlintLight13SpecularIntensityGreenReg              1
#define GlintLight13SpecularIntensityGreenOff         0x9630

#define GlintLight13SpecularIntensityBlue             0xa638
#define GlintLight13SpecularIntensityBlueTag          0x04c7
#define GlintLight13SpecularIntensityBlueReg               1
#define GlintLight13SpecularIntensityBlueOff          0x9638

#define GlintLight13PositionX                         0xa640
#define GlintLight13PositionXTag                      0x04c8
#define GlintLight13PositionXReg                           1
#define GlintLight13PositionXOff                      0x9640

#define GlintLight13PositionY                         0xa648
#define GlintLight13PositionYTag                      0x04c9
#define GlintLight13PositionYReg                           1
#define GlintLight13PositionYOff                      0x9648

#define GlintLight13PositionZ                         0xa650
#define GlintLight13PositionZTag                      0x04ca
#define GlintLight13PositionZReg                           1
#define GlintLight13PositionZOff                      0x9650

#define GlintLight13PositionW                         0xa658
#define GlintLight13PositionWTag                      0x04cb
#define GlintLight13PositionWReg                           1
#define GlintLight13PositionWOff                      0x9658

#define GlintLight13SpotlightDirectionX               0xa660
#define GlintLight13SpotlightDirectionXTag            0x04cc
#define GlintLight13SpotlightDirectionXReg                 1
#define GlintLight13SpotlightDirectionXOff            0x9660

#define GlintLight13SpotlightDirectionY               0xa668
#define GlintLight13SpotlightDirectionYTag            0x04cd
#define GlintLight13SpotlightDirectionYReg                 1
#define GlintLight13SpotlightDirectionYOff            0x9668

#define GlintLight13SpotlightDirectionZ               0xa670
#define GlintLight13SpotlightDirectionZTag            0x04ce
#define GlintLight13SpotlightDirectionZReg                 1
#define GlintLight13SpotlightDirectionZOff            0x9670

#define GlintLight13SpotlightExponent                 0xa678
#define GlintLight13SpotlightExponentTag              0x04cf
#define GlintLight13SpotlightExponentReg                   1
#define GlintLight13SpotlightExponentOff              0x9678

#define GlintLight13CosSpotlightCutoffAngle           0xa680
#define GlintLight13CosSpotlightCutoffAngleTag        0x04d0
#define GlintLight13CosSpotlightCutoffAngleReg             1
#define GlintLight13CosSpotlightCutoffAngleOff        0x9680

#define GlintLight13ConstantAttenuation               0xa688
#define GlintLight13ConstantAttenuationTag            0x04d1
#define GlintLight13ConstantAttenuationReg                 1
#define GlintLight13ConstantAttenuationOff            0x9688

#define GlintLight13LinearAttenuation                 0xa690
#define GlintLight13LinearAttenuationTag              0x04d2
#define GlintLight13LinearAttenuationReg                   1
#define GlintLight13LinearAttenuationOff              0x9690

#define GlintLight13QuadraticAttenuation              0xa698
#define GlintLight13QuadraticAttenuationTag           0x04d3
#define GlintLight13QuadraticAttenuationReg                1
#define GlintLight13QuadraticAttenuationOff           0x9698

#define GlintLight14Mode                              0xa6a0
#define GlintLight14ModeTag                           0x04d4
#define GlintLight14ModeReg                                1
#define GlintLight14ModeOff                           0x96a0

#define GlintLight14AmbientIntensityRed               0xa6a8
#define GlintLight14AmbientIntensityRedTag            0x04d5
#define GlintLight14AmbientIntensityRedReg                 1
#define GlintLight14AmbientIntensityRedOff            0x96a8

#define GlintLight14AmbientIntensityGreen             0xa6b0
#define GlintLight14AmbientIntensityGreenTag          0x04d6
#define GlintLight14AmbientIntensityGreenReg               1
#define GlintLight14AmbientIntensityGreenOff          0x96b0

#define GlintLight14AmbientIntensityBlue              0xa6b8
#define GlintLight14AmbientIntensityBlueTag           0x04d7
#define GlintLight14AmbientIntensityBlueReg                1
#define GlintLight14AmbientIntensityBlueOff           0x96b8

#define GlintLight14DiffuseIntensityRed               0xa6c0
#define GlintLight14DiffuseIntensityRedTag            0x04d8
#define GlintLight14DiffuseIntensityRedReg                 1
#define GlintLight14DiffuseIntensityRedOff            0x96c0

#define GlintLight14DiffuseIntensityGreen             0xa6c8
#define GlintLight14DiffuseIntensityGreenTag          0x04d9
#define GlintLight14DiffuseIntensityGreenReg               1
#define GlintLight14DiffuseIntensityGreenOff          0x96c8

#define GlintLight14DiffuseIntensityBlue              0xa6d0
#define GlintLight14DiffuseIntensityBlueTag           0x04da
#define GlintLight14DiffuseIntensityBlueReg                1
#define GlintLight14DiffuseIntensityBlueOff           0x96d0

#define GlintLight14SpecularIntensityRed              0xa6d8
#define GlintLight14SpecularIntensityRedTag           0x04db
#define GlintLight14SpecularIntensityRedReg                1
#define GlintLight14SpecularIntensityRedOff           0x96d8

#define GlintLight14SpecularIntensityGreen            0xa6e0
#define GlintLight14SpecularIntensityGreenTag         0x04dc
#define GlintLight14SpecularIntensityGreenReg              1
#define GlintLight14SpecularIntensityGreenOff         0x96e0

#define GlintLight14SpecularIntensityBlue             0xa6e8
#define GlintLight14SpecularIntensityBlueTag          0x04dd
#define GlintLight14SpecularIntensityBlueReg               1
#define GlintLight14SpecularIntensityBlueOff          0x96e8

#define GlintLight14PositionX                         0xa6f0
#define GlintLight14PositionXTag                      0x04de
#define GlintLight14PositionXReg                           1
#define GlintLight14PositionXOff                      0x96f0

#define GlintLight14PositionY                         0xa6f8
#define GlintLight14PositionYTag                      0x04df
#define GlintLight14PositionYReg                           1
#define GlintLight14PositionYOff                      0x96f8

#define GlintLight14PositionZ                         0xa700
#define GlintLight14PositionZTag                      0x04e0
#define GlintLight14PositionZReg                           1
#define GlintLight14PositionZOff                      0x9700

#define GlintLight14PositionW                         0xa708
#define GlintLight14PositionWTag                      0x04e1
#define GlintLight14PositionWReg                           1
#define GlintLight14PositionWOff                      0x9708

#define GlintLight14SpotlightDirectionX               0xa710
#define GlintLight14SpotlightDirectionXTag            0x04e2
#define GlintLight14SpotlightDirectionXReg                 1
#define GlintLight14SpotlightDirectionXOff            0x9710

#define GlintLight14SpotlightDirectionY               0xa718
#define GlintLight14SpotlightDirectionYTag            0x04e3
#define GlintLight14SpotlightDirectionYReg                 1
#define GlintLight14SpotlightDirectionYOff            0x9718

#define GlintLight14SpotlightDirectionZ               0xa720
#define GlintLight14SpotlightDirectionZTag            0x04e4
#define GlintLight14SpotlightDirectionZReg                 1
#define GlintLight14SpotlightDirectionZOff            0x9720

#define GlintLight14SpotlightExponent                 0xa728
#define GlintLight14SpotlightExponentTag              0x04e5
#define GlintLight14SpotlightExponentReg                   1
#define GlintLight14SpotlightExponentOff              0x9728

#define GlintLight14CosSpotlightCutoffAngle           0xa730
#define GlintLight14CosSpotlightCutoffAngleTag        0x04e6
#define GlintLight14CosSpotlightCutoffAngleReg             1
#define GlintLight14CosSpotlightCutoffAngleOff        0x9730

#define GlintLight14ConstantAttenuation               0xa738
#define GlintLight14ConstantAttenuationTag            0x04e7
#define GlintLight14ConstantAttenuationReg                 1
#define GlintLight14ConstantAttenuationOff            0x9738

#define GlintLight14LinearAttenuation                 0xa740
#define GlintLight14LinearAttenuationTag              0x04e8
#define GlintLight14LinearAttenuationReg                   1
#define GlintLight14LinearAttenuationOff              0x9740

#define GlintLight14QuadraticAttenuation              0xa748
#define GlintLight14QuadraticAttenuationTag           0x04e9
#define GlintLight14QuadraticAttenuationReg                1
#define GlintLight14QuadraticAttenuationOff           0x9748

#define GlintLight15Mode                              0xa750
#define GlintLight15ModeTag                           0x04ea
#define GlintLight15ModeReg                                1
#define GlintLight15ModeOff                           0x9750

#define GlintLight15AmbientIntensityRed               0xa758
#define GlintLight15AmbientIntensityRedTag            0x04eb
#define GlintLight15AmbientIntensityRedReg                 1
#define GlintLight15AmbientIntensityRedOff            0x9758

#define GlintLight15AmbientIntensityGreen             0xa760
#define GlintLight15AmbientIntensityGreenTag          0x04ec
#define GlintLight15AmbientIntensityGreenReg               1
#define GlintLight15AmbientIntensityGreenOff          0x9760

#define GlintLight15AmbientIntensityBlue              0xa768
#define GlintLight15AmbientIntensityBlueTag           0x04ed
#define GlintLight15AmbientIntensityBlueReg                1
#define GlintLight15AmbientIntensityBlueOff           0x9768

#define GlintLight15DiffuseIntensityRed               0xa770
#define GlintLight15DiffuseIntensityRedTag            0x04ee
#define GlintLight15DiffuseIntensityRedReg                 1
#define GlintLight15DiffuseIntensityRedOff            0x9770

#define GlintLight15DiffuseIntensityGreen             0xa778
#define GlintLight15DiffuseIntensityGreenTag          0x04ef
#define GlintLight15DiffuseIntensityGreenReg               1
#define GlintLight15DiffuseIntensityGreenOff          0x9778

#define GlintLight15DiffuseIntensityBlue              0xa780
#define GlintLight15DiffuseIntensityBlueTag           0x04f0
#define GlintLight15DiffuseIntensityBlueReg                1
#define GlintLight15DiffuseIntensityBlueOff           0x9780

#define GlintLight15SpecularIntensityRed              0xa788
#define GlintLight15SpecularIntensityRedTag           0x04f1
#define GlintLight15SpecularIntensityRedReg                1
#define GlintLight15SpecularIntensityRedOff           0x9788

#define GlintLight15SpecularIntensityGreen            0xa790
#define GlintLight15SpecularIntensityGreenTag         0x04f2
#define GlintLight15SpecularIntensityGreenReg              1
#define GlintLight15SpecularIntensityGreenOff         0x9790

#define GlintLight15SpecularIntensityBlue             0xa798
#define GlintLight15SpecularIntensityBlueTag          0x04f3
#define GlintLight15SpecularIntensityBlueReg               1
#define GlintLight15SpecularIntensityBlueOff          0x9798

#define GlintLight15PositionX                         0xa7a0
#define GlintLight15PositionXTag                      0x04f4
#define GlintLight15PositionXReg                           1
#define GlintLight15PositionXOff                      0x97a0

#define GlintLight15PositionY                         0xa7a8
#define GlintLight15PositionYTag                      0x04f5
#define GlintLight15PositionYReg                           1
#define GlintLight15PositionYOff                      0x97a8

#define GlintLight15PositionZ                         0xa7b0
#define GlintLight15PositionZTag                      0x04f6
#define GlintLight15PositionZReg                           1
#define GlintLight15PositionZOff                      0x97b0

#define GlintLight15PositionW                         0xa7b8
#define GlintLight15PositionWTag                      0x04f7
#define GlintLight15PositionWReg                           1
#define GlintLight15PositionWOff                      0x97b8

#define GlintLight15SpotlightDirectionX               0xa7c0
#define GlintLight15SpotlightDirectionXTag            0x04f8
#define GlintLight15SpotlightDirectionXReg                 1
#define GlintLight15SpotlightDirectionXOff            0x97c0

#define GlintLight15SpotlightDirectionY               0xa7c8
#define GlintLight15SpotlightDirectionYTag            0x04f9
#define GlintLight15SpotlightDirectionYReg                 1
#define GlintLight15SpotlightDirectionYOff            0x97c8

#define GlintLight15SpotlightDirectionZ               0xa7d0
#define GlintLight15SpotlightDirectionZTag            0x04fa
#define GlintLight15SpotlightDirectionZReg                 1
#define GlintLight15SpotlightDirectionZOff            0x97d0

#define GlintLight15SpotlightExponent                 0xa7d8
#define GlintLight15SpotlightExponentTag              0x04fb
#define GlintLight15SpotlightExponentReg                   1
#define GlintLight15SpotlightExponentOff              0x97d8

#define GlintLight15CosSpotlightCutoffAngle           0xa7e0
#define GlintLight15CosSpotlightCutoffAngleTag        0x04fc
#define GlintLight15CosSpotlightCutoffAngleReg             1
#define GlintLight15CosSpotlightCutoffAngleOff        0x97e0

#define GlintLight15ConstantAttenuation               0xa7e8
#define GlintLight15ConstantAttenuationTag            0x04fd
#define GlintLight15ConstantAttenuationReg                 1
#define GlintLight15ConstantAttenuationOff            0x97e8

#define GlintLight15LinearAttenuation                 0xa7f0
#define GlintLight15LinearAttenuationTag              0x04fe
#define GlintLight15LinearAttenuationReg                   1
#define GlintLight15LinearAttenuationOff              0x97f0

#define GlintLight15QuadraticAttenuation              0xa7f8
#define GlintLight15QuadraticAttenuationTag           0x04ff
#define GlintLight15QuadraticAttenuationReg                1
#define GlintLight15QuadraticAttenuationOff           0x97f8

#define GlintSceneAmbientColorRed                     0xa800
#define GlintSceneAmbientColorRedTag                  0x0500
#define GlintSceneAmbientColorRedReg                       1
#define GlintSceneAmbientColorRedOff                  0x9800

#define GlintSceneAmbientColorGreen                   0xa808
#define GlintSceneAmbientColorGreenTag                0x0501
#define GlintSceneAmbientColorGreenReg                     1
#define GlintSceneAmbientColorGreenOff                0x9808

#define GlintSceneAmbientColorBlue                    0xa810
#define GlintSceneAmbientColorBlueTag                 0x0502
#define GlintSceneAmbientColorBlueReg                      1
#define GlintSceneAmbientColorBlueOff                 0x9810

#define GlintFrontAmbientColorRed                     0xa880
#define GlintFrontAmbientColorRedTag                  0x0510
#define GlintFrontAmbientColorRedReg                       1
#define GlintFrontAmbientColorRedOff                  0x9880

#define GlintFrontAmbientColorGreen                   0xa888
#define GlintFrontAmbientColorGreenTag                0x0511
#define GlintFrontAmbientColorGreenReg                     1
#define GlintFrontAmbientColorGreenOff                0x9888

#define GlintFrontAmbientColorBlue                    0xa890
#define GlintFrontAmbientColorBlueTag                 0x0512
#define GlintFrontAmbientColorBlueReg                      1
#define GlintFrontAmbientColorBlueOff                 0x9890

#define GlintFrontDiffuseColorRed                     0xa898
#define GlintFrontDiffuseColorRedTag                  0x0513
#define GlintFrontDiffuseColorRedReg                       1
#define GlintFrontDiffuseColorRedOff                  0x9898

#define GlintFrontDiffuseColorGreen                   0xa8a0
#define GlintFrontDiffuseColorGreenTag                0x0514
#define GlintFrontDiffuseColorGreenReg                     1
#define GlintFrontDiffuseColorGreenOff                0x98a0

#define GlintFrontDiffuseColorBlue                    0xa8a8
#define GlintFrontDiffuseColorBlueTag                 0x0515
#define GlintFrontDiffuseColorBlueReg                      1
#define GlintFrontDiffuseColorBlueOff                 0x98a8

#define GlintFrontAlpha                               0xa8b0
#define GlintFrontAlphaTag                            0x0516
#define GlintFrontAlphaReg                                 1
#define GlintFrontAlphaOff                            0x98b0

#define GlintFrontSpecularColorRed                    0xa8b8
#define GlintFrontSpecularColorRedTag                 0x0517
#define GlintFrontSpecularColorRedReg                      1
#define GlintFrontSpecularColorRedOff                 0x98b8

#define GlintFrontSpecularColorGreen                  0xa8c0
#define GlintFrontSpecularColorGreenTag               0x0518
#define GlintFrontSpecularColorGreenReg                    1
#define GlintFrontSpecularColorGreenOff               0x98c0

#define GlintFrontSpecularColorBlue                   0xa8c8
#define GlintFrontSpecularColorBlueTag                0x0519
#define GlintFrontSpecularColorBlueReg                     1
#define GlintFrontSpecularColorBlueOff                0x98c8

#define GlintFrontEmissiveColorRed                    0xa8d0
#define GlintFrontEmissiveColorRedTag                 0x051a
#define GlintFrontEmissiveColorRedReg                      1
#define GlintFrontEmissiveColorRedOff                 0x98d0

#define GlintFrontEmissiveColorGreen                  0xa8d8
#define GlintFrontEmissiveColorGreenTag               0x051b
#define GlintFrontEmissiveColorGreenReg                    1
#define GlintFrontEmissiveColorGreenOff               0x98d8

#define GlintFrontEmissiveColorBlue                   0xa8e0
#define GlintFrontEmissiveColorBlueTag                0x051c
#define GlintFrontEmissiveColorBlueReg                     1
#define GlintFrontEmissiveColorBlueOff                0x98e0

#define GlintFrontSpecularExponent                    0xa8e8
#define GlintFrontSpecularExponentTag                 0x051d
#define GlintFrontSpecularExponentReg                      1
#define GlintFrontSpecularExponentOff                 0x98e8

#define GlintBackAmbientColorRed                      0xa900
#define GlintBackAmbientColorRedTag                   0x0520
#define GlintBackAmbientColorRedReg                        1
#define GlintBackAmbientColorRedOff                   0x9900

#define GlintBackAmbientColorGreen                    0xa908
#define GlintBackAmbientColorGreenTag                 0x0521
#define GlintBackAmbientColorGreenReg                      1
#define GlintBackAmbientColorGreenOff                 0x9908

#define GlintBackAmbientColorBlue                     0xa910
#define GlintBackAmbientColorBlueTag                  0x0522
#define GlintBackAmbientColorBlueReg                       1
#define GlintBackAmbientColorBlueOff                  0x9910

#define GlintBackDiffuseColorRed                      0xa918
#define GlintBackDiffuseColorRedTag                   0x0523
#define GlintBackDiffuseColorRedReg                        1
#define GlintBackDiffuseColorRedOff                   0x9918

#define GlintBackDiffuseColorGreen                    0xa920
#define GlintBackDiffuseColorGreenTag                 0x0524
#define GlintBackDiffuseColorGreenReg                      1
#define GlintBackDiffuseColorGreenOff                 0x9920

#define GlintBackDiffuseColorBlue                     0xa928
#define GlintBackDiffuseColorBlueTag                  0x0525
#define GlintBackDiffuseColorBlueReg                       1
#define GlintBackDiffuseColorBlueOff                  0x9928

#define GlintBackAlpha                                0xa930
#define GlintBackAlphaTag                             0x0526
#define GlintBackAlphaReg                                  1
#define GlintBackAlphaOff                             0x9930

#define GlintBackSpecularColorRed                     0xa938
#define GlintBackSpecularColorRedTag                  0x0527
#define GlintBackSpecularColorRedReg                       1
#define GlintBackSpecularColorRedOff                  0x9938

#define GlintBackSpecularColorGreen                   0xa940
#define GlintBackSpecularColorGreenTag                0x0528
#define GlintBackSpecularColorGreenReg                     1
#define GlintBackSpecularColorGreenOff                0x9940

#define GlintBackSpecularColorBlue                    0xa948
#define GlintBackSpecularColorBlueTag                 0x0529
#define GlintBackSpecularColorBlueReg                      1
#define GlintBackSpecularColorBlueOff                 0x9948

#define GlintBackEmissiveColorRed                     0xa950
#define GlintBackEmissiveColorRedTag                  0x052a
#define GlintBackEmissiveColorRedReg                       1
#define GlintBackEmissiveColorRedOff                  0x9950

#define GlintBackEmissiveColorGreen                   0xa958
#define GlintBackEmissiveColorGreenTag                0x052b
#define GlintBackEmissiveColorGreenReg                     1
#define GlintBackEmissiveColorGreenOff                0x9958

#define GlintBackEmissiveColorBlue                    0xa960
#define GlintBackEmissiveColorBlueTag                 0x052c
#define GlintBackEmissiveColorBlueReg                      1
#define GlintBackEmissiveColorBlueOff                 0x9960

#define GlintBackSpecularExponent                     0xa968
#define GlintBackSpecularExponentTag                  0x052d
#define GlintBackSpecularExponentReg                       1
#define GlintBackSpecularExponentOff                  0x9968

#define GlintDMAAddr                                  0xa980
#define GlintDMAAddrTag                               0x0530
#define GlintDMAAddrReg                                    1
#define GlintDMAAddrOff                               0x9980

#define GlintGammaDMACount                            0xa988
#define GlintGammaDMACountTag                         0x0531
#define GlintGammaDMACountReg                              1
#define GlintGammaDMACountOff                         0x9988

#define GlintCommandInterrupt                         0xa990
#define GlintCommandInterruptTag                      0x0532
#define GlintCommandInterruptReg                           1
#define GlintCommandInterruptOff                      0x9990

#define GlintDMACall                                  0xa998
#define GlintDMACallTag                               0x0533
#define GlintDMACallReg                                    1
#define GlintDMACallOff                               0x9998

#define GlintDMAReturn                                0xa9a0
#define GlintDMAReturnTag                             0x0534
#define GlintDMAReturnReg                                  1
#define GlintDMAReturnOff                             0x99a0

#define GlintDMARectangularRead                       0xa9a8
#define GlintDMARectangularReadTag                    0x0535
#define GlintDMARectangularReadReg                         1
#define GlintDMARectangularReadOff                    0x99a8

#define GlintDMARectangleReadAddress                  0xa9b0
#define GlintDMARectangleReadAddressTag               0x0536
#define GlintDMARectangleReadAddressReg                    1
#define GlintDMARectangleReadAddressOff               0x99b0

#define GlintDMARectangleReadLinePitch                0xa9b8
#define GlintDMARectangleReadLinePitchTag             0x0537
#define GlintDMARectangleReadLinePitchReg                  1
#define GlintDMARectangleReadLinePitchOff             0x99b8

#define GlintDMARectangleReadTarget                   0xa9c0
#define GlintDMARectangleReadTargetTag                0x0538
#define GlintDMARectangleReadTargetReg                     1
#define GlintDMARectangleReadTargetOff                0x99c0

#define GlintDMARectangleWrite                        0xa9c8
#define GlintDMARectangleWriteTag                     0x0539
#define GlintDMARectangleWriteReg                          1
#define GlintDMARectangleWriteOff                     0x99c8

#define GlintDMARectangleWriteAddress                 0xa9d0
#define GlintDMARectangleWriteAddressTag              0x053a
#define GlintDMARectangleWriteAddressReg                   1
#define GlintDMARectangleWriteAddressOff              0x99d0

#define GlintDMARectangleWriteLinePitch               0xa9d8
#define GlintDMARectangleWriteLinePitchTag            0x053b
#define GlintDMARectangleWriteLinePitchReg                 1
#define GlintDMARectangleWriteLinePitchOff            0x99d8

#define GlintDMAOutputAddress                         0xa9e0
#define GlintDMAOutputAddressTag                      0x053c
#define GlintDMAOutputAddressReg                           1
#define GlintDMAOutputAddressOff                      0x99e0

#define GlintDMAOutputCount                           0xa9e8
#define GlintDMAOutputCountTag                        0x053d
#define GlintDMAOutputCountReg                             1
#define GlintDMAOutputCountOff                        0x99e8

#define GlintDMAReadGLINTSource                       0xa9f0
#define GlintDMAReadGLINTSourceTag                    0x053e
#define GlintDMAReadGLINTSourceReg                         1
#define GlintDMAReadGLINTSourceOff                    0x99f0

#define GlintDMAFeedback                              0xaa10
#define GlintDMAFeedbackTag                           0x0542
#define GlintDMAFeedbackReg                                1
#define GlintDMAFeedbackOff                           0x9a10

#define GlintTransformModeAnd                         0xaa80
#define GlintTransformModeAndTag                      0x0550
#define GlintTransformModeAndReg                           1
#define GlintTransformModeAndOff                      0x9a80

#define GlintTransformModeOr                          0xaa88
#define GlintTransformModeOrTag                       0x0551
#define GlintTransformModeOrReg                            1
#define GlintTransformModeOrOff                       0x9a88

#define GlintGeometryModeAnd                          0xaa90
#define GlintGeometryModeAndTag                       0x0552
#define GlintGeometryModeAndReg                            1
#define GlintGeometryModeAndOff                       0x9a90

#define GlintGeometryModeOr                           0xaa98
#define GlintGeometryModeOrTag                        0x0553
#define GlintGeometryModeOrReg                             1
#define GlintGeometryModeOrOff                        0x9a98

#define GlintNormalizeModeAnd                         0xaaa0
#define GlintNormalizeModeAndTag                      0x0554
#define GlintNormalizeModeAndReg                           1
#define GlintNormalizeModeAndOff                      0x9aa0

#define GlintNormalizeModeOr                          0xaaa8
#define GlintNormalizeModeOrTag                       0x0555
#define GlintNormalizeModeOrReg                            1
#define GlintNormalizeModeOrOff                       0x9aa8

#define GlintLightingModeAnd                          0xaab0
#define GlintLightingModeAndTag                       0x0556
#define GlintLightingModeAndReg                            1
#define GlintLightingModeAndOff                       0x9ab0

#define GlintLightingModeOr                           0xaab8
#define GlintLightingModeOrTag                        0x0557
#define GlintLightingModeOrReg                             1
#define GlintLightingModeOrOff                        0x9ab8

#define GlintColorMaterialModeAnd                     0xaac0
#define GlintColorMaterialModeAndTag                  0x0558
#define GlintColorMaterialModeAndReg                       1
#define GlintColorMaterialModeAndOff                  0x9ac0

#define GlintColorMaterialModeOr                      0xaac8
#define GlintColorMaterialModeOrTag                   0x0559
#define GlintColorMaterialModeOrReg                        1
#define GlintColorMaterialModeOrOff                   0x9ac8

#define GlintDeltaModeAnd                             0xaad0
#define GlintDeltaModeAndTag                          0x055a
#define GlintDeltaModeAndReg                               1
#define GlintDeltaModeAndOff                          0x9ad0

#define GlintDeltaModeOr                              0xaad8
#define GlintDeltaModeOrTag                           0x055b
#define GlintDeltaModeOrReg                                1
#define GlintDeltaModeOrOff                           0x9ad8

#define GlintPointModeAnd                             0xaae0
#define GlintPointModeAndTag                          0x055c
#define GlintPointModeAndReg                               1
#define GlintPointModeAndOff                          0x9ae0

#define GlintPointModeOr                              0xaae8
#define GlintPointModeOrTag                           0x055d
#define GlintPointModeOrReg                                1
#define GlintPointModeOrOff                           0x9ae8

#define GlintLineModeAnd                              0xaaf0
#define GlintLineModeAndTag                           0x055e
#define GlintLineModeAndReg                                1
#define GlintLineModeAndOff                           0x9af0

#define GlintLineModeOr                               0xaaf8
#define GlintLineModeOrTag                            0x055f
#define GlintLineModeOrReg                                 1
#define GlintLineModeOrOff                            0x9af8

#define GlintTriangleModeAnd                          0xab00
#define GlintTriangleModeAndTag                       0x0560
#define GlintTriangleModeAndReg                            1
#define GlintTriangleModeAndOff                       0x9b00

#define GlintTriangleModeOr                           0xab08
#define GlintTriangleModeOrTag                        0x0561
#define GlintTriangleModeOrReg                             1
#define GlintTriangleModeOrOff                        0x9b08

#define GlintMaterialModeAnd                          0xab10
#define GlintMaterialModeAndTag                       0x0562
#define GlintMaterialModeAndReg                            1
#define GlintMaterialModeAndOff                       0x9b10

#define GlintMaterialModeOr                           0xab18
#define GlintMaterialModeOrTag                        0x0563
#define GlintMaterialModeOrReg                             1
#define GlintMaterialModeOrOff                        0x9b18

#define GlintWindowAnd                                0xab80
#define GlintWindowAndTag                             0x0570
#define GlintWindowAndReg                                  1
#define GlintWindowAndOff                             0x9b80

#define GlintWindowOr                                 0xab88
#define GlintWindowOrTag                              0x0571
#define GlintWindowOrReg                                   1
#define GlintWindowOrOff                              0x9b88

#define GlintLBReadModeAnd                            0xab90
#define GlintLBReadModeAndTag                         0x0572
#define GlintLBReadModeAndReg                              1
#define GlintLBReadModeAndOff                         0x9b90

#define GlintLBReadModeOr                             0xab98
#define GlintLBReadModeOrTag                          0x0573
#define GlintLBReadModeOrReg                               1
#define GlintLBReadModeOrOff                          0x9b98
#endif
