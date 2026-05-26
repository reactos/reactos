/*
 * PROJECT:     ReactOS Xbox miniport video driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NVIDIA NV2A (XGPU) 2D engine accelerator
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 *
 * Channel layout
 * --------------
 *
 *   Channel 0, push-buffer mode, subchannels:
 *      0 -> NV04_CONTEXT_SURFACES_2D   (handle 0xBEEF0001)
 *      1 -> NV_IMAGE_BLIT              (handle 0xBEEF0002)
 *      2 -> NV04_GDI_RECTANGLE_TEXT    (handle 0xBEEF0003)
 *
 * RAMIN slot map (offsets relative to PRAMIN start = MMIO + 0x700000):
 *      0x0000 - 0x1FFF   RAMHT (8 KB)
 *      0x2000 - 0x21FF   RAMFC (512 B, channel 0)
 *      0x3000 - 0x300F   DMA object: framebuffer (NV01_CONTEXT_DMA)
 *      0x3010 - 0x301F   DMA object: push buffer (NV01_CONTEXT_DMA)
 *      0x3020 - 0x303F   NV04_CONTEXT_SURFACES_2D engine object
 *      0x3040 - 0x305F   NV_IMAGE_BLIT engine object
 *      0x3060 - 0x307F   NV04_GDI_RECTANGLE_TEXT engine object
 *
 * The push buffer sits inside VRAM, in the 64 KB region immediately above the
 * firmware-allocated framebuffer.  This avoids stomping on anything the boot
 * loader put there.
 * 
 * As you can probably tell: RAM space is ultra limited, so part of this is some exercises to 
 * absoutely minimize the amount of ram we need. Some of the weirdness is because of this!
 */

#include "xboxvmp.h"
#include "nv2a_accel.h"

#include <debug.h>
#include <dpfilter.h>

/* hackies  */
PHYSICAL_ADDRESS NTAPI MmGetPhysicalAddress(PVOID BaseAddress);
VOID NTAPI MmFreeContiguousMemory(PVOID BaseAddress);

/* Handles we publish in RAMHT for our objects */
/* Object handles MUST be small: xemu's ramht_hash folds the handle into
 * 11-bit chunks and asserts hash*8 < ramht_size (4 KB), so a handle < 0x800
 * hashes to itself and stays in range.  Big handles (0xBEEF....) hash out of
 * bounds and trip pfifo.c:518.  pbkit uses small handles for the same reason. */
#define HANDLE_DMA_FB         0x00000011
#define HANDLE_DMA_PB         0x00000012
#define HANDLE_SURFACES_2D    0x00000013
#define HANDLE_IMAGE_BLIT     0x00000014
#define HANDLE_GDI_RECT       0x00000015
#define HANDLE_KELVIN         0x00000016   /* NV097 3D engine */
#define HANDLE_DMA_3D         0x00000017   /* NV_DMA_IN_MEMORY for the 3D surface */

/* xemu (and the Xbox D3D driver) REQUIRE the Kelvin 3D object on subchannel 0
 * (pgraph.c asserts graphics_class != 0x97 for any non-zero subchannel). */
#define SUBCH_3D              0
/* The 2D engine objects live on OTHER subchannels so they coexist with Kelvin:
 * binding SURFACES_2D to subch 0 (as the 2D accel originally did) is clobbered
 * the moment 3D init binds Kelvin there, and then every desktop GDI blit pushes
 * SURFACES_2D methods at Kelvin -> FIFO corruption / freeze.  Keep them split. */
#define SUBCH_SURF2D          3   /* NV04_CONTEXT_SURFACES_2D */
#define SUBCH_BLIT            1   /* NV_IMAGE_BLIT            */

/* RAMIN slot offsets, in bytes from PRAMIN base */
#define RAMIN_RAMHT_OFFSET    0x0000
#define RAMIN_RAMHT_SIZE      0x2000
#define RAMIN_RAMFC_OFFSET    0x2000
#define RAMIN_RAMFC_SIZE      0x0200
#define RAMIN_OBJ_DMA_FB      0x3000
#define RAMIN_OBJ_DMA_PB      0x3010
#define RAMIN_OBJ_SURFACES_2D 0x3020
#define RAMIN_OBJ_IMAGE_BLIT  0x3040
#define RAMIN_OBJ_GDI_RECT    0x3060
#define RAMIN_OBJ_KELVIN      0x3080
#define RAMIN_OBJ_DMA_3D      0x3090
/* 3D: channel-context table + the large per-channel graphics-context block
 * (holds the pb_3D_init state image PGRAPH restores on a context switch). */
#define RAMIN_CTX_TABLE       0x4000
#define RAMIN_GR_CTX          0x10000
#define RAMIN_GR_CTX_SIZE     0x3800

/* Push buffer */
#define PUSHBUFFER_SIZE       0x10000  /* 64 KB */

#define ACCEL_TAG 'lcA2'  /* "2Acl" */

/* ------------------------------------------------------------------------- */
/* MMIO helpers ------------------------------------------------------------- */

static __inline ULONG
NvRead32(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Reg)
{
    return READ_REGISTER_ULONG((PULONG)((ULONG_PTR)Dx->VirtControlStart + Reg));
}

static __inline VOID
NvWrite32(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Reg, ULONG Value)
{
    WRITE_REGISTER_ULONG((PULONG)((ULONG_PTR)Dx->VirtControlStart + Reg), Value);
}

static __inline VOID
NvWritePRamin32(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Offset, ULONG Value)
{
    WRITE_REGISTER_ULONG(
        (PULONG)((ULONG_PTR)Dx->VirtControlStart + NV2A_PRAMIN_OFFSET + Offset),
        Value);
}

/* ------------------------------------------------------------------------- */
/* RAMHT entry encoding ----------------------------------------------------- */

/* Each RAMHT entry is 8 bytes:
 *   [0]: 32-bit object handle
 *   [4]: bits  0..15 -> instance address >> 4 (instance lives in PRAMIN)
 *        bits 16..23 -> engine (0 = software, 1 = graphics, etc.)
 *        bits 24..30 -> channel ID
 *        bit  31     -> valid
 */
#define RAMHT_ENGINE_SW   0
#define RAMHT_ENGINE_GR   1

static VOID
RamhtInsert(PXBOXVMP_DEVICE_EXTENSION Dx,
            ULONG Handle, ULONG InstanceOffset,
            ULONG Engine, ULONG Channel)
{
    ULONG h, hh, slot, base;
    ULONG entry0, entry1;

    /* xemu's exact ramht_hash: XOR of <bits>-wide chunks of the handle, then
     * ^ channel << (bits-4).  ramht_size = 4 KB -> bits = 11.  Computing it the
     * same way guarantees our insert slot == xemu's lookup slot. */
    h = 0;
    hh = Handle;
    while (hh) { h ^= (hh & 0x7FF); hh >>= 11; }
    h ^= (Channel & 0xF) << 7;

    slot = h;
    base = RAMIN_RAMHT_OFFSET + slot * 8;

    entry0 = Handle;
    entry1 = ((InstanceOffset >> 4) & 0xFFFF)
           | ((Engine & 0xFF)   << 16)
           | ((Channel & 0x7F)  << 24)
           | 0x80000000u;

    NvWritePRamin32(Dx, base + 0, entry0);
    NvWritePRamin32(Dx, base + 4, entry1);
}

/* ------------------------------------------------------------------------- */
/* DMA context object (NV01_CONTEXT_DMA) ------------------------------------ */

/* A 16-byte structure that tells the engine what physical memory window an
 * "in VRAM" or "in main RAM" reference resolves to.  Format on NV20 / NV2A:
 *   +0   class | flags | adjust
 *   +4   limit (size - 1)
 *   +8   linear base (frame address << 12 | access flags)
 *   +c   unused
 *
 * Access flags: bits 0..1 = access (0=RW), bit 2 = pte_present.
 */
#define DMA_FLAGS_RW_FB   (NV01_CONTEXT_DMA | (2 << 16))  /* class 0x3, target VRAM */
#define DMA_FLAGS_RW_AGP  (NV01_CONTEXT_DMA | (3 << 16))

static VOID
WriteDmaObject(PXBOXVMP_DEVICE_EXTENSION Dx,
               ULONG SlotOffset,
               ULONG Flags,
               ULONG GpuBase,
               ULONG GpuLimit)
{
    NvWritePRamin32(Dx, SlotOffset + 0, Flags);
    NvWritePRamin32(Dx, SlotOffset + 4, GpuLimit);
    NvWritePRamin32(Dx, SlotOffset + 8, GpuBase | 3); /* pte present, RW */
    NvWritePRamin32(Dx, SlotOffset + 12, 0);
}

/* ------------------------------------------------------------------------- */
/* Engine object (graphics class) ------------------------------------------- */

/* Each graphics object is also 16 bytes, but its meaning is
 *   +0   class id | flags
 *   +4   0
 *   +8   0
 *   +c   0
 *
 * The class id alone is enough to bind a subchannel to the engine; the rest
 * is populated by hardware as graphics state is touched. */
static VOID
WriteEngineObject(PXBOXVMP_DEVICE_EXTENSION Dx,
                  ULONG SlotOffset, ULONG ClassId)
{
    NvWritePRamin32(Dx, SlotOffset + 0, ClassId);
    NvWritePRamin32(Dx, SlotOffset + 4, 0);
    NvWritePRamin32(Dx, SlotOffset + 8, 0);
    NvWritePRamin32(Dx, SlotOffset + 12, 0);
}

/* ------------------------------------------------------------------------- */
/* Push buffer helpers ------------------------------------------------------ */

/* The NV2A FIFO puller HALTS whenever PGRAPH has a pending interrupt and stays
 * halted until the driver ACKS it: a context switch raises
 * NV_PGRAPH_INTR_CONTEXT_SWITCH and sets pgraph.waiting_for_context_switch; an
 * error sets waiting_for_nop.  Under xemu (hw/xbox/nv2a/pgraph.c::pgraph_write),
 * writing NV_PGRAPH_INTR clears the matching wait flag and re-kicks the puller.
 * Our Nv3dPgraphSetup acks once at init; the per-draw FIFO-wait paths must too —
 * otherwise the first context switch the pushbuffer hits wedges the pusher
 * forever (DMA_GET stops advancing) and the guest spins on DMA_GET -> whole-VM
 * freeze.  Re-assert channel-0 validity (so the puller's context-switch retry
 * passes: pgraph_context_switch requires CTX_CONTROL.CHID_VALID && CTX_USER.CHID
 * == channel_id == 0), then ack every pending interrupt. */
static VOID
Nv2aResumePusher(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    ULONG intr = NvRead32(Dx, NV2A_PGRAPH_INTR);
    if (intr != 0)
    {
        NvWrite32(Dx, 0x00400148, 0x00000000);                            /* CTX_USER: CHID = 0      */
        NvWrite32(Dx, 0x00400144, NvRead32(Dx, 0x00400144) | 0x00010000); /* CTX_CONTROL: CHID_VALID */
        NvWrite32(Dx, NV2A_PGRAPH_INTR, intr);                            /* ack all -> puller runs  */
    }
    /* The puller also stalls (with NO interrupt pending) if PGRAPH FIFO access is
     * cleared, or simply if the pfifo thread went idle and needs a kick.  Writing
     * NV_PGRAPH_FIFO re-asserts FIFO_ACCESS and, in xemu, calls pfifo_kick() —
     * re-running the pusher/puller.  Covers the INTR==0 stall seen on mouse-move. */
    NvWrite32(Dx, 0x00400720, 0x00000001);                                /* NV_PGRAPH_FIFO: ACCESS + kick */
}

