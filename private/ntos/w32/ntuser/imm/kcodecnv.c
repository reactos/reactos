
/**************************************************************************\
* Module Name: kcodecnv.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the code for the Korean code conversion functions.
*
* History:
* 15-Jul-1995
* 22-Feb-1996  bklee
\**************************************************************************/
#include "precomp.h"
#pragma hdrstop

#define IDD_2BEOL    100
#define IDD_3BEOL1   101
#define IDD_3BEOL2   102

#define lpSource(lpks) (LPSTR)((LPSTR)lpks+lpks->dchSource)
#define lpDest(lpks)   (LPSTR)((LPSTR)lpks+lpks->dchDest)

#define JOHAB_CP   1361
#define WANSUNG_CP 949
#define TWO_BYTE   2
#define ONE_WORD   1

typedef struct tagHIGH_LOW              // For high byte and low byte
{
    BYTE    low, high;
}   HIGH_LOW;

typedef union tagWANSUNG                // For Wansung character code
{
    HIGH_LOW    e;
    WORD        w;
}   WANSUNG;

/* Hanguel Mnemonic Table for 2 BeolSik and 3 BeolSik */
CONST WORD HMNTable[3][96] =
{
// For 2 Beolsik.
    {
    /*  20  SP  */  0xA1A1,
    /*  21  !   */  0xA3A1,
    /*  22  "   */  0xA1A8,
    /*  23  #   */  0xA3A3,
    /*  24  $   */  0xA3A4,
    /*  25  %   */  0xA3A5,
    /*  26  &   */  0xA3A6,
    /*  27  `   */  0xA1AE,     /* A1AE ? AiA2 */
    /*  28  (   */  0xA3A8,
    /*  29  )   */  0xA3A9,
    /*  2A  *   */  0xA3AA,
    /*  2B  +   */  0xA3AB,
    /*  2C  '   */  0xA3A7,
    /*  2D  -   */  0xA3AD,
    /*  2E  .   */  0xA3AE,
    /*  2F  /   */  0xA3AF,
    /*  30  0   */  0xA3B0,
    /*  31  1   */  0xA3B1,
    /*  32  2   */  0xA3B2,
    /*  33  3   */  0xA3B3,
    /*  34  4   */  0xA3B4,
    /*  35  5   */  0xA3B5,
    /*  36  6   */  0xA3B6,
    /*  37  7   */  0xA3B7,
    /*  38  8   */  0xA3B8,
    /*  39  9   */  0xA3B9,
    /*  3A  :   */  0xA3BA,
    /*  3B  ;   */  0xA3BB,
    /*  3C  <   */  0xA3BC,
    /*  3D  =   */  0xA3BD,
    /*  3E  >   */  0xA3BE,
    /*  3F  ?   */  0xA3BF,
    /*  40  @   */  0xA3C0,
    /*  41  A   */  0xA4B1,
    /*  42  B   */  0xA4D0,
    /*  43  C   */  0xA4BA,
    /*  44  D   */  0xA4B7,
    /*  45  E   */  0xA4A8,
    /*  46  F   */  0xA4A9,
    /*  47  G   */  0xA4BE,
    /*  48  H   */  0xA4C7,
    /*  49  I   */  0xA4C1,
    /*  4A  J   */  0xA4C3,
    /*  4B  K   */  0xA4BF,
    /*  4C  L   */  0xA4D3,
    /*  4D  M   */  0xA4D1,
    /*  4E  N   */  0xA4CC,
    /*  4F  O   */  0xA4C2,
    /*  50  P   */  0xA4C6,
    /*  51  Q   */  0xA4B3,
    /*  52  R   */  0xA4A2,
    /*  53  S   */  0xA4A4,
    /*  54  T   */  0xA4B6,
    /*  55  U   */  0xA4C5,
    /*  56  V   */  0xA4BD,
    /*  57  W   */  0xA4B9,
    /*  58  X   */  0xA4BC,
    /*  59  Y   */  0xA4CB,
    /*  5A  Z   */  0xA4BB,
    /*  5B  [   */  0xA3DB,
    /*  5C  \   */  0xA1AC,
    /*  5D  ]   */  0xA3DD,
    /*  5E  ^   */  0xA3DE,
    /*  5F  _   */  0xA3DF,
    /*  60  `   */  0xA1A2,     /* A1AE ? AiA2 */
    /*  61  a   */  0xA4B1,
    /*  62  b   */  0xA4D0,
    /*  63  c   */  0xA4BA,
    /*  64  d   */  0xA4B7,
    /*  65  e   */  0xA4A7,
    /*  66  f   */  0xA4A9,
    /*  67  g   */  0xA4BE,
    /*  68  h   */  0xA4C7,
    /*  69  i   */  0xA4C1,
    /*  6A  j   */  0xA4C3,
    /*  6B  k   */  0xA4BF,
    /*  6C  l   */  0xA4D3,
    /*  6D  m   */  0xA4D1,
    /*  6E  n   */  0xA4CC,
    /*  6F  o   */  0xA4C0,
    /*  70  p   */  0xA4C4,
    /*  71  q   */  0xA4B2,
    /*  72  r   */  0xA4A1,
    /*  73  s   */  0xA4A4,
    /*  74  t   */  0xA4B5,
    /*  75  u   */  0xA4C5,
    /*  76  v   */  0xA4BD,
    /*  77  w   */  0xA4B8,
    /*  78  x   */  0xA4BC,
    /*  79  y   */  0xA4CB,
    /*  7A  z   */  0xA4BB,
    /*  7B  {   */  0xA3FB,
    /*  7C  |   */  0xA3FC,
    /*  7D  }   */  0xA3FD,
    /*  7E  ~   */  0xA1AD,
                    0x0000
    },
// For KT390.
    {
    /*  Hex Code    KSC Code */
    /*  20  SP  */  0xA1A1,
    /*  21  !   */  0xA4B8,
    /*  22  "   */  0xA1A8,
    /*  23  #   */  0xA3A3,
    /*  24  $   */  0xA3A4,
    /*  25  %   */  0xA3A5,
    /*  26  &   */  0xA3A6,
    /*  27  `   */  0xA1AE,
    /*  28  (   */  0xA3A8,
    /*  29  )   */  0xA3A9,
    /*  2A  *   */  0xA3AA,
    /*  2B  +   */  0xA3AB,
    /*  2C  '   */  0xA4BC,
    /*  2D  -   */  0xA3AD,
    /*  2E  .   */  0xA3AE,
    /*  2F  /   */  0xA4C7,
    /*  30  0   */  0xA4BB,
    /*  31  1   */  0xA4BE,
    /*  32  2   */  0xA4B6,
    /*  33  3   */  0xA4B2,
    /*  34  4   */  0xA4CB,
    /*  35  5   */  0xA4D0,
    /*  36  6   */  0xA4C1,
    /*  37  7   */  0xA4C6,
    /*  38  8   */  0xA4D2,
    /*  39  9   */  0xA4CC,
    /*  3A  :   */  0xA3BA,
    /*  3B  ;   */  0xA4B2,
    /*  3C  <   */  0xA3B2,
    /*  3D  =   */  0xA3BD,
    /*  3E  >   */  0xA3B3,
    /*  3F  ?   */  0xA3BF,
    /*  40  @   */  0xA3C0,
    /*  41  A   */  0xA4A7,
    /*  42  B   */  0xA3A1,
    /*  43  C   */  0xA4AB,
    /*  44  D   */  0xA4AA,
    /*  45  E   */  0xA4BB,
    /*  46  F   */  0xA4A2,
    /*  47  G   */  0xA3AF,
    /*  48  H   */  0xA1AF,
    /*  49  I   */  0xA3B8,
    /*  4A  J   */  0xA3B4,
    /*  4B  K   */  0xA3B5,
    /*  4C  L   */  0xA3B6,
    /*  4D  M   */  0xA3B1,
    /*  4E  N   */  0xA3B0,
    /*  4F  O   */  0xA3B9,
    /*  50  P   */  0xA3BE,
    /*  51  Q   */  0xA4BD,
    /*  52  R   */  0xA4C2,
    /*  53  S   */  0xA4A6,
    /*  54  T   */  0xA4C3,
    /*  55  U   */  0xA3B7,
    /*  56  V   */  0xA4B0,
    /*  57  W   */  0xA4BC,
    /*  58  X   */  0xA4B4,
    /*  59  Y   */  0xA3BC,
    /*  5A  Z   */  0xA4BA,
    /*  5B  [   */  0xA3DB,
    /*  5C  \   */  0xA3DC,
    /*  5D  ]   */  0xA3DD,
    /*  5E  ^   */  0xA3DE,
    /*  5F  _   */  0xA3DF,
    /*  60  `   */  0xA1AE,
    /*  61  a   */  0xA4B7,
    /*  62  b   */  0xA4CC,
    /*  63  c   */  0xA4C4,
    /*  64  d   */  0xA4D3,
    /*  65  e   */  0xA4C5,
    /*  66  f   */  0xA4BF,
    /*  67  g   */  0xA4D1,
    /*  68  h   */  0xA4A4,
    /*  69  i   */  0xA4B1,
    /*  6A  j   */  0xA4B7,
    /*  6B  k   */  0xA4A1,
    /*  6C  l   */  0xA4B8,
    /*  6D  m   */  0xA4BE,
    /*  6E  n   */  0xA4B5,
    /*  6F  o   */  0xA4BA,
    /*  70  p   */  0xA4BD,
    /*  71  q   */  0xA4B5,
    /*  72  r   */  0xA4C0,
    /*  73  s   */  0xA4A4,
    /*  74  t   */  0xA4C3,
    /*  75  u   */  0xA4A7,
    /*  76  v   */  0xA4C7,
    /*  77  w   */  0xA4A9,
    /*  78  x   */  0xA4A1,
    /*  79  y   */  0xA4A9,
    /*  7A  z   */  0xA4B1,
    /*  7B  {   */  0xA3FB,
    /*  7C  |   */  0xA3FC,
    /*  7D  }   */  0xA3FD,
    /*  7E  ~   */  0xA1AD,
                    0x0000
    },
// For 3 Beolsik Final.
    {
    /*  Hex Code    KSC Code */
    /*  20  SP  */  0xA1A1,
    /*  21  !   */  0xA4A2,
    /*  22  "   */  0xA3AE,
    /*  23  #   */  0xA4B8,
    /*  24  $   */  0xA4AF,
    /*  25  %   */  0xA4AE,
    /*  26  &   */  0xA1B0,
    /*  27  `   */  0xA3AA,
    /*  28  (   */  0xA1A2,
    /*  29  )   */  0xA1AD,
    /*  2A  *   */  0xA1B1,
    /*  2B  +   */  0xA3AB,
    /*  2C  '   */  0xA4BC,
    /*  2D  -   */  0xA3A9,
    /*  2E  .   */  0xA3AE,
    /*  2F  /   */  0xA4C7,
    /*  30  0   */  0xA4BB,
    /*  31  1   */  0xA4BE,
    /*  32  2   */  0xA4B6,
    /*  33  3   */  0xA4B2,
    /*  34  4   */  0xA4CB,
    /*  35  5   */  0xA4D0,
    /*  36  6   */  0xA4C1,
    /*  37  7   */  0xA4C6,
    /*  38  8   */  0xA4D2,
    /*  39  9   */  0xA4CC,  //0x0000
    /*  3A  :   */  0xA3B4,
    /*  3B  ;   */  0xA4B2,
    /*  3C  <   */  0xA3A7,
    /*  3D  =   */  0xA1B5,
    /*  3E  >   */  0xA3AE,
    /*  3F  ?   */  0xA3A1,
    /*  40  @   */  0xA4AA,
    /*  41  A   */  0xA4A7,
    /*  42  B   */  0xA3BF,
    /*  43  C   */  0xA4BC,
    /*  44  D   */  0xA4AC,
    /*  45  E   */  0xA4A5,
    /*  46  F   */  0xA4AB,
    /*  47  G   */  0xA4C2,
    /*  48  H   */  0xA3B0,
    /*  49  I   */  0xA3B7,
    /*  4A  J   */  0xA3B1,
    /*  4B  K   */  0xA3B2,
    /*  4C  L   */  0xA3B3,
    /*  4D  M   */  0xA1A8,
    /*  4E  N   */  0xA3AD,
    /*  4F  O   */  0xA3B8,
    /*  50  P   */  0xA3B9,
    /*  51  Q   */  0xA4BD,
    /*  52  R   */  0xA4B0,
    /*  53  S   */  0xA4A6,
    /*  54  T   */  0xA4AD,
    /*  55  U   */  0xA3B6,
    /*  56  V   */  0xA4A3,
    /*  57  W   */  0xA4BC,
    /*  58  X   */  0xA4B4,
    /*  59  Y   */  0xA3B5,
    /*  5A  Z   */  0xA4BA,
    /*  5B  [   */  0xA3A8,
    /*  5C  \   */  0xA3BA,
    /*  5D  ]   */  0xA1B4,
    /*  5E  ^   */  0xA3BD,
    /*  5F  _   */  0xA3BB,
    /*  60  `   */  0xA3AA,
    /*  61  a   */  0xA4B7,
    /*  62  b   */  0xA4CC,
    /*  63  c   */  0xA4C4,
    /*  64  d   */  0xA4D3,
    /*  65  e   */  0xA4C5,
    /*  66  f   */  0xA4BF,
    /*  67  g   */  0xA4D1,
    /*  68  h   */  0xA4A4,
    /*  69  i   */  0xA4B1,
    /*  6A  j   */  0xA4B7,
    /*  6B  k   */  0xA4A1,
    /*  6C  l   */  0xA4B8,
    /*  6D  m   */  0xA4BE,
    /*  6E  n   */  0xA4B5,
    /*  6F  o   */  0xA4BA,
    /*  70  p   */  0xA4BD,
    /*  71  q   */  0xA4B5,
    /*  72  r   */  0xA4C0,
    /*  73  s   */  0xA4A4,
    /*  74  t   */  0xA4C3,
    /*  75  u   */  0xA4A7,
    /*  76  v   */  0xA4C7,
    /*  77  w   */  0xA4A9,
    /*  78  x   */  0xA4A1,
    /*  79  y   */  0xA4B1,
    /*  7A  z   */  0xA4B1,
    /*  7B  {   */  0xA3A5,
    /*  7C  |   */  0xA3CC,
    /*  7D  }   */  0xA3AF,
    /*  7E  ~   */  0xA1AD,
                    0x0000
    }
};

