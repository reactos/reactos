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
if ( !defined('ROSCMS_SYSTEM'))
{
	if ( !defined('ROSCMS_SYSTEM_LOG') ) {
		define ("ROSCMS_SYSTEM_LOG", "Hacking attempt");
	}
	$seclog_section="subsys_utils";
	$seclog_level="50";
	$seclog_reason="Hacking attempt: subsys_utils.php";
	define ("ROSCMS_SYSTEM", "Hacking attempt");
	include('securitylog.php'); // open security log
	die("Hacking attempt");
}

require_once("subsys_bugzilla.php");
require_once("subsys_phpbb.php");
require_once("subsys_wiki.php");

function subsys_mapping_check($subsys_name)
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u " .
           " WHERE u.user_id NOT IN " .
           "       (SELECT m.map_roscms_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_roscms_userid = u.user_id " .
           "           AND m.map_subsys_name = '" .
           mysql_real_escape_string($subsys_name) . "')";
  $query_set = mysql_query($query) or die("DB error (subsys_utils #1)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "No mapping of RosCMS userid " . $result_row['user_id'] .
         "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}

function subsys_userids_exist($subsys_name, $subsys_user_table,
                              $subsys_userid_column)
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id, m.map_subsys_userid " .
           "  FROM users u, " .
           "       subsys_mappings m LEFT OUTER JOIN $subsys_user_table ss " .
           "       ON ss.$subsys_userid_column = m.map_subsys_userid " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = '$subsys_name' " .
           "   AND ss.$subsys_userid_column IS NULL ";
  $query_set = mysql_query($query) or die("DB error (subsys_utils #2)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "RosCMS userid " . $result_row['user_id'] .
         " maps to subsys userid " . $result_row['map_subsys_userid'] .
         " but that subsys userid doesn't exist<br>\n";
    $inconsistency_count++;
    }

  $query = "SELECT ss.$subsys_userid_column AS subsys_userid " .
           "  FROM $subsys_user_table ss " .
           " WHERE ss.$subsys_userid_column NOT IN " .
           "       (SELECT m.map_subsys_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_subsys_userid = ss.$subsys_userid_column " .
           "           AND m.map_subsys_name = '$subsys_name')";
  $query_set = mysql_query($query) or die("DB error (subsys_utils #3)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "No RosCMS userid for subsys userid " . $result_row['subsys_userid'] .
         "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}

function subsys_get_roscms_info($roscms_user_id,
                                &$roscms_user_name,
                                &$roscms_user_email,
                                &$roscms_user_fullname,
                                &$roscms_user_register)
{
  $query = "SELECT user_name, user_email, user_fullname, " .
           "       UNIX_TIMESTAMP(user_register) AS user_reg " .
           "  FROM users " .
           " WHERE user_id = $roscms_user_id";
  $roscms_users_set = mysql_query($query)
                      or die("DB error (subsys_utils #4)");
  if (! ($roscms_users_row = mysql_fetch_array($roscms_users_set)))
    {
      echo "Can't find roscms user details for user id $roscms_user_id <br>\n";
      return FALSE;
    }
  $roscms_user_name = $roscms_users_row['user_name'];
  $roscms_user_email = $roscms_users_row['user_email'];
  $roscms_user_fullname = $roscms_users_row['user_fullname'];
  $roscms_user_register = $roscms_users_row['user_reg'];

  /* We need a valid username and email address in roscms */
  if ($roscms_user_name == '')
    {
      echo "No valid roscms user name found for user id $roscms_user_id <br>\n";
      return FALSE;
    }
  if ($roscms_user_email == '')
    {
      echo "No valid roscms email address found for user id $roscms_user_id <br>\n";
      return FALSE;
    }

  return TRUE;
}

function subsys_update_user($roscms_user_id)
{
  subsys_bugzilla_update_user($roscms_user_id);
  subsys_wiki_update_user($roscms_user_id);
  subsys_phpbb_update_user($roscms_user_id);
}

function subsys_add_roscms_user($user_name,
                                $user_encrypted_password,
                                $user_email,
                                $user_fullname,
                                $subsys,
                                $subsys_user_id)
{
  /* Make sure that the email address and/or user name are not already in
     use in roscms */
  $query = "SELECT COUNT(*) AS inuse " .
           "  FROM users " .
           " WHERE (LOWER(user_name) = LOWER('" .
           mysql_real_escape_string($user_name) . "') OR " .
           "        LOWER(user_email) = LOWER('" .
           mysql_real_escape_string($user_email) . "')) ";
  mysql_query($query);
  $check_set = mysql_query($query)
               or die("DB error (subsys_utils #5)");
  $check_row = mysql_fetch_array($check_set);
  if (0 != $check_row['inuse'])
    {
      echo "User name ($user_name) and/or email address " .
           "($user_email) collision<br>\n";
      return FALSE;
    }

  $query = "INSERT INTO users " .
           "       (user_name, user_roscms_password, user_account_enabled, " .
           "        user_fullname, user_email, user_setting_multisession, " .
           "        user_setting_browseragent, user_setting_ipaddress, " .
           "        user_setting_timeout) " .
           "VALUES ('" . mysql_real_escape_string($user_name) . "', " .
           "        '" . mysql_real_escape_string($user_encrypted_password) .
                    "', " .
           "        'yes', " .
           "        '" . mysql_real_escape_string($user_fullname) . "', " .
           "        '" . mysql_real_escape_string($user_email) . "', " .
           "        'true', " .
           "        'false', " .
           "        'false', " .
           "        'true')";
  mysql_query($query) or die("DB error (subsys_utils #6)");
  $roscms_user_id = mysql_insert_id();

  $query = "INSERT INTO subsys_mappings " .
           "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
           "VALUES ($roscms_user_id, '$subsys', $subsys_user_id)";
  mysql_query($query) or die("DB error (subsys_utils #7)");

  $query = "INSERT INTO usergroup_members " .
           "       (usergroupmember_userid, usergroupmember_usergroupid) " .
           "VALUES ($roscms_user_id, 'user')";
  mysql_query($query) or die("DB error (subsys_utils #8)");

  echo "Added RosCMS user " . htmlentities($user_name) .
       " for subsys user id $subsys_user_id<br>\n";

  subsys_update_user($roscms_user_id);
  
  return TRUE;
}

?>
