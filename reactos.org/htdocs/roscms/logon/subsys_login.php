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

if(!defined("ROSCMS_PATH"))
	define("ROSCMS_PATH", "");

require_once(ROSCMS_PATH . "inc/utils.php");

define('ROSCMS_LOGIN_OPTIONAL', 1);
define('ROSCMS_LOGIN_REQUIRED', 2);

function roscms_subsys_login($subsys, $login_type, $target)
{
  if (ROSCMS_LOGIN_OPTIONAL != $login_type &&
      ROSCMS_LOGIN_REQUIRED != $login_type)
    {
      die("Invalid login_type $login_type for roscms_subsys_login");
    }

  if (isset($_COOKIE['roscmsusrkey']) &&
      preg_match('/^([a-z]{32})$/', $_COOKIE['roscmsusrkey'], $matches))
    {
      $session_id_clean = $matches[1];

      if (isset($_SERVER['REMOTE_ADDR']) &&
          preg_match('/^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})$/',
                     $_SERVER['REMOTE_ADDR'], $matches) )
        {
          $remote_addr_clean = $matches[1];
        }
      else
        {
          $remote_addr_clean = 'invalid';
        }
      if (isset($_SERVER['HTTP_USER_AGENT']))
        {
          $browser_agent_clean = $_SERVER['HTTP_USER_AGENT'];
        }
      else
        {
          $browser_agent_clean = 'unknown';
        }

      require(ROSCMS_PATH . "connect.db.php");

      /* Clean out expired sessions */
      $query = "DELETE FROM user_sessions " .
               " WHERE usersession_expires IS NOT NULL " .
               "   AND usersession_expires < NOW()";
      mysql_query($query, $connect);

      /* Now, see if we have a valid login session */
      $bulk_of_where = " WHERE s.usersession_id = '" .
                               mysql_escape_string($session_id_clean) . "' " .
                       "   AND u.user_id = s.usersession_user_id " .
                       "   AND (u.user_setting_ipaddress = 'false' OR " .
                       "        s.usersession_ipaddress = '" .
                                mysql_escape_string($remote_addr_clean) . "') " .
                       "   AND (u.user_setting_browseragent = 'false' OR " .
                       "        s.usersession_browseragent = '" .
                                mysql_escape_string($browser_agent_clean) . "') ";
      if ($subsys == "roscms" || $subsys == "")
        {
          $query = "SELECT u.user_id, s.usersession_expires " .
                   "  FROM roscms.user_sessions s, " .
                   "       roscms.users u " .
                   $bulk_of_where;
        }
      else
        {
          $query = "SELECT m.map_subsys_userid, s.usersession_expires " .
                   "  FROM roscms.user_sessions s, " .
                   "       roscms.users u, " .
                   "       roscms.subsys_mappings m " .
                   $bulk_of_where .
                   "   AND m.map_roscms_userid = s.usersession_user_id " .
                   "   AND m.map_subsys_name = '" .
                           mysql_escape_string($subsys) . "'";
        }
      $statement = mysql_query($query, $connect)
                   or die('DB error (user login) ' . $query);

      if ($row = mysql_fetch_array($statement))
        {
        /* Login session found */
        $userid = $row[0];
		
        if (isset($row[1]))
          {
            /* Session with timeout. Update the expiry time in the table and 
               the expiry time of the cookie */
            $query = "UPDATE roscms.user_sessions " .
                     "   SET usersession_expires = DATE_ADD(NOW(), INTERVAL 30 MINUTE) " .
                     " WHERE usersession_id = '" .
                             mysql_escape_string($session_id_clean) . "'";
            mysql_query($query, $connect);
            setcookie('roscmsusrkey', $session_id_clean,
                      time() + 60 * 60, '/', cookie_domain());
          }
        }
      else
        {
        $userid = 0;
        }
    }
  else
    {
      $userid = 0;
    }

  if (0 == $userid && ROSCMS_LOGIN_REQUIRED == $login_type)
    {
      $url = "/roscms/?page=login";
      if ("" != $target)
        {
          $url .= "&target=" . urlencode($target);
        }
      header("Location: $url");
      exit;
    }

  return $userid;
}

?>
