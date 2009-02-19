<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008,2009  Danny Götte <dangerground@web.de>

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

// How much debug by php itself should be showed
error_reporting(E_ALL);
ini_set('error_reporting', E_ALL);

if (!defined('ROSCMS_PATH')) {
  define('ROSCMS_PATH',''); // relative path to roscms
}
require_once(ROSCMS_PATH.'lib/RosCMS.class.php');

$config = &RosCMS::getInstance();

// config
$config->setEmailSupport('support at reactos.org'); // email to which users can send mails, if they got a problem
$config->setEmailSystem('ReactOS<noreply@reactos.org>'); // mails which are send from the system and don't require a reply
  
$config->setCookieUserKey('roscmsusrkey'); // session key
$config->setCookieUserName('roscmsusrname'); // user_name
$config->setCookiePassword('rospassword');  // user_password (used for keep login function)
$config->setCookieLoginName('roscmslogon'); // where username is stored for 'save username' in login options
$config->setCookieSecure('roscmsseckey'); // stores security settings

$config->setSiteName('ReactOS.org'); // sites name
$config->setSiteLanguage('en'); // standard language
$config->setSiteTimezone(-2); // time difference to utc time from server time

$config->setMultiLanguage(true);  // is site multilingual

$config->setPathRoscms('/reactos/roscms/'); // path to roscms files
$config->setPathInstance('/reactos/roscms/'); // path to roscms files
$config->setPathGenerationCache('../roscms_cache/'); // path to generated files (relative to roscms folder)
$config->setPathGenerated('../'); // path to generated files (relative to roscms folder)



// RosCMS Table Names
$config->setTable('ROSCMST_AREA'       , 'roscms_area');
$config->setTable('ROSCMST_USERS'      , 'roscms_accounts');
$config->setTable('ROSCMST_FORBIDDEN'  , 'roscms_accounts_forbidden');
$config->setTable('ROSCMST_SESSIONS'   , 'roscms_accounts_sessions');
$config->setTable('ROSCMST_COUNTRIES'  , 'roscms_countries');
$config->setTable('ROSCMST_ENTRIES'    , 'roscms_entries');
$config->setTable('ROSCMST_ACCESS'     , 'roscms_entries_access');
$config->setTable('ROSCMST_RIGHTS'     , 'roscms_entries_areas');
$config->setTable('ROSCMST_REVISIONS'  , 'roscms_entries_revisions');
$config->setTable('ROSCMST_STEXT'      , 'roscms_entries_stext');
$config->setTable('ROSCMST_TAGS'       , 'roscms_entries_tags');
$config->setTable('ROSCMST_TEXT'       , 'roscms_entries_text');
$config->setTable('ROSCMST_FILTER'     , 'roscms_filter');
$config->setTable('ROSCMST_GROUPS'     , 'roscms_groups');
$config->setTable('ROSCMST_JOBS'       , 'roscms_jobs');
$config->setTable('ROSCMST_LANGUAGES'  , 'roscms_languages');
$config->setTable('ROSCMST_SUBSYS'     , 'roscms_rel_accounts_subsys');
$config->setTable('ROSCMST_MEMBERSHIPS', 'roscms_rel_groups_accounts');
$config->setTable('ROSCMST_AREA_ACCESS', 'roscms_rel_groups_area');
$config->setTable('ROSCMST_DEPENCIES'  , 'roscms_rel_revisions_depencies');
$config->setTable('ROSCMST_ACL'        , 'roscms_rel_acl');
$config->setTable('ROSCMST_TIMEZONES'  , 'roscms_timezones');

// do not change, it's needed to apply those config settings
$config->apply();
?>