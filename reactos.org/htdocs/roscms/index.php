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

  require('constants.php');
  require('lib/RosCMS_Autoloader.class.php');

	if (get_magic_quotes_gpc()) {
    ini_set('magic_quotes', 'off');
	}

		
	// config data
	require("roscms_conf.php");
	
	// logon system
	define('ROSCMS_LOGIN', '3');


	if ( !defined('ROSCMS_SYSTEM') ) {
		define ("ROSCMS_SYSTEM", "Version 0.3"); // to prevent hacking activity
	}


	// Global Vars:
	$rpm_page="";
	$rpm_lang="";
		
	
	// this vars will be removed soon
	$roscms_intern_login_check_username="";

	if (array_key_exists("page", $_GET)) $rpm_page=htmlspecialchars($_GET["page"]);
	if (array_key_exists("lang", $_GET)) $rpm_lang=htmlspecialchars($_GET["lang"]);
	
  
	require("lang.php"); // lang code outsourced
	require("custom.php"); // custom on-screen information
	
	
	$rdf_URI_tree = $_SERVER['REQUEST_URI'];
	$rdf_URI_tree = str_replace($roscms_intern_webserver_roscms,'',$rdf_URI_tree);
	$rdf_URI_tree = str_replace('index.php/','',$rdf_URI_tree);
	
	$rdf_URI_tree_split = explode('/', $rdf_URI_tree);
	
	$rdf_uri_2 = @$rdf_URI_tree_split[1];
	$rdf_uri_3 = @$rdf_URI_tree_split[2];

	if ($rpm_page != "") {
		$rdf_URI_tree_split[0] = $rpm_page;
	}
	
	$rdf_uri_str = $rdf_URI_tree_split[0]."/";
  
  
	$RosCMS_GET_d_use = ""; // data usage (where the data will be used)
	$RosCMS_GET_d_flag = ""; // data flag
	$RosCMS_GET_d_value = ""; // data transport value
	$RosCMS_GET_d_value2 = ""; // data transport value
	$RosCMS_GET_d_value3 = ""; // data transport value
	$RosCMS_GET_d_value4 = ""; // data transport value
	
	$RosCMS_GET_d_id = ""; // data_id
	$RosCMS_GET_d_r_id = ""; // data rev id
	$RosCMS_GET_d_r_lang = ""; // data rev language (e.g. "en", "de", etc.)

	if (array_key_exists("d_u", $_GET)) $RosCMS_GET_d_use=htmlspecialchars($_GET["d_u"]);
	if (array_key_exists("d_fl", $_GET)) $RosCMS_GET_d_flag=htmlspecialchars($_GET["d_fl"]);
	if (array_key_exists("d_val", $_GET)) $RosCMS_GET_d_value=htmlspecialchars($_GET["d_val"]);
	if (array_key_exists("d_val2", $_GET)) $RosCMS_GET_d_value2=htmlspecialchars($_GET["d_val2"]);
	if (array_key_exists("d_val3", $_GET)) $RosCMS_GET_d_value3=htmlspecialchars($_GET["d_val3"]);
	if (array_key_exists("d_val4", $_GET)) $RosCMS_GET_d_value4=htmlspecialchars($_GET["d_val4"]);

	if (array_key_exists("d_id", $_GET)) $RosCMS_GET_d_id=htmlspecialchars($_GET["d_id"]);
	if (array_key_exists("d_r_id", $_GET)) $RosCMS_GET_d_r_id=htmlspecialchars($_GET["d_r_id"]);
	if (array_key_exists("d_r_lang", $_GET)) $RosCMS_GET_d_r_lang=htmlspecialchars($_GET["d_r_lang"]);
  
  
if (isset($_GET['d_arch']) && $_GET['d_arch'] == true) {
  $h_a = '_a';
  $h_a2 = 'a';
}
else {
  $h_a = '';
  $h_a2 = '';
}

// strips whitespace from sourcecode
function echo_strip( $text ) {
  $text = str_replace('  ','',$text);
  $text = str_replace("\t",'',$text);
  $text = str_replace("\n",'',$text);
  echo str_replace("\n",'',$text);
}


switch ($rdf_URI_tree_split[0]) {

  // Login user
  case 'login':
    switch (@$rdf_URI_tree_split[1]) {
      case 'lost':
        new HTML_User_LostPassword();
        break;
      case 'activate':
        new HTML_User_Activate();
        break;
      default:
        new HTML_User_Login();
        break;
    }
    break;

  // Logout user
  case 'logout':
    Login::out();
    break;

  // Register new user
  case 'register':
    if (@$rdf_URI_tree_split[1] == 'captcha') {
      new CaptchaSecurityImages();
    }
    else {
      new HTML_User_Register();
    }
    break;

  // User Profile (view | edit)
  case 'my':
  case 'user':
  default:
    // select action
    switch (@$rdf_URI_tree_split[1]) {
      case 'edit':
      case 'activate':
        new HTML_User_ProfileEdit();
        break;
      default:
        new HTML_User_Profile();
        break;
    } // end switch
    break;

  // search user profiles
  case 'search':
    new HTML_User_Profile('',true);
    break;

  // RosCMS Interface Frontend
  case 'data': 
    // select interface menu on top
    switch (@$_GET['branch']) {
      case 'welcome':
        new HTML_CMS_Welcome();
        break;
      case 'user':
        new HTML_CMS_User();
        break;
      case 'maintain':
        new HTML_CMS_Maintain();
        break;
      case 'stats':
        new HTML_CMS_Stats();
        break;
      case 'website':
      default:
        new HTML_CMS_Website();
        break;
    } // end switch
    break;

  // AJAX stuff (RosCMS Interface Backend)
  case 'data_out':
    // select returned item
    switch ($_GET['d_f']) {
      case 'xml':
        new Export_XML(@$_GET['d_u']);
        break;
      case 'text':
        // Website interface interaction
        switch (@$_GET['d_u']) {
          case 'mef': //  Main Edit Frame
            new Editor_Website(@$_GET['d_id'], @$_GET['d_r_id'], @$_GET['d_fl']);
            break;
          case 'asi': // Auto Save Info
            new CMSWebsiteSaveEntry();
            break;
          case 'ufs': // User Filter Storage
            new CMSWebsiteFilter();
            break;
          case 'uqi': // User Quick Info
            new Export_QuickInfo();
            break;
          default:
            die('');
            break;
        } // end $_GET['d_u']
        break;
      case 'page':
        new Export_Page();
        break;
      case 'user':
        new Export_User();
        break;
      case 'maintain':
        new Export_Maintain();
        break;
    } // end switch
    break;

  // No permission
  case 'nopermission':
    new HTML_Nopermission();
    break;

  // site not found
  case '404':
    new HTML_404();
    break;
} // end top switch


?>
