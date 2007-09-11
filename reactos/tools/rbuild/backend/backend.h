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
#ifndef __BACKEND_H
#define __BACKEND_H

#include "../rbuild.h"

class Backend;

typedef Backend* BackendFactory ( Project& project,
                                  Configuration& configuration );

class Backend
{
public:
	class Factory
	{
		static std::map<std::string,Factory*>* factories;
		static int ref;
		std::string m_name;
		std::string m_description;

	protected:

		Factory ( const std::string& name_, const std::string& description_ );
		virtual ~Factory();

		virtual Backend* operator() ( Project&,
		                              Configuration& configuration ) = 0;

	public:
		static Backend* Create ( const std::string& name,
		                         Project& project,
		                         Configuration& configuration );

		static std::map<std::string,Factory*>::iterator map_begin(void)
		{
			return factories->begin();
		}

		static std::map<std::string,Factory*>::iterator map_end(void)
		{
			return factories->end();
		}

		const char *Name(void) { return m_name.c_str(); }
		const char *Description(void) { return m_description.c_str(); }
	};

protected:
	Backend ( Project& project,
	          Configuration& configuration );

public:
	virtual ~Backend();

	virtual std::string GetFullName ( const FileLocation& file ) const;
	virtual std::string GetFullPath ( const FileLocation& file ) const;
	virtual void Process () = 0;
	Project& ProjectNode;
	Configuration& configuration;
};

#endif /* __BACKEND_H */
