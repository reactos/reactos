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
	$seclog_section="subsys_wiki";
	$seclog_level="50";
	$seclog_reason="Hacking attempt: subsys_wiki.php";
	define ("ROSCMS_SYSTEM", "Hacking attempt");
	include('securitylog.php'); // open security log
	die("Hacking attempt");
}

require_once("subsys_utils.php");
require_once("utils.php");

define('SUBSYS_WIKI_DBNAME', "wiki");

function subsys_wiki_info_check()
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id, u.user_name, u.user_email, user_fullname, " .
           "       p.user_name AS subsys_name, p.user_email AS subsys_email, " .
           "       p.user_real_name " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_WIKI_DBNAME . ".user p " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'wiki' " .
           "   AND p.user_id = m.map_subsys_userid " .
           "   AND (REPLACE(u.user_name, '_', ' ') != p.user_name OR " .
           "        u.user_email != p.user_email OR " .
           "        u.user_fullname != p.user_real_name) ";
  $query_set = mysql_query($query) or die("DB error (subsys_wiki #1)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "Info mismatch for RosCMS userid " . $result_row['user_id'] . ": ";
    if ($result_row['user_name'] <> $result_row['subsys_name'])
    {
      echo "user_name " . $result_row['user_name'] . "/" .
           $result_row['subsys_name'] . " ";
    }
    if ($result_row['user_email'] <> $result_row['subsys_email'])
    {
      echo "user_email " . $result_row['user_email'] . "/" .
           $result_row['subsys_email'] . " ";
    }
    if ($result_row['user_fullname'] <> $result_row['user_real_name'])
    {
      echo "user_fullname " . $result_row['user_fullname'] . "/" .
           $result_row['user_real_name'] . " ";
    }
    echo "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}

function subsys_wiki_check()
{
?>
<h2>Wiki</h2>
<?php
  $inconsistency_count = 0;
  $inconsistency_count += subsys_mapping_check('wiki');
  $inconsistency_count += subsys_userids_exist('wiki',
                                               SUBSYS_WIKI_DBNAME .
                                               '.user',
                                               'user_id');
  $inconsistency_count += subsys_wiki_info_check();
  $fix_url = htmlentities('?page=admin&sec=subsys&sec2=fix&subsys=wiki');
  if (0 == $inconsistency_count)
    {
      echo "No problems found.<br/>\n";
    }
  else if (1 == $inconsistency_count)
    {
      echo '<br />1 problem found. <a href="' . $fix_url .
           '">Fix this</a><br/>' . "\n";
    }
  else
    {
      echo "<br />$inconsistency_count problems found." .
           ' <a href="' . $fix_url . '">Fix these</a><br/>' . "\n";
    }

  return $inconsistency_count;
}

function subsys_wiki_update_wiki($roscms_user_id,
                                 $roscms_user_name,
                                 $roscms_user_email,
                                 $roscms_user_fullname,
                                 $wiki_user_id)
{
	$wiki_sql_user_name = mysql_real_escape_string(str_replace("_", " ", $roscms_user_name));
	
  /* Make sure that the email address and/or user name are not already in
     use in wiki */
  $query = "SELECT COUNT(*) AS inuse " .
           "  FROM " . SUBSYS_WIKI_DBNAME .  ".user " .
           " WHERE (LOWER(user_name) = LOWER('" . $wiki_sql_user_name . "') OR " .
           "        LOWER(user_email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')) " .
           "   AND user_id <> $wiki_user_id ";
  $wiki_check_set = mysql_query($query)
                  or die("DB error (subsys_wiki #7)");
  $wiki_check_row = mysql_fetch_array($wiki_check_set);
  if (0 != $wiki_check_row['inuse'])
    {
      echo "User name ($roscms_user_name) and/or email address " .
           "($roscms_user_email) collision<br>\n";
      return FALSE;
    }

  /* Now, make sure that info in wiki matches info in roscms */
  $query = "UPDATE " . SUBSYS_WIKI_DBNAME .  ".user " .
           "   SET user_name = '" .
                   $wiki_sql_user_name . "', " .
           "       user_email = '" .
                   mysql_real_escape_string($roscms_user_email) . "', " .
           "       user_real_name = '" .
                   mysql_real_escape_string($roscms_user_fullname) . "' " .
           " WHERE user_id = $wiki_user_id";
  mysql_query($query) or die("DB error (subsys_wiki #8)");

  return TRUE;
}

function subsys_wiki_add_wiki_user($roscms_user_id,
                                   $roscms_user_name,
                                   $roscms_user_email,
                                   $roscms_user_fullname)
{
  $query = "INSERT INTO " . SUBSYS_WIKI_DBNAME . ".user " .
           "       (user_name, user_real_name, user_password, " .
           "        user_newpassword, user_email, user_options, " .
           "        user_touched)" .
           "VALUES (REPLACE('" . mysql_real_escape_string($roscms_user_name) .  "', '_', ' '), " .
           "        '" . mysql_real_escape_string($roscms_user_fullname) . "', " .
           "        '', " .
           "        '', " .
           "        '" . mysql_real_escape_string($roscms_user_email) . "', " .
           "        '', " .
           "        DATE_FORMAT(NOW(), '%Y%m%d%H%i%s'));";
  mysql_query($query) or die("DB error (subsys_wiki #10)");

  /* Finally, insert a row in the mapping table */
  $query = "INSERT INTO subsys_mappings " .
           "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
           "       VALUES($roscms_user_id, 'wiki', LAST_INSERT_ID())";
  mysql_query($query) or die("DB error (subsys_wiki #11)");

  return TRUE;
}

function subsys_wiki_add_mapping($roscms_user_id)
{
  if (! subsys_get_roscms_info($roscms_user_id,
                               $roscms_user_name,
                               $roscms_user_email,
                               $roscms_user_fullname,
                               $roscms_user_register))
    {
      return FALSE;
    }

  /* First, try to match on email address */
  $query = "SELECT user_id " .
           "  FROM " . SUBSYS_WIKI_DBNAME .  ".user " .
           " WHERE LOWER(user_email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')";
  $wiki_email_set = mysql_query($query)
                    or die("DB error (subsys_wiki #5)");
  if ($wiki_email_row = mysql_fetch_array($wiki_email_set))
    {
      $wiki_user_id = $wiki_email_row['user_id'];
    }
  else
    {
      /* That failed. Let's try to match on user name then */
      $query = "SELECT user_id " .
               "  FROM " . SUBSYS_WIKI_DBNAME .  ".user " .
               " WHERE LOWER(user_name) = LOWER(REPLACE('" .
               mysql_real_escape_string($roscms_user_name) . "', '_', ' '))";
      $wiki_name_set = mysql_query($query)
                     or die("DB error (subsys_wiki #6)");
      if ($wiki_name_row = mysql_fetch_array($wiki_name_set))
        {
          $wiki_user_id = $wiki_name_row['user_id'];
        }
    }

  if (! isset($wiki_user_id))
    {
      /* We haven't found a match, so we need to add a new wiki user */
      $fixed = subsys_wiki_add_wiki_user($roscms_user_id,
                                         $roscms_user_name,
                                         $roscms_user_email,
                                         $roscms_user_fullname);
    }
  else
    {
      /* Synchronize the info in wiki */
      if (! subsys_wiki_update_wiki($roscms_user_id,
                                    $roscms_user_name,
                                    $roscms_user_email,
                                    $roscms_user_fullname,
                                    $wiki_user_id))
        {
          return FALSE;
        }

      /* Insert a row in the mapping table */
      $query = "INSERT INTO subsys_mappings " .
               "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
               "       VALUES($roscms_user_id, 'wiki', $wiki_user_id)";
      mysql_query($query) or die("DB error (subsys_wiki #9)");

      $fixed = TRUE;
    }

  return $fixed;
}

function subsys_wiki_update_existing($roscms_user_id, $wiki_user_id)
{
  if (! subsys_get_roscms_info($roscms_user_id,
                               $roscms_user_name,
                               $roscms_user_email,
                               $roscms_user_fullname,
                               $roscms_user_register))
    {
      return FALSE;
    }

  if (! subsys_wiki_update_wiki($roscms_user_id,
                                $roscms_user_name,
                                $roscms_user_email,
                                $roscms_user_fullname,
                                $wiki_user_id))
    {
      return FALSE;
    }

  return TRUE;
}

function subsys_wiki_update_user($roscms_user_id)
{
  $query = "SELECT map_subsys_userid " .
           "  FROM subsys_mappings " .
           " WHERE map_roscms_userid = $roscms_user_id " .
           "   AND map_subsys_name = 'wiki'";
  $query_set = mysql_query($query) or die("DB error (subsys_wiki #2)");
  if ($result_row = mysql_fetch_array($query_set))
    {
      $fixed = subsys_wiki_update_existing($roscms_user_id,
                                           $result_row['map_subsys_userid']);
    }
  else
    {
      $fixed = subsys_wiki_add_mapping($roscms_user_id);
    }

  return $fixed;
}

function subsys_wiki_fix_mappings()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u " .
           " WHERE u.user_id NOT IN " .
           "       (SELECT m.map_roscms_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_roscms_userid = u.user_id " .
           "           AND m.map_subsys_name = 'wiki')";
  $query_set = mysql_query($query) or die("DB error (subsys_wiki #3)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_wiki_update_user($result_row['user_id']))
        {
          echo "Fixed mapping of RosCMS userid " . $result_row['user_id'] .
               "<br>\n";
          $fix_count++;
        }
      else
        {
          echo "Unable to fix mapping of RosCMS userid " .
               $result_row['user_id'] .  "<br>\n";
        }
    }

  return $fix_count;
}

