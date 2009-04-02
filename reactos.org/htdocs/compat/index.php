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

error_reporting(E_ALL);
ini_set('error_reporting', E_ALL);

if (get_magic_quotes_gpc()) {
	die("ERROR: Disable 'magic quotes' in php.ini (=Off)");
}

//global $HTTP_GET_VARS; // set the Get var global

define('CDB_PATH', '');
require_once("lib/Compat_Autoloader.class.php");



	if ( !defined('RSDB') ) {
		define ("RSDB", "rossupportdb"); // to prevent hacking activity
	}
  define('ROSCMS_PATH', '../roscms/');
	
	
	
	
	// Environment Vars:
	
		$RSDB_intern_selected="";
	
		 // Asyncchronous Javascript and XML (or plain text) - Setting:
		 
		$RSDB_ENV_ajax="?";
		
		if (array_key_exists("ajax", $_GET)) $RSDB_ENV_ajax=htmlspecialchars($_GET["ajax"]);
	
		if ($RSDB_ENV_ajax != "?") {
			if ($RSDB_ENV_ajax == "true") {
				$RSDB_ENV_ajax = "true";
				Cookie::write('rsdb_ajat', 'true', time() + 24 * 3600 * 30 * 5, '/');
			}
			else {
				$RSDB_ENV_ajax = "false";
				Cookie::write('rsdb_ajat', 'false', time() + 24 * 3600 * 30 * 5, '/');
			}
		}
		else {
			if (isset($_COOKIE['rsdb_ajat'])) {
				$RSDB_ENV_ajax = $_COOKIE['rsdb_ajat'];
			}
			else {
				$RSDB_ENV_ajax = "false";
				Cookie::write('rsdb_ajat', 'false', time() + 24 * 3600 * 30 * 5, '/');
			}
		}
		
		
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

			Cookie::write('rsdb_threshold', $RSDB_ENV_forumthreshold, time() + 24 * 3600 * 30 * 5, '/');
			Cookie::write('rsdb_fstyle', $RSDB_ENV_forumsytle, time() + 24 * 3600 * 30 * 5, '/');
			Cookie::write('rsdb_order', $RSDB_ENV_forumorder, time() + 24 * 3600 * 30 * 5, '/');
		}
	
		// Test report bar settings:
		
		$RSDB_ENV_testsave="?";
		$RSDB_ENV_testthreshold="?";
		$RSDB_ENV_testorder="?"; 
		
		if (array_key_exists("testsave", $_POST)) $RSDB_ENV_testsave=htmlspecialchars($_POST["testsave"]);
		if ($RSDB_ENV_testsave == "1") {
			if (array_key_exists("threshold", $_POST)) $RSDB_ENV_testthreshold=htmlspecialchars($_POST["threshold"]);
			if (array_key_exists("order", $_POST)) $RSDB_ENV_testorder=htmlspecialchars($_POST["order"]);
			
			Cookie::write('rsdb_threshold', $RSDB_ENV_testthreshold, time() + 24 * 3600 * 30 * 5, '/');
			Cookie::write('rsdb_order', $RSDB_ENV_testorder, time() + 24 * 3600 * 30 * 5, '/');
		}



	// Global Vars:
	$RSDB_SET_page=""; // Page: Home or DB
	$RSDB_SET_sec=""; // Browse by: Category, Name, Company, Rank, etc.
	$RSDB_SET_view=""; // Compatibility, Packages, Dev Network, Media
	$RSDB_SET_cat="0"; // Category ID
	$RSDB_SET_cat2="flat"; // Category Style: Flat or Tree
	$RSDB_SET_letter=""; // Browse by Name: Letter: All, A, B, C, ..., X, Y, Z
	$RSDB_SET_curpos="0"; // If a table has more than e.g. 25 lines, then this Var is used 
	$RSDB_SET_group=""; // Group ID
	$RSDB_SET_group2=""; // Group page
	$RSDB_SET_item=""; // Item ID
	$RSDB_SET_item2=""; // Item page
	$RSDB_SET_sort=""; // Sort by ... (e.g. "item", "ros", etc.)
	$RSDB_SET_vote=""; // only for "Star Vote" (see inc/stars.php)
	$RSDB_SET_vote2=""; // only for "Star Vote" (see inc/stars.php)
	$RSDB_SET_vendor=""; // Vendor ID
	$RSDB_SET_rank=""; // Rank ID
	$RSDB_SET_rank2=""; // Rank page
	$RSDB_SET_addbox=""; // Submit Box
	$RSDB_SET_entry=""; // Entry ID
	
	
	$RSDB_SET_export=""; // Data export for Ajax, news feed, etc.
	$RSDB_SET_search=""; // Search string for search functions
	
	
	$RSDB_SET_threshold="3";
	$RSDB_SET_fstyle="nested";
	$RSDB_SET_order="new";
	$RSDB_SET_save="";
	$RSDB_SET_msg="";
	$RSDB_SET_filter="cur";
	$RSDB_SET_filter2="";
	
	$rpm_page="";
	$rpm_lang="";
	$rpm_logo="";



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
	if (array_key_exists("sec", $_GET)) $RSDB_SET_sec=htmlspecialchars($_GET["sec"]);
	if (array_key_exists("view", $_GET)) $RSDB_SET_view=htmlspecialchars($_GET["view"]);
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
	
	$RSDB_SET_lang=$rpm_lang;


	// Config
	require_once('rsdb_setting.php');
	
	// Human Readable URI
	if ($RSDB_SET_export == "") {
		require_once('rsdb_human_readable_url.php');
	}
	
	// URI building
	require_once('rsdb_config.php');
	
	
	// Security
	require_once("inc/tools/log.php");

	// Stats