/* Wait until the GPU's DMA_GET pointer is at least <count> DWORDs ahead of
 * our planned write — i.e. there's room for <count> entries.  Spins. */
static BOOLEAN
PbReserve(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Dwords)
{
    ULONG putGpuOffset;
    ULONG getGpuOffset;
    ULONG freeBytes;
    ULONG spin = 0;

    putGpuOffset = Dx->PushBufferGpuOffset + Dx->PushBufferPut * 4;

    /* If we'd wrap past the end of the buffer, jump back to start. */
    if ((Dx->PushBufferPut + Dwords + 1) * 4 >= Dx->PushBufferSize)
    {
        /* Emit a jump-to-start command:
         *   high bit of the 32-bit word selects jump.  On NV2A: 0x20000000
         *   marks a JMP, payload is target offset. */
        Dx->PushBufferVirt[Dx->PushBufferPut] =
            0x20000000 | (Dx->PushBufferGpuOffset & 0x1FFFFFFC);
        Dx->PushBufferPut = 0;
        putGpuOffset = Dx->PushBufferGpuOffset;

        NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_PUT, putGpuOffset);
    }

    for (;;)
    {
        getGpuOffset = NvRead32(Dx, NV2A_PFIFO_CACHE1_DMA_GET);

        if (getGpuOffset <= putGpuOffset)
        {
            /* GPU is behind us in the ring; free space is (end - put) + (get - base). */
            freeBytes = (Dx->PushBufferGpuOffset + Dx->PushBufferSize - putGpuOffset)
                      + (getGpuOffset - Dx->PushBufferGpuOffset);
        }
        else
        {
            freeBytes = getGpuOffset - putGpuOffset;
        }

        if (freeBytes >= (Dwords + 1) * 4)
            return TRUE;

        /* GET isn't advancing -> the puller is almost certainly halted on a
         * pending PGRAPH interrupt.  Ack it so the FIFO drains; else we hang. */
        Nv2aResumePusher(Dx);

        if (++spin > 0x100000)
        {
            ERR_(IHVVIDEO, "NV2A push buffer wait timed out\n");
            return FALSE;
        }
    }
}

static __inline VOID
PbPush(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Word)
{
    Dx->PushBufferVirt[Dx->PushBufferPut++] = Word;
}

static VOID
PbKick(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    ULONG putGpuOffset = Dx->PushBufferGpuOffset + Dx->PushBufferPut * 4;
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_PUT, putGpuOffset);
}

VOID
Nv2aAccelWaitIdle(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    ULONG spin = 0;
    ULONG put;

    if (!Dx->AccelEnabled)
        return;

    put = NvRead32(Dx, NV2A_PFIFO_CACHE1_DMA_PUT);

    while (NvRead32(Dx, NV2A_PFIFO_CACHE1_DMA_GET) != put)
    {
        /* The puller halts on a pending PGRAPH interrupt; ack it each iteration
         * so GET can reach PUT.  Without this the wait spins forever (the
         * original "turning on 3D freezes the VM" hang). */
        Nv2aResumePusher(Dx);
        if (++spin > 0x400000)
        {
            ERR_(IHVVIDEO, "NV2A wait-idle timed out\n");
            return;
        }
    }
}

/* Bind a graphics class to a subchannel. */
static BOOLEAN __attribute__((unused))
PbBindSubchannel(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Subch, ULONG Handle)
{
    if (!PbReserve(Dx, 2))
        return FALSE;
    PbPush(Dx, NV2A_METHOD(Subch, NV_SET_OBJECT, 1));
    PbPush(Dx, Handle);
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Surface programming ------------------------------------------------------ */

static BOOLEAN
PbSetSurface(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    if (!PbReserve(Dx, 7))
        return FALSE;

    /* Program SURFACES_2D once.  PITCH is dst-pitch in high half, src-pitch
     * in low half — we use the same surface for both. */
    PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_FORMAT, 4));
    PbPush(Dx, Dx->SurfaceFormat);
    PbPush(Dx, (Dx->SurfacePitch & 0xFFFF) | ((Dx->SurfacePitch & 0xFFFF) << 16));
    PbPush(Dx, Dx->FrameBufferGpuOffset);   /* OFFSET_SRC */
    PbPush(Dx, Dx->FrameBufferGpuOffset);   /* OFFSET_DST */

    PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_SET_DMA_IMAGE_SRC, 2));
    PbPush(Dx, HANDLE_DMA_FB);
    PbPush(Dx, HANDLE_DMA_FB);

    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Public ops --------------------------------------------------------------- */

BOOLEAN
Nv2aAccelFillRect(PXBOXVMP_DEVICE_EXTENSION Dx,
                  ULONG X, ULONG Y, ULONG Width, ULONG Height,
                  ULONG Color)
{
    /* Solid fills would use NV04_GDI_RECTANGLE_TEXT, but the NV2A has no such
     * 2D class and xemu's PGRAPH does not decode it — the methods would be
     * silently dropped, leaving the fill un-drawn.  Fills are cheap, so always
     * take the caller's CPU fallback.  (Hardware blits below use NV_IMAGE_BLIT
     * 0x9F, which the NV2A / xemu *do* implement.) */
    UNREFERENCED_PARAMETER(Dx);
    UNREFERENCED_PARAMETER(X);
    UNREFERENCED_PARAMETER(Y);
    UNREFERENCED_PARAMETER(Width);
    UNREFERENCED_PARAMETER(Height);
    UNREFERENCED_PARAMETER(Color);
    return FALSE;
}

BOOLEAN
Nv2aAccelScreenBlt(PXBOXVMP_DEVICE_EXTENSION Dx,
                   ULONG SrcX, ULONG SrcY,
                   ULONG DstX, ULONG DstY,
                   ULONG Width, ULONG Height)
{
    if (!Dx->AccelEnabled)
        return FALSE;

    if (Width == 0 || Height == 0)
        return TRUE;

    if (SrcX + Width > Dx->ScreenWidth || SrcY + Height > Dx->ScreenHeight)
        return FALSE;
    if (DstX + Width > Dx->ScreenWidth || DstY + Height > Dx->ScreenHeight)
        return FALSE;

    /* NV_IMAGE_BLIT copies strictly forward (top-left to bottom-right).  If the
     * source and destination rectangles overlap, that direction corrupts the
     * region (e.g. dragging a window down, or a scroll).  Hand overlapping
     * moves to the caller's CPU fallback, which copies in the safe direction. */
    if (SrcX < DstX + Width  && DstX < SrcX + Width &&
        SrcY < DstY + Height && DstY < SrcY + Height)
    {
        return FALSE;
    }

    if (!PbSetSurface(Dx))
        return FALSE;

    if (!PbReserve(Dx, 9))
        return FALSE;

    /* IMAGE_BLIT on its own subchannel. */
    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_SET_CONTEXT_SURFACES, 1));
    PbPush(Dx, HANDLE_SURFACES_2D);

    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_OPERATION, 1));
    PbPush(Dx, NV_IMAGE_BLIT_OPERATION_SRCCOPY);

    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_POINT_IN, 3));
    PbPush(Dx, (SrcX & 0xFFFF) | ((SrcY & 0xFFFF) << 16));
    PbPush(Dx, (DstX & 0xFFFF) | ((DstY & 0xFFFF) << 16));
    PbPush(Dx, (Width & 0xFFFF) | ((Height & 0xFFFF) << 16));

    PbKick(Dx);
    return TRUE;
}

/* General VRAM<->VRAM blit between arbitrary surfaces, addressed by absolute GPU
 * byte offset + pitch.  Used for offscreen device bitmaps (screen<->devbitmap,
 * devbitmap<->devbitmap).  Src and dst are assumed NON-overlapping (different
 * surfaces); same-surface overlapping moves go through Nv2aAccelScreenBlt.  We
 * WaitIdle so the result is coherent for a subsequent CPU read of the bitmap. */
BOOLEAN
Nv2aAccelBltEx(PXBOXVMP_DEVICE_EXTENSION Dx,
               ULONG SrcOffset, ULONG SrcPitch, ULONG SrcX, ULONG SrcY,
               ULONG DstOffset, ULONG DstPitch, ULONG DstX, ULONG DstY,
               ULONG W, ULONG H)
{
    if (!Dx->AccelEnabled)
        return FALSE;
    if (W == 0 || H == 0)
        return TRUE;

    if (!PbReserve(Dx, 16))
        return FALSE;

    /* 2D context surfaces with explicit src/dst offsets + pitches. */
    PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_FORMAT, 4));
    PbPush(Dx, Dx->SurfaceFormat);
    PbPush(Dx, (SrcPitch & 0xFFFF) | ((DstPitch & 0xFFFF) << 16));
    PbPush(Dx, SrcOffset & 0x03FFFFFF);   /* OFFSET_SRC */
    PbPush(Dx, DstOffset & 0x03FFFFFF);   /* OFFSET_DST */

    PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_SET_DMA_IMAGE_SRC, 2));
    PbPush(Dx, HANDLE_DMA_FB);
    PbPush(Dx, HANDLE_DMA_FB);

    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_SET_CONTEXT_SURFACES, 1));
    PbPush(Dx, HANDLE_SURFACES_2D);
    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_OPERATION, 1));
    PbPush(Dx, NV_IMAGE_BLIT_OPERATION_SRCCOPY);
    PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_POINT_IN, 3));
    PbPush(Dx, (SrcX & 0xFFFF) | ((SrcY & 0xFFFF) << 16));
    PbPush(Dx, (DstX & 0xFFFF) | ((DstY & 0xFFFF) << 16));
    PbPush(Dx, (W & 0xFFFF) | ((H & 0xFFFF) << 16));

    PbKick(Dx);
    Nv2aAccelWaitIdle(Dx);
    return TRUE;
}

/* ------------------------------------------------------------------------- */
/* Initialisation ----------------------------------------------------------- */

