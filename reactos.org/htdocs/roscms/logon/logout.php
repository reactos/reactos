<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>

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

global $roscms_SET_path_ex;
global $rdf_login_cookie_usrkey;

//require_once("logon_utils.php");

if (isset($_COOKIE[$rdf_login_cookie_usrkey])) 
{
	$del_session_id = $_COOKIE[$rdf_login_cookie_usrkey];
	setcookie($rdf_login_cookie_usrkey, "", time() - 3600, "/", cookie_domain());
	$logout_usr_key_post = "DELETE FROM user_sessions " .
	                       " WHERE usersession_id = '" .
	                               mysql_escape_string($del_session_id) .
	                               "'";
	$logout_usr_key_post_list = mysql_query($logout_usr_key_post)
	                              or die("DB error (logout)!");
	
	// Set the Logout cookie for the Wiki, so the user won't see cached pages
	// 5 = $wgClockSkewFudge in the Wiki
	setcookie("wikiLoggedOut", gmdate("YmdHis", time() + 5), time() + 86400, "/", cookie_domain());
}


$target = $_SERVER[ 'PHP_SELF' ];
if ( IsSet( $_SERVER[ 'QUERY_STRING' ] ) ) {
	$target .= '?' . $_SERVER[ 'QUERY_STRING' ];
}
if (isset($_REQUEST['target'])) {
	header("Location: http://" . $_SERVER['HTTP_HOST'] .
                       $_REQUEST['target']);
		exit;
}

header("Location: ".$roscms_SET_path_ex);

exit;

?>
