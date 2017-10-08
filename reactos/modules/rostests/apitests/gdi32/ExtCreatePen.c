/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPL - See COPYING in the top level directory
 * PURPOSE:         Test for ExtCreatePen
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <apitest.h>

#include <wingdi.h>
#include <winddi.h>
#include <include/ntgdityp.h>
#include <include/ntgdihdl.h>
#include <stdio.h>

#define ok_lasterror(err) \
    ok(GetLastError() == err, "expected last error " #err " but got 0x%lx\n", GetLastError());

#define ok_elp(hPen, elp, pstyle, width, bstyle, color, hatch, cstyle) \
    ok(GetObjectA(hPen, sizeof(elpBuffer), elp) != 0, "GetObject failed\n"); \
    ok((elp)->elpPenStyle == (pstyle), "Wrong elpPenStyle, expected 0x%lx, got 0x%lx\n", (DWORD)pstyle, (elp)->elpPenStyle); \
    ok((elp)->elpWidth == width, "Wrong elpWidth, expected %lu, got %lu\n", (DWORD)width, (elp)->elpWidth); \
    ok((elp)->elpBrushStyle == (bstyle), "Wrong elpBrushStyle, expected 0x%x, got 0x%x\n", bstyle, (elp)->elpBrushStyle); \
    ok((elp)->elpColor == color, "Wrong elpColor, expected 0x%lx, got 0x%lx\n", (COLORREF)color, (elp)->elpColor); \
    ok((elp)->elpHatch == hatch, "Wrong elpHatch, expected 0x%p, got 0x%p\n", (PVOID)hatch, (PVOID)(elp)->elpHatch); \
    ok((elp)->elpNumEntries == cstyle, "Wrong elpNumEntries, expected %lu got %lu\n", (DWORD)cstyle, (elp)->elpNumEntries);

void Test_ExtCreatePen_Params()
{
    HPEN hPen;
    LOGBRUSH logbrush;
    struct
    {
        EXTLOGPEN extlogpen;
        ULONG styles[16];
    } elpBuffer;
    PEXTLOGPEN pelp = &elpBuffer.extlogpen;

    DWORD adwStyles[17] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

    /* Test NULL logbrush */
    SetLastError(0);
    _SEH2_TRY
    {
        hPen = ExtCreatePen(PS_COSMETIC, 1, NULL, 0, 0);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(0xdeadc0de);
    }
    _SEH2_END;
    ok_lasterror(0xdeadc0de);

    logbrush.lbStyle = BS_SOLID;
    logbrush.lbColor = RGB(1, 2, 3);
    logbrush.lbHatch = 0;
    hPen = ExtCreatePen(PS_COSMETIC, 1, &logbrush, 0, 0);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_elp(hPen, pelp, PS_COSMETIC, 1, BS_SOLID, RGB(1,2,3), 0, 0);

    /* Test if we have an EXTPEN */
    ok(GDI_HANDLE_GET_TYPE(hPen) == GDI_OBJECT_TYPE_EXTPEN, "hPen=%p\n", hPen);
    DeleteObject(hPen);

    /* Test invalid style mask (0x0F) */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(9, 1, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_ALTERNATE with PS_GEOMETRIC */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_ALTERNATE | PS_GEOMETRIC, 1, &logbrush, 0, NULL);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test invalid endcap mask (0xF00) */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(0x300, 1, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test invalid join mask (F000) */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(0x3000, 1, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test invalid type mask (F0000) */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(0x20000, 1, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_COSMETIC with dwWidth != 1 */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_COSMETIC, -1, &logbrush, 0, 0);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_COSMETIC, 1, BS_SOLID, RGB(1,2,3), 0, 0);
    DeleteObject(hPen);
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_COSMETIC, 2, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_COSMETIC, 0, &logbrush, 0, 0);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_COSMETIC with PS_ENDCAP_SQUARE */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_COSMETIC | PS_ENDCAP_SQUARE, 1, &logbrush, 0, NULL);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_COSMETIC | PS_ENDCAP_SQUARE, 1, BS_SOLID, RGB(1,2,3), 0, 0);
    DeleteObject(hPen);

    /* Test styles without PS_USERSTYLE */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC, 1, &logbrush, 16, adwStyles);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC, 1, &logbrush, 0, adwStyles);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC, 1, &logbrush, 16, NULL);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_USERSTYLE */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 16, adwStyles);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_GEOMETRIC | PS_USERSTYLE, 5, BS_SOLID, RGB(1,2,3), 0, 16);
    DeleteObject(hPen);

    /* Test PS_USERSTYLE with PS_COSMETIC */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_COSMETIC | PS_USERSTYLE, 5, &logbrush, 16, adwStyles);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_USERSTYLE with 17 styles */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 17, adwStyles);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_USERSTYLE with 1 style */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 5, &logbrush, 1, adwStyles);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_GEOMETRIC | PS_USERSTYLE, 5, BS_SOLID, RGB(1,2,3), 0, 1);
    DeleteObject(hPen);

    /* Test PS_USERSTYLE with NULL lpStyles */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC | PS_USERSTYLE, 1, &logbrush, 2, NULL);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test PS_NULL */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_NULL, 1, &logbrush, 0, NULL);
    ok(hPen == GetStockObject(NULL_PEN), "ExtCreatePen should return NULL_PEN, but returned %p\n", hPen);
    ok_lasterror(0xdeadc0de);

    /* When the size is anything other than sizeof(EXTLOGPEN), we will get a LOGPEN! */
    ok(GetObjectA(hPen, sizeof(EXTLOGPEN) + 1, pelp) == sizeof(LOGPEN), "GetObject failed\n");

    /* ACHTUNG: special handling, we want sizeof(EXTLOGPEN) and nothing else */
    ok(GetObjectA(hPen, sizeof(EXTLOGPEN), pelp) == sizeof(EXTLOGPEN), "GetObject failed\n");
    ok(pelp->elpPenStyle == PS_NULL, "Wrong elpPenStyle, expected PS_NULL, got 0x%lx\n", pelp->elpPenStyle);
    ok(pelp->elpWidth == 0, "Wrong elpWidth, expected 0, got %lu\n", pelp->elpWidth);
    ok(pelp->elpBrushStyle == BS_SOLID, "Wrong elpBrushStyle, expected BS_SOLID, got 0x%x\n", pelp->elpBrushStyle);
    ok(pelp->elpColor == 0, "Wrong elpColor, expected 0, got 0x%lx\n", pelp->elpColor);
    ok(pelp->elpHatch == 0, "Wrong elpHatch, expected 0, got 0x%p\n", (PVOID)pelp->elpColor);
    ok(pelp->elpNumEntries == 0, "Wrong elpNumEntries, expected %u got %lu\n", 0, pelp->elpNumEntries);

    /* Test PS_NULL with styles */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_NULL, 1, &logbrush, 1, adwStyles);
    ok(hPen == NULL, "ExtCreatePen should fail\n");
    ok_lasterror(ERROR_INVALID_PARAMETER);

    /* Test 0 width */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC, 0, &logbrush, 0, 0);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_GEOMETRIC, 0, BS_SOLID, RGB(1,2,3), 0, 0);
    DeleteObject(hPen);

    /* Test negative width */
    SetLastError(0xdeadc0de);
    hPen = ExtCreatePen(PS_GEOMETRIC, -7942, &logbrush, 0, 0);
    ok(hPen != NULL, "ExtCreatePen failed\n");
    ok_lasterror(0xdeadc0de);
    ok_elp(hPen, pelp, PS_GEOMETRIC, 7942, BS_SOLID, RGB(1,2,3), 0, 0);
    DeleteObject(hPen);

}