static BOOLEAN __attribute__((unused))
PfifoBringUp(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    ULONG savedFb;

    /* Preserve CRTC framebuffer pointer (re-written below to be safe). */
    savedFb = NvRead32(Dx, NV2A_CRTC_FRAMEBUFFER_START);

    /* DO NOT reset PMC (no NV2A_PMC_ENABLE disable/re-enable cycle).  The BIOS
     * (Cromwell) already enabled PMC/PFIFO/PGRAPH and — crucially — performed the
     * full PGRAPH initialisation (CONTROL/SETUPRASTER/shader/cheops state) that
     * the vertex+raster pipeline needs.  A PMC disable->enable cycle WIPES that
     * PGRAPH init, leaving only enough state for CLEAR_SURFACE + 2D blits but NOT
     * for a real DRAW (which then rasterises zero fragments — the exact symptom).
     * We reconfigure PFIFO fully via its own registers below, so PFIFO needs no
     * PMC reset.  Relying on Cromwell's engine-enable preserves PGRAPH. */

    /* Mask interrupts; we don't service them in this driver yet. */
    NvWrite32(Dx, NV2A_PMC_INTR_EN_0, 0);
    NvWrite32(Dx, NV2A_PFIFO_INTR_EN_0, 0);
    NvWrite32(Dx, NV2A_PGRAPH_INTR_EN, 0);

    /* Tell PFIFO where RAMHT and RAMFC live.
     *   RAMHT  base = (PRAMIN + 0x0000) >> 8 = 0
     *   RAMFC  base = (PRAMIN + 0x2000) >> 8 = 0x20
     * RAMHT search size = 128 (bits 24-26 = 0x3). */
    NvWrite32(Dx, NV2A_PFIFO_RAMHT, (0x0  >> 8) | (0x3 << 24));
    NvWrite32(Dx, NV2A_PFIFO_RAMFC, (0x2000 >> 8));

    /* Restore CRTC framebuffer start so the display continues to scan our fb. */
    NvWrite32(Dx, NV2A_CRTC_FRAMEBUFFER_START, savedFb);

    /* Disable caches, switch channel 0 to DMA mode. */
    NvWrite32(Dx, NV2A_PFIFO_CACHES, 0);
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_PUSH0, 0);
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_PULL0, 0);

    NvWrite32(Dx, NV2A_PFIFO_CACHE1_PUSH1,
              NV2A_PFIFO_CACHE1_PUSH1_MODE_DMA /* DMA */ | 0 /* ch 0 */);
    /* The pusher derives the pushbuffer length (dma_len) from the DMA object at
     * this instance.  Point it at the base-0 framebuffer DMA object (covers all
     * VRAM) so DMA_GET (an absolute VRAM offset) is always in bounds.  Leaving
     * it 0 made the pusher read RAMHT as a bogus DMA object -> wrong dma_len ->
     * pfifo.c:299 assert(dma_get >= dma_len) once the stream grew. */
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_INSTANCE, RAMIN_OBJ_DMA_FB >> 4);

    /* Mark channel 0 as DMA in the per-channel mode table.  Without this the
     * DMA pusher has no channel to run (PFIFO_MODE defaults to all-PIO), and
     * kicking DMA_PUT later does nothing on real hardware / trips an assertion
     * in emulators.  Caches are disabled here, which is the correct window. */
    NvWrite32(Dx, NV2A_PFIFO_MODE, (1 << 0));

    /* Push buffer offset into VRAM, encoded as low 28 bits. */
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_PUT, Dx->PushBufferGpuOffset);
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_GET, Dx->PushBufferGpuOffset);

    /* Re-enable caches and the DMA pusher. */
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_PUSH0, 1);
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_PULL0, 1);
    NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_PUSH, NV2A_PFIFO_CACHE1_DMA_PUSH_ENABLE);
    NvWrite32(Dx, NV2A_PFIFO_CACHES, NV2A_PFIFO_CACHES_REASSIGN);

    return TRUE;
}

