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
		global $roscms_connected;
		
		$RSDB_TEMP_usrname = "";
		
		$query_roscms_user = @mysql_query("SELECT * 
				FROM roscms.users 
				WHERE `user_id` = '".mysql_escape_string($RSDB_TEMP_getusrid)."' LIMIT 1;") ;
		$result_roscms_user = @mysql_fetch_array($query_roscms_user);
		
		$RSDB_TEMP_usrname = $result_roscms_user['user_name'];
		
		if ($roscms_connected == 0) {
			$RSDB_TEMP_usrname = "??? (Anonymous)";
		}
		/*if ($RSDB_TEMP_usrname == "") {
			$RSDB_TEMP_usrname = "Anonymous";
		}*/
		
		return $RSDB_TEMP_usrname;
	}
	
	function usrfunc_IsAdmin($RSDB_TEMP_getusrid) { // Check if the user is an developer, admin or super admin
		global $roscms_connected;
		
		$RSDB_TEMP_isadmin = false;
		
		$query_roscms_user = @mysql_query("SELECT * 
				FROM roscms.usergroup_members 
				WHERE `usergroupmember_userid` = '".mysql_escape_string($RSDB_TEMP_getusrid)."' AND ( `usergroupmember_usergroupid` = 'ros_sadmin' OR `usergroupmember_usergroupid` = 'ros_admin' OR `usergroupmember_usergroupid` = 'developer' ) LIMIT 1;") ;
		$result_roscms_user = @mysql_fetch_array($query_roscms_user);
		
		if ($result_roscms_user['usergroupmember_usergroupid'] == "ros_admin" || $result_roscms_user['usergroupmember_usergroupid'] == "ros_sadmin" || $result_roscms_user['usergroupmember_usergroupid'] == "developer") {
		
			$RSDB_TEMP_isadmin = true;
		}
		
		return $RSDB_TEMP_isadmin;
	}

	function usrfunc_IsModerator($RSDB_TEMP_getusrid) { // Check if the user is an RSDB Moderator
		global $roscms_connected;
		
		$RSDB_TEMP_ismoderator = false;
		
		$query_roscms_user = @mysql_query("SELECT * 
				FROM roscms.usergroup_members 
				WHERE `usergroupmember_userid` = '".mysql_escape_string($RSDB_TEMP_getusrid)."' AND `usergroupmember_usergroupid` = 'moderator' LIMIT 1;") ;
		$result_roscms_user = @mysql_fetch_array($query_roscms_user);
		
		if ($result_roscms_user['usergroupmember_usergroupid'] == "moderator") {
		
			$RSDB_TEMP_ismoderator = true;
		}
		
		return $RSDB_TEMP_ismoderator;
	}
	
?>