<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>
	                    Ge van Geldorp <gvg@reactos.org>

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

// To prevent hacking activity:
if ( !defined('ROSCMS_LOGIN') )
{
	die("Hacking attempt");
}

require_once("subsys_login.php");


/*
 * User Settings:
 *
 * user_setting_multisession == "true" (default: false) [multi sessions are allowed for this user]
 * user_setting_browseragent == "true" (default: true) [no one should deactivate ("false") this option or only if he change the user agent very often (e.g. in opera: IE <=> Opera)]
 * user_setting_ipaddress == "true" (default: true) [IP address check; avoid this setting if the user is behind a proxy or use more than one pc the same time (a possible security risk, but some persons wanted that behavior ...); Note: this is a per user setting, everyone can change it!]
 * user_setting_timeout == "true" (default: false) [NO timeout; so user can use the ros homepage systems without to login everytime]
 *
 */
 
// Intern Member Groups: (set all to false)
$roscms_intern_usrgrp_sadmin = false;
$roscms_intern_usrgrp_admin = false;
$roscms_intern_usrgrp_dev = false;
$roscms_intern_usrgrp_team = false;
$roscms_intern_usrgrp_men = false;
$roscms_intern_usrgrp_transm = false;
$roscms_intern_usrgrp_trans = false;
$roscms_intern_usrgrp_user = false;

global $roscms_intern_usrgrp_sadmin;
global $roscms_intern_usrgrp_admin;
global $roscms_intern_usrgrp_deve;
global $roscms_intern_usrgrp_team;
global $roscms_intern_usrgrp_men;
global $roscms_intern_usrgrp_transm;
global $roscms_intern_usrgrp_trans;
global $roscms_intern_usrgrp_user;



$roscms_intern_login_check = "";
$roscms_intern_login_check_usrgroup = "";
$roscms_intern_account_group = "";
$roscms_intern_account_level = "";


$target = $_SERVER[ 'PHP_SELF' ];
if ( IsSet( $_SERVER[ 'QUERY_STRING' ] ) ) {
	$target .= '?' . $_SERVER[ 'QUERY_STRING' ];
}
$roscms_currentuser_id = roscms_subsys_login("roscms",
                                             ROSCMS_LOGIN_REQUIRED,
                                             $target);
if (0 == $roscms_currentuser_id) {
	die("login.php: unexpected return from roscms_subsys_login");
}
	
$query = "SELECT user_id, " .
         "       user_name, " .
         "       user_roscms_password, " .
         "       user_timestamp_touch, " .
         "       user_setting_timeout " .
         "  FROM users " .
         " WHERE user_id = " .
                 $roscms_currentuser_id;
$query_usr = mysql_query($query)
               or die('DB error (login script #1)!');
$result_usr = mysql_fetch_array($query_usr)
                or die('DB error (login script #2)');

if ($result_usr['user_name'] != "") {
	$roscms_intern_timeout_option = $result_usr['user_setting_timeout'];
}
else { // only hacker/cracker/script kiddy/brute force script/bot/etc. is able to reach this part
	die("<blink>No valid user session found!</blink><p><a href='http://www.reactos.org/roscms/?page=getpwd'>Delete all old sessions and login again!</a> (this function will also allow you to set a new password)</p>");
	// TODO: add it to the watchlist!
}

if (isset($_POST['logout'])) {
	header("location:?page=logout");
}

$querya = "SELECT user_login_counter, user_account_enabled, " .
          "       user_setting_multisession, " .
          "       user_setting_browseragent, user_setting_ipaddress " .
          "  FROM users  " .
          " WHERE user_id = $roscms_currentuser_id";
$login_usr_keya_query = mysql_query($querya)
                        or die('DB error (login)!');
$login_usr_keya_result = mysql_fetch_array($login_usr_keya_query);

