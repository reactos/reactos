/****************************************************************************
*   Copyright (C) 1991-2004 SciTech Software, Inc. All rights reserved.
*
*   Permission is hereby granted, free of charge, to any person obtaining a
*   copy of this software and associated documentation files (the "Software"),
*   to deal in the Software without restriction, including without limitation
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,
*   and/or sell copies of the Software, and to permit persons to whom the
*   Software is furnished to do so, subject to the following conditions:
*
*   The above copyright notice and this permission notice shall be included
*   in all copies or substantial portions of the Software.
*
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
*   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
*   SCITECH SOFTWARE INC BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
*   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
*   OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*   SOFTWARE.
****************************************************************************/

#include "opengl32.h"
#include <math.h>

#define LINE_BUF_QUANT 4000
#define VERT_BUF_QUANT 4000

static HFONT    hNewFont, hOldFont;
static FLOAT    ScaleFactor;
static FLOAT*   LineBuf;
static DWORD    LineBufSize;
static DWORD    LineBufIndex;
static FLOAT*   VertBuf;
static DWORD    VertBufSize;
static DWORD    VertBufIndex;
static GLenum   TessErrorOccurred;

/*****************************************************************************
* AppendToLineBuf
*
* Appends one floating-point value to the global LineBuf array.  Return value
* is non-zero for success, zero for failure.
*****************************************************************************/

INT AppendToLineBuf(FLOAT value)
{
    if (LineBufIndex >= LineBufSize)
    {
        FLOAT* f;
        LineBufSize += LINE_BUF_QUANT;

        f = (FLOAT*) HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  LineBuf, (LineBufSize) * sizeof(FLOAT));
        if (!f)
            return 0;
        LineBuf = f;
    }
    LineBuf[LineBufIndex++] = value;
    return 1;
}

/*****************************************************************************
* AppendToVertBuf
*
* Appends one floating-point value to the global VertBuf array.  Return value
* is non-zero for success, zero for failure.
*
* Note that we can't realloc this one, because the tessellator is using
* pointers into it.
*****************************************************************************/

INT AppendToVertBuf(FLOAT value)
{
    if (VertBufIndex >= VertBufSize)
        return 0;
    VertBuf[VertBufIndex++] = value;
    return 1;
}

/*****************************************************************************
* GetWord
*
* Fetch the next 16-bit word from a little-endian byte stream, and increment
* the stream pointer to the next unscanned byte.
*****************************************************************************/

LONG GetWord(UCHAR** p)
{
    LONG value;

    value = ((*p)[1] << 8) + (*p)[0];
    *p += 2;
    return value;
}

/*****************************************************************************
* GetDWord
*
* Fetch the next 32-bit word from a little-endian byte stream, and increment
* the stream pointer to the next unscanned byte.
*****************************************************************************/

LONG GetDWord(UCHAR** p)
{
    LONG value;

    value = ((*p)[3] << 24) + ((*p)[2] << 16) + ((*p)[1] << 8) + (*p)[0];
    *p += 4;
    return value;
}

/*****************************************************************************
* GetFixed
*
* Fetch the next 32-bit fixed-point value from a little-endian byte stream,
* convert it to floating-point, and increment the stream pointer to the next
* unscanned byte.
*****************************************************************************/
double GetFixed(UCHAR** p)
{
    LONG hiBits, loBits;
    double value;

    loBits = GetWord(p);
    hiBits = GetWord(p);
    value = (double) ((hiBits << 16) | loBits) / 65536.0;

    return value * ScaleFactor;
}


/*****************************************************************************
**
** InvertGlyphBitmap.
**
** Invert the bitmap so that it suits OpenGL's representation.
** Each row starts on a double word boundary.
**
*****************************************************************************/

VOID InvertGlyphBitmap(INT w, INT h, DWORD *fptr, DWORD *tptr)
{
    INT dWordsInRow = (w+31)/32;
    INT i, j;

    if (w <= 0 || h <= 0) {
        return;
    }

    tptr += ((h-1)*dWordsInRow);
    for (i = 0; i < h; i++) {
        for (j = 0; j < dWordsInRow; j++) {
            *(tptr + j) = *(fptr + j);
        }
        tptr -= dWordsInRow;
        fptr += dWordsInRow;
    }
}

/*****************************************************************************
* CreateHighResolutionFont
*
* Gets metrics for the current font and creates an equivalent font
* scaled to the design units of the font.
* 
*****************************************************************************/

