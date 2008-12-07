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

?>