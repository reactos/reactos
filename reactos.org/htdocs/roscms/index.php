<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005-2008  Klemens Friedl <frik85@reactos.org>
	              2005       Ge van Geldorp <gvg@reactos.org>

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


	//error_reporting(0);
	error_reporting(E_ALL);
	ini_set('error_reporting', E_ALL);


	if (get_magic_quotes_gpc()) {
		die("ERROR: Disable 'magic quotes' in php.ini (=Off)");
	}

	// database connect data
	require("connect.db.php");
		
	// config data
	require("roscms_conf.php");
	
	// logon system
	require_once('inc/utils.php');
	require("logon/timezone.php");
	define('ROSCMS_LOGIN', '3');


	if ( !defined('ROSCMS_SYSTEM') ) {
		define ("ROSCMS_SYSTEM", "Version 0.3"); // to prevent hacking activity
	}


	// Global Vars:
	$rpm_page="";
	$rpm_sec="";
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
	$rpm_newcontent="";
	$rpm_export="";
	
	$varlang="";
	$varw3cformat="";
	$varformat="";
	$page_active="";
	$page_active_set="";
	$rpm_lang_id="";
	$rpm_sort="";
	$roscms_intern_account_level="";
	$roscms_intern_login_check="false";
	
	// Central Color Settings:
	//include("colors.php");
	
	$roscms_infotable="";
	
	// this vars will be removed soon
	$roscms_intern_login_check_username="";

	//if (array_key_exists("page", $HTTP_GET_VARS)) $rpm_page=$HTTP_GET_VARS["page"];
	if (array_key_exists("page", $_GET)) $rpm_page=htmlspecialchars($_GET["page"]);
	if (array_key_exists("sec", $_GET)) $rpm_sec=htmlspecialchars($_GET["sec"]);
	if (array_key_exists("sec2", $_GET)) $rpm_sec2=htmlspecialchars($_GET["sec2"]);
	if (array_key_exists("sec3", $_GET)) $rpm_sec3=htmlspecialchars($_GET["sec3"]);
	if (array_key_exists("site", $_GET)) $rpm_site=htmlspecialchars($_GET["site"]);
	if (array_key_exists("opt", $_GET)) $rpm_opt=htmlspecialchars($_GET["opt"]);
	if (array_key_exists("sort", $_GET)) $rpm_sort=htmlspecialchars($_GET["sort"]);
	if (array_key_exists("filt", $_GET)) $rpm_filt=htmlspecialchars($_GET["filt"]);
	if (array_key_exists("filt2", $_GET)) $rpm_filt2=htmlspecialchars($_GET["filt2"]);
	if (array_key_exists("lang", $_GET)) $rpm_lang=htmlspecialchars($_GET["lang"]);
	if (array_key_exists("langid", $_GET)) $rpm_lang_id=htmlspecialchars($_GET["langid"]);
	if (array_key_exists("forma", $_GET)) $rpm_forma=htmlspecialchars($_GET["forma"]);
	if (array_key_exists("skin", $_GET)) $rpm_skin=htmlspecialchars($_GET["skin"]);
	if (array_key_exists("debug", $_GET)) $rpm_debug=htmlspecialchars($_GET["debug"]);
	if (array_key_exists("logo", $_GET)) $rpm_logo=htmlspecialchars($_GET["logo"]);
	if (array_key_exists("db_id", $_GET)) $rpm_db_id=htmlspecialchars($_GET["db_id"]);
	if (array_key_exists("newcontent", $_GET)) $rpm_newcontent=htmlspecialchars($_GET["newcontent"]);
	if (array_key_exists("export", $_GET)) $rpm_export=htmlspecialchars($_GET["export"]);
	
	
	if (array_key_exists('HTTP_REFERER', $_SERVER)) $roscms_referrer=htmlspecialchars($_SERVER['HTTP_REFERER']);
	
	if(isset($_COOKIE['roscms_usrset_lang'])) {
		$roscms_usrsetting_lang=$_COOKIE["roscms_usrset_lang"];
	}
	else {
		$roscms_usrsetting_lang="";
	}


	$roscms_intern_dynamic="true";
	

	//echo "<h1>:".$rpm_lang."; ".$roscms_usrsetting_lang."</h1>";
	//die();


	//$rpm_lang = "en";
	//require("inc/lang/en.php"); // preload the english language text
	require_once("inc/language_detection.php");
	require("lang.php"); // lang code outsourced
	require("custom.php"); // custom on-screen information
	
	
	$rdf_URI_tree = $_SERVER['REQUEST_URI'];
	//echo $rdf_URI_tree . "<br>";
	
	
	$rdf_URI_tree = str_replace($roscms_SET_path."index.php/","",$rdf_URI_tree);
	$rdf_URI_tree = str_replace($roscms_SET_path,"",$rdf_URI_tree);
	//echo $rdf_URI_tree . "<br>";
	
	
	$rdf_URI_tree_split = explode("/", $rdf_URI_tree);
	
	
	$rdf_URI_tree_array = array();

	foreach($rdf_URI_tree_split as $value) {
		//echo "<br>".$value;
		$rdf_URI_tree_array[$rdf_URI_tree_counter] = $value;
		$rdf_URI_tree_counter++;
	}
	
	$rdf_uri_1 = @$rdf_URI_tree_array[0];
	$rdf_uri_2 = @$rdf_URI_tree_array[1];
	$rdf_uri_3 = @$rdf_URI_tree_array[2];
	$rdf_uri_4 = @$rdf_URI_tree_array[3];
	$rdf_uri_5 = @$rdf_URI_tree_array[4];
	$rdf_uri_6 = @$rdf_URI_tree_array[5];
	$rdf_uri_7 = @$rdf_URI_tree_array[6];
	$rdf_uri_8 = @$rdf_URI_tree_array[7];

	if ($rpm_page != "") {
		$rdf_uri_1 = $rpm_page;
	}
	//echo $rpm_page;
	
	$rdf_uri_str = $rdf_uri_1."/";
	/*if ($rdf_uri_2 != "") {
		$rdf_uri_str .= $rdf_uri_2;
	}*/
	

	switch ($rdf_uri_1) {
		case "404":
			require("inc/header.php");
			create_header("Page not found");
			require("inc/404.php");
			require("inc/footer.php");
			break;
			
		case "data": // RosCMS v3 Interface
			require("logon/login.php");
			require("inc/usergroups.php");
			$rpm_page_title = $roscms_extern_brand ." ".$roscms_extern_version;
			include("inc/header.php");
			$rpm_page = "data";
			include("inc/data.php"); 
			include("inc/footer.php");
			break;
		case "data_out": // data to client
			require("logon/login.php");
			require("inc/usergroups.php");
			$rpm_page = "data_out";
			include("inc/tools.php"); 
			include("inc/data_export.php"); 
			break;
		case "my":
		case "user":
		default:
			require("logon/login.php");
			require("inc/header.php");
			switch ($rdf_uri_2) {
				default:
					create_header("", "logon");
					require("logon/user_profil_menubar.php");
					require("logon/user_profil.php");
					break;
				case "edit":
				case "activate":
					require("logon/user_profil_edit.php");
					break;
			}
			require("inc/footer_closetable.php");
			require("inc/footer.php");
			break;
		case "nopermission":
			require("inc/header.php");
			create_header("No Permission!");
			require("inc/nopermission.php");
			require("inc/footer.php");
			break;
			
		case "search":
			require("logon/login.php");
			require("inc/header.php");
			create_header("", "logon");
			require("logon/user_profil_menubar.php");
			require("logon/user_profil_public.php");
			require("inc/footer_closetable.php");
			require("inc/footer.php");
			break;
		case "login":		
			require("inc/header.php");
			switch ($rdf_uri_2) {
				default:
					require("logon/user_login.php");
					break;
				case "lost":
					require("logon/user_login_lost.php");
					break;
				case "activate":
					require("logon/user_login_activate.php");
					break;
			}
			require("inc/footer_closetable.php");
			require("inc/footer.php");
			break;
		case "logout":
			include("logon/logout.php");
			break;
		case "register":
			if ($rdf_uri_2 == "captcha") {
				require("logon/captcha/captcha_image.php");
			}
			else {
				require("inc/header.php");
				require("logon/user_register.php");
				require("inc/footer.php");
			}
			break;
	}
?>
