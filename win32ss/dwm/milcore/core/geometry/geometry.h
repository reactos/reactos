// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#ifndef GEOMETRY_H
#define GEOMETRY_H

// Forward declarations
class CShapeBase;
class CShape;
class IFigureData;
class IFigureBuilder;
class CFigureBase;
class CFigure;
class CBezier;
class CPenGeometry;
class CPen;
class CWideningSink;
class CWidener;
class CPenInterface;
class CSimplePen;
class CFigureTask;
class CFlatteningSink;
class CBounds;
class CPlainPen;
class CLineShape;
class CScanner;
class CFillTessellator;
class CTessellator;
class CAnimationPath;
class CStartMarker;
class CEndMarker;
class CParallelogram;
class CLooseRectClip;

#include "utils.h"
#include "BaseTypes.h"
#include "ExactArithmetic.h"
#include "LineSegmentIntersection.h"
#include "IntervalArithmetic.h"
#include "populationsink.h"
#include "GeometrySink.h"
#include "BezierD.h"
#include "BezierFlattener.h"
#include "ShapeBuilder.h"
#include "Scanner.h"
#include "Boolean.h"
#include "FigureTask.h"
#include "ShapeFlattener.h"
#include "AnimationPath.h"
#include "ShapeData.h"
#include "ShapeBase.h"
#include "FigureBase.h"
#include "figure.h"
#include "shape.h"
#include "CompactShapes.h"
#include "FillTessellator.h"
#include "Tessellate.h"
#include "cpen.h"
#include "strokefigure.h"
#include "LineShape.h"
#include "bezier.h"
#include "Area.h"
#include "StripClipper.h"
#include "AxisAlignedStripClipper.h"
#include "PopulationSinkAdapter.h"

#endif