function subsys_wiki_fix_info()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_WIKI_DBNAME . ".user w " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'wiki' " .
           "   AND w.user_id = m.map_subsys_userid " .
           "   AND (REPLACE(u.user_name, '_', ' ') != w.user_name OR " .
           "        u.user_email != w.user_email OR " .
           "        u.user_fullname != w.user_real_name) ";
  $query_set = mysql_query($query) or die("DB error (subsys_wiki #12)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_wiki_update_user($result_row['user_id']))
        {
          echo "Fixed info for RosCMS userid " . $result_row['user_id'] .
               "<br>\n";
          $fix_count++;
        }
      else
        {
          echo "Unable to fix info for RosCMS userid " .
               $result_row['user_id'] .  "<br>\n";
        }
    }

  return $fix_count;
}

function subsys_wiki_add_roscms_users()
{
  $fix_count = 0;

  $query = "SELECT w.user_id AS wiki_user_id, w.user_name AS wiki_user_name, " .
           "       w.user_email AS wiki_user_email, " .
           "       w.user_real_name AS wiki_user_fullname " .
           "  FROM " . SUBSYS_WIKI_DBNAME . ".user w " .
           " WHERE w.user_id NOT IN " .
           "       (SELECT m.map_subsys_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_subsys_userid = w.user_id " .
           "           AND m.map_subsys_name = 'wiki')";
  $query_set = mysql_query($query) or die("DB error (subsys_wiki #13)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      $wiki_user_id = $result_row['wiki_user_id'];
      $wiki_user_name = $result_row['wiki_user_name'];
      $wiki_user_email = $result_row['wiki_user_email'];
      $wiki_user_fullname = $result_row['wiki_user_fullname'];

      if ($wiki_user_fullname == '')
        {
          $wiki_user_fullname = $wiki_user_name;
        }
      if ($wiki_user_email == '')
        {
          echo "Can't add RosCMS user for subsys userid $wiki_user_id, " .
               "no email address known<br>\n";
        }
      else if (subsys_add_roscms_user($wiki_user_name,
                                 '*UNKNOWN*',
                                 $wiki_user_email,
                                 $wiki_user_fullname,
                                 'wiki',
                                 $wiki_user_id))
        {
          $fix_count++;
        }
    }

  return $fix_count;
}