VP_STATUS
Nv2aAccelInitialize(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    ULONG i;
    PHYSICAL_ADDRESS pbPhys;
    ULONG pbMapLength;
    ULONG inIoSpace;

    if (Dx->AccelInitAttempted)
        return Dx->AccelEnabled ? NO_ERROR : ERROR_DEV_NOT_EXIST;

    Dx->AccelInitAttempted = TRUE;
    Dx->AccelEnabled = FALSE;

    if (Dx->FrameBufferLength < PUSHBUFFER_SIZE * 2)
    {
        WARN_(IHVVIDEO, "NV2A accel: framebuffer too small for push buffer\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Park the push buffer at the very top of VRAM.  This region is below the
     * scanout, since the CRTC reads from FrameBufferGpuOffset upward by
     * stride * height, which is strictly less than PUSHBUFFER_SIZE * 2 above
     * the framebuffer base for any realistic mode. */
    Dx->PushBufferSize = PUSHBUFFER_SIZE;
    Dx->PushBufferGpuOffset = Dx->FrameBufferGpuOffset
                            + Dx->FrameBufferLength
                            - PUSHBUFFER_SIZE;

    /* Map it CPU-side so we can write commands into it. */
    pbPhys.QuadPart = Dx->PhysFrameBufferStart.QuadPart + Dx->PushBufferGpuOffset;
    pbMapLength = PUSHBUFFER_SIZE;
    inIoSpace = VIDEO_MEMORY_SPACE_MEMORY;
    Dx->PushBufferVirt = NULL;

    if (VideoPortMapMemory(Dx, pbPhys, &pbMapLength, &inIoSpace,
                           (PVOID*)&Dx->PushBufferVirt) != NO_ERROR)
    {
        ERR_(IHVVIDEO, "NV2A accel: failed to map push buffer\n");
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Dx->PushBufferPut = 0;

    /* Texture heap.  The NV2A is UMA and the base-0 DMA_3D object spans all of
     * RAM, so the GPU can texture from ANY guest-physical address — not just the
     * cramped 4 MB framebuffer reservation.  Prefer a large physically-contiguous
     * system-RAM pool (real games like Unreal Tournament need tens of MB of
     * textures; the old 384 KB FB carve-out held barely one 256x256 texture and
     * everything past it rendered white).  Its MmGetPhysicalAddress IS the GPU
     * offset (the FB BAR aliases main RAM from physical 0).  Fall back to the
     * small FB carve-out if the contiguous allocation fails. */
    Dx->TextureHeapVirt   = NULL;
    Dx->TextureHeapContig = NULL;
    {
        static const ULONG poolSizes[] = { 2u<<20, 1u<<20 };
        PHYSICAL_ADDRESS hi;
        ULONG s;
        /* Keep the pool strictly below the framebuffer reservation so its GPU
         * offset stays within the DMA_3D limit and never overlaps the FB. */
        hi.QuadPart = (LONGLONG)Dx->FrameBufferGpuOffset - 1;
        for (s = 0; s < RTL_NUMBER_OF(poolSizes); ++s)
        {
            PVOID v = VideoPortAllocateContiguousMemory(Dx, poolSizes[s], hi);
            if (v)
            {
                PHYSICAL_ADDRESS phys = MmGetPhysicalAddress(v);
                Dx->TextureHeapContig    = v;
                Dx->TextureHeapVirt      = v;
                Dx->TextureHeapGpuOffset = (ULONG)phys.QuadPart;
                Dx->TextureHeapSize      = poolSizes[s];
                Dx->TextureHeapNext      = Dx->TextureHeapGpuOffset;
                INFO_(IHVVIDEO, "NV2A: %lu MB contiguous texture pool at phys 0x%08lx\n",
                      poolSizes[s] >> 20, Dx->TextureHeapGpuOffset);
                break;
            }
        }
    }
    if (!Dx->TextureHeapVirt)
    {
        /* Fallback: 384 KB carved from the slice between the 3D offscreen colour
         * surface (FB+0x200000) and the push buffer, mapped through the FB BAR. */
        PHYSICAL_ADDRESS texPhys;
        ULONG texLen, texIo = VIDEO_MEMORY_SPACE_MEMORY;
        Dx->TextureHeapGpuOffset = Dx->FrameBufferGpuOffset + 0x00352000;
        Dx->TextureHeapSize      = 0x00060000;
        Dx->TextureHeapNext      = Dx->TextureHeapGpuOffset;
        texLen = Dx->TextureHeapSize;
        texPhys.QuadPart = Dx->PhysFrameBufferStart.QuadPart + Dx->TextureHeapGpuOffset;
        if (VideoPortMapMemory(Dx, texPhys, &texLen, &texIo,
                               &Dx->TextureHeapVirt) != NO_ERROR)
            Dx->TextureHeapVirt = NULL;   /* texturing disabled if this fails */
    }

    /* Zero RAMHT — entries are validated by bit 31 of the second DWORD. */
    for (i = 0; i < RAMIN_RAMHT_SIZE; i += 4)
        NvWritePRamin32(Dx, RAMIN_RAMHT_OFFSET + i, 0);

    /* Zero RAMFC. */
    for (i = 0; i < RAMIN_RAMFC_SIZE; i += 4)
        NvWritePRamin32(Dx, RAMIN_RAMFC_OFFSET + i, 0);

    /* DMA contexts:
     *  - framebuffer DMA covers the whole VRAM region we own (so OFFSET_DST /
     *    OFFSET_SRC values are GPU-relative to the start of VRAM).
     *  - push-buffer DMA covers just the push buffer.
     */
    WriteDmaObject(Dx, RAMIN_OBJ_DMA_FB, DMA_FLAGS_RW_FB,
                   0, Dx->FrameBufferGpuOffset + Dx->FrameBufferLength - 1);
    WriteDmaObject(Dx, RAMIN_OBJ_DMA_PB, DMA_FLAGS_RW_FB,
                   Dx->PushBufferGpuOffset, PUSHBUFFER_SIZE - 1);

    /* Engine objects */
    WriteEngineObject(Dx, RAMIN_OBJ_SURFACES_2D, NV04_CONTEXT_SURFACES_2D);
    WriteEngineObject(Dx, RAMIN_OBJ_IMAGE_BLIT,  NV_IMAGE_BLIT);
    WriteEngineObject(Dx, RAMIN_OBJ_GDI_RECT,    NV04_GDI_RECTANGLE_TEXT);

    /* Publish in RAMHT */
    RamhtInsert(Dx, HANDLE_DMA_FB,      RAMIN_OBJ_DMA_FB,      RAMHT_ENGINE_GR, 0);
    RamhtInsert(Dx, HANDLE_DMA_PB,      RAMIN_OBJ_DMA_PB,      RAMHT_ENGINE_GR, 0);
    RamhtInsert(Dx, HANDLE_SURFACES_2D, RAMIN_OBJ_SURFACES_2D, RAMHT_ENGINE_GR, 0);
    RamhtInsert(Dx, HANDLE_IMAGE_BLIT,  RAMIN_OBJ_IMAGE_BLIT,  RAMHT_ENGINE_GR, 0);
    RamhtInsert(Dx, HANDLE_GDI_RECT,    RAMIN_OBJ_GDI_RECT,    RAMHT_ENGINE_GR, 0);

    /* Decide on a 2D surface format from the current bpp. */
    switch (Dx->BytesPerPixel)
    {
        case 2: Dx->SurfaceFormat = NV04_SURFACES_2D_FORMAT_R5G6B5; break;
        case 4: Dx->SurfaceFormat = NV04_SURFACES_2D_FORMAT_X8R8G8B8_Z8R8G8B8; break;
        default:
            WARN_(IHVVIDEO,
                  "NV2A accel: unsupported bpp %u; staying CPU-only\n",
                  Dx->BytesPerPixel);
            VideoPortUnmapMemory(Dx, Dx->PushBufferVirt, NULL);
            Dx->PushBufferVirt = NULL;
            return ERROR_INVALID_PARAMETER;
    }
    Dx->SurfacePitch = Dx->ScreenStride;
    if (!PfifoBringUp(Dx))
    {
        VideoPortUnmapMemory(Dx, Dx->PushBufferVirt, NULL);
        Dx->PushBufferVirt = NULL;
        return ERROR_DEV_NOT_EXIST;
    }

    /* Bind subchannels.  SURFACES_2D goes on SUBCH_SURF2D (NOT subch 0) so the
     * 3D engine can own subch 0 (Kelvin) without clobbering it — otherwise the
     * first 3D draw breaks every later 2D blit and hangs the FIFO. */
    if (!PbBindSubchannel(Dx, SUBCH_SURF2D, HANDLE_SURFACES_2D) ||
        !PbBindSubchannel(Dx, SUBCH_BLIT,   HANDLE_IMAGE_BLIT)  ||
        !PbBindSubchannel(Dx, 2,            HANDLE_GDI_RECT))
    {
        VideoPortUnmapMemory(Dx, Dx->PushBufferVirt, NULL);
        Dx->PushBufferVirt = NULL;
        return ERROR_DEV_NOT_EXIST;
    }
    PbKick(Dx);

    Dx->AccelEnabled = TRUE;
    INFO_(IHVVIDEO,
          "NV2A accel: live (pushbuf @ GPU 0x%lx, %lu bytes)\n",
          Dx->PushBufferGpuOffset, Dx->PushBufferSize);

    return NO_ERROR;
}

VOID
Nv2aAccelShutdown(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    if (!Dx->AccelInitAttempted)
        return;

    if (Dx->AccelEnabled)
    {
        Nv2aAccelWaitIdle(Dx);
        /* Quiesce DMA pusher; leave PMC/PCRTC alone so the framebuffer stays
         * lit for whatever takes over (e.g. the bugcheck blue screen). */
        NvWrite32(Dx, NV2A_PFIFO_CACHE1_DMA_PUSH, 0);
        NvWrite32(Dx, NV2A_PFIFO_CACHE1_PUSH0, 0);
        NvWrite32(Dx, NV2A_PFIFO_CACHE1_PULL0, 0);
        Dx->AccelEnabled = FALSE;
    }

    if (Dx->PushBufferVirt != NULL)
    {
        VideoPortUnmapMemory(Dx, Dx->PushBufferVirt, NULL);
        Dx->PushBufferVirt = NULL;
    }

    /* Texture pool: a contiguous system-RAM allocation is freed with
     * MmFreeContiguousMemory; the FB carve-out fallback was a VideoPortMapMemory. */
    if (Dx->TextureHeapContig != NULL)
    {
        MmFreeContiguousMemory(Dx->TextureHeapContig);
        Dx->TextureHeapContig = NULL;
        Dx->TextureHeapVirt = NULL;
    }
    else if (Dx->TextureHeapVirt != NULL)
    {
        VideoPortUnmapMemory(Dx, Dx->TextureHeapVirt, NULL);
        Dx->TextureHeapVirt = NULL;
    }

    /* Z24S8 zeta lives in a contiguous system-RAM allocation. */
    if (Dx->DepthZetaContig != NULL)
    {
        MmFreeContiguousMemory(Dx->DepthZetaContig);
        Dx->DepthZetaContig = NULL;
    }
}

/* ------------------------------------------------------------------------- */
/* NV2A 3D (Kelvin, class 0x97) --------------------------------------------- */
/* nxdk's canonical "matrix * vertex" VP — the verbatim vp20compiler output for
 * nxdk's triangle sample (samples/triangle/vs.vs.cg), which is a known-good,
 * xemu-tested program.  It computes
 *     R0   = v0.x*c[0] + v0.y*c[1] + v0.z*c[2] + c[3]   (NOTE: ignores v0.w)
 *     oPos = (R0.xyz / R0.w, R0.w)        oCol0 = v[3]
 * The xboxogl ICD feeds us FULL-SURFACE CLIP-SPACE positions (w=1), and we load
 * an IDENTITY matrix into c[0..3] (see Nv2a3dInitialize), so R0 = (v0.xyz, 1)
 * and oPos = (v0.xyz, 1) — i.e. this acts as a pure passthrough.  xemu maps the
 * program's oPos straight to gl_Position (no inverse-viewport), so emitting clip
 * space here is exactly what's required i guess
 */
static const ULONG s_vsPassthrough[] =
{
    0x00000000, 0x004c2055, 0x0836186c, 0x2f0007f8,  /* MUL R0, v0.y, c[1]        */
    0x00000000, 0x008c0000, 0x0836186c, 0x1f0007f8,  /* MAD R0, v0.x, c[0], R0    */
    0x00000000, 0x008c40aa, 0x0836186c, 0x1f0007f8,  /* MAD R0, v0.z, c[2], R0    */
    0x00000000, 0x006c601b, 0x0436106c, 0x3f0007f8,  /* ADD R0, R0, c[3]          */
    0x00000000, 0x0400001b, 0x08361300, 0x101807f8,  /* RCP R1.x, R0.w            */
    0x00000000, 0x0040001b, 0x0400286c, 0x2070e800,  /* MUL o[HPOS].xyz, R0, R1.x */
    0x00000000, 0x0020061b, 0x0836106c, 0x2070f818,  /* MOV o[COL0], v[3]         */
    0x00000000, 0x0020121b, 0x0836106c, 0x2070f848,  /* MOV o[TEX0], v[9]  (q-component carries 1/w for textureProj) */
    0x00000000, 0x00200c1b, 0x0836106c, 0x2070f830,  /* MOV oPts, v[6]     (point size: gl_PointSize = oPts.x) */
    0x00000000, 0x0020001b, 0x0436106c, 0x20701801,  /* MOV o[HPOS].w, R0  (END)  */
};

/* Convert a (small) signed integer to its IEEE-754 single bit pattern without
 * using the FPU — kernel code must not touch FP registers casually. */
static ULONG
Int2Float(LONG v)
{
    ULONG sign = 0, a, mant;
    int   e;

    if (v == 0)
        return 0;
    if (v < 0) { sign = 0x80000000u; a = (ULONG)(-v); }
    else       { a = (ULONG)v; }

    e = 0;
    { ULONG t = a; while (t > 1u) { t >>= 1; e++; } }

    if (e >= 23)
        mant = (a >> (e - 23)) & 0x7FFFFFu;
    else
        mant = (a << (23 - e)) & 0x7FFFFFu;

    return sign | (((ULONG)(e + 127)) << 23) | mant;
}

/* ----- PGRAPH 3D enable + channel-0 context validity (Probably) ------ */
static VOID
Nv3dPgraphSetup(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    /* This looks like a bunch of garbage, 
     * You're right it is! This was done with XEMU's debugging capabilities and a retail
     * Xbox ROM/BIOS thing. I just needed to see how the initial GPU is seutp, but i tried to document what i could!
     */
    NvWrite32(Dx, 0x00400080, 0x00000000);  /* DEBUG_0 */
    NvWrite32(Dx, 0x00400084, 0x01118700);  /* DEBUG_1 (note: 0x0111_8700, not 0x0011_8700) */
    NvWrite32(Dx, 0x00400088, 0x00000000);
    NvWrite32(Dx, 0x0040008C, 0xF3DE0479);  /* DEBUG_3 */
    NvWrite32(Dx, 0x00400090, 0x00000000);  /* DEBUG_4 */
    NvWrite32(Dx, 0x00400094, 0x00000004);  /* DEBUG_5 */
    NvWrite32(Dx, 0x00400098, 0x00000078);
    NvWrite32(Dx, 0x0040009C, 0x00000040);
    NvWrite32(Dx, 0x00400880, 0x0008CFFF);  /* DEBUG_7 */

    NvWrite32(Dx, 0x00400140, 0xFFFFFFFF);  /* INTR_EN (store only; PMC masks delivery) */
    NvWrite32(Dx, 0x00400144, 0x10010100);  /* CTX_CONTROL: CHID_VALID + more */
    NvWrite32(Dx, 0x00400148, 0x00000001);  /* CTX_USER: CHANNEL_3D */
    NvWrite32(Dx, 0x0040014C, 0x00000000);
    NvWrite32(Dx, 0x00400150, 0x00000000);
    NvWrite32(Dx, 0x00400154, 0x00000000);
    NvWrite32(Dx, 0x00400158, 0x00000000);

    /* NOT writing 0x40071c NV_PGRAPH_INCREMENT — confirmed hard-crash trigger. */
    NvWrite32(Dx, 0x00400720, 0x00000001);  /* PGRAPH_FIFO access enable */
    NvWrite32(Dx, 0x00400750, 0x00DF0008);  /* RDI_INDEX */
    NvWrite32(Dx, 0x00400754, 0x00000001);  /* RDI_DATA  */
    NvWrite32(Dx, 0x00400764, 0x08000000);
    NvWrite32(Dx, 0x00400780, 0x0000110A);  /* CHANNEL_CTX_TABLE (store) */
    NvWrite32(Dx, 0x00400784, 0x0000111D);  /* CHANNEL_CTX_POINTER (store; NO trigger 0x788) */

    /* Raster / setup / clip block — THIS is what was missing. */
    NvWrite32(Dx, 0x00400900, 0x03628001);
    NvWrite32(Dx, 0x00400904, 0x0387FFFF);
    NvWrite32(Dx, 0x00400908, 0x00001400);
    NvWrite32(Dx, 0x00400910, 0x033D0003);
    NvWrite32(Dx, 0x00400914, 0x03627FFF);
    NvWrite32(Dx, 0x00400918, 0x00001400);
    NvWrite32(Dx, 0x00400920, 0x00000000);
    NvWrite32(Dx, 0x00400924, 0x00000000);
    NvWrite32(Dx, 0x00400928, 0x00000000);
    NvWrite32(Dx, 0x00400930, 0x00000000);
    NvWrite32(Dx, 0x00400934, 0x00000000);
    NvWrite32(Dx, 0x00400938, 0x00000000);
    NvWrite32(Dx, 0x00400940, 0x00000000);
    NvWrite32(Dx, 0x00400944, 0x00000000);
    NvWrite32(Dx, 0x00400948, 0x00000000);
    NvWrite32(Dx, 0x00400950, 0x00000000);
    NvWrite32(Dx, 0x00400954, 0x00000000);
    NvWrite32(Dx, 0x00400958, 0x00000000);
    NvWrite32(Dx, 0x00400960, 0x00000000);
    NvWrite32(Dx, 0x00400964, 0x00000000);
    NvWrite32(Dx, 0x00400968, 0x00000000);
    NvWrite32(Dx, 0x00400970, 0x00000000);
    NvWrite32(Dx, 0x00400974, 0x00000000);
    NvWrite32(Dx, 0x00400978, 0x00000000);
    NvWrite32(Dx, 0x00400980, 0x00000000);
    NvWrite32(Dx, 0x00400984, 0x84000000);
    NvWrite32(Dx, 0x00400988, 0x00000000);
    NvWrite32(Dx, 0x0040098C, 0x00000000);
    NvWrite32(Dx, 0x00400990, 0x00000000);
    NvWrite32(Dx, 0x00400994, 0x00000000);
    NvWrite32(Dx, 0x00400998, 0x00000000);
    NvWrite32(Dx, 0x0040099C, 0x00000000);
    NvWrite32(Dx, 0x004009A0, 0x00000000);
    NvWrite32(Dx, 0x004009A4, 0x03070003);
    NvWrite32(Dx, 0x004009A8, 0x11448000);

    NvWrite32(Dx, 0x00400B80, 0x45EAD10E);
    NvWrite32(Dx, 0x00400B84, 0x00000000);
    NvWrite32(Dx, 0x00400B88, 0x00000000);

    NvWrite32(Dx, 0x00400100, 0x00100000);  /* PGRAPH_INTR: clear pending */
}

BOOLEAN
Nv2a3dInitialize(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    LONG hw, hh;
    ULONG i;
    LONG  zMax;                          /* depth zmax: 2^24-1 (Z24S8) or 2^16-1 (Z16 fallback) */
    ULONG zetaFmt, zetaPitch, zetaOff;   /* surface-format zeta fields */

    if (Dx->Nv3dInitAttempted)
        return Dx->Nv3dReady;

    Dx->Nv3dInitAttempted = TRUE;
    Dx->Nv3dReady = FALSE;

    /* Needs the pushbuffer / PFIFO / RAMHT that accel bring-up established. */
    if (!Dx->AccelEnabled || Dx->PushBufferVirt == NULL)
        return FALSE;

    /* Only 32 bpp X8R8G8B8 is wired up here. */
    if (Dx->BytesPerPixel != 4 || Dx->ScreenWidth == 0 || Dx->ScreenHeight == 0)
        return FALSE;

    /* OFFSCREEN 3D colour surface.  Kelvin renders here, NOT into the visible
     * framebuffer — a 3D render surface that overlaps the VGA scanout makes
     * xemu's display double ("dual screen").  After each draw we CPU-copy this
     * offscreen surface into the visible framebuffer (CPU writes present
     * reliably, exactly like the desktop's GDI).  Depth test is disabled, so we
     * reuse the old depth region (1.2 MB, clear of the fb/vertex/pushbuffer). */
    Dx->DepthBufferGpuOffset = Dx->FrameBufferGpuOffset + 0x200000;

    /* Enable the 3D pipeline and pre-validate channel 0 so Kelvin's SET_OBJECT
     * context switch passes on the first try (no stall, no FIFO poking). */
    Nv2aAccelWaitIdle(Dx);   /* drain in-flight 2D init */
    Nv3dPgraphSetup(Dx);

    /* Kelvin object: class 0x97, ctx flags 0x00000A00 in dword1. */
    NvWritePRamin32(Dx, RAMIN_OBJ_KELVIN + 0,  NV_KELVIN_PRIMITIVE);
    NvWritePRamin32(Dx, RAMIN_OBJ_KELVIN + 4,  0x00000A00);
    NvWritePRamin32(Dx, RAMIN_OBJ_KELVIN + 8,  0);
    NvWritePRamin32(Dx, RAMIN_OBJ_KELVIN + 12, 0);
    RamhtInsert(Dx, HANDLE_KELVIN, RAMIN_OBJ_KELVIN, RAMHT_ENGINE_GR, 0);

    /* Dedicated DMA object for the 3D color/zeta surface.  xemu's GL surface
     * backend asserts the surface's DMA object is NV_DMA_IN_MEMORY_CLASS (0x3D);
     * the shared 2D framebuffer object uses class 0x3, so make a 0x3D one
     * (base 0, covering VRAM, so the absolute COLOR/ZETA offsets resolve). */
    WriteDmaObject(Dx, RAMIN_OBJ_DMA_3D, 0x0003D | (2 << 16), 0,
                   Dx->FrameBufferGpuOffset + Dx->FrameBufferLength - 1);
    RamhtInsert(Dx, HANDLE_DMA_3D, RAMIN_OBJ_DMA_3D, RAMHT_ENGINE_GR, 0);

    if (!PbReserve(Dx, 512))
        return FALSE;

    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV_SET_OBJECT, 1));
    PbPush(Dx, HANDLE_KELVIN);

    /* Render target + depth both live in the framebuffer DMA object. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CONTEXT_DMA_COLOR, 2));
    PbPush(Dx, HANDLE_DMA_3D);   /* color */
    PbPush(Dx, HANDLE_DMA_3D);   /* zeta  */

    /* Vertex fetch (DRAW_ARRAYS path) also reads from the base-0 VRAM DMA
     * object, so SET_VERTEX_DATA_ARRAY_OFFSET takes absolute VRAM offsets. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CONTEXT_DMA_VERTEX_A, 1));
    PbPush(Dx, HANDLE_DMA_3D);

    /* Texture DMA context A = base-0 VRAM object, so SET_TEXTURE_OFFSET takes
     * absolute VRAM offsets (texture FORMAT context_dma field 0 selects DMA_A). */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CONTEXT_DMA_A, 1));
    PbPush(Dx, HANDLE_DMA_3D);

    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SURFACE_CLIP_HORIZONTAL, 2));
    PbPush(Dx, (Dx->ScreenWidth  << 16));   /* x=0, width  */
    PbPush(Dx, (Dx->ScreenHeight << 16));   /* y=0, height */

    /* Depth/stencil zeta surface.  A 32-bit Z24S8 buffer (4 B/px) won't fit the
     * VRAM gap between the framebuffer and the offscreen colour surface, but the
     * NV2A is UMA and the base-0 DMA_3D object spans all RAM, so we put the zeta
     * in a contiguous system-RAM allocation (same trick as the texture pool) and
     * get full 24-bit depth precision.  Games with aggressive near/far frustums
     * (mcclone/UT use 0.1/100) z-fought badly on the old in-gap Z16.  Falls back
     * to in-gap Z16 if the allocation fails. */
    if (!Dx->DepthZetaContig)
    {
        ULONG zbytes = Dx->ScreenWidth * 4 * Dx->ScreenHeight;   /* Z24S8 pitch * height */
        PHYSICAL_ADDRESS hi; PVOID zv;
        hi.QuadPart = (LONGLONG)Dx->FrameBufferGpuOffset - 1;    /* below the FB, within the DMA limit */
        zv = VideoPortAllocateContiguousMemory(Dx, zbytes, hi);
        if (zv)
        {
            Dx->DepthZetaContig    = zv;
            Dx->DepthZetaGpuOffset = (ULONG)MmGetPhysicalAddress(zv).QuadPart;
        }
    }
    if (Dx->DepthZetaContig)
    {
        zMax      = 16777215;                         /* 2^24-1 */
        zetaFmt   = 0x00000124;                        /* TYPE_PITCH | ZETA_Z24S8(0x20) | COLOR_X8R8G8B8(0x04) */
        zetaPitch = Dx->ScreenWidth * 4;
        zetaOff   = Dx->DepthZetaGpuOffset & 0x03FFFFFF;
    }
    else
    {
        zMax      = 65535;                             /* 2^16-1 */
        zetaFmt   = 0x00000114;                        /* TYPE_PITCH | ZETA_Z16(0x10) | COLOR_X8R8G8B8(0x04) */
        zetaPitch = Dx->ScreenWidth * 2;
        zetaOff   = (Dx->FrameBufferGpuOffset +
                     ((Dx->ScreenStride * Dx->ScreenHeight + 0xFFF) & ~0xFFFUL)) & 0x03FFFFFF;
    }
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SURFACE_FORMAT, 4));
    PbPush(Dx, zetaFmt);
    PbPush(Dx, (zetaPitch << 16) | (Dx->ScreenStride & 0xFFFF)); /* zeta pitch | colour pitch */
    PbPush(Dx, Dx->DepthBufferGpuOffset & 0x03FFFFFF);   /* COLOR -> offscreen FB+0x200000 */
    PbPush(Dx, zetaOff);                                 /* ZETA -> system-RAM Z24S8 (or in-gap Z16) */

    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CONTROL0, 1));
    PbPush(Dx, 0x00110001);

    /* Viewport: map NDC [-1,1] to the full surface (origin top-left). */
    hw = (LONG)Dx->ScreenWidth  / 2;
    hh = (LONG)Dx->ScreenHeight / 2;
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_VIEWPORT_OFFSET, 4));
    PbPush(Dx, Int2Float(hw));
    PbPush(Dx, Int2Float(hh));
    PbPush(Dx, 0);
    PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_VIEWPORT_SCALE, 4));
    PbPush(Dx, Int2Float(hw));
    PbPush(Dx, Int2Float(-hh));          /* flip Y: GL bottom-left -> screen top-left */
    PbPush(Dx, Int2Float(zMax));         /* z scale = zeta zmax (Z24S8 2^24-1 / Z16 2^16-1); xemu divides oPos.z by this */
    PbPush(Dx, 0);

    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CLIP_MIN, 2));
    PbPush(Dx, 0);                       /* 0.0f */
    PbPush(Dx, Int2Float(zMax));         /* far = zeta zmax (clipRange.w clamp bound) */

    /* Minimal "draw a flat/Gouraud triangle" render state — disable the parts
     * that would blank or assert, enable colour writes + smooth shading. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_ALPHA_TEST_ENABLE, 1));   PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_ENABLE, 1));        PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CULL_FACE_ENABLE, 1));    PbPush(Dx, 0);
    /* Depth: default to enabled + LEQUAL + write (the per-draw call overrides
     * enable/func/mask from the GL state).  ZSTENCIL clear value = max (far) so a
     * cleared depth buffer lets nearer geometry pass LESS/LEQUAL. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_TEST_ENABLE, 1));   PbPush(Dx, 1);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_FUNC, 1));          PbPush(Dx, NV097_SET_DEPTH_FUNC_LEQUAL);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_MASK, 1));          PbPush(Dx, 1);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_ZSTENCIL_CLEAR_VALUE, 1)); PbPush(Dx, 0xFFFFFFFF);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DITHER_ENABLE, 1));       PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_LIGHTING_ENABLE, 1));     PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_TEST_ENABLE, 1)); PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_FOG_ENABLE, 1));          PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SPECULAR_ENABLE, 1));     PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COLOR_MASK, 1));          PbPush(Dx, 0x01010101);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADE_MODEL, 1));         PbPush(Dx, NV097_SET_SHADE_MODEL_SMOOTH);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_FRONT_POLYGON_MODE, 1));  PbPush(Dx, NV097_SET_POLYGON_MODE_FILL);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BACK_POLYGON_MODE, 1));   PbPush(Dx, NV097_SET_POLYGON_MODE_FILL);

    /* Disable all four texture stages. */
    for (i = 0; i < 4; i++)
    {
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_CONTROL0 + i * 0x40, 1));
        PbPush(Dx, 0);
    }

    /* Texture-shader stages + clip-plane mode = 0.  pbkit/nxdk set these; we
     * never did, so they were left at whatever the desktop's 2D ops / boot left
     * behind.  CRITICAL: if SET_SHADER_CLIP_PLANE_MODE is non-zero, xemu's vertex
     * shader emits gl_ClipDistance for user clip planes we never defined -> EVERY
     * fragment is clipped (CLEAR is unaffected) -> the exact "clear shows, no
     * triangle" symptom.  Zero = no texture stages, no user clip planes. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADER_STAGE_PROGRAM, 1));     PbPush(Dx, 0x00000000);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADER_OTHER_STAGE_INPUT, 1)); PbPush(Dx, 0x00000000);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADER_CLIP_PLANE_MODE, 1));   PbPush(Dx, 0x00000000);

    /* Full render-state defaults (pbkit sets these; our minimal init left them at
     * leftover values from the desktop 2D engine / boot).  CRITICAL one:
     * SET_ANTI_ALIASING_CONTROL — if AA is left enabled, xemu scales the viewport
     * & scissor by 2x/4x vs our 1x surface, mapping the triangle outside the FBO
     * (no visible fragments) while CLEAR still fills.  Force AA off + a full-screen
     * window clip + sane setup-raster/alpha defaults. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00001D7C, 1)); PbPush(Dx, 0x00000000);  /* SET_ANTI_ALIASING_CONTROL = off (1x) */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00001D78, 1)); PbPush(Dx, 0x00000001);  /* SET_ZMIN_MAX_CONTROL */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x000002B4, 1)); PbPush(Dx, 0x00000000);  /* SET_WINDOW_CLIP_TYPE */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x000002C0, 1)); PbPush(Dx, ((Dx->ScreenWidth  - 1) << 16)); /* WINDOW_CLIP_H: 0..W-1 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x000002E0, 1)); PbPush(Dx, ((Dx->ScreenHeight - 1) << 16)); /* WINDOW_CLIP_V: 0..H-1 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x0000039C, 1)); PbPush(Dx, 0x00000405);  /* SET_CULL_FACE = BACK (cull off anyway) */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x000003A0, 1)); PbPush(Dx, 0x00000901);  /* SET_FRONT_FACE = CCW (geometry reaches the rasteriser net-unflipped; GL default front is CCW, cull BACK) */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x000003A4, 1)); PbPush(Dx, 0x00000000);  /* SET_NORMALIZATION_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000318, 1)); PbPush(Dx, 0x00000000);  /* SET_POINT_PARAMS_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000320, 1)); PbPush(Dx, 0x00000000);  /* SET_LINE_SMOOTH_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000324, 1)); PbPush(Dx, 0x00000000);  /* SET_POLY_SMOOTH_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000330, 1)); PbPush(Dx, 0x00000000);  /* POLY_OFFSET_POINT_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000334, 1)); PbPush(Dx, 0x00000000);  /* POLY_OFFSET_LINE_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000338, 1)); PbPush(Dx, 0x00000000);  /* POLY_OFFSET_FILL_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x0000017BC, 1));PbPush(Dx, 0x00000000);  /* SET_LOGIC_OP_ENABLE = 0 */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x0000033C, 1)); PbPush(Dx, 0x00000207);  /* SET_ALPHA_FUNC = ALWAYS */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000340, 1)); PbPush(Dx, 0x00000000);  /* SET_ALPHA_REF = 0 */

    /* Register combiners: fragment output = interpolated diffuse colour. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_ICW, 1));        PbPush(Dx, 0x10101010);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_OCW, 1));        PbPush(Dx, 0x00000000);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_ICW, 1));        PbPush(Dx, 0x10101010);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_OCW, 1));        PbPush(Dx, 0x00000000);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_CONTROL, 1));          PbPush(Dx, 0x00000001);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 1)); PbPush(Dx, 0x00000004);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 1)); PbPush(Dx, 0x00001400);

    /* Load nxdk's VIEWPORT matrix (NDC -> window pixels) into VP constants
     * c[0..3] (nxdk maps c[0] to hardware constant index 96).  This is exactly
     * nxdk's matrix_viewport(0,0,W,H,0,65536), the matrix its known-good triangle
     * sample feeds its 8-instruction VP.  The VP computes
     *   R0   = v0.x*c[0] + v0.y*c[1] + v0.z*c[2] + c[3]
     *   oPos = (R0.xyz / R0.w, R0.w)
     * so for NDC input v0 ([-1,1], w=1):
     *   oPos = ( ndc.x*hw + hw, ndc.y*-hh + hh, ndc.z*65536, 1 )  -> WINDOW px.
     * xemu's vertex epilogue then maps those window coords back to clip space
     * (its inverse-viewport step — nxdk proves the VP must emit window coords,
     * not clip), so the triangle lands on screen.  The ICD therefore feeds NDC. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_CONSTANT_LOAD, 1));
    PbPush(Dx, 96);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_CONSTANT, 16));
    PbPush(Dx, Int2Float(hw)); PbPush(Dx, 0);              PbPush(Dx, 0);                PbPush(Dx, 0);          /* c[0]=(hw,0,0,0)   */
    PbPush(Dx, 0);             PbPush(Dx, Int2Float(-hh)); PbPush(Dx, 0);                PbPush(Dx, 0);          /* c[1]=(0,-hh,0,0)  */
    PbPush(Dx, 0);             PbPush(Dx, 0);              PbPush(Dx, Int2Float(zMax));  PbPush(Dx, 0);          /* c[2]=(0,0,zsc,0)  */
    PbPush(Dx, Int2Float(hw)); PbPush(Dx, Int2Float(hh));  PbPush(Dx, 0);                PbPush(Dx, 0x3F800000); /* c[3]=(hw,hh,0,1)  */

    /* Upload the vertex program (4 dwords / instruction per burst). */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_PROGRAM_START, 1));
    PbPush(Dx, 0);
    /* FIXED-FUNCTION transform mode (MODE=0): xemu computes the vertex transform
     * from the composite matrix in its own generated shader and does NOT execute
     * the vertex-program microcode (the program-mode path produced a byte-perfect
     * draw yet zero fragments. */
    /* PROGRAM mode (MODE=2|RANGE_PRIV).  xemu's program-mode shader does
     * gl_Position = oPos DIRECTLY (no inverse-viewport — confirmed in xemu
     * source vsh.c), so the VP must output CLIP SPACE.  With the identity matrix
     * loaded per-draw, the 8-instruction VP is a passthrough: oPos = v0.  The ICD
     * (and our diagnostic verts) supply NDC/clip coords, which land in the clip
     * volume and rasterise. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_EXECUTION_MODE, 1));
    PbPush(Dx, 0x00000006);   /* MODE=PROGRAM(2) | RANGE_MODE=PRIV(1<<2) */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_PROGRAM_CXT_WRITE_EN, 1));
    PbPush(Dx, 0);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_PROGRAM_LOAD, 1));
    PbPush(Dx, 0);
    for (i = 0; i < sizeof(s_vsPassthrough) / sizeof(s_vsPassthrough[0]); i += 4)
    {
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_PROGRAM, 4));
        PbPush(Dx, s_vsPassthrough[i + 0]);
        PbPush(Dx, s_vsPassthrough[i + 1]);
        PbPush(Dx, s_vsPassthrough[i + 2]);
        PbPush(Dx, s_vsPassthrough[i + 3]);
    }

    /* Load the VIEWPORT matrix into VP constants c[96..99] ONCE, here in init */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_CONSTANT_LOAD, 1));
    PbPush(Dx, 96);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TRANSFORM_CONSTANT, 16));
    PbPush(Dx, Int2Float(hw)); PbPush(Dx, 0);              PbPush(Dx, 0);                PbPush(Dx, 0);          /* c96=(W/2,  0,    0,  0) */
    PbPush(Dx, 0);             PbPush(Dx, Int2Float(-hh)); PbPush(Dx, 0);                PbPush(Dx, 0);          /* c97=(0,    -H/2, 0,  0) */
    PbPush(Dx, 0);             PbPush(Dx, 0);              PbPush(Dx, Int2Float(zMax));  PbPush(Dx, 0);          /* c98=(0,    0,    Zr, 0)  zeta zmax */
    PbPush(Dx, Int2Float(hw)); PbPush(Dx, Int2Float(hh));  PbPush(Dx, 0);                PbPush(Dx, 0x3F800000); /* c99=(W/2,  H/2,  0,  1) */

    /* Vertex attribute formats for the INLINE_ARRAY draw path: attr 0 = position
     * (4 floats), attr 3 = diffuse (4 floats).  Format = (stride<<8)|(size<<4)|
     * type; both attrs present => 32-byte stride. */
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + NV2A_VTX_ATTR_POSITION * 4, 1));
    PbPush(Dx, (32 << 8) | (4 << 4) | NV097_VTXFMT_TYPE_F);
    PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_VERTEX_DATA_ARRAY_FORMAT + NV2A_VTX_ATTR_DIFFUSE * 4, 1));
    PbPush(Dx, (32 << 8) | (4 << 4) | NV097_VTXFMT_TYPE_F);

    PbKick(Dx);

    /* Channel 0 was pre-validated, so the switch should pass without stalling.
     * As a safety net, if a context-switch interrupt did latch, ack it (the
     * only thing that clears the puller stall in xemu) after re-asserting
     * validity — no FIFO poking. */
    {
        ULONG spin;
        for (spin = 0; spin < 0x40000; spin++)
        {
            if (NvRead32(Dx, NV2A_PGRAPH_INTR) & 0x00001000)
            {
                NvWrite32(Dx, 0x00400148, 0x00000000);
                NvWrite32(Dx, 0x00400144, NvRead32(Dx, 0x00400144) | 0x00010000);
                NvWrite32(Dx, 0x00400100, 0x00001000);   /* ack -> pfifo retries */
            }
        }
    }
    Nv2aAccelWaitIdle(Dx);

    Dx->Nv3dReady = TRUE;
    INFO_(IHVVIDEO, "NV2A 3D (Kelvin) ready: %lux%lu, zeta @ GPU 0x%lx\n",
          Dx->ScreenWidth, Dx->ScreenHeight, Dx->DepthBufferGpuOffset);
    return TRUE;
}

