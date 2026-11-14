/* Shared, size-dependent card silhouette for cards.dll and CardLib.
 * Coordinates are logical DC coordinates, including clipping and scaling. */
#pragma once

/* About 1/14 of the short edge: 5 px at 72x112, 8 px at 110x198. */
static __inline INT CardCornerRadius(INT width, INT height)
{
    INT side = min(width, height);
    return min(side / 2, max(1, (side + 7) / 14));
}

/* Sample the corner circle at pixel centres, symmetrically on all four sides. */
static __inline INT CardCornerInset(INT radius, INT row)
{
    INT inset = 0;
    INT dx = 2 * radius - 1;
    INT dy = 2 * (radius - row) - 1;
    while (dx * dx + dy * dy > 4 * radius * radius)
    {
        ++inset;
        dx -= 2;
    }
    return inset;
}

/* Save the caller's clip, then remove only the pixels outside the silhouette. */
static __inline INT ClipCardShape(HDC dc, INT x, INT y, INT width, INT height, INT radius)
{
    INT row, inset, saved = SaveDC(dc);
    if (!saved)
        return 0;
    radius = min(radius, min(width, height) / 2);
    for (row = 0; row < radius; ++row)
    {
        inset = CardCornerInset(radius, row);
        if (!inset)
            continue;
        ExcludeClipRect(dc, x, y + row, x + inset, y + row + 1);
        ExcludeClipRect(dc, x + width - inset, y + row, x + width, y + row + 1);
        ExcludeClipRect(dc, x, y + height - row - 1, x + inset, y + height - row);
        ExcludeClipRect(dc, x + width - inset, y + height - row - 1, x + width, y + height - row);
    }
    return saved;
}

static __inline VOID ExcludeCardShape(HDC dc, INT x, INT y, INT width, INT height, INT radius)
{
    INT row, inset;
    radius = min(radius, min(width, height) / 2);
    ExcludeClipRect(dc, x, y + radius, x + width, y + height - radius);
    for (row = 0; row < radius; ++row)
    {
        inset = CardCornerInset(radius, row);
        ExcludeClipRect(dc, x + inset, y + row, x + width - inset, y + row + 1);
        ExcludeClipRect(dc, x + inset, y + height - row - 1, x + width - inset, y + height - row);
    }
}

static __inline VOID BltCardShape(HDC dc, INT x, INT y, INT width, INT height,
                                  HDC source, INT sx, INT sy, INT radius)
{
    INT saved = ClipCardShape(dc, x, y, width, height, radius);
    if (!saved)
        return;
    BitBlt(dc, x, y, width, height, source, sx, sy, SRCCOPY);
    RestoreDC(dc, saved);
}

/* Outline the new curve, so cropping the original rectangular border does not
 * leave the corners without an edge. Called when preparing the card bitmaps. */
static __inline VOID FrameCardShape(HDC dc, INT x, INT y, INT width, INT height,
                                    INT radius, COLORREF color)
{
    INT row, inset, previous, length;
    HBRUSH brush = CreateSolidBrush(color);
    HGDIOBJ old;
    if (!brush)
        return;
    old = SelectObject(dc, brush);
    radius = min(radius, min(width, height) / 2);
    previous = width / 2;
    for (row = 0; row < radius; ++row)
    {
        inset = CardCornerInset(radius, row);
        length = row ? max(1, previous - inset) : width - 2 * inset;
        PatBlt(dc, x + inset, y + row, length, 1, PATCOPY);
        PatBlt(dc, x + inset, y + height - row - 1, length, 1, PATCOPY);
        if (row)
        {
            PatBlt(dc, x + width - inset - length, y + row, length, 1, PATCOPY);
            PatBlt(dc, x + width - inset - length, y + height - row - 1, length, 1, PATCOPY);
        }
        previous = inset;
    }
    PatBlt(dc, x, y + radius, 1, height - 2 * radius, PATCOPY);
    PatBlt(dc, x + width - 1, y + radius, 1, height - 2 * radius, PATCOPY);
    SelectObject(dc, old);
    DeleteObject(brush);
}
