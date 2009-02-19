<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005-2008  Klemens Friedl <frik85@reactos.org>
                  2005       Ge van Geldorp <gvg@reactos.org>
                  2008-2009  Danny Götte <dangerground@web.de>

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

if (get_magic_quotes_gpc()) {
  ini_set('magic_quotes', 'off');
}

// config data
require_once('config.php');

require_once('lib/RosCMS_Autoloader.class.php');

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
      case 'maintain':
        new HTML_CMS_Maintain();
        break;
      case 'stats':
        new HTML_CMS_Stats();
        break;
      case 'help':
        new HTML_CMS_Help();
        break;
      case 'website':
        new HTML_CMS_Website();
        break;
      default:
      case 'welcome':
        new HTML_CMS_Welcome();
        break;
    } // end switch
    break;

  // AJAX stuff (RosCMS Interface Backend)
  case 'backend':

    // select returned item
    switch ($_GET['type']) {
      case 'xml':
        switch ($_GET['subtype']) {
          case 'ptm': //  Main Edit Frame
            new Backend_ViewEntryTable();
            break;
          case 'ust': //  user search table
            new Backend_ViewUserTable();
            break;
        } // end switch subtype
        break;
      case 'text':

        // Website interface interaction
        switch ($_GET['subtype']) {
          case 'mef': //  Main Edit Frame
            new Backend_ViewEditor();
            break;
          case 'ned': //  New Entry Dialog
            new Backend_ViewAddEntry();
            break;
          case 'eta': //  Entry Table Actions
            new Backend_EntryTable();
            break;
          case 'asi': // Auto Save Info
            new Backend_SaveDraft();
            break;
          case 'usf': // User Search Filter (maintain tab)
            new Backend_SmartFilter('user');
            break;
          case 'ufs': // User Filter Storage (website tab)
            new Backend_SmartFilter('entry');
            break;
          case 'ut': // User Tags
            new Backend_Label();
            break;
          case 'tt': // EntryTable Tooltip
            new Backend_EntryTableToolTip();
            break;
          case 'prv': // Preview
            new Backend_Preview();
            break;
          case 'ud': // User Details
            new Backend_UserDetails();
            break;
        } // end switch subtype
        break;
      case 'page':
        new Backend_ViewPreview();
        break;
      case 'user':
        new Backend_ViewUserDetails();
        break;
      case 'maintain':
        new Backend_Maintain();
        break;
      case 'admin':

        // Admin interface interaction
        switch ($_GET['subtype']) {
          case 'acl': // Access Control Lists
            new Admin_ACL();
            break;
          case 'group':
            new Admin_Groups();
            break;
          case 'lang':
            new Admin_Languages();
            break;
          case 'system':
            new Admin_System();
            break;
        } // end switch subtype
        break;
    } // end switch type
    break;

  // presentation
  case 'presentation':
    //switch ($_GET['type']) {
    //  case 'vtext': // vertical text
        if (isset($_GET['bgcolor']) && isset($_GET['textc'])) {
          Presentation::verticalText($_GET['text'], $_GET['bgcolor'], $_GET['textc']);
        }
        else {
          Presentation::verticalText($_GET['text']);
        }
    //    break;
    //} // end switch type
    break;

  // No permission
  case 'nopermission':
    new HTML_Nopermission();
    break;

  // site not found
  case '404':
    new HTML_404();
    break;
} // end switch page


?>