BOOL
Test_ExtCreatePen_Expect(
    DWORD dwPenStyle,
    DWORD dwWidth,
    DWORD dwStyleCount,
    PDWORD pdwStyles,
    UINT lbStyle,
    ULONG_PTR lbHatch,
    PBOOL pbExpectException,
    PEXTLOGPEN pelpExpect)
{
    *pbExpectException = FALSE;

    if ((dwPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if (pdwStyles == NULL)
        {
            return FALSE;
        }
    }
    else
    {
        if ((dwStyleCount != 0) || (pdwStyles != NULL))
        {
            return FALSE;
        }
    }

    if (lbStyle == BS_PATTERN)
    {
        if (lbHatch == 0) return FALSE;
    }

    if (lbStyle == BS_DIBPATTERNPT)
    {
        if (lbHatch == 0) return FALSE;
        if (lbHatch < 0xFFFF)
        {
            *pbExpectException = TRUE;
            return FALSE;
        }
    }

    if (lbStyle == BS_DIBPATTERN)
    {
        return FALSE;
    }

    if ((dwPenStyle & PS_STYLE_MASK) == PS_USERSTYLE)
    {
        if (dwStyleCount == 0)
        {
            return FALSE;
        }

        if (dwStyleCount > 16)
        {
            return FALSE;
        }

        if ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC)
        {
            if (pdwStyles[0] == 0)
            {
                return FALSE;
            }
        }
        else
        {
            if ((pdwStyles[0] == 0) && (dwStyleCount == 1))
            {
                return FALSE;
            }
        }
    }

    if ((dwPenStyle & PS_STYLE_MASK) == PS_NULL)
    {
        pelpExpect->elpPenStyle = PS_NULL;
        pelpExpect->elpWidth = 0;
        pelpExpect->elpBrushStyle = BS_SOLID;
        pelpExpect->elpColor = 0;
        pelpExpect->elpHatch = 0;
        pelpExpect->elpNumEntries = 0;
        return TRUE;
    }


    if (((dwPenStyle & PS_STYLE_MASK) >> 0) > PS_ALTERNATE) return FALSE;
    if (((dwPenStyle & PS_ENDCAP_MASK) >> 8) > 2) return FALSE;
    if (((dwPenStyle & PS_JOIN_MASK) >> 12) > 2) return FALSE;
    if (((dwPenStyle & PS_TYPE_MASK) >> 16) > 1) return FALSE;

    dwWidth = abs(((LONG)dwWidth));

    if ((dwPenStyle & PS_TYPE_MASK) == PS_COSMETIC)
    {
        if (dwWidth != 1) return FALSE;

        if ((lbStyle != BS_SOLID) &&
            (lbStyle != BS_HATCHED))
        {
            return FALSE;
        }

        if (lbStyle == BS_HATCHED)
        {
            if ((lbHatch != 8) &&
                (lbHatch != 10) &&
                (lbHatch != 12))
            {
                return FALSE;
            }

            if (lbHatch >= HS_API_MAX)
            {
                return FALSE;
            }
        }

        if ((dwPenStyle & PS_STYLE_MASK) == PS_INSIDEFRAME)
        {
            return FALSE;
        }
    }
    else
    {
        if ((dwPenStyle & PS_STYLE_MASK) == PS_ALTERNATE)
        {
            return FALSE;
        }

        if (((dwPenStyle & PS_STYLE_MASK) != PS_SOLID) &&
            ((dwPenStyle & PS_STYLE_MASK) != PS_INSIDEFRAME) &&
            ((dwPenStyle & PS_STYLE_MASK) != PS_USERSTYLE))
        {
            if (dwWidth == 0)
            {
                return FALSE;
            }
        }

        if (lbStyle == BS_NULL)
        {
            pelpExpect->elpPenStyle = PS_NULL;
            pelpExpect->elpWidth = 0;
            pelpExpect->elpBrushStyle = BS_SOLID;
            pelpExpect->elpColor = 0;
            pelpExpect->elpHatch = 0;
            pelpExpect->elpNumEntries = 0;
            return TRUE;
        }

        if (lbStyle > BS_HATCHED)
        {
            return FALSE;
        }

        if (lbStyle == BS_HATCHED)
        {
            if (lbHatch >= HS_API_MAX)
            {
                return FALSE;
            }
        }

    }

    pelpExpect->elpPenStyle = dwPenStyle;
    pelpExpect->elpWidth = dwWidth;
    pelpExpect->elpBrushStyle = lbStyle;
    pelpExpect->elpColor = RGB(1,2,3);
    pelpExpect->elpHatch = lbHatch;
    pelpExpect->elpNumEntries = dwStyleCount;
    //pelpExpect->elpStyleEntry[1];

    return TRUE;
}

