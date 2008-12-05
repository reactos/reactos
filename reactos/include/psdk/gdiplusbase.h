/*
 * GdiPlusBase.h
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

#ifndef _GDIPLUSBASE_H
#define _GDIPLUSBASE_H

class GdiplusBase {
public:
  void operator delete(void *in_pVoid)
  {
    DllExports::GdipFree(in_pVoid);
  }

  void operator delete[](void *in_pVoid)
  {
    DllExports::GdipFree(in_pVoid);
  }

  void *operator new(size_t in_size)
  {
    return DllExports::GdipAlloc(in_size);
  }

  void *operator new[](size_t in_size)
  {
    return DllExports::GdipAlloc(in_size);
  }
};

#endif /* _GDIPLUSBASE_H */
