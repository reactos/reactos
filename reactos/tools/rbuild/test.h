/*
 * Copyright (C) 2005 Casper S. Hornstrup
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __TEST_H
#define __TEST_H

#include "rbuild.h"
#include "backend/mingw/mingw.h"

#define SSEP DEF_SSEP

#define RBUILD_BASE "tools" SSEP "rbuild" SSEP
	
class BaseTest
{
public:
	bool Failed;
	BaseTest ();
	virtual ~BaseTest ();
	virtual void Run () = 0;
protected:
	void Assert ( const char *message,
	              ... );
	void IsNull ( void* reference,
	              const char* file,
	              int line );
	void IsNotNull ( void* reference,
	                 const char* file,
	                 int line );
	void IsTrue ( bool condition,
	              const char* file,
	              int line );
	void IsFalse ( bool condition,
	               const char* file,
	               int line );
	void AreEqual ( int expected,
	                int actual,
	                const char* file,
	                int line );
	void AreEqual ( const std::string& expected,
	                const std::string& actual,
	                const char* file,
	                int line );
	void AreNotEqual ( int expected,
	                   int actual,
	                   const char* file,
	                   int line );
private:
	void Fail ();
};

#define IS_NULL(reference) IsNull((void*)reference,__FILE__,__LINE__)
#define IS_NOT_NULL(reference) IsNotNull((void*)reference,__FILE__,__LINE__)
#define IS_TRUE(condition) IsTrue(condition,__FILE__,__LINE__)
#define IS_FALSE(condition) IsFalse(condition,__FILE__,__LINE__)
#define ARE_EQUAL(expected,actual) AreEqual(expected,actual,__FILE__,__LINE__)
#define ARE_NOT_EQUAL(expected,actual) AreNotEqual(expected,actual,__FILE__,__LINE__)

class ProjectTest : public BaseTest
{
public:
	void Run ();
};


class ModuleTest : public BaseTest
{
public:
	void Run ();
};


class DefineTest : public BaseTest
{
public:
	void Run ();
};


class IncludeTest : public BaseTest
{
public:
	void Run ();
};


class InvokeTest : public BaseTest
{
public:
	void Run ();
};


class LinkerFlagTest : public BaseTest
{
public:
	void Run ();
};


class IfTest : public BaseTest
{
public:
	void Run ();
private:
	void TestProjectIf ( Project& project );
	void TestModuleIf ( Project& project );
};


class FunctionTest : public BaseTest
{
public:
	void Run ();
};


class SourceFileTest : public BaseTest
{
public:
	void Run ();
	void IncludeTest ();
	void FullParseTest ();
private:
	bool IsParentOf ( const SourceFile* parent,
	                  const SourceFile* child );

};


class CDFileTest : public BaseTest
{
public:
	void Run ();
};


class SymbolTest : public BaseTest
{
public:
	void Run ();
};

class CompilationUnitTest : public BaseTest
{
public:
	void Run ();
};

#endif /* __TEST_H */
