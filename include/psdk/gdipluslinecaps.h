/*
 * GdiPlusLineCaps.h
 *
 * Windows GDI+
 *
 * This file is part of the w32api package.
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef _GDIPLUSLINECAPS_H
#define _GDIPLUSLINECAPS_H

inline
CustomLineCap::CustomLineCap(const GraphicsPath *fillPath,
  const GraphicsPath *strokePath, LineCap baseCap, REAL baseInset)
{
}

inline CustomLineCap *
CustomLineCap::Clone(VOID)
{
  return NULL;
}

inline LineCap
CustomLineCap::GetBaseCap(VOID)
{
  return LineCapFlat;
}

inline REAL
CustomLineCap::GetBaseInset(VOID)
{
  return 0;
}

inline Status
CustomLineCap::GetLastStatus(VOID)
{
  return Ok;
}

inline Status
CustomLineCap::GetStrokeCaps(LineCap *startCap, LineCap *endCap)
{
  return Ok;
}

inline LineJoin
CustomLineCap::GetStrokeJoin(VOID)
{
  return LineJoinMiter;
}

inline REAL
CustomLineCap::GetWidthScale(VOID)
{
  return 0;
}

inline Status
CustomLineCap::SetBaseCap(LineCap baseCap)
{
  return Ok;
}

inline Status
CustomLineCap::SetBaseInset(REAL inset)
{
  return Ok;
}

inline Status
CustomLineCap::SetStrokeCap(LineCap strokeCap)
{
  return Ok;
}

inline Status
CustomLineCap::SetStrokeCaps(LineCap startCap, LineCap endCap)
{
  return Ok;
}

inline Status
CustomLineCap::SetStrokeJoin(LineJoin lineJoin)
{
  return Ok;
}

inline Status
CustomLineCap::SetWidthScale(IN REAL widthScale)
{
  return Ok;
}


class AdjustableArrowCap : public CustomLineCap
{
public:
  AdjustableArrowCap(REAL height, REAL width, BOOL isFilled)
  {
  }

  REAL GetHeight(VOID)
  {
    return 0;
  }

  REAL GetMiddleInset(VOID)
  {
    return 0;
  }

  REAL GetWidth(VOID)
  {
    return 0;
  }

  BOOL IsFilled(VOID)
  {
    return FALSE;
  }

  Status SetFillState(BOOL isFilled)
  {
    return Ok;
  }

  Status SetHeight(REAL height)
  {
    return Ok;
  }

  Status SetMiddleInset(REAL middleInset)
  {
    return Ok;
  }

  Status SetWidth(REAL width)
  {
    return Ok;
  }
};

#endif /* _GDIPLUSLINECAPS_H */
