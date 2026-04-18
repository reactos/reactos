/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UEFI GOP-specific boot UI helpers
 * COPYRIGHT:   Copyright 2026 Ahmed ARIF (arif.ing@outlook.com)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <drivers/bootvid/bootvid.h>
#include "logo.h"
#include "resource.h"
#include "inbvgop.h"

extern BOOLEAN ShowProgressBar;

#ifndef BI_RGB
#define BI_RGB 0
#endif
#ifndef BI_RLE4
#define BI_RLE4 2
#endif
#define INBV_WORDMARK_SCALE         0.40f
#define INBV_WORDMARK_TOP_VPOS      0.08f
#define INBV_WORDMARK_BGRT_SAFE_TOP 0.35f
#define INBV_CENTERMARK_VPOS        0.625f

#define INBV_SPINNER_FRAME_MS         33
#define INBV_SPINNER_PERIOD_MS        4000
#define INBV_SPINNER_STEPS            (INBV_SPINNER_PERIOD_MS / INBV_SPINNER_FRAME_MS)
#define INBV_SPINNER_PERIOD_100NS     ((ULONGLONG)INBV_SPINNER_PERIOD_MS * 10000ULL)
#define INBV_SPINNER_RING_RADIUS      20
#define INBV_SPINNER_STROKE_HALF_Q8   640
#define INBV_SPINNER_BBOX_HALF        26
#define INBV_SPINNER_MAX_ROTATION_DEG 900.0f
#define INBV_SPINNER_MAX_TRIM         0.5f

#define INBV_PI                     3.14159265f

typedef struct _INBV_GOP_RECT
{
    ULONG X;
    ULONG Y;
    ULONG Width;
    ULONG Height;
} INBV_GOP_RECT, *PINBV_GOP_RECT;

#define INBV_SPINNER_BBOX_SIZE     (INBV_SPINNER_BBOX_HALF * 2)
#define INBV_SPINNER_FB_CAPACITY   (INBV_SPINNER_BBOX_SIZE * INBV_SPINNER_BBOX_SIZE)

typedef struct _INBV_SPINNER_STATE
{
    LONG  DestCX, DestCY;
    ULONG BBoxHalf;
    ULONG BBoxSize;

    ULONG RedMask, GreenMask, BlueMask;
    ULONG RedShift, GreenShift, BlueShift;
    ULONG RedMax, GreenMax, BlueMax;

    ULONG GrayLut[256];
    ULONGLONG StartTime;
} INBV_SPINNER_STATE;

static INBV_SPINNER_STATE g_Spinner;
static ULONG   g_SpinnerFrameBuf[INBV_SPINNER_FB_CAPACITY];
static KEVENT  g_SpinnerStop;
static KEVENT  g_SpinnerDone;
static BOOLEAN g_SpinnerReady = FALSE;

extern VOID NTAPI InbvAcquireLock(VOID);
extern VOID NTAPI InbvReleaseLock(VOID);

CODE_SEG("INIT")
static
BOOLEAN
InbvGopQueryInfo(
    _Out_ LOADER_PARAMETER_FRAMEBUFFER *FbInfo)
{
    if (!FbInfo)
        return FALSE;

    RtlZeroMemory(FbInfo, sizeof(*FbInfo));

    if (!InbvGetGopFrameBufferInfo(FbInfo))
        return FALSE;

    if (FbInfo->FrameBufferSize == 0 ||
        FbInfo->HorizontalResolution == 0 ||
        FbInfo->VerticalResolution == 0)
    {
        return FALSE;
    }

    return TRUE;
}

static ULONG InbvMaskShift(ULONG Mask)
{
    ULONG Shift = 0;
    if (!Mask) return 0;
    while ((Mask & 1) == 0) { Shift++; Mask >>= 1; }
    return Shift;
}

static ULONG InbvMaskMax(ULONG Mask)
{
    ULONG Value = 0;
    while (Mask) { Value = (Value << 1) | 1; Mask >>= 1; }
    return Value;
}

