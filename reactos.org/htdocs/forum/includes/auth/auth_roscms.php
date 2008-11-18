<?php
/**
* RosCMS auth plug-in for phpBB3
* Written by Colin Finck (mail@colinfinck.de) based on the DB plug-in
*/

/**
* @ignore
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

define("ROSCMS_PATH", "$phpbb_root_path/../roscms/");
require_once(ROSCMS_PATH . "logon/subsys_login.php");

/**
* Login function
*/
define(ROSCMS_DB_NAME, "roscms");

function login_roscms(&$username, &$password)
{
	global $db, $config;
	
	// We ignore both username and password here and retrieve the login data on our own using roscms_subsys_login
	// This will either retrieve the phpbb user ID of the user currently logged in or redirect us to the RosCMS login page.
	$userid = (int)roscms_subsys_login("phpbb", ROSCMS_LOGIN_REQUIRED, "/forum");
	
	// Now get the user row based on this ID
	$sql = "SELECT * FROM " . USERS_TABLE . " WHERE user_id = $userid";
	$result = $db->sql_query($sql);
	$row = $db->sql_fetchrow($result);
	$db->sql_freeresult($result);

	if (!$row)
	{
		return array(
			'status'	=> LOGIN_ERROR_USERNAME,
			'error_msg'	=> 'LOGIN_ERROR_USERNAME',
			'user_row'	=> array('user_id' => ANONYMOUS),
		);
	}

	// User inactive...
	if ($row['user_type'] == USER_INACTIVE || $row['user_type'] == USER_IGNORE)
	{
		return array(
			'status'		=> LOGIN_ERROR_ACTIVE,
			'error_msg'		=> 'ACTIVE_ERROR',
			'user_row'		=> $row,
		);
	}

	// Successful login
	return array(
		'status'		=> LOGIN_SUCCESS,
		'error_msg'		=> false,
		'user_row'		=> $row,
	);
}

/* This function is called, when a session cookie already exists and we try to verify if it's valid. */
function validate_session_roscms(&$user)
{
	// Check if our current RosCMS login is (still) valid, check the session expiration time and perform session cleanups.
	$valid_login = (roscms_subsys_login("phpbb", ROSCMS_LOGIN_OPTIONAL, "") != 0);
	
	// If we have a valid login, but the phpBB user ID is still ANONYMOUS, the user was logged in to RosCMS, but not yet to phpBB.
	// So do that now.
	if($valid_login && $user["user_id"] == ANONYMOUS)
		login_box();
	
	return $valid_login;
}

/* This function is called, when no phpBB session exists and we're in the process of creating the session cookie. */
function autologin_roscms()
{
	global $db;
	
	// Get the User ID of the logged in user (if any), check the session expiration time and perform session cleanups.
	$userid = (int)roscms_subsys_login("phpbb", ROSCMS_LOGIN_OPTIONAL, "");
	
	if($userid)
	{
		// Return the phpBB user row if a user is logged in.
		$sql = "SELECT * FROM " . USERS_TABLE . " WHERE user_id = $userid";
		$result = $db->sql_query($sql);
		$row = $db->sql_fetchrow($result);
		$db->sql_freeresult($result);
		
		return $row;
	}
}

?>
