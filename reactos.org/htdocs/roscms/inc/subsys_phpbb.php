<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Michael Wirth

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
  $seclog_section="subsys_phpbb";
  $seclog_level="50";
  $seclog_reason="Hacking attempt: subsys_phpbb.php";
  define ("ROSCMS_SYSTEM", "Hacking attempt");
  include('securitylog.php'); // open security log
  die("Hacking attempt");
}

require_once("subsys_utils.php");

define('SUBSYS_PHPBB_DBNAME', "forum");

function subsys_phpbb_info_check()
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id, u.user_name, u.user_email, u.user_register, " .
           "       p.username, p.user_email AS phpbb_email, " .
           "       FROM_UNIXTIME(p.user_regdate) AS phpbb_register " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_PHPBB_DBNAME . ".phpbb_users p " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'phpbb' " .
           "   AND p.user_id = m.map_subsys_userid " .
           "   AND (u.user_name != p.username OR " .
           "        u.user_email != p.user_email OR " .
           "        u.user_register != FROM_UNIXTIME(p.user_regdate)) ";
  $query_set = mysql_query($query) or die("DB error (subsys_phpbb #1)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "Info mismatch for RosCMS userid " . $result_row['user_id'] . ": ";
    if ($result_row['user_name'] <> $result_row['username'])
    {
      echo "user_name " . $result_row['user_name'] . "/" .
           $result_row['username'] . " ";
    }
    if ($result_row['user_email'] <> $result_row['phpbb_email'])
    {
      echo "user_email " . $result_row['user_email'] . "/" .
           $result_row['phpbb_email'];
    }
    if ($result_row['user_register'] <> $result_row['phpbb_register'])
    {
      echo "user_register " . $result_row['user_register'] . "/" .
           $result_row['phpbb_register'];
    }
    echo "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}

function subsys_phpbb_check()
{
?>
<h2>phpBB</h2>
<?php
  $inconsistency_count = 0;
  $inconsistency_count += subsys_mapping_check('phpbb');
  $inconsistency_count += subsys_userids_exist('phpbb',
                                               SUBSYS_PHPBB_DBNAME .
                                               '.phpbb_users',
                                               'user_id');
  $inconsistency_count += subsys_phpbb_info_check();

  $fix_url = htmlentities('?page=admin&sec=subsys&sec2=fix&subsys=phpbb');
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

function subsys_phpbb_update_phpbb($roscms_user_id,
                                   $roscms_user_name,
                                   $roscms_user_email,
                                   $roscms_user_register,
                                   $phpbb_user_id)
{
  /* Make sure that the email address and/or user name are not already in
     use in phpbb */
  $query = "SELECT COUNT(*) AS inuse " .
           "  FROM " . SUBSYS_PHPBB_DBNAME .  ".phpbb_users " .
           " WHERE (LOWER(username) = LOWER('" .
           mysql_real_escape_string($roscms_user_name) . "') OR " .
           "        LOWER(user_email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')) " .
           "   AND user_id <> $phpbb_user_id ";
  $phpbb_check_set = mysql_query($query)
                  or die("DB error (subsys_phpbb #7)");
  $phpbb_check_row = mysql_fetch_array($phpbb_check_set);
  if (0 != $phpbb_check_row['inuse'])
    {
      echo "User name ($roscms_user_name) and/or email address " .
           "($roscms_user_email) collision<br>\n";
      return FALSE;
    }

  /* Now, make sure that info in phpbb matches info in roscms */
  $query = "UPDATE " . SUBSYS_PHPBB_DBNAME .  ".phpbb_users " .
           "   SET username = '" .
                   mysql_real_escape_string($roscms_user_name) . "', " .
           "       user_email = '" .
                   mysql_real_escape_string($roscms_user_email) . "', " .
           "       user_regdate = $roscms_user_register " .
           " WHERE user_id = $phpbb_user_id";
  mysql_query($query) or die("DB error (subsys_phpbb #8)");

  return TRUE;
}

function subsys_phpbb_add_phpbb_user($roscms_user_id,
                                     $roscms_user_name,
                                     $roscms_user_email,
                                     $roscms_user_register)
{
  /* Determine the next available userid */
  $query = "SELECT MAX(user_id) AS last_user_id " .
           "  FROM " . SUBSYS_PHPBB_DBNAME . ".phpbb_users ";
  $user_id_set = mysql_query($query) or die("DB error (subsys_phpbb #20)");
  $user_id_row = mysql_fetch_array($user_id_set);
  $phpbb_user_id = $user_id_row['last_user_id'] + 1;
  
  $query = "INSERT INTO " . SUBSYS_PHPBB_DBNAME . ".phpbb_users " .
           "       (user_id, username, username_clean, user_password, user_email, user_regdate) " .
           "VALUES ($phpbb_user_id, " .
           "        '" . mysql_real_escape_string($roscms_user_name) .  "', " .
           "        '" . mysql_real_escape_string(strtolower($roscms_user_name)) .  "', " .
           "        '*', " .
           "        '" . mysql_real_escape_string($roscms_user_email) . "', " .
           "        $roscms_user_register)";
  mysql_query($query) or die("DB error (subsys_phpbb #10)");

  /* Put the user in the REGISTERED group */
  $query = "SELECT group_id FROM " . SUBSYS_PHPBB_DBNAME . ".phpbb_groups WHERE group_name = 'REGISTERED'";
  $result = mysql_query($query) or die("DB error (subsys_phpbb #18)");
  $group_id = (int)mysql_result($result, 0);
  
  if(!$group_id)
  	die("DB error (subsys_phpbb #20)");
  
  $query = "INSERT INTO " . SUBSYS_PHPBB_DBNAME . ".phpbb_user_group " .
           "       (group_id, user_id, user_pending) " .
           "VALUES ($group_id, $phpbb_user_id, 0)";
  mysql_query($query) or die("DB error (subsys_phpbb #19)");

  /* Finally, insert a row in the mapping table */
  $query = "INSERT INTO subsys_mappings " .
           "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
           "       VALUES($roscms_user_id, 'phpbb', $phpbb_user_id)";
  mysql_query($query) or die("DB error (subsys_phpbb #11)");

  return TRUE;
}

function subsys_phpbb_add_mapping($roscms_user_id)
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
           "  FROM " . SUBSYS_PHPBB_DBNAME .  ".phpbb_users " .
           " WHERE LOWER(user_email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')";
  $phpbb_email_set = mysql_query($query)
                    or die("DB error (subsys_phpbb #5)");
  if ($phpbb_email_row = mysql_fetch_array($phpbb_email_set))
    {
      $phpbb_user_id = $phpbb_email_row['user_id'];
    }
  else
    {
      /* That failed. Let's try to match on user name then */
      $query = "SELECT user_id " .
               "  FROM " . SUBSYS_PHPBB_DBNAME .  ".phpbb_users " .
               " WHERE LOWER(username) = LOWER('" .
               mysql_real_escape_string($roscms_user_name) . "')";
      $phpbb_name_set = mysql_query($query)
                     or die("DB error (subsys_phpbb #6)");
      if ($phpbb_name_row = mysql_fetch_array($phpbb_name_set))
        {
          $phpbb_user_id = $phpbb_name_row['user_id'];
        }
    }

  if (! isset($phpbb_user_id))
    {
      /* We haven't found a match, so we need to add a new phpbb user */
      $fixed = subsys_phpbb_add_phpbb_user($roscms_user_id,
                                           $roscms_user_name,
                                           $roscms_user_email,
                                           $roscms_user_register);
    }
  else
    {
      /* Synchronize the info in phpbb */
      if (! subsys_phpbb_update_phpbb($roscms_user_id,
                                      $roscms_user_name,
                                      $roscms_user_email,
                                      $roscms_user_register,
                                      $phpbb_user_id))
        {
          return FALSE;
        }

      /* Insert a row in the mapping table */
      $query = "INSERT INTO subsys_mappings " .
               "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
               "       VALUES($roscms_user_id, 'phpbb', $phpbb_user_id)";
      mysql_query($query) or die("DB error (subsys_phpbb #9)");

      $fixed = TRUE;
    }

  return $fixed;
}

function subsys_phpbb_update_existing($roscms_user_id, $phpbb_user_id)
{
  if (! subsys_get_roscms_info($roscms_user_id,
                               $roscms_user_name,
                               $roscms_user_email,
                               $roscms_user_fullname,
                               $roscms_user_register))
    {
      return FALSE;
    }

  if (! subsys_phpbb_update_phpbb($roscms_user_id,
                                  $roscms_user_name,
                                  $roscms_user_email,
                                  $roscms_user_register,
                                  $phpbb_user_id))
    {
      return FALSE;
    }

  return TRUE;
}

function subsys_phpbb_update_user($roscms_user_id)
{
  $query = "SELECT map_subsys_userid " .
           "  FROM subsys_mappings " .
           " WHERE map_roscms_userid = $roscms_user_id " .
           "   AND map_subsys_name = 'phpbb'";
  $query_set = mysql_query($query) or die("DB error (subsys_phpbb #2)");
  if ($result_row = mysql_fetch_array($query_set))
    {
      $fixed = subsys_phpbb_update_existing($roscms_user_id,
                                            $result_row['map_subsys_userid']);
    }
  else
    {
      $fixed = subsys_phpbb_add_mapping($roscms_user_id);
    }

  return $fixed;
}

function subsys_phpbb_fix_mappings()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u " .
           " WHERE u.user_id NOT IN " .
           "       (SELECT m.map_roscms_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_roscms_userid = u.user_id " .
           "           AND m.map_subsys_name = 'phpbb')";
  $query_set = mysql_query($query) or die("DB error (subsys_phpbb #3)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_phpbb_update_user($result_row['user_id']))
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

function subsys_phpbb_fix_info()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_PHPBB_DBNAME . ".phpbb_users p " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'phpbb' " .
           "   AND p.user_id = m.map_subsys_userid " .
           "   AND (u.user_name != p.username OR " .
           "        u.user_email != p.user_email OR " .
           "        u.user_register != FROM_UNIXTIME(p.user_regdate)) ";
  $query_set = mysql_query($query) or die("DB error (subsys_phpbb #12)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_phpbb_update_user($result_row['user_id']))
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

function subsys_phpbb_add_roscms_users()
{
  $fix_count = 0;

  $query = "SELECT p.user_id AS phpbb_user_id, " .
           "       p.username AS phpbb_user_name, " .
           "       p.user_email AS phpbb_user_email, " .
           "       p.user_password AS phpbb_user_password " .
           "  FROM " . SUBSYS_PHPBB_DBNAME . ".phpbb_users p " .
           " WHERE p.user_id NOT IN " .
           "       (SELECT m.map_subsys_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_subsys_userid = p.user_id " .
           "           AND m.map_subsys_name = 'phpbb')";
  $query_set = mysql_query($query) or die("DB error (subsys_phpbb #13)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      $phpbb_user_id = $result_row['phpbb_user_id'];
      $phpbb_user_name = $result_row['phpbb_user_name'];
      $phpbb_user_email = $result_row['phpbb_user_email'];
      $phpbb_user_password = $result_row['phpbb_user_password'];

      if ($phpbb_user_email == '')
        {
          echo "Can't add RosCMS user for subsys userid $phpbb_user_id, " .
               "no email address known<br>\n";
        }
      else if (subsys_add_roscms_user($phpbb_user_name,
                                      $phpbb_user_password,
                                      $phpbb_user_email,
                                      $phpbb_user_name,
                                      'phpbb',
                                      $phpbb_user_id))
        {
          $fix_count++;
        }
    }

  return $fix_count;
}

function subsys_phpbb_fix_all()
{
?>
<h2>phpBB</h2>
<?php
  $fix_count = 0;
  $fix_count += subsys_phpbb_fix_mappings();
  $fix_count += subsys_phpbb_add_roscms_users();
  $fix_count += subsys_phpbb_fix_info();
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

?>
