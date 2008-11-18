<?php
    /*
    RSDB - ReactOS Support Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    */

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}



function please_register() {
	global $RSDB_intern_path_server;
	global $RSDB_intern_loginsystem_path;
	
	msg_bar("<b>Login required!</b><br /><br />Please use the <a href=\"".$RSDB_intern_path_server.$RSDB_intern_loginsystem_path."?page=login&amp;target=%2Fsupport%2F\">login function</a> to get access or 
				<a href=\"".$RSDB_intern_path_server.$RSDB_intern_loginsystem_path."?page=register\">register an account</a> for free!");
}

?>