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


/*
 * User Settings:
 *
 * user_setting_multisession == "true" (default: false) [multi sessions are allowed for this user]
 * user_setting_browseragent == "true" (default: true) [no one should deactivate ("false") this option or only if he change the user agent very often (e.g. in opera: IE <=> Opera)]
 * user_setting_ipaddress == "true" (default: true) [IP address check; avoid this setting if the user is behind a proxy or use more than one pc the same time (a possible security risk, but some persons wanted that behavior ...); Note: this is a per user setting, everyone can change it!]
 * user_setting_timeout == "true" (default: false) [NO timeout; so user can use the ros homepage systems without to login everytime]
 *
 */

// needed for includes inside methods
global $roscms_intern_login_check_username;
global $roscms_intern_account_id;
global $roscms_security_level;
global $roscms_security_memberships;

// check if user wants to logout
if (isset($_POST['logout'])) {
	header("location:?page=logout");
}

// get current location (for redirection, if the login succeds)
$target = $_SERVER[ 'PHP_SELF' ];
if ( IsSet( $_SERVER[ 'QUERY_STRING' ] ) ) {
  $target .= '?'.$_SERVER[ 'QUERY_STRING' ];
}

// get information about script executer
$roscms_intern_account_id = Login::in(Login::REQUIRED, $target);
if (0 == $roscms_intern_account_id) {
  die('login.php: unexpected return from roscms_subsys_login');
}
$stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_name, user_roscms_password, user_timestamp_touch, user_setting_timeout, user_login_counter, user_account_enabled, user_setting_multisession, user_setting_browseragent, user_setting_ipaddress FROM users WHERE user_id = :user_id LIMIT 1");
$stmt->bindparam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
$stmt->execute() or die('DB error (login script #1)!');
$thisuser = $stmt->fetchOnce();
if($thisuser === false) {
  die('DB error (login script #2)');
}

// if the account is NOT enabled; e.g. a reason could be that a member of the admin group has disabled this account because of spamming, etc.
if ($thisuser['user_account_enabled'] != 'yes') { 
  die('Account is not enabled!<br /><br />System message: '.$thisuser['user_account_enabled']);
}

$sql_usergroup_list=''; // list of group_ids
$roscms_security_memberships = "|";

// check usergroup membership
$stmt=DBConnection::getInstance()->prepare("SELECT usergroupmember_usergroupid FROM usergroup_members WHERE usergroupmember_userid = :user_id");
$stmt->bindparam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
$stmt->execute();
while($membership = $stmt->fetch()) {

  // first ',' is removed in the statement itself
  $sql_usergroup_list .= ",".DBConnection::getInstance()->quote($membership['usergroupmember_usergroupid']); 
  $roscms_security_memberships .= $membership['usergroupmember_usergroupid']."|";
}


// get highest security level
$roscms_security_level = 0;
if ($sql_usergroup_list != ''){
  $stmt=DBConnection::getInstance()->prepare("SELECT MAX(usrgroup_seclev) FROM usergroups WHERE usrgroup_name_id IN(".substr($sql_usergroup_list,1).")");
  $stmt->execute();
  $roscms_security_level = $stmt->fetchColumn();
}


//@REMOVEME
$roscms_intern_login_check_username=$thisuser['user_name'];

?>