HFONT CreateHighResolutionFont(HDC hDC)
{
    UINT otmSize;
    OUTLINETEXTMETRIC *otm;
    LONG fontHeight, fontWidth, fontUnits;
    LOGFONTW logFont, logFontFaceName;

    otmSize = GetOutlineTextMetricsW(hDC, 0, NULL);
    if (!otmSize)
        return NULL;

    otm = (OUTLINETEXTMETRIC *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, otmSize);
    if (!otm)
        return NULL;

    otm->otmSize = otmSize;
    if (!GetOutlineTextMetricsW(hDC, otmSize, otm)) 
        return NULL;

    GetObjectW(GetCurrentObject(hDC, OBJ_FONT), sizeof(logFontFaceName), &logFontFaceName);

    fontHeight = otm->otmTextMetrics.tmHeight -
        otm->otmTextMetrics.tmInternalLeading;
    fontWidth = otm->otmTextMetrics.tmAveCharWidth;
    fontUnits = (LONG) otm->otmEMSquare;

    ScaleFactor = 1.0F / (FLOAT) fontUnits;

    logFont.lfHeight = - ((LONG) fontUnits);
    logFont.lfWidth = (LONG)((FLOAT) (fontWidth * fontUnits) / (FLOAT) fontHeight);
    logFont.lfEscapement = 0;
    logFont.lfOrientation = 0;
    logFont.lfWeight = otm->otmTextMetrics.tmWeight;
    logFont.lfItalic = otm->otmTextMetrics.tmItalic;
    logFont.lfUnderline = otm->otmTextMetrics.tmUnderlined;
    logFont.lfStrikeOut = otm->otmTextMetrics.tmStruckOut;
    logFont.lfCharSet = otm->otmTextMetrics.tmCharSet;
    logFont.lfOutPrecision = OUT_OUTLINE_PRECIS;
    logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    logFont.lfQuality = DEFAULT_QUALITY;
    logFont.lfPitchAndFamily =
        otm->otmTextMetrics.tmPitchAndFamily & 0xf0;
    wcscpy(logFont.lfFaceName, logFontFaceName.lfFaceName);

    hNewFont = CreateFontIndirectW(&logFont);

    HeapFree(GetProcessHeap(), 0, otm);

    return hNewFont;
}

/*****************************************************************************
* MakeLinesFromArc
*
* Subdivides one arc of a quadratic spline until the chordal deviation
* tolerance requirement is met, then places the resulting set of line
* segments in the global LineBuf.
*****************************************************************************/
INT MakeLinesFromArc(FLOAT x0, FLOAT y0, FLOAT x1, FLOAT y1, FLOAT x2, FLOAT y2,
                     DWORD vertexCountIndex, FLOAT chordalDeviationSquared)
{
    FLOAT x01;
    FLOAT y01;
    FLOAT x12;
    FLOAT y12;
    FLOAT midPointX;
    FLOAT midPointY;
    FLOAT deltaX;
    FLOAT deltaY;

    /*
    * Calculate midpoint of the curve by de Casteljau:
    */
    x01 = 0.5F * (x0 + x1);
    y01 = 0.5F * (y0 + y1);
    x12 = 0.5F * (x1 + x2);
    y12 = 0.5F * (y1 + y2);
    midPointX = 0.5F * (x01 + x12);
    midPointY = 0.5F * (y01 + y12);


    /*
    * Estimate chordal deviation by the distance from the midpoint
    * of the curve to its non-pointpolated control point.  If this
    * distance is greater than the specified chordal deviation
    * constraint, then subdivide.  Otherwise, generate polylines
    * from the three control points.
    */
    deltaX = midPointX - x1;
    deltaY = midPointY - y1;

    if (deltaX * deltaX + deltaY * deltaY > chordalDeviationSquared)
    {
        MakeLinesFromArc(	x0, y0,
            x01, y01,
            midPointX, midPointY,
            vertexCountIndex,
            chordalDeviationSquared);

        MakeLinesFromArc(	midPointX, midPointY,
            x12, y12,
            x2, y2,
            vertexCountIndex,
            chordalDeviationSquared);
    }
    else
    {
        /*
        * The "pen" is already at (x0, y0), so we don't need to
        * add that point to the LineBuf.
        */
        if (!AppendToLineBuf(x1)
            || !AppendToLineBuf(y1)
            || !AppendToLineBuf(x2)
            || !AppendToLineBuf(y2))
            return 0;
        LineBuf[vertexCountIndex] += 2.0F;
    }

    return 1;
}

/*****************************************************************************
* MakeLinesFromTTQSpline
*
* Converts points from the poly quadratic spline in a TT_PRIM_QSPLINE
* structure to polyline points in the global LineBuf.
*****************************************************************************/