static ULONG InbvPackColor(ULONG RedMask, ULONG GreenMask, ULONG BlueMask,
                           ULONG RedShift, ULONG GreenShift, ULONG BlueShift,
                           ULONG RedMax, ULONG GreenMax, ULONG BlueMax,
                           UCHAR r, UCHAR g, UCHAR b)
{
    ULONG R = RedMask   ? ((ULONG)r * RedMax   + 127) / 255 : 0;
    ULONG G = GreenMask ? ((ULONG)g * GreenMax + 127) / 255 : 0;
    ULONG B = BlueMask  ? ((ULONG)b * BlueMax  + 127) / 255 : 0;
    return (RedMask   ? ((R << RedShift)   & RedMask)   : 0) |
           (GreenMask ? ((G << GreenShift) & GreenMask) : 0) |
           (BlueMask  ? ((B << BlueShift)  & BlueMask)  : 0);
}

static float
InbvApproxSin(float x)
{
    float sign = 1.0f, xpi;
    while (x < 0.0f) x += 2.0f * INBV_PI;
    while (x >= 2.0f * INBV_PI) x -= 2.0f * INBV_PI;
    if (x >= INBV_PI) { x -= INBV_PI; sign = -1.0f; }
    if (x > INBV_PI / 2.0f) x = INBV_PI - x;
    xpi = x * (INBV_PI - x);
    return sign * (16.0f * xpi) / (5.0f * INBV_PI * INBV_PI - 4.0f * xpi);
}