void
Test_ExtCreatePen_Helper(
    DWORD dwPenStyle,
    DWORD dwWidth,
    DWORD dwStyleCount,
    PDWORD pdwStyles,
    UINT lbStyle,
    ULONG_PTR lbHatch)
{
    LOGBRUSH lb;
    HPEN hpen;
    BOOL bExpectSuccess, bExpectException, bGotException = FALSE;
    struct
    {
        EXTLOGPEN extlogpen;
        ULONG styles[16];
    } elpBuffer;
    PEXTLOGPEN pelp = &elpBuffer.extlogpen;
    EXTLOGPEN elpExpect;

    lb.lbStyle = lbStyle;
    lb.lbColor = RGB(1,2,3);
    lb.lbHatch = lbHatch;

    bExpectSuccess = Test_ExtCreatePen_Expect(
                             dwPenStyle,
                             dwWidth,
                             dwStyleCount,
                             pdwStyles,
                             lbStyle,
                             lbHatch,
                             &bExpectException,
                             &elpExpect);

    _SEH2_TRY
    {
        hpen = ExtCreatePen(dwPenStyle, dwWidth, &lb, dwStyleCount, pdwStyles);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bGotException = TRUE;
    }
    _SEH2_END;

#define ok2(expression, text, expected, got) \
    ok(expression, text \
       "(dwPenStyle=0x%lx, dwWidth=%lu, dwStyleCount=%lu, pdwStyles=%p, lbStyle=%u, lbHatch=%p)\n", \
       expected, got, dwPenStyle, dwWidth, dwStyleCount, pdwStyles, lbStyle, (PVOID)lbHatch);

    //ok(bGotException == bExpectException, "ExtCreatePen expected exception=%lu for "
    //   "dwPenStyle=0x%lx, dwWidth=%lu, dwStyleCount=%lu, pdwStyles=%p, lbStyle=%lu, lbHatch=%p\n",
    //   bExpectException, dwPenStyle, dwWidth, dwStyleCount, pdwStyles, lbStyle, (PVOID)lbHatch);

    ok2(bGotException == bExpectException, "ExtCreatePen expception, expected %u, got %u", bExpectException, bGotException);

    if (!bExpectSuccess)
    {
        ok(hpen == NULL, "ExtCreatePen should fail for "
           "dwPenStyle=0x%lx, dwWidth=%lu, dwStyleCount=%lu, pdwStyles=%p, lbStyle=%u, lbHatch=%p\n",
           dwPenStyle, dwWidth, dwStyleCount, pdwStyles, lbStyle, (PVOID)lbHatch);
    }
    else
    {
        ok(hpen != NULL, "ExtCreatePen failed for "
           "dwPenStyle=0x%lx, dwWidth=%lu, dwStyleCount=%lu, pdwStyles=%p, lbStyle=%u, lbHatch=%p\n",
           dwPenStyle, dwWidth, dwStyleCount, pdwStyles, lbStyle, (PVOID)lbHatch);
        if (hpen != NULL)
        {
            if (GetObjectA(hpen, sizeof(elpBuffer), pelp) < sizeof(EXTLOGPEN))
            {
                if (!GetObjectA(hpen, sizeof(EXTLOGPEN), pelp))
                {
                    ok(0, "failed again?\n");
                    return;
                }
            }

            ok2(pelp->elpPenStyle == elpExpect.elpPenStyle, "elpPenStyle, expected 0x%lx, got 0x%lx\n", elpExpect.elpPenStyle, pelp->elpPenStyle);
            ok2(pelp->elpWidth == elpExpect.elpWidth, "elpWidth, expected 0x%lx, got 0x%lx\n", elpExpect.elpWidth, pelp->elpWidth);
            ok2(pelp->elpBrushStyle == elpExpect.elpBrushStyle, "elpBrushStyle, expected 0x%x, got 0x%x\n", elpExpect.elpBrushStyle, pelp->elpBrushStyle);
            ok2(pelp->elpColor == elpExpect.elpColor, "elpColor, expected 0x%lx, got 0x%lx\n", elpExpect.elpColor, pelp->elpColor);
            ok2(pelp->elpHatch == elpExpect.elpHatch, "elpHatch, expected 0x%lx, got 0x%lx\n", elpExpect.elpHatch, pelp->elpHatch);
            ok2(pelp->elpNumEntries == elpExpect.elpNumEntries, "elpNumEntries, expected 0x%lx, got 0x%lx\n", elpExpect.elpNumEntries, pelp->elpNumEntries);
            //for (i = 0; i < pelp->elpNumEntries; i++)
            //{
            //    ok2(pelp->elpStyleEntry[i] == elpExpect.elpStyleEntry[i], "elpHatch, expected 0x%lx, got 0x%lx\n", elpExpect.elpStyleEntry[i], pelp->elpStyleEntry[i]);
            //}
        }
    }

}