INT MakeLinesFromTTQSpline( UCHAR** pp, DWORD vertexCountIndex, WORD pointCount, FLOAT chordalDeviation)
{
    FLOAT x0, y0, x1, y1, x2, y2;
    WORD point;

    /*
    * Process each of the non-pointpolated points in the outline.
    * To do this, we need to generate two pointpolated points (the
    * start and end of the arc) for each non-pointpolated point.
    * The first pointpolated point is always the one most recently
    * stored in LineBuf, so we just extract it from there.  The
    * second pointpolated point is either the average of the next
    * two points in the QSpline, or the last point in the QSpline
    * if only one remains.
    */
    for (point = 0; point < pointCount - 1; ++point)
    {
        x0 = LineBuf[LineBufIndex - 2];
        y0 = LineBuf[LineBufIndex - 1];

        x1 = (FLOAT) GetFixed(pp);
        y1 = (FLOAT) GetFixed(pp);

        if (point == pointCount - 2)
        {
            /*
            * This is the last arc in the QSpline.  The final
            * point is the end of the arc.
            */
            x2 = (FLOAT) GetFixed(pp);
            y2 = (FLOAT) GetFixed(pp);
        }
        else
        {
            /*
            * Peek at the next point in the input to compute
            * the end of the arc:
            */
            x2 = 0.5F * (x1 + (FLOAT) GetFixed(pp));
            y2 = 0.5F * (y1 + (FLOAT) GetFixed(pp));
            /*
            * Push the point back onto the input so it will
            * be reused as the next off-curve point:
            */
            *pp -= 8;
        }

        if (!MakeLinesFromArc(	x0, y0,
            x1, y1,
            x2, y2,
            vertexCountIndex,
            chordalDeviation * chordalDeviation))
            return 0;
    }

    return 1;
}

/*****************************************************************************
* MakeLinesFromTTLine
*
* Converts points from the polyline in a TT_PRIM_LINE structure to
* equivalent points in the global LineBuf.
*****************************************************************************/
INT MakeLinesFromTTLine(UCHAR** pp, DWORD vertexCountIndex, WORD pointCount)
{
    /*
    * Just copy the line segments into the line buffer (converting
    * type as we go):
    */
    LineBuf[vertexCountIndex] += pointCount;
    while (pointCount--)
    {
        if (!AppendToLineBuf((FLOAT) GetFixed(pp))	/* X coord */
            || !AppendToLineBuf((FLOAT) GetFixed(pp)))	/* Y coord */
            return 0;
    }

    return 1;
}

/*****************************************************************************
* MakeLinesFromTTPolyCurve
*
* Converts the lines and splines in a single TTPOLYCURVE structure to points
* in the global LineBuf.
*****************************************************************************/

INT MakeLinesFromTTPolycurve(UCHAR** pp, DWORD vertexCountIndex, FLOAT chordalDeviation)
{
    WORD type;
    WORD pointCount;

    /*
    * Pick up the relevant fields of the TTPOLYCURVE structure:
    */
    type = (WORD) GetWord(pp);
    pointCount = (WORD) GetWord(pp);

    /*
    * Convert the "curve" to line segments:
    */
    if (type == TT_PRIM_LINE)
        return MakeLinesFromTTLine(	pp,
        vertexCountIndex,
        pointCount);
    else if (type == TT_PRIM_QSPLINE)
        return MakeLinesFromTTQSpline(	pp,
        vertexCountIndex,
        pointCount,
        chordalDeviation);
    else
        return 0;
}

/*****************************************************************************
* MakeLinesFromTTPolygon
*
* Converts a TTPOLYGONHEADER and its associated curve structures into a
* single polyline loop in the global LineBuf.
*****************************************************************************/

INT MakeLinesFromTTPolygon(UCHAR** pp, FLOAT chordalDeviation)
{
    DWORD polySize;
    UCHAR* polyStart;
    DWORD vertexCountIndex;

    /*
    * Record where the polygon data begins, and where the loop's
    * vertex count resides:
    */
    polyStart = *pp;
    vertexCountIndex = LineBufIndex;
    if (!AppendToLineBuf(0.0F))
        return 0;

    /*
    * Extract relevant data from the TTPOLYGONHEADER:
    */
    polySize = GetDWord(pp);
    if (GetDWord(pp) != TT_POLYGON_TYPE)	/* polygon type */
        return 0;
    if (!AppendToLineBuf((FLOAT) GetFixed(pp)))	/* first X coord */
        return 0;
    if (!AppendToLineBuf((FLOAT) GetFixed(pp)))	/* first Y coord */
        return 0;
    LineBuf[vertexCountIndex] += 1.0F;

    /*
    * Process each of the TTPOLYCURVE structures in the polygon:
    */
    while (*pp < polyStart + polySize)
        if (!MakeLinesFromTTPolycurve(	pp,
            vertexCountIndex,
            chordalDeviation))
            return 0;

    return 1;
}