//	require_once("inc/rsdb_stats.php");

	// Tools
	require_once("inc/tools/stars.php");
	require_once("inc/tools/awards.php");
	require_once("inc/tools/message_bar.php");
	require_once("inc/tools/forum_bar.php");
	require_once("inc/tools/test_bar.php");
	require_once("inc/tools/osversion.php");
	require_once("inc/tools/noscript.php");
	require_once("inc/tools/please_register.php");	
	require_once("inc/tools/user_functions.php");
	require_once("inc/tools/plugins.php");
	

//	echo "<hr />db: ".$RSDB_SET_page.", view: ".$RSDB_SET_view.", sec: ".$RSDB_SET_sec."<hr />";

	switch ($RSDB_SET_page) {
		case "home": // Frontpage
			//require($RSDB_intern_path_server.$RSDB_intern_loginsystem_path."inc/login.php");
			$rpm_page_title="Support Database - Overview";
			include("inc/header.php"); // Head
			create_head($rpm_page_title, $rpm_logo, $RSDB_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure($rpm_page);
			include("inc/home.php"); // Content
			include("inc/footer.php"); // Body
			break;
		case "about": // RSDB About Page
			//require($RSDB_intern_path_server.$RSDB_intern_loginsystem_path."inc/login.php");
			$rpm_page_title="Support Database - About";
			include("inc/header.php"); // Head
			create_head($rpm_page_title, $rpm_logo, $RSDB_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure($rpm_page);
			include("inc/about.php"); // Content
			include("inc/footer.php"); // Body
			break;
		case "conditions": // RSDB Submit Conditions Page
			$rpm_page_title="Support Database - Submit Conditions";
			include("inc/header.php"); // Head
			create_head($rpm_page_title, $rpm_logo, $RSDB_langres);
			include("inc/structure.php");  // Layout-Structure
			create_structure($rpm_page);
			include("inc/conditions.php"); // Content
			include("inc/footer.php"); // Body
			break;
		default:
		case "db": // developer interface
			//require($RSDB_intern_path_server.$RSDB_intern_loginsystem_path."inc/login.php");
					$RSDB_SET_view = "comp";
					$rpm_page_title="Support Database - Compatibility";
					$RSDB_intern_section_script = "inc/comp.php";

			include("inc/header.php");
			create_head($rpm_page_title, $rpm_logo, $RSDB_langres);
			include("inc/structure.php");
			create_structure($rpm_page);
			
			include($RSDB_intern_section_script);
			
			include("inc/footer.php");
			
			break;
		case "dat": // export data
			switch ($RSDB_SET_export) {
				case "grpitemlst": /* Compatibility versions list for one item */
					include("inc/comp/data/group_item_list.php");
					break;
				case "grplst": /* AppGroup - Search results */
					include("inc/comp/data/group_list.php");
					break;
				case "vdrlst": /* Vendor - Search results */
					include("inc/comp/data/vendor_list.php");
					break;
				case "compitemsubmit": /* Search results for the compatibility item submit page */
					include("inc/comp/data/comp_app_submit_list.php");
					break;
			}
			break;

	}
?>
