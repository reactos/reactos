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



	// Main Settings:
	// **************
	
		// Config: (please sync this with the database)
		$RSDB_intern_version = "RSDB 0.1 - http://www.reactos.org/"; // RSDB version info
		$RSDB_intern_path_server = "http://www.reactos.org/"; // complete server path
		$RSDB_intern_path = "support/"; // the dirs after http://www.reactos.org
		
		// script file fix (for Safari browser)
		$RSDB_intern_index_php = $RSDB_intern_path_server.$RSDB_intern_path."index.php";
		
		// Global Login System
		$RSDB_intern_loginsystem_path = "roscms/"; // RosCMS dir
		$RSDB_intern_loginsystem_fullpath = $RSDB_intern_path_server.$RSDB_intern_loginsystem_path; // RosCMS dir
	
		// Items per Page
		$RSDB_intern_items_per_page = 25;
		
		@define("ROOT_PATH", "../");
		require_once(ROOT_PATH . "roscms/logon/subsys_login.php");

		// User ID
		$RSDB_intern_user_id = roscms_subsys_login('', ROSCMS_LOGIN_OPTIONAL, "/" . $RSDB_intern_path);
		
		require_once('inc/user_settings.php');
		
		if($RSDB_intern_user_id != 0) {
			$query_roscms_user = mysql_query("SELECT * 
					FROM roscms.users 
					WHERE `user_id` = '".mysql_escape_string($RSDB_intern_user_id)."' LIMIT 1;") ;
			$result_roscms_user = mysql_fetch_array($query_roscms_user);
			
			// Name
			$RSDB_USER_name = $result_roscms_user['user_name'];
			
			
			// RSDB user settings
			
				// Items per page
				$RSDB_USER_setting_itemsperpage = 30; //$result_roscms_user['user_setting_itemsperpage'];
				$RSDB_intern_items_per_page = $RSDB_USER_setting_itemsperpage;

		}
		else {
			$RSDB_USER_name = "Anonymous";
		}
?>
