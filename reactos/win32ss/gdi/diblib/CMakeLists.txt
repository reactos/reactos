

list(APPEND DIBLIB_SOURCE
    BitBlt.c
    BitBlt_DSTINVERT.c
    BitBlt_MERGECOPY.c
    BitBlt_MERGEPAINT.c
    BitBlt_NOTPATCOPY.c
    BitBlt_NOTSRCCOPY.c
    BitBlt_NOTSRCERASE.c
    BitBlt_other.c
    BitBlt_PATCOPY.c
    BitBlt_PATINVERT.c
    BitBlt_PATPAINT.c
    BitBlt_SRCAND.c
    BitBlt_SRCCOPY.c
    BitBlt_SRCERASE.c
    BitBlt_SRCINVERT.c
    BitBlt_SRCPAINT.c
    DibLib.c
    MaskBlt.c
    MaskCopy.c
    MaskPaint.c
    MaskPatBlt.c
    MaskPatPaint.c
    MaskSrcBlt.c
    MaskSrcPaint.c
    MaskSrcPatBlt.c
    PatPaint.c
    RopFunctions.c
    SrcPaint.c
    SrcPatBlt.c
)

add_library(diblib ${DIBLIB_SOURCE})