/*****************************************************************************
* TessVertexOut
*
* Used by tessellator to handle output vertexes.
*****************************************************************************/

VOID CALLBACK TessVertexOutData(FLOAT p[3], GLfloat *pz)
{
    GLfloat v[3];
    v[0] = (GLfloat) p[0];
    v[1] = (GLfloat) p[1];
    v[2] = *pz;
    glVertex3fv(v);
}

/*****************************************************************************
* TessCombine
*
* Used by tessellator to handle self-pointsecting contours and degenerate
* geometry.
*****************************************************************************/
VOID CALLBACK TessCombine(double  coords[3], VOID* vertex_data[4], FLOAT weight[4], VOID** outData)
{
    if (!AppendToVertBuf((FLOAT) coords[0])
        || !AppendToVertBuf((FLOAT) coords[1])
        || !AppendToVertBuf((FLOAT) coords[2]))
        TessErrorOccurred = GL_OUT_OF_MEMORY;

    *outData = VertBuf + (VertBufIndex - 3);
}

/*****************************************************************************
* TessError
*
* Saves the last tessellator error code in the global TessErrorOccurred.
*****************************************************************************/

VOID CALLBACK TessError(GLenum error)
{
    TessErrorOccurred = error;
}

/*****************************************************************************
* MakeLinesFromGlyph
* 
* Converts the outline of a glyph from the TTPOLYGON format to a simple
* array of floating-point values containing one or more loops.
*
* The first element of the output array is a count of the number of loops.
* The loop data follows this count.  Each loop consists of a count of the
* number of vertices it contains, followed by the vertices.  Each vertex
* is an X and Y coordinate.  For example, a single triangle might be
* described by this array:
*
*   1.,	3.,	0., 0.,		1., 0.,		0., 1.
*       ^	 ^	 ^    ^		 ^    ^		 ^    ^
*     #loops	#verts	 x1   y1	 x2   y2	 x3   y3
*
* A two-loop glyph would look like this:
*
*	2.,	3.,  0.,0.,  1.,0.,  0.,1.,	3.,  .2,.2,  .4,.2,  .2,.4
*
* Line segments from the TTPOLYGON are transferred to the output array in
* the obvious way.  Quadratic splines in the TTPOLYGON are converted to
* collections of line segments
*****************************************************************************/

INT MakeLinesFromGlyph(UCHAR* glyphBuf, DWORD glyphSize, FLOAT chordalDeviation)
{
    UCHAR* p;
    INT status = 0;

    /*
    * Pick up all the polygons (aka loops) that make up the glyph:
    */
    if (!AppendToLineBuf(0.0F)) /* loop count at LineBuf[0] */
        goto exit;

    p = glyphBuf;
    while (p < glyphBuf + glyphSize)
    {
        if (!MakeLinesFromTTPolygon(&p, chordalDeviation))
            goto exit;
        LineBuf[0] += 1.0F; /* increment loop count */
    }

    status = 1;

exit:
    return status;
}

/*****************************************************************************
* DrawGlyph
* 
* Converts the outline of a glyph to OpenGL drawing primitives, tessellating
* as needed, and then draws the glyph.  Tessellation of the quadratic splines
* in the outline is controlled by "chordalDeviation", and the drawing
* primitives (lines or polygons) are selected by "format".
*
* Return value is nonzero for success, zero for failure.
*
* Does not check for OpenGL errors, so if the caller needs to know about them,
* it should call glGetError().
*****************************************************************************/

