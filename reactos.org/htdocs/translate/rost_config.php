<?php
/*
    React Operating System Translate - ROST
    Copyright (C) 2006  Klemens Friedl <frik85@reactos.org>

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



	// To prevent hacking activity:
	if ( !defined('ROST') )
	{
		die(" ");
	}



	// Main Settings:
	// **************
	
		// Config: (please sync this with the database)
		$ROST_intern_path_server = "http://localhost/reactos.org/"; // complete server path
		$ROST_intern_path = $ROST_intern_path_server."translate/"; // the dirs after http://www.reactos.org
		$ROST_intern_path_ex = $ROST_intern_path."index.php/"; // "index.php/" if 'mod_rewrite' is not available
		$ROST_intern_dirs = "reactos.org/translate/";
		
		// operating system name:
		$ROST_intern_projectname = "ReactOS";	
		
		// script file fix (for Safari browser)
		$ROST_intern_index_php = $ROST_intern_path."index.php";
		
		// Global Login System
		$ROST_intern_loginsystem_path = "roscms/"; // RosCMS dir
		$ROST_intern_loginsystem_fullpath = $ROST_intern_path_server.$ROST_intern_loginsystem_path; // RosCMS dir
	
		// Items per Page
		$ROST_intern_items_per_page = 25;
		
		
		//require_once("../roscms/inc/subsys_login.php");
		//require_once("H:/Dateien von Klemens/FTP/xampp-win32-1.4.15/xampp/htdocs/reactos.org/roscms/inc/subsys_login.php");
//		require_once("/web/reactos.org/htdocs/roscms/inc/subsys_login.php");

		// User ID
		//$ROST_intern_user_id = roscms_subsys_login('', ROSCMS_LOGIN_OPTIONAL, "/" . $ROST_intern_path);
		$ROST_intern_user_id = 2;
		
		require_once('inc/user_settings.php');
		
		// search for a valid ROST user setting db entry
		if (check_user_settings($ROST_intern_user_id) == false) {
			$ROST_intern_user_id = 0;
		}
		
		if($ROST_intern_user_id != 0) {
			$query_roscms_user = mysql_query("SELECT * 
					FROM roscms.users 
					WHERE `user_id` = '".mysql_escape_string($ROST_intern_user_id)."' LIMIT 1;") ;
			$result_roscms_user = mysql_fetch_array($query_roscms_user);
			
			// Name
			$ROST_USER_name = $result_roscms_user['user_name'];
			
			
			// ROST user settings
			
				// Items per page
				$ROST_USER_setting_itemsperpage = 30; //$result_roscms_user['user_setting_itemsperpage'];
				$ROST_intern_items_per_page = $ROST_USER_setting_itemsperpage;

		}
		else {
			$ROST_USER_name = "Anonymous";
		}
		
//		$ROST_intern_user_id = 2;
//		$ROST_USER_name="TEST";
?>
