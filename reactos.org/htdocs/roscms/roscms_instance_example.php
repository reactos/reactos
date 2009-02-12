<?php
    /*
    RosCMS Instance Example
    Copyright (C) 2009  Danny Götte <dangerground@web.de>

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
   To use RosCMS Instances:
  ==========================
  1. you need to copy this file to another folder, where the instance
     shall be located.
  2. rename this file to 'index.php'
  3. set ROSCMS_PATH, this is a path to the original RosCMS folder
  4. apply your special instance config data in the area below, you are
     allowed to modify all settings to your needs. everything what is
     not set in this config, is inherited from original RosCMS

*/

// ROSCMS PATH
define('ROSCMS_PATH', '../roscms/');

// do not modify
require_once(ROSCMS_PATH.'lib/RosCMS.class.php');
$config = &RosCMS::getInstance();

///////////////////////////////////////////////////////
// Config start ///////////////////////////////////////
  
$config->setCookieUserKey('roscmsusrkey'); // session key
$config->setCookieUserName('roscmsusrname'); // user_name
$config->setCookiePassword('rospassword');  // user_password (used for keep login function)
$config->setCookieLoginName('roscmslogon'); // where username is stored for 'save username' in login options
$config->setCookieSecure('roscmsseckey'); // stores security settings

$config->setSiteName('foundation.ReactOS.org'); // sites name

$config->setPathInstance('/reactos/roscms_instance/'); // path to roscms files
$config->setPathGenerationCache('../roscms_cache/'); // path to generated files (relative to roscms folder)
$config->setPathGenerated('../'); // path to generated files (relative to roscms folder)


// RosCMS Table Names
$config->setTable('ROSCMST_ENTRIES'    , 'roscms_entries');
$config->setTable('ROSCMST_REVISIONS'  , 'roscms_entries_revisions');
$config->setTable('ROSCMST_STEXT'      , 'roscms_entries_stext');
$config->setTable('ROSCMST_TAGS'       , 'roscms_entries_tags');
$config->setTable('ROSCMST_TEXT'       , 'roscms_entries_text');

// Config end /////////////////////////////////////////
///////////////////////////////////////////////////////

// do not modify
$config->apply();
include_once(ROSCMS_PATH.'index.php');
exit;

?>