INT DrawGlyph(UCHAR* glyphBuf, DWORD glyphSize, FLOAT chordalDeviation, FLOAT extrusion, INT format)
{
    INT status = 0;
    FLOAT* p;
    DWORD loop;
    DWORD point;
    GLUtesselator* tess = NULL;

    /*
    * Initialize the global buffer into which we place the outlines:
    */
    LineBuf = (FLOAT*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (LINE_BUF_QUANT) * sizeof(FLOAT));

    if(!LineBuf)
        goto exit;

    LineBufSize = LINE_BUF_QUANT;
    LineBufIndex = 0;

    /*
    * Convert the glyph outlines to a set of polyline loops.
    * (See MakeLinesFromGlyph() for the format of the loop data
    * structure.)
    */
    if (!MakeLinesFromGlyph(glyphBuf, glyphSize, chordalDeviation))
        goto exit;
    p = LineBuf;


    /*
    * Now draw the loops in the appropriate format:
    */
    if (format == WGL_FONT_LINES)
    {
        /*
        * This is the easy case.  Just draw the outlines.
        */
        for (loop = (DWORD) *p++; loop; --loop)
        {
            glBegin(GL_LINE_LOOP);
            for (point = (DWORD) *p++; point; --point)
            {
                glVertex2fv(p);
                p += 2;
            }
            glEnd();
        }
        status = 1;
    }

    else if (format == WGL_FONT_POLYGONS)
    {
        double v[3];
        FLOAT *save_p = p;
        GLfloat z_value;

        /*
        * This is the hard case.  We have to set up a tessellator
        * to convert the outlines into a set of polygonal
        * primitives, which the tessellator passes to some
        * auxiliary routines for drawing.
        */

        VertBuf = (FLOAT*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (VERT_BUF_QUANT) * sizeof(FLOAT));

        if (!VertBuf)
            goto exit;

        VertBufSize = VERT_BUF_QUANT;
        VertBufIndex = 0;

        if (!(tess = gluNewTess()))
            goto exit;

        gluTessCallback(tess, GLU_BEGIN,	(VOID(CALLBACK *)()) glBegin);
        gluTessCallback(tess, GLU_TESS_VERTEX_DATA, (VOID(CALLBACK *)()) TessVertexOutData);
        gluTessCallback(tess, GLU_END,	(VOID(CALLBACK *)()) glEnd);
        gluTessCallback(tess, GLU_ERROR,	(VOID(CALLBACK *)()) TessError);
        gluTessCallback(tess, GLU_TESS_COMBINE, (VOID(CALLBACK *)()) TessCombine);
        gluTessNormal(tess, 0.0F, 0.0F, 1.0F);

        TessErrorOccurred = 0;
        glNormal3f(0.0f, 0.0f, 1.0f);
        v[2] = 0.0;
        z_value = 0.0f;

        gluTessBeginPolygon(tess, &z_value);

        for (loop = (DWORD) *p++; loop; --loop)
        {
            gluTessBeginContour(tess);

            for (point = (DWORD) *p++; point; --point)
            {
                v[0] = p[0];
                v[1] = p[1];
                gluTessVertex(tess, v, p);
                p += 2;
            }

            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);

        status = !TessErrorOccurred;

        /* Extrusion code */
        if (extrusion) 
        {
            DWORD loops;
            GLfloat thickness = (GLfloat) - extrusion;
            FLOAT *vert, *vert2;
            DWORD count;

            p = save_p;
            loops = (DWORD) *p++;

            for (loop = 0; loop < loops; loop++)
            {
                GLfloat dx, dy, len;
                DWORD last;

                count = (DWORD) *p++;
                glBegin(GL_QUAD_STRIP);

                /* Check if the first and last vertex are identical
                * so we don't draw the same quad twice.
                */
                vert = p + (count-1)*2;
                last = (p[0] == vert[0] && p[1] == vert[1]) ? count-1 : count;

                for (point = 0; point <= last; point++)
                {
                    vert  = p + 2 * (point % last);
                    vert2 = p + 2 * ((point+1) % last);

                    dx = vert[0] - vert2[0];
                    dy = vert[1] - vert2[1];
                    len = (GLfloat)sqrt(dx * dx + dy * dy);

                    glNormal3f(dy / len, -dx / len, 0.0f);
                    glVertex3f((GLfloat) vert[0],
                        (GLfloat) vert[1], thickness);
                    glVertex3f((GLfloat) vert[0],
                        (GLfloat) vert[1], 0.0f);
                }

                glEnd();
                p += count*2;
            }

            /* Draw the back face */
            p = save_p;
            v[2] = thickness;
            glNormal3f(0.0f, 0.0f, -1.0f);
            gluTessNormal(tess, 0.0F, 0.0F, -1.0F);

            gluTessBeginPolygon(tess, &thickness);

            for (loop = (DWORD) *p++; loop; --loop)
            {
                count = (DWORD) *p++;

                gluTessBeginContour(tess);

                for (point = 0; point < count; point++)
                {
                    vert = p + ((count-point-1)<<1);
                    v[0] = vert[0];
                    v[1] = vert[1];
                    gluTessVertex(tess, v, vert);
                }
                p += count*2;

                gluTessEndContour(tess);
            }
            gluTessEndPolygon(tess);
        }

#if !defined(NDEBUG)
        if (TessErrorOccurred)
            DBGPRINT("Tessellation error %s\n", gluErrorString(TessErrorOccurred));
#endif
    }


exit:

    if(LineBuf)
        HeapFree(GetProcessHeap(), 0, LineBuf);

    if(VertBuf)
        HeapFree(GetProcessHeap(), 0, VertBuf);

    if (tess)
        gluDeleteTess(tess);

    return status;
}


