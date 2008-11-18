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


error_reporting(E_ALL);
ini_set('error_reporting', E_ALL);

if (get_magic_quotes_gpc()) {
	die("ERROR: Disable 'magic quotes' in php.ini (=Off)");
}

//global $HTTP_GET_VARS; // set the Get var global


	require_once("rost_config_db.php");



	if ( !defined('ROST') ) {
		define ("ROST", "rostservice"); // to prevent hacking activity
	}
	/*if ( !defined('ROSCMS_SYSTEM') ) {
		define ("ROSCMS_SYSTEM", "Version 0.1"); // to prevent hacking activity
	}*/
	
	
	
	
	// Environment Vars:
	
		$ROST_intern_selected="";
	
		 // Asyncchronous Javascript and XML (or plain text) - Setting:
		 
		$ROST_ENV_ajax="?";
		
		if (array_key_exists("ajax", $_GET)) $ROST_ENV_ajax=htmlspecialchars($_GET["ajax"]);
	
		require_once('inc/utils.php');
	
		if ($ROST_ENV_ajax != "?") {
			if ($ROST_ENV_ajax == "true") {
				$ROST_ENV_ajax = "true";
				setcookie('rsdb_ajat', 'true', time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			}
			else {
				$ROST_ENV_ajax = "false";
				setcookie('rsdb_ajat', 'false', time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			}
		}
		else {
			if (isset($_COOKIE['rsdb_ajat'])) {
				$ROST_ENV_ajax = $_COOKIE['rsdb_ajat'];
			}
			else {
				$ROST_ENV_ajax = "false";
				setcookie('rsdb_ajat', 'false', time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			}
		}
		
/*		
		// Forum bar settings:
		
		$RSDB_ENV_forumsave="?";
		$RSDB_ENV_forumthreshold="?";
		$RSDB_ENV_forumsytle="?";
		$RSDB_ENV_forumorder="?"; 
		
		if (array_key_exists("forumsave", $_POST)) $RSDB_ENV_forumsave=htmlspecialchars($_POST["forumsave"]);
		
		if ($RSDB_ENV_forumsave == "1") {
			if (array_key_exists("threshold", $_POST)) $RSDB_ENV_forumthreshold=htmlspecialchars($_POST["threshold"]);
			if (array_key_exists("fstyle", $_POST)) $RSDB_ENV_forumsytle=htmlspecialchars($_POST["fstyle"]);
			if (array_key_exists("order", $_POST)) $RSDB_ENV_forumorder=htmlspecialchars($_POST["order"]);

			setcookie('rsdb_threshold', $RSDB_ENV_forumthreshold, time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			setcookie('rsdb_fstyle', $RSDB_ENV_forumsytle, time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			setcookie('rsdb_order', $RSDB_ENV_forumorder, time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
		}
	
		// Test report bar settings:
		
		$RSDB_ENV_testsave="?";
		$RSDB_ENV_testthreshold="?";
		$RSDB_ENV_testorder="?"; 
		
		if (array_key_exists("testsave", $_POST)) $RSDB_ENV_testsave=htmlspecialchars($_POST["testsave"]);
		if ($RSDB_ENV_testsave == "1") {
			if (array_key_exists("threshold", $_POST)) $RSDB_ENV_testthreshold=htmlspecialchars($_POST["threshold"]);
			if (array_key_exists("order", $_POST)) $RSDB_ENV_testorder=htmlspecialchars($_POST["order"]);
			
			setcookie('rsdb_threshold', $RSDB_ENV_testthreshold, time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
			setcookie('rsdb_order', $RSDB_ENV_testorder, time() + 24 * 3600 * 30 * 5, '/', cookie_domain());
		}
*/

	
/*	
	$RSDB_SET_view=""; // Compatibility, Packages, Dev Network, Media
	$RSDB_SET_cat="0"; // Category ID
	$RSDB_SET_cat2="flat"; // Category Style: Flat or Tree
	$RSDB_SET_letter=""; // Browse by Name: Letter: All, A, B, C, ..., X, Y, Z
	$RSDB_SET_curpos="0"; // If a table has more than e.g. 25 lines, then this Var is used 
	$RSDB_SET_group=""; // Group ID
	$RSDB_SET_group2=""; // Group page
	$RSDB_SET_item2=""; // Item page
	$RSDB_SET_sort=""; // Sort by ... (e.g. "item", "ros", etc.)
	$RSDB_SET_vote=""; // only for "Star Vote" (see inc/stars.php)
	$RSDB_SET_vote2=""; // only for "Star Vote" (see inc/stars.php)
	$RSDB_SET_vendor=""; // Vendor ID
	$RSDB_SET_rank=""; // Rank ID
	$RSDB_SET_rank2=""; // Rank page
	$RSDB_SET_addbox=""; // Submit Box
	
	
	$RSDB_SET_export=""; // Data export for Ajax, news feed, etc.
	$RSDB_SET_search=""; // Search string for search functions
	
	
	$RSDB_SET_threshold="3";
	$RSDB_SET_fstyle="nested";
	$RSDB_SET_order="new";
	$RSDB_SET_save="";
	$RSDB_SET_msg="";
	$RSDB_SET_filter="cur";
	$RSDB_SET_filter2="";
	
		
	// old unused Vars from RosCMS
	/*$rpm_page="home";
	$rpm_sec="category";
	$rpm_sec2="";
	$rpm_sec3="";
	$rpm_site="";
	$rpm_opt="";
	$rpm_filt="";
	$rpm_filt2="";
	$rpm_lang="";
	$rpm_langid="";
	$rpm_forma="";
	$rpm_skin="";
	$rpm_debug="";
	$rpm_logo="";
	$rpm_db_id="";

	$varlang="";
	$varw3cformat="";
	$varformat="";
	$page_active="";
	$page_active_set="";
	$rpm_lang_id="";
	$rpm_sort="";
	$roscms_intern_account_level="";
	$roscms_intern_login_check="false";
	
	
	$roscms_infotable="";


	if (isset($_COOKIE['rsdb_threshold'])) {
		$RSDB_SET_threshold = $_COOKIE['rsdb_threshold'];
	}
	if (isset($_COOKIE['rsdb_fstyle'])) {
		$RSDB_SET_fstyle = $_COOKIE['rsdb_fstyle'];
	}
	if (isset($_COOKIE['rsdb_order'])) {
		$RSDB_SET_order = $_COOKIE['rsdb_order'];
	}

	
	if (array_key_exists("page", $_GET)) $RSDB_SET_page=htmlspecialchars($_GET["page"]);
	if (array_key_exists("view", $_GET)) $RSDB_SET_view=htmlspecialchars($_GET["view"]);
	if (array_key_exists("sec", $_GET)) $RSDB_SET_sec=htmlspecialchars($_GET["sec"]);
	if (array_key_exists("cat", $_GET)) $RSDB_SET_cat=htmlspecialchars($_GET["cat"]);
	if (array_key_exists("cat2", $_GET)) $RSDB_SET_cat2=htmlspecialchars($_GET["cat2"]);
	if (array_key_exists("letter", $_GET)) $RSDB_SET_letter=htmlspecialchars($_GET["letter"]);
	if (array_key_exists("curpos", $_GET)) $RSDB_SET_curpos=htmlspecialchars($_GET["curpos"]);
	if (array_key_exists("group", $_GET)) $RSDB_SET_group=htmlspecialchars($_GET["group"]);
	if (array_key_exists("group2", $_GET)) $RSDB_SET_group2=htmlspecialchars($_GET["group2"]);
	if (array_key_exists("item", $_GET)) $RSDB_SET_item=htmlspecialchars($_GET["item"]);
	if (array_key_exists("item2", $_GET)) $RSDB_SET_item2=htmlspecialchars($_GET["item2"]);
	if (array_key_exists("sort", $_GET)) $RSDB_SET_sort=htmlspecialchars($_GET["sort"]);
	if (array_key_exists("vote", $_GET)) $RSDB_SET_vote=htmlspecialchars($_GET["vote"]);
	if (array_key_exists("vote2", $_GET)) $RSDB_SET_vote2=htmlspecialchars($_GET["vote2"]);
	if (array_key_exists("vendor", $_GET)) $RSDB_SET_vendor=htmlspecialchars($_GET["vendor"]);
	if (array_key_exists("rank", $_GET)) $RSDB_SET_rank=htmlspecialchars($_GET["rank"]);
	if (array_key_exists("rank2", $_GET)) $RSDB_SET_rank2=htmlspecialchars($_GET["rank2"]);
	if (array_key_exists("addbox", $_GET)) $RSDB_SET_addbox=htmlspecialchars($_GET["addbox"]);
	if (array_key_exists("entry", $_GET)) $RSDB_SET_entry=htmlspecialchars($_GET["entry"]);


	if (array_key_exists("export", $_GET)) $RSDB_SET_export=htmlspecialchars($_GET["export"]);
	if (array_key_exists("search", $_GET)) $RSDB_SET_search=htmlspecialchars($_GET["search"]);
		
	
	if (array_key_exists("threshold", $_GET)) $RSDB_SET_threshold=htmlspecialchars($_GET["threshold"]);
	if (array_key_exists("fstyle", $_GET)) $RSDB_SET_fstyle=htmlspecialchars($_GET["fstyle"]);
	if (array_key_exists("order", $_GET)) $RSDB_SET_order=htmlspecialchars($_GET["order"]);
	if (array_key_exists("save", $_GET)) $RSDB_SET_save=htmlspecialchars($_GET["save"]);
	if (array_key_exists("msg", $_GET)) $RSDB_SET_msg=htmlspecialchars($_GET["msg"]);
	if (array_key_exists("filter", $_GET)) $RSDB_SET_filter=htmlspecialchars($_GET["filter"]);
	if (array_key_exists("filter2", $_GET)) $RSDB_SET_filter2=htmlspecialchars($_GET["filter2"]);
	
	if (array_key_exists("threshold", $_POST)) $RSDB_SET_threshold=htmlspecialchars($_POST["threshold"]);
	if (array_key_exists("fstyle", $_POST)) $RSDB_SET_fstyle=htmlspecialchars($_POST["fstyle"]);
	if (array_key_exists("order", $_POST)) $RSDB_SET_order=htmlspecialchars($_POST["order"]);
	if (array_key_exists("save", $_POST)) $RSDB_SET_save=htmlspecialchars($_POST["save"]);
	if (array_key_exists("msg", $_POST)) $RSDB_SET_msg=htmlspecialchars($_POST["msg"]);
	if (array_key_exists("filter", $_POST)) $RSDB_SET_filter=htmlspecialchars($_POST["filter"]);
	if (array_key_exists("filter2", $_POST)) $RSDB_SET_filter2=htmlspecialchars($_POST["filter2"]);
*/

	// Global Vars:
	$ROST_SET_page=""; // Page: Home or DB
	$ROST_SET_sec=""; // Section
	$ROST_SET_item=""; // Item ID
	$ROST_SET_entry=""; // Entry ID
	$ROST_SET_lang=""; // Entry ID
	$ROST_SET_action=""; // Action

	if (array_key_exists("page", $_GET)) $ROST_SET_page=htmlspecialchars($_GET["page"]);
	if (array_key_exists("sec", $_GET)) $ROST_SET_sec=htmlspecialchars($_GET["sec"]);
	if (array_key_exists("item", $_GET)) $ROST_SET_item=htmlspecialchars($_GET["item"]);
	if (array_key_exists("entry", $_GET)) $ROST_SET_entry=htmlspecialchars($_GET["entry"]);
	if (array_key_exists("lang", $_GET)) $ROST_SET_lang=htmlspecialchars($_GET["lang"]);
	if (array_key_exists("action", $_GET)) $ROST_SET_action=htmlspecialchars($_GET["action"]);


	$RSDB_intern_user_id = "";
	$ROST_intern_title = "";
	$ROST_intern_logo = "";
	$ROST_intern_langres = "";
	$ROST_xml_content = "";

	session_start();



	
	if (array_key_exists('HTTP_REFERER', $_SERVER)) $roscms_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
	
	if(isset($_COOKIE['roscms_usrset_lang'])) {
		$roscms_usrsetting_lang=$_COOKIE["roscms_usrset_lang"];
	}
	else {
		$roscms_usrsetting_lang="";
	}


	$roscms_intern_dynamic="true";
	
		
	require_once('inc/lang.php');
	
	$RSDB_SET_lang=$ROST_SET_lang;
	
	require_once('inc/tools.php');
	require_once('rsdb_human_readable_url.php');
	

	// Config
	require_once('rost_config.php');
	
	// Useful functions
	require_once('inc/functions.php');
	require_once('inc/parser/parsers.php');
	

	// Links
	require_once('rost_vars.php');


	echo "<hr>page: ".$ROST_SET_page;

	switch ($ROST_SET_sec) {
		default:
			if (check_langcode($ROST_SET_sec)) {
				$ROST_intern_title="Translate ".$ROST_intern_projectname;
				include("inc/header.php"); // Head
				create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
				include("inc/structure.php");  // Layout-Structure
				create_structure();
				if (strlen($ROST_SET_sec) == 2) {
					include("inc/translate_list_sublang.php"); // Content
				}
				else {
					include("inc/translate_list.php"); // Content
				}
				include("inc/footer.php"); // Body
				break;
			}
		case "home": // Frontpage
			//require($RSDB_intern_path_server.$RSDB_intern_loginsystem_path."inc/login.php");
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/home.php"); // Content
			include("inc/footer.php"); // Body
			break;

		case "all":
			//require($RSDB_intern_path_server.$RSDB_intern_loginsystem_path."inc/login.php");
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/translate_list_all.php"); // Content
			include("inc/footer.php"); // Body
			break;
			
		case "translate":
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/translate_interface.php"); // Content
			include("inc/footer.php"); // Body
			break;
			
		case "import":
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/admin_import.php"); // Content
			include("inc/footer.php"); // Body
			break;
		case "admin":
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/admin.php"); // Content
			include("inc/footer.php"); // Body
			break;
		case "help":
			$ROST_intern_title="Translate ".$ROST_intern_projectname;
			include("inc/header.php"); // Head
			create_head($ROST_intern_title, $ROST_intern_logo, $ROST_intern_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure();
			include("inc/help.php"); // Content
			include("inc/footer.php"); // Body
			break;
			
		case "test":
			include("inc/parser/parser_xml_transfer.php");
			echo '<hr /><textarea name="textfield" cols="100" rows="5">'."<obj type=\"STRING\" rc_name=\"STRING_NOTEXT\"><![CDATA[\"You didn't enter any text. \\ \nPlease type something and try again\"]]></obj>".'</textarea><br />';
			$RTRANS_temp_contentb = parser_transfer("notepad_de.xml", "STRING_NOTEXT");
			echo '<textarea name="textfield" cols="100" rows="5">'.$RTRANS_temp_contentb.'</textarea>';
			
			

			
			break;
	}

?>