CONST WORD  wKSCompCode[51] =   // from 'GiYuk' to 'Yi'.
{
    0x8841,0x8C41,0x8444,0x9041,0x8446,0x8447,0x9441,0x9841,0x9C41,0x844A,
    0x844B,0x844C,0x844D,0x844E,0x844F,0x8450,0xA041,0xA441,0xA841,0x8454,
    0xAC41,0xB041,0xB441,0xB841,0xBC41,0xC041,0xC441,0xC841,0xCC41,0xD041,
    0x8461,0x8481,0x84A1,0x84C1,0x84E1,0x8541,0x8561,0x8581,0x85A1,0x85C1,
    0x85E1,0x8641,0x8661,0x8681,0x86A1,0x86C1,0x86E1,0x8741,0x8761,0x8781,
    0x87A1
};

CONST WORD  wKSCompCode2[30] =   // from 'GiYuk' to 'HiEut'.
{
    0x8442,0x8443,0x8444,0x8445,0x8446,0x8447,0x8448,0x9841,0x8449,0x844A,
    0x844B,0x844C,0x844D,0x844E,0x844F,0x8450,0x8451,0x8453,0xA841,0x8454,
    0x8455,0x8456,0x8457,0x8458,0xBC41,0x8459,0x845A,0x845B,0x845C,0x845D
};