/*****************************************************************************
* MakeDisplayListFromGlyph
* 
* Converts the outline of a glyph to an OpenGL display list.
*
* Return value is nonzero for success, zero for failure.
*
* Does not check for OpenGL errors, so if the caller needs to know about them,
* it should call glGetError().
*****************************************************************************/

INT MakeDisplayListFromGlyph(DWORD listName, UCHAR* glyphBuf, DWORD glyphSize, LPGLYPHMETRICSFLOAT glyphMetricsFloat,
                             FLOAT chordalDeviation, FLOAT extrusion, INT format)
{
    INT status;

    glNewList(listName, GL_COMPILE);
        status = DrawGlyph(glyphBuf, glyphSize, chordalDeviation, extrusion, format);
        glTranslatef(glyphMetricsFloat->gmfCellIncX, glyphMetricsFloat->gmfCellIncY, 0.0F);
    glEndList();

    return status;
}

// ***********************************************************************

/*****************************************************************************
* IntUseFontBitmaps
*
* Converts a subrange of the glyphs in a GDI font to OpenGL display
* lists.
*
* Extended to support any GDI font, not just TrueType fonts. (DaveM)
*
*****************************************************************************/

BOOL APIENTRY IntUseFontBitmapsW(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
    INT i, ox, oy, ix, iy;
    INT w = 0, h = 0;
    INT iBufSize, iCurBufSize = 0;
    DWORD *bitmapBuffer = NULL;
    DWORD *invertedBitmapBuffer = NULL;
    BOOL bSuccessOrFail = TRUE;
    BOOL bTrueType = FALSE;
    TEXTMETRIC tm;
    GLYPHMETRICS gm;
    RASTERIZER_STATUS rs;
    MAT2 mat;
    SIZE size;
    RECT rect;
    HDC hDCMem;
    HBITMAP hBitmap;
    BITMAPINFO bmi;
    HFONT hFont;

    // Set up a unity matrix.
    ZeroMemory(&mat, sizeof(mat));
    mat.eM11.value = 1;
    mat.eM22.value = 1;

    // Test to see if selected font is TrueType or not
    ZeroMemory(&tm, sizeof(tm));
    if (!GetTextMetrics(hDC, &tm))
    {
        DBGPRINT("Font metrics error\n");
        return FALSE;
    }
    bTrueType = (tm.tmPitchAndFamily & TMPF_TRUETYPE) ? TRUE : FALSE;

    // Test to see if TRUE-TYPE capabilities are installed
    // (only necessary if TrueType font selected)
    ZeroMemory(&rs, sizeof(rs));

    if (bTrueType)
    {
        if (!GetRasterizerCaps (&rs, sizeof (RASTERIZER_STATUS)) || !(rs.wFlags & TT_ENABLED))
        {
            DBGPRINT("No TrueType caps\n");
            bTrueType = FALSE;
        }
    }

    // Trick to get the current font handle
    hFont = SelectObject(hDC, GetStockObject(SYSTEM_FONT));
    SelectObject(hDC, hFont);

    // Have memory device context available for holding bitmaps of font glyphs
    hDCMem = CreateCompatibleDC(hDC);
    SelectObject(hDCMem, hFont);
    SetTextColor(hDCMem, RGB(0xFF, 0xFF, 0xFF));
    SetBkColor(hDCMem, 0);

    for (i = first; (DWORD) i < (first + count); i++)
    {
        // Find out how much space is needed for the bitmap so we can
        // Set the buffer size correctly.
        if (bTrueType)
        {
            // Use TrueType support to get bitmap size of glyph
            iBufSize = GetGlyphOutline(hDC, i, GGO_BITMAP, &gm, 0, NULL, &mat);
            if (iBufSize == GDI_ERROR)
            {
                bSuccessOrFail = FALSE;
                break;
            }
        }
        else
        {
            // Use generic GDI support to compute bitmap size of glyph
            w = tm.tmMaxCharWidth;
            h = tm.tmHeight;
            if (GetTextExtentPoint32(hDC, (LPCTSTR)&i, 1, &size))
            {
                w = size.cx;
                h = size.cy;
            }
            iBufSize = w * h;
            // Use DWORD multiple for compatibility
            iBufSize += 3;
            iBufSize /= 4;
            iBufSize *= 4;
        }

        // If we need to allocate Larger Buffers, then do so - but allocate
        // An extra 50 % so that we don't do too many mallocs !
        if (iBufSize > iCurBufSize)
        {
            if (bitmapBuffer)
            {
                HeapFree(GetProcessHeap(), 0, bitmapBuffer);
            }
            if (invertedBitmapBuffer)
            {
                HeapFree(GetProcessHeap(), 0, invertedBitmapBuffer);
            }

            iCurBufSize = iBufSize * 2;
            bitmapBuffer = (DWORD *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, iCurBufSize);
            invertedBitmapBuffer = (DWORD *) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, iCurBufSize);

            if (bitmapBuffer == NULL || invertedBitmapBuffer == NULL)
            {
                bSuccessOrFail = FALSE;
                break;
            }
        }

        // If we fail to get the Glyph data, delete the display lists
        // Created so far and return FALSE.
        if (bTrueType)
        {
            // Use TrueType support to get bitmap of glyph
            if (GetGlyphOutline(hDC, i, GGO_BITMAP, &gm, iBufSize, bitmapBuffer, &mat) == GDI_ERROR)
            {
                    bSuccessOrFail = FALSE;
                    break;
            }

            // Setup glBitmap parameters for current font glyph
            w  = gm.gmBlackBoxX;
            h  = gm.gmBlackBoxY;
            ox = gm.gmptGlyphOrigin.x;
            oy = gm.gmptGlyphOrigin.y;
            ix = gm.gmCellIncX;
            iy = gm.gmCellIncY;
        }
        else
        {
            // Use generic GDI support to create bitmap of glyph
            ZeroMemory(bitmapBuffer, iBufSize);

            if (i >= tm.tmFirstChar && i <= tm.tmLastChar)
            {
                // Only create bitmaps for actual font glyphs
                hBitmap = CreateBitmap(w, h, 1, 1, NULL);
                SelectObject(hDCMem, hBitmap);
                // Make bitmap of current font glyph
                SetRect(&rect, 0, 0, w, h);
                DrawText(hDCMem, (LPCTSTR)&i, 1, &rect,
                    DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_NOCLIP);
                // Make copy of bitmap in our local buffer
                ZeroMemory(&bmi, sizeof(bmi));
                bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmiHeader.biWidth = w;
                bmi.bmiHeader.biHeight = -h;
                bmi.bmiHeader.biPlanes = 1;
                bmi.bmiHeader.biBitCount = 1;
                bmi.bmiHeader.biCompression = BI_RGB;
                GetDIBits(hDCMem, hBitmap, 0, h, bitmapBuffer, &bmi, 0);
                DeleteObject(hBitmap);
            }
            else 
            {
                // Otherwise use empty display list for non-existing glyph
                iBufSize = 0;
            }

            // Setup glBitmap parameters for current font glyph
            ox = 0;
            oy = tm.tmDescent;
            ix = w;
            iy = 0;
        }

        // Create an OpenGL display list.
        glNewList((listBase + i), GL_COMPILE);

        // Some fonts have no data for the space character, yet advertise
        // a non-zero size.
        if (0 == iBufSize)
        {
            glBitmap(0, 0, 0.0f, 0.0f, (GLfloat) ix, (GLfloat) iy, NULL);
        }
        else
        {
            // Invert the Glyph data.
            InvertGlyphBitmap(w, h, bitmapBuffer, invertedBitmapBuffer);

            // Render an OpenGL bitmap and invert the origin.
            glBitmap(w, h,
                (GLfloat) ox, (GLfloat) (h-oy),
                (GLfloat) ix, (GLfloat) iy,
                (GLubyte *) invertedBitmapBuffer);
        }

        // Close this display list.
        glEndList();
    }

    if (bSuccessOrFail == FALSE)
    {
        DBGPRINT("DGL_UseFontBitmaps: Get glyph failed\n");
        glDeleteLists((i+listBase), (i-first));
    }

    // Release resources used
    DeleteObject(hFont);
    DeleteDC(hDCMem);

    if (bitmapBuffer)
        HeapFree(GetProcessHeap(), 0, bitmapBuffer);

    if (invertedBitmapBuffer)
        HeapFree(GetProcessHeap(), 0, invertedBitmapBuffer);

    return(bSuccessOrFail);
}

