<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005-2008  Klemens Friedl <frik85@reactos.org>
                  2005       Ge van Geldorp <gvg@reactos.org>
                  2008       Danny Götte <dangerground@web.de>

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

//@MOVEME to config
error_reporting(E_ALL);
ini_set('error_reporting', E_ALL);

if (get_magic_quotes_gpc()) {
  ini_set('magic_quotes', 'off');
}

// config data
require_once('config.php');

require_once('lib/RosCMS_Autoloader.class.php');
require_once('lib/DBConnection.class.php');

// strips whitespace from sourcecode
function echo_strip( $text ) {
  $text = str_replace('  ','',$text);
  $text = str_replace("\t",'',$text);
  $text = str_replace("\n",'',$text);
  echo   str_replace("\r",'',$text);
}

// select page
switch (@$_GET['page']) {

  // Login user
  case 'login':
    switch (@$_GET['subpage']) {
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
    new HTML_User_Register();
    break;

  // Captcha
  case 'captcha':
    new CaptchaSecurityImages();
    break;

  // User Profile (view | edit)
  case 'my':
  case 'user':
  default:
    // select action
    switch (@$_GET['subpage']) {
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
      case 'admin':
        new HTML_CMS_Admin();
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
          case 'ut': // User Tags
            new CMSWebsiteLabel();
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
      case 'admin':
        // Admin interface interaction
        switch (@$_GET['d_u']) {
          case 'acl': // Access Control Lists
            new Admin_ACL();
            break;
          case 'group':
            new Admin_Groups();
            break;
          case 'lang':
            new Admin_Languages();
            break;
          default:
            die('');
            break;
        } // end $_GET['d_u']
        break;
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
