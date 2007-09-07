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

#include "rbuild.h"

using std::string;

AutoRegister::AutoRegister ( const Project& project_,
                             const Module* module_,
                             const XMLElement& node_ )
	: XmlNode(project_, node_),
	  module(module_)
{
	Initialize();
}

bool
AutoRegister::IsSupportedModuleType ( ModuleType type )
{
	if ( type == Win32DLL ||
	     type == Win32OCX )
	{
		return true;
	}
	else
	{
		return false;
	}
}

AutoRegisterType
AutoRegister::GetAutoRegisterType( const string& type )
{
	if ( type == "DllRegisterServer" )
		return DllRegisterServer;
	if ( type == "DllInstall" )
		return DllInstall;
	if ( type == "Both" )
		return Both;
	throw XMLInvalidBuildFileException (
		node.location,
		"<autoregister> type attribute must be DllRegisterServer, DllInstall or Both." );
}

void
AutoRegister::Initialize ()
{
	if ( !IsSupportedModuleType ( module->type ) )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<autoregister> is not applicable for this module type." );
	}

	const XMLAttribute* att = node.GetAttribute ( "infsection", true );
	if ( !att )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<autoregister> must have a 'infsection' attribute." );
	}
	infSection = att->value;

	att = node.GetAttribute ( "type", true );
	if ( !att )
	{
		throw XMLInvalidBuildFileException (
			node.location,
			"<autoregister> must have a 'type' attribute." );
	}
	type = GetAutoRegisterType ( att->value );
}
