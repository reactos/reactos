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


error_reporting(E_ALL);
ini_set('error_reporting', E_ALL);

if (get_magic_quotes_gpc()) {
	die("ERROR: Disable 'magic quotes' in php.ini (=Off)");
}


	$ros_paste_SET_path = "http://www.reactos.org/paste/";
	$ros_paste_SET_path_ex = "http://www.reactos.org/paste/index.php/";
	$ros_paste_SET_dirs = "reactos.org/paste/";
	$ros_paste_SET_content = "storage";



	$ros_paste_SET_page = "";
	if (array_key_exists("page", $_GET)) $ros_paste_SET_page=htmlspecialchars($_GET["page"]);


	
	$RSDB_URI_tree = $_SERVER['REQUEST_URI'];
	//echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree = str_replace($ros_paste_SET_dirs."index.php/","",$RSDB_URI_tree);
	$RSDB_URI_tree = str_replace($ros_paste_SET_dirs,"",$RSDB_URI_tree);
	//echo $RSDB_URI_tree . "<br>";
	
	
	$RSDB_URI_tree_split = explode("/", $RSDB_URI_tree);
	
	
	$RSDB_URI_tree_array = array();
	$RSDB_URI_tree_counter = 0;

	foreach($RSDB_URI_tree_split as $value) {
		//echo "<br>".$value;
		$RSDB_URI_tree_array[$RSDB_URI_tree_counter] = $value;
		$RSDB_URI_tree_counter++;
	}
	
	/*echo "<br>#0: ".@$RSDB_URI_tree_array[0];
	echo "<br>#1: ".@$RSDB_URI_tree_array[1];
	echo "<br>#2: ".@$RSDB_URI_tree_array[2];*/
	
	if (!strchr($RSDB_URI_tree,"?page=")) {
		//echo "<p>###</p>";
		$ros_paste_SET_page = @$RSDB_URI_tree_array[3];
		if ($ros_paste_SET_page == "id") {
			$ros_paste_SET_page = "paste";
			$ros_paste_SET_pasteid = "";
			$ros_paste_SET_pasteid = @$RSDB_URI_tree_array[3];
			$ros_paste_SET_pasteflag = "";
			$ros_paste_SET_pasteflag = @$RSDB_URI_tree_array[4];
		}
		else if ($ros_paste_SET_page != "conditions" && $ros_paste_SET_page != "help" && $ros_paste_SET_page != "recent") {
			$ros_paste_SET_page = "paste";
			$ros_paste_SET_pasteid = "";
			$ros_paste_SET_pasteid = @$RSDB_URI_tree_array[3];
			$ros_paste_SET_pasteflag = "";
			$ros_paste_SET_pasteflag = @$RSDB_URI_tree_array[4];
		}
	}

	
	include("inc/tools.php");

	switch ($ros_paste_SET_page) {
		default:
		case "paste": // Paste Your Content
			if (@$ros_paste_SET_pasteflag == "text" || @$ros_paste_SET_pasteflag == "textw") {
				include("inc/account.php");
				$query_pasteid=mysql_query("SELECT * 
											FROM `paste_service` 
											WHERE `paste_id` = '".mysql_real_escape_string($ros_paste_SET_pasteid)."' 
											LIMIT 1 ;");	
				$result_pasteid = mysql_fetch_array($query_pasteid);
				
				header('Content-type: text/plain');
				//echo "<!-- ReactOS Paste Service - http://www.reactos.org/paste/".$result_pasteid['paste_id']." --> \n\n";
				$filename = $ros_paste_SET_content."/".$result_pasteid['paste_id'].".txt";
				$handle = @fopen($filename, "r");
				$contents = @fread($handle, @filesize($filename));
				@fclose($handle);
				$ros_paste_SET_textcontent = $contents;
				//$ros_paste_SET_textcontent = str_replace("&nbsp;"," ",$ros_paste_SET_textcontent);
				if ($result_pasteid['paste_tabs'] != "0") {
					$PASTE_var_tabs = "";
					for($xxx=0; $xxx<$result_pasteid['paste_tabs']; $xxx++){
						$PASTE_var_tabs .= " ";
					}
					$ros_paste_SET_textcontent = str_replace("\t",$PASTE_var_tabs,$ros_paste_SET_textcontent);
				}
				if (@$ros_paste_SET_pasteflag == "textw") {
					echo wordwrap($ros_paste_SET_textcontent, 80, "\n", true);
				} else {
					echo $ros_paste_SET_textcontent;
				}
			}
			else {
				include("inc/header.php");
				include("inc/account.php");
				include("inc/highlight.php");
				include("inc/paste.php");
				include("inc/footer.php");
			}
			break;
		case "conditions": // Conditions
			include("inc/header.php");
			create_header();
			include("inc/conditions.php");
			include("inc/footer.php");
			break;
		case "help": // Help & FAQ
			include("inc/header.php");
			create_header();
			include("inc/help.php");
			include("inc/footer.php");
			break;
		case "recent": // Recent Pastes
			include("inc/header.php");
			create_header();
			include("inc/account.php");
			include("inc/recent.php");
			include("inc/footer.php");
			break;
	}
	
	
?>
