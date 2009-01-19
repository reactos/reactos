<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008  Danny Götte <dangerground@web.de>

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

$config->setPathGenerated('/reactos/'); // path to generated files
$config->setPathRoscms('/reactos/roscms/'); // path to roscms files



// RosCMS Table Names
define('ROSCMST_AREA'       , 'roscms_area');
define('ROSCMST_USERS'      , 'roscms_accounts');
define('ROSCMST_FORBIDDEN'  , 'roscms_accounts_forbidden');
define('ROSCMST_SESSIONS'   , 'roscms_accounts_sessions');
define('ROSCMST_COUNTRIES'  , 'roscms_countries');
define('ROSCMST_ENTRIES'    , 'roscms_entries');
define('ROSCMST_ACCESS'     , 'roscms_entries_access');
define('ROSCMST_RIGHTS'     , 'roscms_entries_areas');
define('ROSCMST_REVISIONS'  , 'roscms_entries_revisions');
define('ROSCMST_STEXT'      , 'roscms_entries_stext');
define('ROSCMST_TAGS'       , 'roscms_entries_tags');
define('ROSCMST_TEXT'       , 'roscms_entries_text');
define('ROSCMST_FILTER'     , 'roscms_filter');
define('ROSCMST_GROUPS'     , 'roscms_groups');
define('ROSCMST_JOBS'       , 'roscms_jobs');
define('ROSCMST_LANGUAGES'  , 'roscms_languages');
define('ROSCMST_SUBSYS'     , 'roscms_rel_accounts_subsys');
define('ROSCMST_MEMBERSHIPS', 'roscms_rel_groups_accounts');
define('ROSCMST_AREA_ACCESS', 'roscms_rel_groups_area');
define('ROSCMST_DEPENCIES'  , 'roscms_rel_revisions_depencies');
define('ROSCMST_ACL'        , 'roscms_rel_acl');
define('ROSCMST_TIMEZONES'  , 'roscms_timezones');


?>