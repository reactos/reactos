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
#include "pch.h"
#include <assert.h>

#include "rbuild.h"

using std::string;
using std::vector;

SysSetupGenerator::SysSetupGenerator ( const Project& project )
	: project ( project )
{
}

SysSetupGenerator::~SysSetupGenerator ()
{
}

void
SysSetupGenerator::Generate ()
{
	HINF inf;
	ULONG errorLine;

	string syssetupTemplate = "media" + sSep + "inf" + sSep + "syssetup.inf.tpl";
	string syssetup = Environment::GetOutputPath() + sSep + "media" + sSep + "inf" + sSep + "syssetup.inf";

	if ( 0 != InfHostOpenFile ( &inf, syssetupTemplate.c_str (), &errorLine ) )
		throw new FileNotFoundException ( syssetupTemplate );

	for( std::map<std::string, Module*>::const_iterator p = project.modules.begin(); p != project.modules.end(); ++ p )
	{
		const Module& module = *p->second;
		if ( module.autoRegister != NULL )
			Generate ( inf, module );
	}

	if ( 0 != InfHostWriteFile ( inf, syssetup.c_str (), "" ) )
	{
		InfHostCloseFile ( inf );
		throw new AccessDeniedException ( syssetup );
	}

	InfHostCloseFile ( inf );
}

#define DIRECTORYID_SYSTEM32	"11"

string
SysSetupGenerator::GetDirectoryId ( const Module& module )
{
	if ( module.install && ToLower ( module.install->relative_path ) == "system32" )
		return DIRECTORYID_SYSTEM32;
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

#define FLG_REGSVR_DLLREGISTER "1"
#define FLG_REGSVR_DLLINSTALL  "2"
#define FLG_REGSVR_BOTH        "3"

string
SysSetupGenerator::GetFlags ( const Module& module )
{
	if ( module.autoRegister->type == DllRegisterServer )
		return FLG_REGSVR_DLLREGISTER;
	if ( module.autoRegister->type == DllInstall )
		return FLG_REGSVR_DLLINSTALL;
	if ( module.autoRegister->type == Both )
		return FLG_REGSVR_BOTH;
	throw InvalidOperationException ( __FILE__,
	                                  __LINE__ );
}

void
SysSetupGenerator::Generate ( HINF inf,
                              const Module& module )
{
	PINFCONTEXT context;

	string infSection = module.autoRegister->infSection;
	if ( 0 != InfHostFindOrAddSection ( inf, infSection.c_str (), &context ) )
	{
		throw new Exception ( ".inf section '%s' not found", infSection.c_str () );
		InfHostCloseFile ( inf );
	}

	if ( 0 != InfHostAddLine ( context, NULL ) ||
	     0 != InfHostAddField ( context, GetDirectoryId ( module ).c_str () ) ||
	     0 != InfHostAddField ( context, "" ) ||
	     ( module.install && 0 != InfHostAddField ( context, module.install->name.c_str () ) ) ||
	     0 != InfHostAddField ( context, GetFlags ( module ).c_str () ) )
	{
		InfHostFreeContext ( context );
		InfHostCloseFile ( inf );
		throw InvalidOperationException ( __FILE__,
		                                  __LINE__ );
	}

	InfHostFreeContext ( context );
}
