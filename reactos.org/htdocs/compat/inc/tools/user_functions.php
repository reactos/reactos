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


	$roscms_connected = 1;
	
	function usrfunc_GetUsername($RSDB_TEMP_getusrid) { // Get the user name from the RosCMS user table

    require_once(ROSCMS_PATH.'lib/RosCMS_Autoloader.class.php');
    $RSDB_USER_name = Subsystem::getUserName($RSDB_TEMP_getusrid);
    if ($RSDB_USER_name !== false) {
      return $RSDB_USER_name;
    }
    return '';
	}
	
	function usrfunc_IsAdmin($RSDB_TEMP_getusrid) { // Check if the user is an developer, admin or super admin

    //@IMPLEMENT Usergroup check
    return false;
	}

	function usrfunc_IsModerator($RSDB_TEMP_getusrid) { // Check if the user is an RSDB Moderator

    //@IMPLEMENT Usergroup check
		return false;
	}
	
?>