$roscms_currentuser_login_counter = $login_usr_keya_result['user_login_counter'];
$roscms_currentuser_login_user_account_enabled = $login_usr_keya_result['user_account_enabled'];
$roscms_currentuser_login_user_setting_multisession = $login_usr_keya_result['user_setting_multisession'];
$roscms_currentuser_login_user_setting_browseragent = $login_usr_keya_result['user_setting_browseragent'];
$roscms_currentuser_login_user_setting_ipaddress = $login_usr_keya_result['user_setting_ipaddress'];

// if the account is NOT enabled; e.g. a reason could be that a member
// of the admin group has disabled this account because of spamming,
// etc.
if ($roscms_currentuser_login_user_account_enabled != "yes") { 
	die("Account is not enabled!<br><br>System message: " .
		$roscms_currentuser_login_user_account_enabled);
}


$roscms_security_level = 0;
$roscms_security_memberships = "|";


$rdf_user_id = $roscms_currentuser_id;

// check usergroup membership
$query = "SELECT * " .
         "  FROM usergroup_members " .
         " WHERE usergroupmember_userid = $roscms_currentuser_id";		
$roscms_login_usrgrp_member_query = mysql_query($query);
while($roscms_login_usrgrp_member_list = mysql_fetch_array($roscms_login_usrgrp_member_query)) {
	switch ($roscms_login_usrgrp_member_list['usergroupmember_usergroupid']) { // Membership
		case "user": // normal user
			$roscms_intern_usrgrp_user = true;
			break;
		case "transmaint": // translation maintainer
		case "translator": // translator
			$roscms_intern_usrgrp_trans = true;
			break;
		case "m_lang": // language maintainer
			$roscms_intern_usrgrp_transm = true;
			break;
		case "m_en": // maintainer for english (original language)
			$roscms_intern_usrgrp_men = true;
			break;
		case "moderator": // moderator
		case "mediateam": // mediateam (add here all other team's but not the 'web-team' => admin-group)
			$roscms_intern_usrgrp_team = true;
			break;
		case "developer": // developer
			$roscms_intern_usrgrp_dev = true;
			break;
		case "ros_admin": // administrator
			$roscms_intern_usrgrp_admin = true;
			break;
		case "ros_sadmin": // super administrator (if someone is member of this group he is also a member of the normal admin group!)
			$roscms_intern_usrgrp_admin = true;
			$roscms_intern_usrgrp_sadmin = true;
			break;
		default: // nothing
			break;
	}
	
		
	$query_usergroup = mysql_query("SELECT * 
									FROM usergroups 
									WHERE usrgroup_name_id = '".mysql_real_escape_string($roscms_login_usrgrp_member_list['usergroupmember_usergroupid'])."'
									LIMIT 1;");
	$result_usergroup = mysql_fetch_array($query_usergroup);
	
	if ($result_usergroup['usrgroup_seclev'] > $roscms_security_level) {
		$roscms_security_level = $result_usergroup['usrgroup_seclev'];
	}
	
	$roscms_security_memberships .= $roscms_login_usrgrp_member_list['usergroupmember_usergroupid']."|";

}

// Account level:

if ($roscms_intern_usrgrp_sadmin == true) {
	$roscms_intern_account_level = 100;
}
else {
	if ($roscms_intern_usrgrp_admin == true) {
		$roscms_intern_account_level = 75;
	}
	else {
		if ($roscms_intern_usrgrp_dev == true) {
			$roscms_intern_account_level = 50;
		}
		else {
			if ($roscms_intern_usrgrp_team == true || 
				$roscms_intern_usrgrp_trans == true || 
				$roscms_intern_usrgrp_transm == true || 
				$roscms_intern_usrgrp_men == true)
			{
				$roscms_intern_account_level = 25;
			}
			else {
				$roscms_intern_account_level = 0;
			}
		}
	}
}

//require_once("usergroups.php");


// user id
$roscms_intern_account_id = $roscms_currentuser_id;
$roscms_intern_login_check_username=$result_usr['user_name'];

// quick hack to test RosCMS; the following vars will change soon
$roscms_intern_login_check_usrgroup = "ros_sadmin";
$roscms_intern_account_group = "ros_sadmin";
$roscms_intern_login_check = "valid"; // valid login sequenze

?>
