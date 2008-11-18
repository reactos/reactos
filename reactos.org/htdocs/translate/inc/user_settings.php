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
	if ( !defined('ROST') )
	{
		die(" ");
	}



// search for a valid RSDB user setting db entry; if not exist, create a default entry
function check_user_settings($RSDB_TEMP_userid) {
	$query_roscms_user_setting_exist = mysql_query("SELECT * 
					FROM roscms.users 
					WHERE `user_id` = '".mysql_escape_string($RSDB_TEMP_userid)."' LIMIT 1;") ;
	$result_roscms_user_setting_exist = @mysql_fetch_array($query_roscms_user_setting_exist);
	
	if ($result_roscms_user_setting_exist['user_id'] != "" && $result_roscms_user_setting_exist['user_id'] != "0") { 
		$RSDB_TEMP_setting_okay = true;
	}
	else {
		
		$RSDB_TEMP_setting_okay = false;
	}
	
	return $RSDB_TEMP_setting_okay;
}

		
?>