WORD
JunjaToBanja(
    LPSTR lpSrc,
    LPSTR lpDest
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WANSUNG wsCode;
    WORD    wCount = 0;

    while (*lpSrc)
    {
        if ((BYTE)(*lpSrc) < (BYTE)0x80)
        {
            *lpDest++ = *lpSrc++;
            wCount++;
        }
        else
        {
            wsCode.e.high = *lpSrc++;
            wsCode.e.low = *lpSrc++;
            if (wsCode.w == 0xA1A1)
            {
                *lpDest++ = ' ';
                wCount++;
            }
            else if (wsCode.w >= 0xA3A1 && wsCode.w <= 0xA3FE)
            {
                *lpDest++ = wsCode.e.low - (BYTE)0x80;
                wCount++;
            }
            else
            {
                *lpDest++ = wsCode.e.high;
                *lpDest++ = wsCode.e.low;
                wCount += 2;
            }
        }
    }
    *lpDest = '\0';
    return (wCount);
}

WORD
BanjaToJunja(
    LPSTR lpSrc,
    LPSTR lpDest
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WORD    wCount = 0;

    while (*lpSrc)
    {
        if ((BYTE)(*lpSrc) < (BYTE)0x80)
        {
            if (*lpSrc++ == ' ')
            {
                *lpDest++ = (BYTE)0xA1;
                *lpDest++ = (BYTE)0xA1;
                wCount += 2;
            }
            else
            {
                *lpDest++ = (BYTE)0xA3;
                *lpDest++ = *(lpSrc - 1) + (BYTE)0x80;
                wCount += 2;
            }
        }
        else
        {
            *lpDest++ = *lpSrc++;
            *lpDest++ = *lpSrc++;
            wCount += 2;
        }
    }
    *lpDest = '\0';
    return (wCount);
}