void Test_ExtCreatePen_Params2()
{
    ULONG aflPenType[] = {PS_COSMETIC, PS_GEOMETRIC, 0x20000};
    ULONG iType, iStyle, iEndCap, iJoin, iWidth, iStyleCount, iStyles, iBrushStyle, iHatch;
    DWORD adwStyles[17] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};
    DWORD adwStyles2[17] = {0,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

    printf("adwStyles=%p, adwStyles2=%p\n", adwStyles, adwStyles2);

    //for (iType = 0; iType < sizeof(aflPenType) / sizeof(aflPenType[0]); iType++)
    for (iType = 0; iType < 3; iType++)
    {
        ULONG aflPenStyle[] = {PS_SOLID, PS_DASH, PS_DOT, PS_DASHDOT, PS_DASHDOTDOT, PS_NULL, PS_INSIDEFRAME, PS_USERSTYLE, PS_ALTERNATE, 9};
        //for (iStyle = 0; iStyle < sizeof(aflPenStyle) / sizeof(aflPenStyle[0]); iStyle++)
        for (iStyle = 0; iStyle < 10; iStyle++)
        {
            ULONG aflEndCap[] = {PS_ENDCAP_ROUND, PS_ENDCAP_SQUARE, PS_ENDCAP_FLAT, 0x300, 0x400};
            for (iEndCap = 0; iEndCap < sizeof(aflEndCap) / sizeof(aflEndCap[0]); iEndCap++)
            {
                ULONG aflJoin[] = {PS_JOIN_ROUND, PS_JOIN_BEVEL, PS_JOIN_MITER, 0x3000, 0x4000};
                for (iJoin = 0; iJoin < sizeof(aflJoin) / sizeof(aflJoin[0]); iJoin++)
                {
                    DWORD adwWidth[] = {0, 1, 2};
                    ULONG flPenStyle = aflPenType[iType] | aflPenStyle[iStyle] | aflEndCap[iEndCap] | aflJoin[iJoin];

                    for (iWidth = 0; iWidth < sizeof(adwWidth) / sizeof(adwWidth[0]); iWidth++)
                    {
                        ULONG adwStyleCount[] = {0, 1, 2, 16, 17};
                        for (iStyleCount = 0; iStyleCount < sizeof(adwStyleCount) / sizeof(adwStyleCount[0]); iStyleCount++)
                        {
                            PULONG apdwStyles[] = {NULL, adwStyles, adwStyles2};
                            for (iStyles = 0; iStyles < sizeof(apdwStyles) / sizeof(apdwStyles[0]); iStyles++)
                            {
                                UINT albStyle[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
                                for (iBrushStyle = 0; iBrushStyle < sizeof(albStyle) / sizeof(albStyle[0]); iBrushStyle++)
                                {
                                    ULONG_PTR alHatch[] = {0, 1, 6, 7, 8, 9, 10, 11, 12, 13};

                                    for (iHatch = 0; iHatch < sizeof(alHatch) / sizeof(alHatch[0]); iHatch++)
                                    {
                                        Test_ExtCreatePen_Helper(flPenStyle,
                                                                 adwWidth[iWidth],
                                                                 adwStyleCount[iStyleCount],
                                                                 apdwStyles[iStyles],
                                                                 albStyle[iBrushStyle],
                                                                 alHatch[iHatch]);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

}

START_TEST(ExtCreatePen)
{
    Test_ExtCreatePen_Params();
    //Test_ExtCreatePen_Params2();
}

