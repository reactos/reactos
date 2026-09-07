// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


!include ..\..\sources.inc

SOURCES_USED=$(SOURCES_USED) $(WGX_PATH)\core\geometry\sources.inc

MSC_WARNING_LEVEL=$(MSC_WARNING_LEVEL:W4=W3)

SOURCES=\
    ..\utils.cpp \
    ..\cpen.cpp\
    ..\FigureBase.cpp\
    ..\figure.cpp\
    ..\FillTessellator.cpp\
    ..\shapeBase.cpp \
    ..\shape.cpp \
    ..\strokefigure.cpp \
    ..\bezier.cpp\
    ..\Scanner.cpp\
    ..\Tessellate.cpp\
    ..\Boolean.cpp\
    ..\FigureTask.cpp\
    ..\AnimationPath.cpp\
    ..\Area.cpp\
    ..\ExactArithmetic.cpp\
    ..\LineSegmentIntersection.cpp\
    ..\CompactShapes.cpp\
    ..\BezierD.cpp\
    ..\BezierFlattener.cpp\
    ..\StripClipper.cpp\
    ..\AxisAlignedStripClipper.cpp