WORD
JohabToKs(
    LPSTR lpSrc,
    LPSTR lpDest
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    WORD    wCount = 0;
#if defined(OLD_CONV)
    WANSUNG wsSCode, wsDCode;
    int     iHead = 0, iTail = 2349, iMid;
    BYTE    bCount;
#endif

    while (*lpSrc)
    {
        if ((BYTE)(*lpSrc) < (BYTE)0x80)
        {
            *lpDest++ = *lpSrc++;
            wCount++;
        }
        else
#if defined(OLD_CONV)
        {
            wsSCode.e.high = *lpSrc++;
            wsSCode.e.low = *lpSrc++;
            for (bCount = 0; bCount < 51 && wKSCompCode[bCount] != wsSCode.w; bCount++)
                ;
            wsDCode.w = (bCount == 51)? 0: bCount + 0xA4A1;
            if (wsDCode.w)
            {
                *lpDest++ = wsDCode.e.high;
                *lpDest++ = wsDCode.e.low;
                wCount += 2;
                continue;
            }
            for (bCount = 0; bCount < 30 && wKSCompCode2[bCount] != wsSCode.w; bCount++)
                ;
            wsDCode.w = (bCount == 30)? 0: bCount + 0xA4A1;
            if (wsDCode.w)
            {
                *lpDest++ = wsDCode.e.high;
                *lpDest++ = wsDCode.e.low;
                wCount += 2;
                continue;
            }
            while (iHead <= iTail && !wsDCode.w)
            {
                iMid = (iHead + iTail) / 2;
                if (wKSCharCode[iMid] > wsSCode.w)
                    iTail = iMid - 1;
                else if (wKSCharCode[iMid] < wsSCode.w)
                    iHead = iMid + 1;
                else
                    wsDCode.w = ((iMid / 94 + 0xB0) << 8) | (iMid % 94 + 0xA1);
            }
            if (wsDCode.w)
            {
                *lpDest++ = wsDCode.e.high;
                *lpDest++ = wsDCode.e.low;
                wCount += 2;
            }
            else
            {
                *lpDest++ = wsSCode.e.high;
                *lpDest++ = wsSCode.e.low;
                wCount += 2;
            }
        }
#else
        {
                // for simple implementation, converting one character by character
                // we have to change it string to string conversion.
                WCHAR wUni;
                CHAR  chTmp[2];

                chTmp[0] = *lpSrc++;
                chTmp[1] = *lpSrc++;

                MultiByteToWideChar(JOHAB_CP, MB_PRECOMPOSED, chTmp, TWO_BYTE, &wUni, ONE_WORD);

                WideCharToMultiByte(WANSUNG_CP, 0, &wUni, ONE_WORD, chTmp, TWO_BYTE, NULL, NULL);

                *lpDest++ = chTmp[0];
                *lpDest++ = chTmp[1];

                wCount += 2;
        }
#endif
    }
    *lpDest = '\0';
    return (wCount);
}

