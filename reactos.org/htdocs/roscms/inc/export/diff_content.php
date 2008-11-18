<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
	if ( !defined('ROSCMS_SYSTEM') )
	{
		if ( !defined('ROSCMS_SYSTEM_LOG') ) {
			define ("ROSCMS_SYSTEM_LOG", "Hacking attempt");
		}
		$seclog_section="roscms_interface";
		$seclog_level="50";
		$seclog_reason="Hacking attempt: admin_content.php";
		define ("ROSCMS_SYSTEM", "Hacking attempt");
		include('securitylog.php'); // open security log
		die("Hacking attempt");
	}
	
	include("roscms_config.php");
	


	$roscms_temp_diff2="";
	if (array_key_exists("diff2", $_GET)) $roscms_temp_diff2=htmlspecialchars($_GET["diff2"]);

	if ($roscms_temp_diff2 != "") {
		$query_diff_content2=mysql_query("SELECT *
											FROM `content`
											WHERE `content_id` = '".mysql_escape_string($roscms_temp_diff2)."' ;");
		$result_diff_content2 = mysql_fetch_array($query_diff_content2);
	
		echo wordwrap(nl2br(htmlentities($result_diff_content2['content_text'], ENT_QUOTES)));
	}
?>
