<?php
    /*
    ReactOS Paste Service
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

	define("ROOT_PATH", "../");
	require_once(ROOT_PATH . "roscms/logon/subsys_login.php");
	$RSDB_intern_user_id = roscms_subsys_login('', ROSCMS_LOGIN_OPTIONAL, "/" . "http://localhost/reactos.org/");

	include("connect.db.php");
	
	if($RSDB_intern_user_id != 0) {
		$query_roscms_user = mysql_query("SELECT * 
				FROM roscms.users 
				WHERE user_id = '".mysql_escape_string($RSDB_intern_user_id)."' LIMIT 1;") ;
		$result_roscms_user = mysql_fetch_array($query_roscms_user);
		
		// Name
		$RSDB_USER_name = $result_roscms_user['user_name'];
		
	}
	else {
		$RSDB_USER_name = "Anonymous";
	}

?>