/* Copy a linear A8R8G8B8 texture into the VRAM texture heap (simple bump
 * allocator, 64-byte aligned).  Returns the assigned GPU offset, 0 on failure. */
ULONG
Nv2aTexUpload(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG Width, ULONG Height, ULONG ExistingOffset, const ULONG *Pixels)
{
    ULONG bytes, offset, heapEnd;
    PUCHAR dst;

    if (!Dx->TextureHeapVirt || Width == 0 || Height == 0 || Pixels == NULL)
        return 0;
    if (Width > 4096 || Height > 4096)
        return 0;
    bytes = Width * Height * 4;
    heapEnd = Dx->TextureHeapGpuOffset + Dx->TextureHeapSize;

    /* Re-upload in place when the ICD gave us a prior slot of the same size (it
     * only does so when the dimensions are unchanged, so `bytes` matches the
     * original allocation and we can't stomp a neighbouring texture). */
    if (ExistingOffset >= Dx->TextureHeapGpuOffset &&
        ExistingOffset + bytes <= heapEnd &&
        ExistingOffset < Dx->TextureHeapNext)
    {
        dst = (PUCHAR)Dx->TextureHeapVirt + (ExistingOffset - Dx->TextureHeapGpuOffset);
        VideoPortMoveMemory(dst, (PVOID)Pixels, bytes);
        return ExistingOffset;
    }

    offset = (Dx->TextureHeapNext + 63) & ~63u;
    if (offset + bytes > heapEnd)
        return 0;   /* heap exhausted */

    dst = (PUCHAR)Dx->TextureHeapVirt + (offset - Dx->TextureHeapGpuOffset);
    VideoPortMoveMemory(dst, (PVOID)Pixels, bytes);
    Dx->TextureHeapNext = offset + bytes;
    return offset;
}