WORD
KsToJohab(
    LPSTR lpSrc,
    LPSTR lpDest
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
#if defined(OLD_CONV)
    WANSUNG wsSCode, wsDCode;
    WORD    wCount = 0, wLoc;
#else
    WORD    wCount = 0;
#endif

    while (*lpSrc)
    {
        if ((BYTE)(*lpSrc) < (BYTE)0x80)
        {
            *lpDest++ = *lpSrc++;
            wCount++;
        }
        else
#if defined(OLD_CONV)
        {
            wsSCode.e.high = *lpSrc++;
            wsSCode.e.low = *lpSrc++;
            if (wsSCode.w >= (WORD)0xA4A1 && wsSCode.w <= (WORD)0xA4D3)
            {
                wsDCode.w = wKSCompCode[wsSCode.w - 0xA4A1];
                *lpDest++ = wsDCode.e.high;
                *lpDest++ = wsDCode.e.low;
            }
            else if (wsSCode.w >= (WORD)0xB0A1 && wsSCode.w <= (WORD)0xC8FE
                    && wsSCode.e.low != (BYTE)0xFF)
            {
                wLoc = (wsSCode.e.high - 176) * 94;
                wLoc += wsSCode.e.low  - 161;
                wsDCode.w = wKSCharCode[wLoc];
                *lpDest++ = wsDCode.e.high;
                *lpDest++ = wsDCode.e.low;
            }
            else
            {
                *lpDest++ = wsSCode.e.high;
                *lpDest++ = wsSCode.e.low;
            }
            wCount += 2;
        }
#else
        {
            WCHAR wUni;
            CHAR  chTmp[2];

            chTmp[0] = *lpSrc++;
            chTmp[1] = *lpSrc++;

            MultiByteToWideChar(WANSUNG_CP, MB_PRECOMPOSED, chTmp, TWO_BYTE, &wUni, ONE_WORD);

            WideCharToMultiByte(JOHAB_CP, 0, &wUni, ONE_WORD, chTmp, TWO_BYTE, NULL, NULL);

            *lpDest++ = chTmp[0];
            *lpDest++ = chTmp[1];

            wCount += 2;
        }
#endif
    }
    *lpDest = '\0';
    return (wCount);
}

