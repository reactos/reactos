<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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
		die("Hacking attempt");
	}


	/*if ( !defined('ROSCMS_SYSTEM_ADMIN') ) {
		define ("ROSCMS_SYSTEM_ADMIN", "Admin Interface"); // to prevent hacking activity
	}*/

	global $roscms_security_level;

	
	$RosCMS_GET_branch = "";
	$RosCMS_GET_debug = "";
	$RosCMS_GET_d_arch = ""; // data archive flag, means that the data is stored in the archive tables

	if (array_key_exists("branch", $_GET)) $RosCMS_GET_branch=htmlspecialchars($_GET["branch"]);
	if (array_key_exists("debug", $_GET) && $roscms_security_level > 1) $RosCMS_GET_debug=htmlspecialchars($_GET["debug"]);
	if (array_key_exists("d_arch", $_GET)) $RosCMS_GET_d_arch=htmlspecialchars($_GET["d_arch"]);


	if ($RosCMS_GET_d_arch == "true") {
		$h_a = "_a";
		$h_a2 = "a";
	}
	else {
		$h_a = "";
		$h_a2 = "";
	}

	
	require("inc/data_tools.php");
	require("inc/data_log.php"); // event log functions


	if ($roscms_security_level >= 1) {
		if ($rpm_site == "") {
			create_header("RosCMS v3");
		}
	
		switch ($RosCMS_GET_branch) {
			default:
			case "website":
				$RosCMS_GET_branch = "website";
				include("inc/data_list.php");
				break;
			case "welcome":
				include("inc/data_welcome.php");
				break;
			case "user":
				include("inc/data_user.php");
				break;
			case "maintain":
				include("inc/data_maintain.php");
				break;
			case "reactos":
				require("inc/data_menu.php");
				echo "<br />";
				echo "<p>Currently not implemented!</p>";
				break;
			case "stats":
				require("inc/data_menu.php");
				echo "<br />";
				require("../stats/admin/view_stats.php");
				break;
		}

	}
	else { // for all other user groups
		header("location:?page=nopermission");
	}
?>