BOOLEAN
Nv2a3dDrawTriangles(PXBOXVMP_DEVICE_EXTENSION Dx, const NV2A_DRAW_3D *Draw)
{
    ULONG i;
    const NV2A_3D_VERTEX *Verts = Draw->Verts;
    ULONG Count           = Draw->VertexCount;
    ULONG Topology        = Draw->Topology ? Draw->Topology : NV097_SET_BEGIN_END_OP_TRIANGLES;
    ULONG DstX            = Draw->DstX, DstY = Draw->DstY, DstW = Draw->DstW, DstH = Draw->DstH;
    ULONG Flags           = Draw->Flags;
    ULONG ClearColor      = Draw->ClearColor;
    ULONG DepthTestEnable = Draw->DepthTestEnable;
    ULONG DepthFunc       = Draw->DepthFunc;
    ULONG DepthMask       = Draw->DepthMask;
    ULONG CullEnable      = Draw->CullEnable;
    ULONG TexEnable       = Draw->TexEnable;
    ULONG TexOffset       = Draw->TexOffset;
    ULONG TexW            = Draw->TexWidth;
    ULONG TexH            = Draw->TexHeight;
    ULONG TexFilter       = Draw->TexFilter;
    ULONG BlendEnable     = Draw->BlendEnable;
    ULONG BlendSrc        = Draw->BlendSrc;
    ULONG BlendDst        = Draw->BlendDst;
    ULONG AlphaTestEnable = Draw->AlphaTestEnable;
    ULONG AlphaFunc       = Draw->AlphaFunc;
    ULONG AlphaRef        = Draw->AlphaRef;
    ULONG PolygonMode     = Draw->PolygonMode ? Draw->PolygonMode : NV097_SET_POLYGON_MODE_FILL;
    BOOLEAN doClear      = (Flags & NV2A_DRAW3D_FLAG_CLEAR)       != 0;
    BOOLEAN doClearDepth = (Flags & NV2A_DRAW3D_FLAG_CLEAR_DEPTH) != 0;
    /* Stencil only exists with the Z24S8 zeta (DepthZetaContig); ignore otherwise. */
    BOOLEAN doClearStencil = (Flags & NV2A_DRAW3D_FLAG_CLEAR_STENCIL) != 0 && Dx->DepthZetaContig != NULL;
    BOOLEAN doPresent    = (Flags & NV2A_DRAW3D_FLAG_PRESENT)     != 0;
    BOOLEAN doDraw;

    if (!Dx->Nv3dReady)
        return FALSE;
    /* Any topology: points (>=1), lines (>=2), triangles (>=3).  The ICD already
     * expanded fans/quads/polys to triangle lists; points/lines pass through. */
    if (Count != 0 && (Count > NV2A_3D_MAX_VERTS || Verts == NULL))
        return FALSE;
    doDraw = (Count >= 1);

    if (!doClear && !doClearDepth && !doClearStencil && !doDraw && !doPresent)
        return TRUE;   /* nothing requested */

    /* Default to the whole screen, then clamp the destination rect to it. */
    if (DstW == 0 || DstH == 0)
    {
        DstX = 0; DstY = 0;
        DstW = Dx->ScreenWidth; DstH = Dx->ScreenHeight;
    }
    if (DstX >= Dx->ScreenWidth || DstY >= Dx->ScreenHeight)
        return FALSE;
    if (DstX + DstW > Dx->ScreenWidth)  DstW = Dx->ScreenWidth  - DstX;
    if (DstY + DstH > Dx->ScreenHeight) DstH = Dx->ScreenHeight - DstY;
    if (DstW == 0 || DstH == 0)
        return FALSE;

    /* One push-buffer batch.  The offscreen colour surface (FB+0x200000) is
     * PERSISTENT across draws: the ICD clears it once per frame (FLAG_CLEAR),
     * submits any number of triangle batches into it, then presents once
     * (FLAG_PRESENT).  The viewport matrix in VP constants c[96..99] is loaded
     * once in Nv2a3dInitialize (a per-draw SET_TRANSFORM_CONSTANT stalls PGRAPH). */
    if (!PbReserve(Dx, Count * 18 + 384))   /* +headroom for the stencil state block */
        return FALSE;

    if (doClear || doClearDepth || doClearStencil)
    {
        /* CLEAR_SURFACE bits: 0xF0 = colour R|G|B|A, 0x01 = Z, 0x02 = stencil. */
        ULONG clearBits = (doClear ? NV097_CLEAR_SURFACE_COLOR : 0) |
                          (doClearDepth ? 0x01u : 0) |
                          (doClearStencil ? 0x02u : 0);

        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CLEAR_RECT_HORIZONTAL, 1));
        PbPush(Dx, (DstX & 0xFFFF) | (((DstX + DstW - 1) & 0xFFFF) << 16));
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CLEAR_RECT_VERTICAL, 1));
        PbPush(Dx, (DstY & 0xFFFF) | (((DstY + DstH - 1) & 0xFFFF) << 16));
        if (doClear)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COLOR_CLEAR_VALUE, 1));
            PbPush(Dx, ClearColor & 0x00FFFFFF);
        }
        /* Depth clears to the full-range max so the first LEQUAL test passes for
         * every fragment.  On Z24S8 the low 8 bits are the stencil clear value
         * (depth stays 0xFFFFFF); on the Z16 fallback keep the plain 0xFFFFFFFF
         * (its low 16 bits = max depth, which 0xFFFFFF00 would corrupt). */
        if (doClearDepth || doClearStencil)
        {
            ULONG zsval = (Dx->DepthZetaContig != NULL)
                          ? (0xFFFFFF00u | (Draw->ClearStencil & 0xFF))
                          : 0xFFFFFFFF;
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_ZSTENCIL_CLEAR_VALUE, 1));
            PbPush(Dx, zsval);
        }
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_CLEAR_SURFACE, 1));
        PbPush(Dx, clearBits);
    }

    if (doDraw)
    {
        /* Per-batch depth state — the ICD tracks glEnable(GL_DEPTH_TEST)/glDepthFunc/
         * glDepthMask.  GL depth-func enums (0x200..0x207) map 1:1 onto the NV2A. */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_TEST_ENABLE, 1));
        PbPush(Dx, DepthTestEnable ? 1 : 0);
        if (DepthFunc >= 0x200 && DepthFunc <= 0x207)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_FUNC, 1));
            PbPush(Dx, DepthFunc);
        }
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DEPTH_MASK, 1));
        PbPush(Dx, DepthMask ? 1 : 0);

        /* Back-face culling (glEnable(GL_CULL_FACE)).  FRONT_FACE=CCW + CULL=BACK
         * are set in init (the geometry reaches xemu's rasteriser net-unflipped,
         * so GL's default CCW front winding holds), so culling BACK removes the
         * true back faces — without this their dark (N.L<0) shading bled through. */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_CULL_FACE_ENABLE, 1));
        PbPush(Dx, CullEnable ? 1 : 0);

        /* Alpha blending (glEnable(GL_BLEND) + glBlendFunc).  NV2A blend-factor
         * values are identical to the GL blend enums, so pass them straight. */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_ENABLE, 1));
        PbPush(Dx, BlendEnable ? 1 : 0);
        if (BlendEnable)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_FUNC_SFACTOR, 1));
            PbPush(Dx, BlendSrc);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_FUNC_DFACTOR, 1));
            PbPush(Dx, BlendDst);
            /* GL 1.2 imaging: glBlendEquation + glBlendColor (constant-colour factors).
             * Draw->BlendEquation carries the raw GL enum; ADD(0x8006)/MIN(0x8007)/
             * MAX(0x8008) share the NV2A value, SUBTRACT/REVERSE_SUBTRACT remap. */
            {
                ULONG nvEq;
                switch (Draw->BlendEquation)
                {
                    case 0x800A: nvEq = NV097_SET_BLEND_EQUATION_V_FUNC_SUBTRACT; break;         /* GL_FUNC_SUBTRACT */
                    case 0x800B: nvEq = NV097_SET_BLEND_EQUATION_V_FUNC_REVERSE_SUBTRACT; break; /* GL_FUNC_REVERSE_SUBTRACT */
                    case 0x8007: nvEq = NV097_SET_BLEND_EQUATION_V_MIN; break;                   /* GL_MIN */
                    case 0x8008: nvEq = NV097_SET_BLEND_EQUATION_V_MAX; break;                   /* GL_MAX */
                    default:     nvEq = NV097_SET_BLEND_EQUATION_V_FUNC_ADD; break;              /* GL_FUNC_ADD / unset */
                }
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_EQUATION, 1));
                PbPush(Dx, nvEq);
            }
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BLEND_COLOR, 1));
            PbPush(Dx, Draw->BlendColor);
        }

        /* Alpha test (glAlphaFunc) — GL alpha-func enums map 1:1 to the NV2A. */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_ALPHA_TEST_ENABLE, 1));
        PbPush(Dx, AlphaTestEnable ? 1 : 0);
        if (AlphaTestEnable)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x0000033C, 1));   /* SET_ALPHA_FUNC */
            PbPush(Dx, (AlphaFunc >= 0x200 && AlphaFunc <= 0x207) ? AlphaFunc : 0x207);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, 0x00000340, 1));   /* SET_ALPHA_REF */
            PbPush(Dx, AlphaRef & 0xFF);
        }

        /* Stencil — only with the Z24S8 zeta (Z16 has no stencil bits).  GL func
         * (0x200..0x207) and op enums match the NV2A, so pass them through. */
        if (Dx->DepthZetaContig != NULL)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_TEST_ENABLE, 1));
            PbPush(Dx, Draw->StencilEnable ? 1 : 0);
            if (Draw->StencilEnable)
            {
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_FUNC, 1));
                PbPush(Dx, (Draw->StencilFunc >= 0x200 && Draw->StencilFunc <= 0x207) ? Draw->StencilFunc : 0x207);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_FUNC_REF, 1));
                PbPush(Dx, Draw->StencilRef & 0xFF);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_FUNC_MASK, 1));
                PbPush(Dx, Draw->StencilFuncMask & 0xFF);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_MASK, 1));
                PbPush(Dx, Draw->StencilWriteMask & 0xFF);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_OP_FAIL, 1));
                PbPush(Dx, Draw->StencilOpFail);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_OP_ZFAIL, 1));
                PbPush(Dx, Draw->StencilOpZFail);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_STENCIL_OP_ZPASS, 1));
                PbPush(Dx, Draw->StencilOpZPass);
            }
        }

        /* Polygon fill mode (glPolygonMode: point/line/fill). */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_FRONT_POLYGON_MODE, 1));
        PbPush(Dx, PolygonMode);
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BACK_POLYGON_MODE, 1));
        PbPush(Dx, PolygonMode);

        /* Polygon offset (glPolygonOffset).  Disabled by default so ordinary draws
         * are untouched; when an app enables it we push the per-mode enables plus
         * the slope factor and constant bias (passed through as IEEE float bits). */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_POLY_OFFSET_FILL_ENABLE, 1));
        PbPush(Dx, (Draw->PolyOffsetEnable & 1) ? 1 : 0);
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_POLY_OFFSET_LINE_ENABLE, 1));
        PbPush(Dx, (Draw->PolyOffsetEnable & 2) ? 1 : 0);
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_POLY_OFFSET_POINT_ENABLE, 1));
        PbPush(Dx, (Draw->PolyOffsetEnable & 4) ? 1 : 0);
        if (Draw->PolyOffsetEnable)
        {
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_POLYGON_OFFSET_SCALE_FACTOR, 1));
            PbPush(Dx, Draw->PolyOffsetFactor);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_POLYGON_OFFSET_BIAS, 1));
            PbPush(Dx, Draw->PolyOffsetUnits);
        }

        if (TexEnable && TexW && TexH)
        {
            /* Bind a linear A8R8G8B8 texture to stage 0 (DMA context A = VRAM base 0,
             * set in init).  texcoords arrive in TEXEL units; xemu divides by the GL
             * texture size to normalise, so [0,W]x[0,H] -> [0,1]. */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_OFFSET, 1));
            PbPush(Dx, TexOffset);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_FORMAT, 1));
            PbPush(Dx, (2u << 4) | (NV097_TEXFMT_COLOR_LU_IMAGE_A8R8G8B8 << 8) | (1u << 16));
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_ADDRESS, 1));
            PbPush(Dx, Draw->TexAddress ? Draw->TexAddress : 0x00030303);  /* glTexParameteri wrap (U|V<<8|P<<16) */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_CONTROL0, 1));
            PbPush(Dx, NV097_SET_TEXTURE_CONTROL0_ENABLE);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_CONTROL1, 1));
            PbPush(Dx, (TexW * 4) << 16);   /* row pitch in bytes */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_FILTER, 1));
            PbPush(Dx, TexFilter ? TexFilter : 0x01010000);  /* honour glTexParameteri (mag<<24|min<<16) */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_IMAGE_RECT, 1));
            PbPush(Dx, (TexW << 16) | (TexH & 0xFFFF));

            /* Texture-shader stage 0 = PROJECT2D so the fragment shader actually
             * samples texture0 (init leaves all stages NONE). */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADER_STAGE_PROGRAM, 1));
            PbPush(Dx, 0x00000001);

            /* Register combiners selected by glTexEnv mode.  Combiner input byte =
             * (mapping<<5)|(alpha<<4)|source (source: 1=const0, 4=diffuse, 8=tex0;
             * mapping 1<<5 = 1-x invert); ICW packs A<<24|B<<16|C<<8|D.  Colour OCW
             * 0xC00 routes the AB+CD sum -> R0 (a lerp), 0xC0 routes AB -> R0.
             *   MODULATE: R0 = tex0 * diffuse
             *   REPLACE : final = tex0 (ignore vertex colour)
             *   DECAL   : rgb = diffuse*(1-tex.a) + tex.rgb*tex.a ; a = diffuse.a
             *   BLEND   : rgb = diffuse*(1-tex.rgb) + envcol*tex.rgb ; a = diffuse.a*tex.a
             * These are partly guesswork! I don't really know what I'm doing here. */
            {
                ULONG cIcw, cOcw, aIcw, aOcw, fCw0, fCw1;
                switch (Draw->TexEnvMode)
                {
                    case 2: /* DECAL */
                        cIcw=0x04380818; cOcw=0x00000C00; aIcw=0x14300000; aOcw=0x000000C0; fCw0=0x0000000C; fCw1=0x00001C00;
                        break;
                    case 3: /* BLEND */
                        cIcw=0x04280108; cOcw=0x00000C00; aIcw=0x14180000; aOcw=0x000000C0; fCw0=0x0000000C; fCw1=0x00001C00;
                        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_FACTOR0, 1)); PbPush(Dx, Draw->TexEnvColor);
                        break;
                    case 1: /* REPLACE */
                        cIcw=0x08040000; cOcw=0x000000C0; aIcw=0x18140000; aOcw=0x000000C0; fCw0=0x00000008; fCw1=0x00001800;
                        break;
                    default: /* MODULATE */
                        cIcw=0x08040000; cOcw=0x000000C0; aIcw=0x18140000; aOcw=0x000000C0; fCw0=0x0000000C; fCw1=0x00001C00;
                        break;
                }
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_ICW, 1));        PbPush(Dx, cIcw);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_OCW, 1));        PbPush(Dx, cOcw);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_ICW, 1));        PbPush(Dx, aIcw);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_OCW, 1));        PbPush(Dx, aOcw);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 1)); PbPush(Dx, fCw0);
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 1)); PbPush(Dx, fCw1);
            }
        }
        else
        {
            /* Untextured: disable stage 0 texture, restore the diffuse-only combiner. */
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXTURE_CONTROL0, 1));          PbPush(Dx, 0);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_SHADER_STAGE_PROGRAM, 1));      PbPush(Dx, 0);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_ICW, 1));        PbPush(Dx, 0x10101010);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_COLOR_OCW, 1));        PbPush(Dx, 0x00000000);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_ICW, 1));        PbPush(Dx, 0x10101010);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_ALPHA_OCW, 1));        PbPush(Dx, 0x00000000);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW0, 1)); PbPush(Dx, 0x00000004);
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COMBINER_SPECULAR_FOG_CW1, 1)); PbPush(Dx, 0x00001400);
        }

        /* Colour write mask (glColorMask).  ColorMask packs alpha<<24|red<<16|
         * green<<8|blue (each byte 0/1); 0x01010101 = all channels on, 0 = none
         * (a legit depth/stencil-only pass).  The ICD always sets it explicitly.
         * Also doubles as the per-draw "is_nop_draw" guard register. */
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_COLOR_MASK, 1));
        PbPush(Dx, Draw->ColorMask);

        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BEGIN_END, 1));
        PbPush(Dx, Topology);
        for (i = 0; i < Count; i++)
        {
            const NV2A_3D_VERTEX *v = &Verts[i];
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_DIFFUSE_COLOR4F, 4));
            PbPush(Dx, *(const ULONG *)&v->r);
            PbPush(Dx, *(const ULONG *)&v->g);
            PbPush(Dx, *(const ULONG *)&v->b);
            PbPush(Dx, *(const ULONG *)&v->a);
            if (Topology == NV097_SET_BEGIN_END_OP_POINTS && Draw->PointSize)
            {
                /* Point size via the PTSIZE attribute (VP copies v6->oPts.x).  MUST be
                 * set per-vertex: xemu only "populates" an inline attribute once a
                 * vertex already exists, so a single set before the batch is dropped. */
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_PTSIZE_4F, 4));
                PbPush(Dx, Draw->PointSize);
                PbPush(Dx, 0);
                PbPush(Dx, 0);
                PbPush(Dx, 0x3F800000);
            }
            if (TexEnable && TexW && TexH)
            {
                /* 4-float texcoord (u,v,0,q): u,v are texel*1/w and q=1/w, both
                 * computed by the ICD.  PROJECT2D's textureProj divides u,v by the
                 * interpolated q => hardware perspective-correct texture mapping. */
                PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_TEXCOORD0_4F, 4));
                PbPush(Dx, *(const ULONG *)&v->u);
                PbPush(Dx, *(const ULONG *)&v->v);
                PbPush(Dx, 0);
                PbPush(Dx, *(const ULONG *)&v->q);
            }
            PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_VERTEX4F, 4));
            PbPush(Dx, *(const ULONG *)&v->x);
            PbPush(Dx, *(const ULONG *)&v->y);
            PbPush(Dx, *(const ULONG *)&v->z);
            PbPush(Dx, *(const ULONG *)&v->w);
        }
        PbPush(Dx, NV2A_METHOD(SUBCH_3D, NV097_SET_BEGIN_END, 1));
        PbPush(Dx, NV097_SET_BEGIN_END_OP_END);
    }

    if (doPresent)
    {
        /* HW 2D IMAGE_BLIT, offscreen (FB+0x200000) -> visible fb (FB+0), dst rect. */
        PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_FORMAT, 4));
        PbPush(Dx, Dx->SurfaceFormat);
        PbPush(Dx, (Dx->SurfacePitch & 0xFFFF) | ((Dx->SurfacePitch & 0xFFFF) << 16));
        PbPush(Dx, Dx->FrameBufferGpuOffset + 0x200000); /* OFFSET_SRC = offscreen */
        PbPush(Dx, Dx->FrameBufferGpuOffset);            /* OFFSET_DST = visible   */
        PbPush(Dx, NV2A_METHOD(SUBCH_SURF2D, NV04_SURFACES_2D_SET_DMA_IMAGE_SRC, 2));
        PbPush(Dx, HANDLE_DMA_FB);
        PbPush(Dx, HANDLE_DMA_FB);

        PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_SET_CONTEXT_SURFACES, 1));
        PbPush(Dx, HANDLE_SURFACES_2D);
        PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_OPERATION, 1));
        PbPush(Dx, NV_IMAGE_BLIT_OPERATION_SRCCOPY);
        PbPush(Dx, NV2A_METHOD(SUBCH_BLIT, NV_IMAGE_BLIT_POINT_IN, 3));
        PbPush(Dx, (DstX & 0xFFFF) | ((DstY & 0xFFFF) << 16));
        PbPush(Dx, (DstX & 0xFFFF) | ((DstY & 0xFFFF) << 16));
        PbPush(Dx, (DstW & 0xFFFF) | ((DstH & 0xFFFF) << 16));
    }

    PbKick(Dx);
    Nv2aAccelWaitIdle(Dx);
    return TRUE;
}