BOOL APIENTRY IntUseFontBitmapsA(HDC hDC, DWORD first, DWORD count, DWORD listBase)
{
    /* Just call IntUseFontBitmapsW for now */
    return IntUseFontBitmapsW(hDC, first, count, listBase);
}



/*****************************************************************************
* IntUseFontOutlines
*
* Converts a subrange of the glyphs in a TrueType font to OpenGL display
* lists.
*****************************************************************************/

BOOL APIENTRY IntUseFontOutlinesW(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT chordalDeviation,
                                 FLOAT extrusion, INT format, GLYPHMETRICSFLOAT *glyphMetricsFloatArray)
{
    DWORD  glyphIndex;
    UCHAR* glyphBuf;
    DWORD  glyphBufSize;

    /*
    * Flush any previous OpenGL errors.  This allows us to check for
    * new errors so they can be reported via the function return value.
    */
    while (glGetError() != GL_NO_ERROR);

    /*
    * Make sure that the current font can be sampled accurately.
    */
    hNewFont = CreateHighResolutionFont(hDC);

    if (!hNewFont)
        return FALSE;

    hOldFont = SelectObject(hDC, hNewFont);
    if (!hOldFont)
        return FALSE;

    /*
    * Preallocate a buffer for the outline data, and track its size:
    */
    glyphBuf = (UCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,  glyphBufSize = 10240);

    if (!glyphBuf)
        return FALSE; /*WGL_STATUS_NOT_ENOUGH_MEMORY*/

    /*
    * Process each glyph in the given range:
    */
    for (glyphIndex = first; glyphIndex - first < count; ++glyphIndex)
    {
        GLYPHMETRICS glyphMetrics;
        DWORD glyphSize;
        static MAT2 matrix =
        {
            {0, 1},    {0, 0},
            {0, 0},    {0, 1}
        };
        LPGLYPHMETRICSFLOAT glyphMetricsFloat = &glyphMetricsFloatArray[glyphIndex - first];

        /*
        * Determine how much space is needed to store the glyph's
        * outlines.  If our glyph buffer isn't large enough,
        * resize it.
        */

        glyphSize = GetGlyphOutline(hDC, glyphIndex, GGO_NATIVE, &glyphMetrics, 0, NULL, &matrix);

        if (glyphSize == GDI_ERROR)
            return FALSE; /*WGL_STATUS_FAILURE*/

        if (glyphSize > glyphBufSize)
        {
            HeapFree(GetProcessHeap(), 0, glyphBuf);
            glyphBuf = (UCHAR*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, glyphBufSize = glyphSize);
            if (!glyphBuf)
                return FALSE; /*WGL_STATUS_NOT_ENOUGH_MEMORY*/
        }


        /*
        * Get the glyph's outlines.
        */
        if (GetGlyphOutline(hDC, glyphIndex, GGO_NATIVE, &glyphMetrics, glyphBufSize, glyphBuf, &matrix) == GDI_ERROR)
        {
            HeapFree(GetProcessHeap(), 0, glyphBuf);
            return FALSE; /*WGL_STATUS_FAILURE*/
        }

        glyphMetricsFloat->gmfBlackBoxX =
            (FLOAT) glyphMetrics.gmBlackBoxX * ScaleFactor;
        glyphMetricsFloat->gmfBlackBoxY =
            (FLOAT) glyphMetrics.gmBlackBoxY * ScaleFactor;
        glyphMetricsFloat->gmfptGlyphOrigin.x =
            (FLOAT) glyphMetrics.gmptGlyphOrigin.x * ScaleFactor;
        glyphMetricsFloat->gmfptGlyphOrigin.y =
            (FLOAT) glyphMetrics.gmptGlyphOrigin.y * ScaleFactor;
        glyphMetricsFloat->gmfCellIncX =
            (FLOAT) glyphMetrics.gmCellIncX * ScaleFactor;
        glyphMetricsFloat->gmfCellIncY =
            (FLOAT) glyphMetrics.gmCellIncY * ScaleFactor;

        /*
        * Turn the glyph into a display list:
        */
        if (!MakeDisplayListFromGlyph((glyphIndex - first) + listBase, glyphBuf, glyphSize, glyphMetricsFloat,
                                       chordalDeviation + ScaleFactor, extrusion, format))
        {
            HeapFree(GetProcessHeap(), 0, glyphBuf);
            return FALSE; /*WGL_STATUS_FAILURE*/
        }
    }

    /*
    * Clean up temporary storage and return.  If an error occurred,
    * clear all OpenGL error flags and return FAILURE status;
    * otherwise just return SUCCESS.
    */
    HeapFree(GetProcessHeap(), 0, glyphBuf);

    DeleteObject(SelectObject(hDC, hOldFont));

    if (glGetError() == GL_NO_ERROR)
    {
        return TRUE; /*WGL_STATUS_SUCCESS*/
    }
    else
    {
        while (glGetError() != GL_NO_ERROR);

        return FALSE; /*WGL_STATUS_FAILURE*/
    }
}

BOOL APIENTRY IntUseFontOutlinesA(HDC hDC, DWORD first, DWORD count, DWORD listBase, FLOAT chordalDeviation,
                                 FLOAT extrusion, INT format, GLYPHMETRICSFLOAT *glyphMetricsFloatArray)
{
    /* Just call IntUseFontOutlinesW for now */
    return IntUseFontOutlinesW(hDC, first, count, listBase, chordalDeviation, extrusion, format, glyphMetricsFloatArray);
}
