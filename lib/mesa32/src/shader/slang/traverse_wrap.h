/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 2005  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file traverse_wrap.h
 * Handy TIntermTraverser class wrapper
 * \author Michal Krol
 */

#ifndef __TRAVERSE_WRAP_H__
#define __TRAVERSE_WRAP_H__

#include "Include/intermediate.h"

/*
	The original TIntermTraverser class that is used to walk the intermediate tree,
	is not very elegant in its design. One must define static functions with
	appropriate prototypes, construct TIntermTraverser object, and set its member
	function pointers to one's static functions. If traversal-specific data
	is needed, a new class must be derived, and one must up-cast the object
	passed as a parameter to the static function.

	The class below eliminates this burden by providing virtual methods that are
	to be overridden in the derived class.
*/

class traverse_wrap: private TIntermTraverser
{
private:
	static void _visitSymbol (TIntermSymbol *S, TIntermTraverser *T) {
		static_cast<traverse_wrap *> (T)->Symbol (*S);
	}
    static void _visitConstantUnion (TIntermConstantUnion *U, TIntermTraverser *T) {
		static_cast<traverse_wrap *> (T)->ConstantUnion (*U);
	}
    static bool _visitBinary (bool preVisit, TIntermBinary *B, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Binary (preVisit, *B);
	}
    static bool _visitUnary (bool preVisit, TIntermUnary *U, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Unary (preVisit, *U);
	}
    static bool _visitSelection (bool preVisit, TIntermSelection *S, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Selection (preVisit, *S);
	}
    static bool _visitAggregate (bool preVisit, TIntermAggregate *A, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Aggregate (preVisit, *A);
	}
    static bool _visitLoop (bool preVisit, TIntermLoop *L, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Loop (preVisit, *L);
	}
    static bool _visitBranch (bool preVisit, TIntermBranch *B, TIntermTraverser *T) {
		return static_cast<traverse_wrap *> (T)->Branch (preVisit, *B);
	}
public:
	traverse_wrap () {
		visitSymbol = _visitSymbol;
		visitConstantUnion = _visitConstantUnion;
		visitBinary = _visitBinary;
		visitUnary = _visitUnary;
		visitSelection = _visitSelection;
		visitAggregate = _visitAggregate;
		visitLoop = _visitLoop;
		visitBranch = _visitBranch;
	}
protected:
	virtual void Symbol (const TIntermSymbol &) {
	}
	virtual void ConstantUnion (const TIntermConstantUnion &) {
	}
	virtual bool Binary (bool, const TIntermBinary &) {
		return true;
	}
    virtual bool Unary (bool, const TIntermUnary &) {
		return true;
	}
    virtual bool Selection (bool, const TIntermSelection &) {
		return true;
	}
    virtual bool Aggregate (bool, const TIntermAggregate &) {
		return true;
	}
    virtual bool Loop (bool, const TIntermLoop &) {
		return true;
	}
    virtual bool Branch (bool, const TIntermBranch &) {
		return true;
	}
};

#endif