function subsys_wiki_fix_all()
{
?>
<h2>Wiki</h2>
<?php
  $fix_count = 0;
  $fix_count += subsys_wiki_fix_mappings();
  $fix_count += subsys_wiki_add_roscms_users();
  $fix_count += subsys_wiki_fix_info();
  if (0 == $fix_count)
    {
      echo "No problems fixed.<br/>\n";
    }
  else if (1 == $fix_count)
    {
      echo "<br>1 problem fixed.<br/>\n";
    }
  else
    {
      echo "<br>$fix_count problems fixed.<br/>\n";
    }

  return $fix_count;
}

function subsys_wiki_user_exists($table_name, $column_name, $wiki_user_id)
{
  $exists = row_exists(SUBSYS_WIKI_DBNAME . ".$table_name",
                       "$column_name = $wiki_user_id");
  if ($exists)
    {
      echo "References to $wiki_user_id found: $table_name.$column_name<br>\n";
    }

  return $exists;
}

function subsys_wiki_can_delete($wiki_user_id)
{
  $can_delete = TRUE;
  if (subsys_wiki_user_exists('archive', 'ar_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('cur', 'cur_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('image', 'img_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('ipblocks', 'ipb_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('logging', 'log_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('old', 'old_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('oldimage', 'oi_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('recentchanges', 'rc_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('user_newtalk', 'user_id', $wiki_user_id))
    {
      $can_delete = FALSE;
    }
  if (subsys_wiki_user_exists('watchlist', 'wl_user', $wiki_user_id))
    {
      $can_delete = FALSE;
    }

  return $can_delete;
}
?>