static ULONG
InbvIsqrt(ULONG n)
{
    ULONG x, y;
    if (n <= 1) return n;
    x = n;
    y = (x + 1) >> 1;
    while (y < x)
    {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

static
BOOLEAN
InbvGopComputeWordmarkRect(
    _In_ ULONG ScreenWidth,
    _In_ ULONG ScreenHeight,
    _Out_ PINBV_GOP_RECT Rect)
{
    PBITMAPINFOHEADER Header;
    ULONG SrcWidth, SrcHeight;
    ULONG DstWidth, DstHeight;

    if (!Rect)
        return FALSE;

    Header = (PBITMAPINFOHEADER)InbvGetResourceAddress(IDB_REACTOS_GOP_LOGO);
    if (!Header || Header->biPlanes != 1 || Header->biBitCount != 4)
        return FALSE;

    if (Header->biCompression != BI_RGB && Header->biCompression != BI_RLE4)
        return FALSE;

    SrcWidth = (ULONG)Header->biWidth;
    SrcHeight = (Header->biHeight < 0) ? (ULONG)(-Header->biHeight) : (ULONG)Header->biHeight;
    if (!SrcWidth || !SrcHeight)
        return FALSE;

    DstWidth = (ULONG)(SrcWidth * INBV_WORDMARK_SCALE + 0.5f);
    if (DstWidth == 0)
        DstWidth = 1;

    DstHeight = (ULONG)(SrcHeight * INBV_WORDMARK_SCALE + 0.5f);
    if (DstHeight == 0)
        DstHeight = 1;

    Rect->Width = DstWidth;
    Rect->Height = DstHeight;
    Rect->X = (ScreenWidth > DstWidth) ? ((ScreenWidth - DstWidth) / 2) : 0;

    {
        ULONG TopMargin = (ULONG)(ScreenHeight * INBV_WORDMARK_TOP_VPOS + 0.5f);
        ULONG SafeBottom = (ULONG)(ScreenHeight * INBV_WORDMARK_BGRT_SAFE_TOP);
        if (TopMargin + DstHeight > SafeBottom)
            TopMargin = (SafeBottom > DstHeight) ? (SafeBottom - DstHeight) : 0;
        if (TopMargin + DstHeight > ScreenHeight)
            TopMargin = (ScreenHeight > DstHeight) ? (ScreenHeight - DstHeight) : 0;
        Rect->Y = TopMargin;
    }
    return TRUE;
}

static float
InbvPiecewise(float t, const float *KeyTimes, const float *KeyVals, ULONG N)
{
    ULONG i;
    if (N == 0) return 0.0f;
    if (t <= KeyTimes[0]) return KeyVals[0];
    if (t >= KeyTimes[N - 1]) return KeyVals[N - 1];
    for (i = 1; i < N; i++)
    {
        if (t <= KeyTimes[i])
        {
            float t0 = KeyTimes[i - 1], t1 = KeyTimes[i];
            float v0 = KeyVals [i - 1], v1 = KeyVals [i];
            float span = t1 - t0;
            return (span > 0.0f) ? (v0 + (v1 - v0) * (t - t0) / span) : v1;
        }
    }
    return KeyVals[N - 1];
}

static VOID
InbvGopSpinnerResetState(VOID)
{
    RtlZeroMemory(&g_Spinner, sizeof(g_Spinner));
}

static VOID
InbvGopSpinnerBlitBuffer(
    _In_reads_(g_Spinner.BBoxSize * g_Spinner.BBoxSize) const ULONG *FrameBuffer)
{
    const INBV_SPINNER_STATE *S = &g_Spinner;
    const ULONG fullW = S->BBoxSize;
    const ULONG x0 = ((ULONG)S->DestCX > S->BBoxHalf) ?
                     ((ULONG)S->DestCX - S->BBoxHalf) : 0;
    const ULONG y0 = ((ULONG)S->DestCY > S->BBoxHalf) ?
                     ((ULONG)S->DestCY - S->BBoxHalf) : 0;

    if (!FrameBuffer || fullW == 0)
        return;

    InbvAcquireLock();
    VidBufferToScreenBlt((PUCHAR)FrameBuffer, x0, y0,
                         fullW, fullW, fullW * sizeof(ULONG));
    InbvReleaseLock();
}

static VOID
InbvGopSpinnerRasterizeFrame(
    _In_ float t,
    _Out_writes_(INBV_SPINNER_FB_CAPACITY) PULONG FrameBuffer)
{
    static const float RotationTimes[3] = { 0.0f, 0.5f, 1.0f };
    static const float RotationVals [3] = { 0.0f, 450.0f, INBV_SPINNER_MAX_ROTATION_DEG };
    static const float TrimEndTimes [3] = { 0.0f, 0.5f, 1.0f };
    static const float TrimEndVals  [3] = { 0.0f, INBV_SPINNER_MAX_TRIM, INBV_SPINNER_MAX_TRIM };
    static const float TrimStartTimes[3] = { 0.0f, 0.5f, 1.0f };
    static const float TrimStartVals [3] = { 0.0f, 0.0f, INBV_SPINNER_MAX_TRIM };

    const INBV_SPINNER_STATE *S = &g_Spinner;
    const ULONG fullW = S->BBoxSize;
    float rotation_deg = InbvPiecewise(t, RotationTimes, RotationVals, 3);
    float trim_end     = InbvPiecewise(t, TrimEndTimes,  TrimEndVals,  3);
    float trim_start   = InbvPiecewise(t, TrimStartTimes,TrimStartVals,3);

    float as_rad = (rotation_deg + 360.0f * trim_start) * (INBV_PI / 180.0f);
    float ae_rad = (rotation_deg + 360.0f * trim_end)   * (INBV_PI / 180.0f);

    float sin_start = InbvApproxSin(as_rad);
    float cos_start = InbvApproxSin(as_rad + INBV_PI / 2.0f);
    float sin_end   = InbvApproxSin(ae_rad);
    float cos_end   = InbvApproxSin(ae_rad + INBV_PI / 2.0f);

    float span_deg = (trim_end - trim_start) * 360.0f;
    BOOLEAN arc_visible = (span_deg > 0.0f);

    const float R_f = (float)INBV_SPINNER_RING_RADIUS;
    const LONG  cap1_cx_q8 = (LONG)(cos_start * R_f * 256.0f);
    const LONG  cap1_cy_q8 = (LONG)(sin_start * R_f * 256.0f);
    const LONG  cap2_cx_q8 = (LONG)(cos_end   * R_f * 256.0f);
    const LONG  cap2_cy_q8 = (LONG)(sin_end   * R_f * 256.0f);

    const LONG  sin_start_q8 = (LONG)(sin_start * 256.0f);
    const LONG  cos_start_q8 = (LONG)(cos_start * 256.0f);
    const LONG  sin_end_q8   = (LONG)(sin_end   * 256.0f);
    const LONG  cos_end_q8   = (LONG)(cos_end   * 256.0f);

    const LONG  ring_q8      = INBV_SPINNER_RING_RADIUS * 256;
    const LONG  stroke_q8    = INBV_SPINNER_STROKE_HALF_Q8;
    const LONG  stroke_in    = (stroke_q8 > 128) ? (stroke_q8 - 128) : 0;
    const LONG  stroke_out   = stroke_q8 + 128;

    const LONG  annulus_outer = ring_q8 + stroke_out;
    const LONG  annulus_inner = (ring_q8 > stroke_out) ? (ring_q8 - stroke_out) : 0;
    const LONGLONG annulus_outer_sq = (LONGLONG)annulus_outer * annulus_outer;
    const LONGLONG annulus_inner_sq = (LONGLONG)annulus_inner * annulus_inner;

    const LONGLONG cap_out_sq = (LONGLONG)stroke_out * stroke_out;
    const LONGLONG cap_in_sq  = (LONGLONG)stroke_in  * stroke_in;
    const LONGLONG cap_span   = (cap_out_sq > cap_in_sq) ? (cap_out_sq - cap_in_sq) : 1;
    LONG ry;

    if (!FrameBuffer || fullW == 0)
        return;

    RtlZeroMemory(FrameBuffer, (SIZE_T)fullW * fullW * sizeof(ULONG));

    for (ry = -(LONG)S->BBoxHalf; ry < (LONG)S->BBoxHalf; ry++)
    {
        PULONG Row   = FrameBuffer + (SIZE_T)(ry + (LONG)S->BBoxHalf) * fullW;
        LONG   py_q8 = (ry * 256) + 128;
        LONG   rx;

        for (rx = -(LONG)S->BBoxHalf; rx < (LONG)S->BBoxHalf; rx++)
        {
            LONG px_q8 = (rx * 256) + 128;
            LONGLONG d2 = (LONGLONG)px_q8 * px_q8 + (LONGLONG)py_q8 * py_q8;
            ULONG cov = 0;

            if (arc_visible && d2 < annulus_outer_sq && d2 > annulus_inner_sq)
            {
                LONGLONG cross_start =
                    (LONGLONG)cos_start_q8 * py_q8 - (LONGLONG)sin_start_q8 * px_q8;
                LONGLONG cross_end =
                    (LONGLONG)cos_end_q8   * py_q8 - (LONGLONG)sin_end_q8   * px_q8;

                if (cross_start >= 0 && cross_end <= 0)
                {
                    ULONG d_q8     = InbvIsqrt((ULONG)d2);
                    LONG  radial   = (LONG)d_q8 - ring_q8;
                    if (radial < 0) radial = -radial;

                    if (radial <= stroke_in)
                    {
                        cov = 255;
                    }
                    else if (radial < stroke_out)
                    {
                        ULONG frac = (ULONG)((stroke_out - radial) * 255) /
                                     (ULONG)(stroke_out - stroke_in);
                        cov = frac > 255 ? 255 : frac;
                    }
                }
            }

            {
                LONG dx = px_q8 - cap1_cx_q8;
                LONG dy = py_q8 - cap1_cy_q8;
                LONGLONG cd2 = (LONGLONG)dx * dx + (LONGLONG)dy * dy;
                if (cd2 < cap_out_sq)
                {
                    ULONG c;
                    if (cd2 <= cap_in_sq)
                        c = 255;
                    else
                    {
                        LONGLONG frac = ((cap_out_sq - cd2) * 255) / cap_span;
                        c = (frac < 0) ? 0 : (frac > 255) ? 255 : (ULONG)frac;
                    }
                    if (c > cov) cov = c;
                }
            }

            {
                LONG dx = px_q8 - cap2_cx_q8;
                LONG dy = py_q8 - cap2_cy_q8;
                LONGLONG cd2 = (LONGLONG)dx * dx + (LONGLONG)dy * dy;
                if (cd2 < cap_out_sq)
                {
                    ULONG c;
                    if (cd2 <= cap_in_sq)
                        c = 255;
                    else
                    {
                        LONGLONG frac = ((cap_out_sq - cd2) * 255) / cap_span;
                        c = (frac < 0) ? 0 : (frac > 255) ? 255 : (ULONG)frac;
                    }
                    if (c > cov) cov = c;
                }
            }

            if (cov > 0)
            {
                ULONG idx = (ULONG)(rx + (LONG)S->BBoxHalf);
                if (idx < fullW)
                    Row[idx] = S->GrayLut[cov];
            }
        }
    }
}

static VOID NTAPI
InbvGopSpinnerThread(PVOID Context)
{
    INBV_SPINNER_STATE *S = &g_Spinner;
    LARGE_INTEGER Delay;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Context);
    Delay.QuadPart = -(LONGLONG)(INBV_SPINNER_FRAME_MS * 10000LL);
    S->StartTime = KeQueryInterruptTime();

    while (TRUE)
    {
        ULONGLONG Elapsed100ns = KeQueryInterruptTime() - S->StartTime;
        ULONGLONG Phase100ns   = Elapsed100ns % INBV_SPINNER_PERIOD_100NS;
        float t = (float)((double)Phase100ns / (double)INBV_SPINNER_PERIOD_100NS);

        InbvGopSpinnerRasterizeFrame(t, g_SpinnerFrameBuf);
        InbvGopSpinnerBlitBuffer(g_SpinnerFrameBuf);

        Status = KeWaitForSingleObject(&g_SpinnerStop,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       &Delay);
        if (Status != STATUS_TIMEOUT)
            break;
    }

    RtlZeroMemory(g_SpinnerFrameBuf, sizeof(g_SpinnerFrameBuf));
    InbvGopSpinnerBlitBuffer(g_SpinnerFrameBuf);

    KeSetEvent(&g_SpinnerDone, IO_NO_INCREMENT, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

CODE_SEG("INIT")
static BOOLEAN
InbvGopSpinnerSetup(
    _In_ ULONG ScreenWidth,
    _In_ ULONG ScreenHeight)
{
    INBV_SPINNER_STATE *S = &g_Spinner;
    LOADER_PARAMETER_FRAMEBUFFER FbInfo;
    ULONG HalfHeight, DestY, i;

    RtlZeroMemory(S, sizeof(*S));

    S->BBoxHalf = INBV_SPINNER_BBOX_HALF;
    if (S->BBoxHalf == 0 ||
        (S->BBoxHalf * 2) > ScreenWidth ||
        (S->BBoxHalf * 2) > ScreenHeight)
    {
        return FALSE;
    }

    S->BBoxSize = S->BBoxHalf * 2;
    S->DestCX = (LONG)(ScreenWidth / 2);

    HalfHeight = ScreenHeight / 2;
    DestY = (ULONG)(HalfHeight +
                    (ScreenHeight - HalfHeight) * INBV_CENTERMARK_VPOS);
    if (DestY + S->BBoxHalf > ScreenHeight)
        DestY = ScreenHeight - S->BBoxHalf;
    if (DestY < S->BBoxHalf)
        DestY = S->BBoxHalf;
    S->DestCY = (LONG)DestY;

    if (!InbvGopQueryInfo(&FbInfo))
        return FALSE;

    S->RedMask    = FbInfo.RedMask;
    S->GreenMask  = FbInfo.GreenMask;
    S->BlueMask   = FbInfo.BlueMask;
    S->RedShift   = InbvMaskShift(S->RedMask);
    S->GreenShift = InbvMaskShift(S->GreenMask);
    S->BlueShift  = InbvMaskShift(S->BlueMask);
    S->RedMax     = InbvMaskMax(S->RedMask   >> S->RedShift);
    S->GreenMax   = InbvMaskMax(S->GreenMask >> S->GreenShift);
    S->BlueMax    = InbvMaskMax(S->BlueMask  >> S->BlueShift);

    for (i = 0; i < RTL_NUMBER_OF(S->GrayLut); ++i)
    {
        UCHAR v = (UCHAR)i;
        S->GrayLut[i] = InbvPackColor(S->RedMask, S->GreenMask, S->BlueMask,
                                      S->RedShift, S->GreenShift, S->BlueShift,
                                      S->RedMax, S->GreenMax, S->BlueMax,
                                      v, v, v);
    }

    return TRUE;
}

CODE_SEG("INIT")
static VOID
InbvGopSpinnerStart(
    _In_ ULONG ScreenWidth,
    _In_ ULONG ScreenHeight)
{
    HANDLE Thread;
    OBJECT_ATTRIBUTES ObjAttr;

    if (!InbvGopSpinnerSetup(ScreenWidth, ScreenHeight))
        return;

    KeInitializeEvent(&g_SpinnerStop, NotificationEvent, FALSE);
    KeInitializeEvent(&g_SpinnerDone, NotificationEvent, FALSE);
    g_SpinnerReady = TRUE;

    InitializeObjectAttributes(&ObjAttr, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    if (NT_SUCCESS(PsCreateSystemThread(&Thread, 0, &ObjAttr, NULL, NULL,
                                        InbvGopSpinnerThread, NULL)))
    {
        ZwClose(Thread);
    }
    else
    {
        InbvGopSpinnerResetState();
        g_SpinnerReady = FALSE;
    }
}

VOID
NTAPI
InbvGopSpinnerStop(VOID)
{
    if (!g_SpinnerReady)
        return;

    KeSetEvent(&g_SpinnerStop, IO_NO_INCREMENT, FALSE);
    KeWaitForSingleObject(&g_SpinnerDone, Executive, KernelMode, FALSE, NULL);
    InbvGopSpinnerResetState();
    g_SpinnerReady = FALSE;
}

static
BOOLEAN
InbvGopDrawWordmark(
    _In_ ULONG ScreenWidth,
    _In_ ULONG ScreenHeight,
    _In_opt_ const INBV_GOP_RECT *WordmarkRectOverride)
{
    PUCHAR Bitmap;
    PBITMAPINFOHEADER Header;
    ULONG SrcWidth, SrcHeight;
    INBV_GOP_RECT WordmarkRect;
    ULONG DstWidth, DstHeight, DestX, DestY;
    BOOLEAN TopDown = FALSE;

    LOADER_PARAMETER_FRAMEBUFFER FbInfo;
    ULONG RedMask = 0, GreenMask = 0, BlueMask = 0;
    ULONG RedShift = 0, GreenShift = 0, BlueShift = 0;
    ULONG RedMax = 0, GreenMax = 0, BlueMax = 0;

    Bitmap = InbvGetResourceAddress(IDB_REACTOS_GOP_LOGO);
    Header = (PBITMAPINFOHEADER)Bitmap;
    if (!Header || Header->biPlanes != 1 || Header->biBitCount != 4)
        return FALSE;

    if (Header->biCompression != BI_RGB && Header->biCompression != BI_RLE4)
        return FALSE;

    SrcWidth = (ULONG)Header->biWidth;
    SrcHeight = (Header->biHeight < 0) ? (ULONG)(-Header->biHeight) : (ULONG)Header->biHeight;
    TopDown = (Header->biHeight < 0);
    if (!SrcWidth || !SrcHeight)
        return FALSE;

    if (WordmarkRectOverride)
    {
        WordmarkRect = *WordmarkRectOverride;
    }
    else
    {
        if (!InbvGopComputeWordmarkRect(ScreenWidth, ScreenHeight, &WordmarkRect))
            return FALSE;
    }

    DstWidth = WordmarkRect.Width;
    DstHeight = WordmarkRect.Height;
    DestX = WordmarkRect.X;
    DestY = WordmarkRect.Y;

    if (!InbvGopQueryInfo(&FbInfo))
        return FALSE;

    RedMask = FbInfo.RedMask; GreenMask = FbInfo.GreenMask; BlueMask = FbInfo.BlueMask;
    RedShift = InbvMaskShift(RedMask); GreenShift = InbvMaskShift(GreenMask); BlueShift = InbvMaskShift(BlueMask);
    RedMax = InbvMaskMax(RedMask >> RedShift); GreenMax = InbvMaskMax(GreenMask >> GreenShift); BlueMax = InbvMaskMax(BlueMask >> BlueShift);

    ULONG PaletteCount = Header->biClrUsed ? Header->biClrUsed : 16;
    if (PaletteCount == 0)
        PaletteCount = 16;
    if (PaletteCount > 16)
        PaletteCount = 16;
    typedef struct _BMP_RGBQUAD { UCHAR b,g,r,a; } BMP_RGBQUAD;
    BMP_RGBQUAD* Pal = (BMP_RGBQUAD*)(Bitmap + Header->biSize);
    ULONG Palette32[16] = {0};
    for (ULONG i = 0; i < 16; i++)
    {
        UCHAR r = (i < PaletteCount) ? Pal[i].r : Pal[0].r;
        UCHAR g = (i < PaletteCount) ? Pal[i].g : Pal[0].g;
        UCHAR b = (i < PaletteCount) ? Pal[i].b : Pal[0].b;
        Palette32[i] = InbvPackColor(RedMask,GreenMask,BlueMask,RedShift,GreenShift,BlueShift,RedMax,GreenMax,BlueMax,r,g,b);
    }

    PUCHAR Bits = Bitmap + Header->biSize + PaletteCount * sizeof(BMP_RGBQUAD);
    LONG SrcDelta = ((SrcWidth + 1) / 2);
    SrcDelta = (SrcDelta + 3) & ~3;

#define INBV_EXTRACT(C,Mask,Shift,Maxv) \
    ((Mask) ? ((Maxv) ? ((((((C) & (Mask)) >> (Shift)) * 255) + ((Maxv)/2)) / (Maxv)) : 0) : 0)

    for (ULONG dy = 0; dy < DstHeight; dy++)
    {
        ULONG sy = (ULONG)min((ULONGLONG)SrcHeight - 1,
                              (ULONGLONG)(dy / INBV_WORDMARK_SCALE));
        ULONG effRow = TopDown ? sy : (SrcHeight - 1 - sy);
        PUCHAR Row = Bits + effRow * SrcDelta;

        static ULONG LineBuf[1024];
        ULONG produced = 0;
        while (produced < DstWidth)
        {
            ULONG toDo = min((ULONG)1024, DstWidth - produced);
            for (ULONG dx = 0; dx < toDo; dx++)
            {

                ULONG sx0 = (ULONG)min((ULONGLONG)SrcWidth - 1,
                                       (ULONGLONG)((produced + dx) / INBV_WORDMARK_SCALE));
                ULONG sy0 = sy;
                ULONG sx1 = min(sx0 + 1, SrcWidth  - 1);
                ULONG sy1 = min(sy0 + 1, SrcHeight - 1);

                UCHAR b00 = Row[sx0 / 2];
                UCHAR i00 = (sx0 & 1) ? (b00 & 0x0F) : ((b00 >> 4) & 0x0F);

                UCHAR b10 = Row[sx1 / 2];
                UCHAR i10 = (sx1 & 1) ? (b10 & 0x0F) : ((b10 >> 4) & 0x0F);

                ULONG effRow1 = TopDown ? sy1 : (SrcHeight - 1 - sy1);
                PUCHAR Row1 = Bits + effRow1 * SrcDelta;

                UCHAR b01 = Row1[sx0 / 2];
                UCHAR i01 = (sx0 & 1) ? (b01 & 0x0F) : ((b01 >> 4) & 0x0F);
                UCHAR b11 = Row1[sx1 / 2];
                UCHAR i11 = (sx1 & 1) ? (b11 & 0x0F) : ((b11 >> 4) & 0x0F);

                ULONG c00 = Palette32[i00];
                ULONG c10 = Palette32[i10];
                ULONG c01 = Palette32[i01];
                ULONG c11 = Palette32[i11];

                ULONG r = (INBV_EXTRACT(c00, RedMask,   RedShift,   RedMax)   +
                           INBV_EXTRACT(c10, RedMask,   RedShift,   RedMax)   +
                           INBV_EXTRACT(c01, RedMask,   RedShift,   RedMax)   +
                           INBV_EXTRACT(c11, RedMask,   RedShift,   RedMax)) / 4;
                ULONG g = (INBV_EXTRACT(c00, GreenMask, GreenShift, GreenMax) +
                           INBV_EXTRACT(c10, GreenMask, GreenShift, GreenMax) +
                           INBV_EXTRACT(c01, GreenMask, GreenShift, GreenMax) +
                           INBV_EXTRACT(c11, GreenMask, GreenShift, GreenMax)) / 4;
                ULONG b = (INBV_EXTRACT(c00, BlueMask,  BlueShift,  BlueMax)  +
                           INBV_EXTRACT(c10, BlueMask,  BlueShift,  BlueMax)  +
                           INBV_EXTRACT(c01, BlueMask,  BlueShift,  BlueMax)  +
                           INBV_EXTRACT(c11, BlueMask,  BlueShift,  BlueMax)) / 4;

                LineBuf[dx] = InbvPackColor(RedMask,GreenMask,BlueMask,
                                             RedShift,GreenShift,BlueShift,
                                             RedMax,GreenMax,BlueMax,
                                             (UCHAR)r,(UCHAR)g,(UCHAR)b);
            }
            VidBufferToScreenBlt((PUCHAR)LineBuf,
                                 DestX + produced,
                                 DestY + dy,
                                 toDo,
                                 1,
                                 toDo * sizeof(ULONG));
            produced += toDo;
        }
    }

    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
InbvGopHandleBootBitmap(
    _In_ BOOLEAN TextMode)
{
    LOADER_PARAMETER_FRAMEBUFFER FbInfo;
    ULONG Width, Height;
    BOOLEAN WordmarkDrawn;

    if (!InbvGopQueryInfo(&FbInfo))
        return FALSE;

    Width = FbInfo.HorizontalResolution;
    Height = FbInfo.VerticalResolution;

    if (TextMode)
    {
        InbvResetDisplay();
        return FALSE;
    }

    InbvSetTextColor(BV_COLOR_WHITE);

    WordmarkDrawn = InbvGopDrawWordmark(Width, Height, NULL);
    InbvGopSpinnerStart(Width, Height);

    if (!WordmarkDrawn)
    {
        static const CHAR LoadingMsg[] = "ReactOS";
        const ULONG CharW = 8;
        const ULONG CharH = 13;
        ULONG msgPx = (ULONG)(sizeof(LoadingMsg) - 1) * CharW;
        ULONG x = (Width > msgPx) ? ((Width - msgPx) / 2) : 0;
        ULONG y = (Height > (CharH + 4)) ? (Height - (CharH + 4)) : 0;
        VidDisplayStringXY(LoadingMsg, x, y, TRUE);
    }

    ShowProgressBar = FALSE;
    InbvEnableDisplayString(FALSE);
    return TRUE;
}