/* Lazily map the offscreen colour surface (FB+0x200000) for CPU read/write. */
static ULONG *Nv2aOffscreenMap(PXBOXVMP_DEVICE_EXTENSION Dx)
{
    PHYSICAL_ADDRESS phys;
    ULONG len, io = VIDEO_MEMORY_SPACE_MEMORY;
    if (Dx->OffscreenVirt)
        return (ULONG *)Dx->OffscreenVirt;
    if (!Dx->ScreenWidth || !Dx->ScreenHeight || !Dx->ScreenStride)
        return NULL;
    len = Dx->ScreenStride * Dx->ScreenHeight;
    phys.QuadPart = Dx->PhysFrameBufferStart.QuadPart + Dx->DepthBufferGpuOffset;
    if (VideoPortMapMemory(Dx, phys, &len, &io, &Dx->OffscreenVirt) != NO_ERROR)
        Dx->OffscreenVirt = NULL;
    return (ULONG *)Dx->OffscreenVirt;
}

/* Read a Width*Height ARGB rect at (X,Y) of the offscreen surface into Out
 * (row stride = Width; pixels outside the surface read as 0). */
BOOLEAN
Nv2aReadback(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG X, ULONG Y, ULONG Width, ULONG Height, ULONG *Out)
{
    ULONG *surf, row, col, pitchPx;
    if (!Dx->Nv3dReady || !Out || !Width || !Height)
        return FALSE;
    surf = Nv2aOffscreenMap(Dx);
    if (!surf)
        return FALSE;
    Nv2aAccelWaitIdle(Dx);   /* prior GPU draws into the offscreen must be complete */
    pitchPx = Dx->ScreenStride / 4;
    for (row = 0; row < Height; row++)
        for (col = 0; col < Width; col++)
        {
            ULONG sx = X + col, sy = Y + row;
            Out[row * Width + col] = (sx < Dx->ScreenWidth && sy < Dx->ScreenHeight)
                                     ? surf[sy * pitchPx + sx] : 0;
        }
    return TRUE;
}

