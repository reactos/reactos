/*
 * SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
 * Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice including the dates of first publication and
 * either this permission notice or a reference to
 * http://oss.sgi.com/projects/FreeB/
 * shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Silicon Graphics, Inc.
 * shall not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization from
 * Silicon Graphics, Inc.
 */
/*
*/

#ifndef _DATA_TRANSFORM_H
#define _DATA_TRANSFORM_H

#include "reader.h"
#include "directedLine.h"
#include "bin.h"
directedLine* bin_to_DLineLoops(Bin& bin);

/*transform the pwlcurve into a number of directedline lines
 *insert these directedlines into orignal which is supposed to be
 *the part of the trimming loop obtained so far.
 *return the updated trimkming loop.
 */
directedLine* o_pwlcurve_to_DLines(directedLine* original, O_pwlcurve* pwl);

/*transform a trim loop (curve) into a directedLine loop
 */
directedLine* o_curve_to_DLineLoop(O_curve* curve);

/*transform a list of trim loops (trim) into
 *a list of polygons represented as directedLine*.
 */
directedLine* o_trim_to_DLineLoops(O_trim* trim);


#endif