LRESULT
TransCodeConvert(
    HIMC hIMC,
    LPIMESTRUCT lpIme
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    UNREFERENCED_PARAMETER(hIMC);

    switch (lpIme->wParam)
    {
        case IME_JUNJAtoBANJA:
            lpIme->wCount = JunjaToBanja(lpSource(lpIme), lpDest(lpIme));
            break;

        case IME_BANJAtoJUNJA:
            lpIme->wCount = BanjaToJunja(lpSource(lpIme), lpDest(lpIme));
            break;

        case IME_JOHABtoKS:
            lpIme->wCount = JohabToKs(lpSource(lpIme), lpDest(lpIme));
            break;

        case IME_KStoJOHAB:
            lpIme->wCount = KsToJohab(lpSource(lpIme), lpDest(lpIme));
            break;

        default:
            lpIme->wCount = 0;
    }
    return (lpIme->wCount);
}

LRESULT TransConvertList( HIMC hImc, LPIMESTRUCT lpIme)
{
    LPSTR           lpSrc;
    LPSTR           lpDst;
    HGLOBAL         hCandList;
    LPCANDIDATELIST lpCandList;
    LPSTR           lpCandStr;
    UINT            i, uBufLen;
    LRESULT         lRet = 0;

    lpSrc = lpSource(lpIme);
    lpDst = lpDest(lpIme);
    uBufLen = ImmGetConversionListA(GetKeyboardLayout(0), hImc, (LPCSTR)lpSrc,
            NULL, 0, GCL_CONVERSION);
    if (uBufLen)
    {
        hCandList = GlobalAlloc(GHND, uBufLen);
        lpCandList = (LPCANDIDATELIST)GlobalLock(hCandList);
        lRet = ImmGetConversionListA(GetKeyboardLayout(0), hImc, (LPCSTR)lpSrc,
                lpCandList, uBufLen, GCL_CONVERSION);
        for (i = 0; i < lpCandList->dwCount; i++)
        {
            lpCandStr = (LPSTR)lpCandList + lpCandList->dwOffset[i];
            *lpDst++ = *lpCandStr++;
            *lpDst++ = *lpCandStr++;
        }
        *lpDst = '\0';
        lpIme->wCount = (WORD)lpCandList->dwCount * 2;
        GlobalUnlock(hCandList);
        GlobalFree(hCandList);
    }
    return (lRet);
}

LRESULT TransGetMNTable( HIMC hImc, LPIMESTRUCT lpIme)
{
    LPSTR   lpMnemonic;
    int     iCount, iCIM;

    UNREFERENCED_PARAMETER(hImc);

    lpMnemonic = (LPSTR)(lpIme->lParam1);
// BUGBUG: Will be changed to use Registry instead of WIN.INI
    iCIM = GetProfileInt(L"WANSUNG", L"InputMethod", IDD_2BEOL) - IDD_2BEOL;
    for (iCount = 0; iCount < 96; iCount++, lpMnemonic += 2)
        {
        *lpMnemonic = LOBYTE(HMNTable[iCIM][iCount]);
        *(lpMnemonic+1) = HIBYTE(HMNTable[iCIM][iCount]);
        }
    return TRUE;
}