/* Write a Width*Height ARGB rect (row stride = Width) to (X,Y) of the offscreen
 * surface; with SKIP_TRANSPARENT, pixels with alpha 0 are left unchanged. */
BOOLEAN
Nv2aWriteback(PXBOXVMP_DEVICE_EXTENSION Dx, ULONG X, ULONG Y, ULONG Width, ULONG Height,
              ULONG Flags, const ULONG *Pixels)
{
    ULONG *surf, row, col, pitchPx;
    BOOLEAN skipT = (Flags & NV2A_WRITEBACK_FLAG_SKIP_TRANSPARENT) != 0;
    if (!Dx->Nv3dReady || !Pixels || !Width || !Height)
        return FALSE;
    surf = Nv2aOffscreenMap(Dx);
    if (!surf)
        return FALSE;
    Nv2aAccelWaitIdle(Dx);   /* don't race a pending GPU draw into the same surface */
    pitchPx = Dx->ScreenStride / 4;
    for (row = 0; row < Height; row++)
        for (col = 0; col < Width; col++)
        {
            ULONG sx = X + col, sy = Y + row, px = Pixels[row * Width + col];
            if (sx >= Dx->ScreenWidth || sy >= Dx->ScreenHeight)
                continue;
            if (skipT && (px >> 24) == 0)
                continue;
            surf[sy * pitchPx + sx] = px;
        }
    return TRUE;
}

/* EOF */
