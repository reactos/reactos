/// Define the A vector polynomial bits
// Each bit corresponds to one of the terms in the polynomial
//
// Rop(D,S,P) = a + a D + a S + a P + a  DS + a  DP + a  SP + a   DSP
//               0   d     s     p     ds      dp      sp      dsp

#define AVEC_NOT    0x01
#define AVEC_D      0x02
#define AVEC_S      0x04
#define AVEC_P      0x08
#define AVEC_DS     0x10
#define AVEC_DP     0x20
#define AVEC_SP     0x40
#define AVEC_DSP    0x80

#define AVEC_NEED_SOURCE  (AVEC_S | AVEC_DS | AVEC_SP | AVEC_DSP)
#define AVEC_NEED_PATTERN (AVEC_P | AVEC_DP | AVEC_SP | AVEC_DSP)

#define BB_TARGET_SCREEN 0x0001
#define BB_TARGET_ONLY   0x0002
#define BB_SOURCE_COPY   0x0004
#define BB_PATTERN_COPY  0x0008
