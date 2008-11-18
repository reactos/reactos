<?php

// To prevent hacking activity:
if ( !defined('ROSCMS_SYSTEM'))
{
  if ( !defined('ROSCMS_SYSTEM_LOG') ) {
    define ("ROSCMS_SYSTEM_LOG", "Hacking attempt");
  }
  $seclog_section="subsys_blogs";
  $seclog_level="50";
  $seclog_reason="Hacking attempt: subsys_blogs.php";
  define ("ROSCMS_SYSTEM", "Hacking attempt");
  include('securitylog.php'); // open security log
  die("Hacking attempt");
}

require_once("subsys_utils.php");

define('SUBSYS_BLOGS_DBNAME', "blogs");

function subsys_blogs_info_check()
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id, u.user_name, u.user_fullname, u.user_email, " .
           "       b.username, b.realname, b.email " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_BLOGS_DBNAME . ".ser_authors b " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'blogs' " .
           "   AND b.authorid = m.map_subsys_userid " .
           "   AND (u.user_name != b.username OR " .
           "        u.user_fullname != b.realname OR " .
           "        u.user_email != b.email) ";
  $query_set = mysql_query($query) or die("DB error (subsys_blogs #1)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "Info mismatch for RosCMS userid " . $result_row['user_id'] . ": ";
    if ($result_row['user_name'] <> $result_row['username'])
    {
      echo "user_login " . $result_row['user_name'] . "/" .
           $result_row['username'] . " ";
    }
    if ($result_row['user_email'] <> $result_row['email'])
    {
      echo "user_email " . $result_row['user_email'] . "/" .
           $result_row['email'];
    }
    if ($result_row['user_fullname'] <> $result_row['realname'])
    {
      echo "user_fullname " . $result_row['user_fullname'] . "/" .
           $result_row['realname'];
    }
    echo "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}

function subsys_blogs_mapping_check()
{
  $inconsistency_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u, " .
           "       usergroup_members ug " .
           " WHERE ug.usergroupmember_usergroupid = 'developer' " .
           "   AND ug.usergroupmember_userid = u.user_id " .
           "   AND u.user_id NOT IN " .
           "       (SELECT m.map_roscms_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_roscms_userid = u.user_id " .
           "           AND m.map_subsys_name = 'blogs') ";
  $query_set = mysql_query($query) or die("DB error (subsys_blogs #4)");
  while ($result_row = mysql_fetch_array($query_set))
    {
    echo "No mapping of RosCMS userid " . $result_row['user_id'] .
         "<br>\n";
    $inconsistency_count++;
    }

  return $inconsistency_count;
}


function subsys_blogs_check()
{
?>
<h2>Blogs</h2>
<?php
  $inconsistency_count = 0;
  $inconsistency_count += subsys_blogs_mapping_check();
  $inconsistency_count += subsys_userids_exist('blogs',
                                               SUBSYS_BLOGS_DBNAME .
                                               '.ser_authors',
                                               'authorid');
  $inconsistency_count += subsys_blogs_info_check();

  $fix_url = htmlentities('?page=admin&sec=subsys&sec2=fix&subsys=blogs');
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

function subsys_blogs_update_blogs($roscms_user_id,
                                   $roscms_user_name,
                                   $roscms_user_fullname,
                                   $roscms_user_email,
                                   $blogs_user_id)
{
  /* Make sure that the email address and/or user name are not already in
     use in blogs */
  $query = "SELECT COUNT(*) AS inuse " .
           "  FROM " . SUBSYS_BLOGS_DBNAME .  ".ser_authors " .
           " WHERE (LOWER(username) = LOWER('" .
           mysql_real_escape_string($roscms_user_name) . "') OR " .
           "        LOWER(email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')) " .
           "   AND authorid <> $blogs_user_id ";
  $blogs_check_set = mysql_query($query)
                  or die("DB error (subsys_blogs #7)");
  $blogs_check_row = mysql_fetch_array($blogs_check_set);
  if (0 != $blogs_check_row['inuse'])
    {
      echo "User name ($roscms_user_name) and/or email address " .
           "($roscms_user_email) collision<br>\n";
      return FALSE;
    }

  /* Now, make sure that info in blogs matches info in roscms */
  $query = "UPDATE " . SUBSYS_BLOGS_DBNAME .  ".ser_authors " .
           "   SET realname = '" .
                   mysql_real_escape_string($roscms_user_fullname) . "', " .
           "       username = '" .
                   mysql_real_escape_string($roscms_user_name) . "', " .
           "       email = '" .
                   mysql_real_escape_string($roscms_user_email) . "' " .
           " WHERE authorid = $blogs_user_id";
  mysql_query($query) or die("DB error (subsys_blogs #8)");

  return TRUE;
}

function subsys_blogs_add_blogs_user($roscms_user_id,
                                     $roscms_user_name,
                                     $roscms_user_fullname,
                                     $roscms_user_email)
{
  $query = "INSERT INTO " . SUBSYS_BLOGS_DBNAME . ".ser_authors " .
           "       (realname, username, email) " .
           "VALUES (" .
           "        '" . mysql_real_escape_string($roscms_user_fullname) .  "', " .
           "        '" . mysql_real_escape_string($roscms_user_name) . "', " .
           "        '" . mysql_real_escape_string($roscms_user_email) . "') ";
  mysql_query($query) or die("DB error (subsys_phpbb #17)");

  /* Add user to Developers group */
  $query = "INSERT INTO " . SUBSYS_BLOGS_DBNAME . ".ser_authorgroups " .
           "       (groupid, authorid) " .
           "SELECT id, LAST_INSERT_ID() " .
           "  FROM " . SUBSYS_BLOGS_DBNAME . ".ser_groups " .
           " WHERE LOWER(name) = 'developer' ";
  mysql_query($query) or die("DB error (subsys_phpbb #18)");

  /* Finally, insert a row in the mapping table */
  $query = "INSERT INTO subsys_mappings " .
           "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
           "       VALUES($roscms_user_id, 'blogs', LAST_INSERT_ID())";
  mysql_query($query) or die("DB error (subsys_phpbb #11)");

  return TRUE;
}

function subsys_blogs_add_mapping($roscms_user_id)
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
  $query = "SELECT authorid " .
           "  FROM " . SUBSYS_BLOGS_DBNAME .  ".ser_authors " .
           " WHERE LOWER(email) = LOWER('" .
           mysql_real_escape_string($roscms_user_email) . "')";
  $blogs_email_set = mysql_query($query)
                     or die("DB error (subsys_blogs #5)");
  if ($blogs_email_row = mysql_fetch_array($blogs_email_set))
    {
      $blogs_user_id = $blogs_email_row['authorid'];
    }
  else
    {
      /* That failed. Let's try to match on user name then */
      $query = "SELECT authorid " .
               "  FROM " . SUBSYS_BLOGS_DBNAME .  ".ser_authors " .
               " WHERE LOWER(username) = LOWER('" .
               mysql_real_escape_string($roscms_user_name) . "')";
      $blogs_name_set = mysql_query($query)
                     or die("DB error (subsys_blogs #6)");
      if ($blogs_name_row = mysql_fetch_array($blogs_name_set))
        {
          $blogs_user_id = $blogs_name_row['authorid'];
        }
    }

  if (! isset($blogs_user_id))
    {
      /* We haven't found a match, so we need to add a new blogs user */
      $fixed = subsys_blogs_add_blogs_user($roscms_user_id,
                                           $roscms_user_name,
                                           $roscms_user_fullname,
                                           $roscms_user_email);
    }
  else
    {
      /* Synchronize the info in blogs */
      if (! subsys_blogs_update_blogs($roscms_user_id,
                                      $roscms_user_name,
                                      $roscms_user_fullname,
                                      $roscms_user_email,
                                      $blogs_user_id))
        {
          return FALSE;
        }

      /* Insert a row in the mapping table */
      $query = "INSERT INTO subsys_mappings " .
               "       (map_roscms_userid, map_subsys_name, map_subsys_userid) " .
               "       VALUES($roscms_user_id, 'blogs', $blogs_user_id)";
      mysql_query($query) or die("DB error (subsys_blogs #9)");

      $fixed = TRUE;
    }

  return $fixed;
}

function subsys_blogs_update_existing($roscms_user_id, $blogs_user_id)
{
  if (! subsys_get_roscms_info($roscms_user_id,
                               $roscms_user_name,
                               $roscms_user_email,
                               $roscms_user_fullname,
                               $roscms_user_register))
    {
      return FALSE;
    }

  if (! subsys_blogs_update_blogs($roscms_user_id,
                                  $roscms_user_name,
                                  $roscms_user_fullname,
                                  $roscms_user_email,
                                  $blogs_user_id))
    {
      return FALSE;
    }

  return TRUE;
}

function subsys_blogs_update_user($roscms_user_id)
{
  $query = "SELECT map_subsys_userid " .
           "  FROM subsys_mappings " .
           " WHERE map_roscms_userid = $roscms_user_id " .
           "   AND map_subsys_name = 'blogs'";
  $query_set = mysql_query($query) or die("DB error (subsys_blogs #2)");
  if ($result_row = mysql_fetch_array($query_set))
    {
      $fixed = subsys_blogs_update_existing($roscms_user_id,
                                            $result_row['map_subsys_userid']);
    }
  else
    {
      $query = "SELECT COUNT(*) AS count " .
               "  FROM usergroup_members ug " .
               " WHERE ug.usergroupmember_usergroupid = 'developer' " .
               "   AND ug.usergroupmember_userid = $roscms_user_id ";
      $query_set = mysql_query($query) or die("DB error (subsys_blogs #14)");
      if (($result_row = mysql_fetch_array($query_set)) &&
          0 != $result_row['count'])
        {
          $fixed = subsys_blogs_add_mapping($roscms_user_id);
        }
      else
        {
          $fixed = 0;
        }
    }

  return $fixed;
}

function subsys_blogs_fix_mappings()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u, " .
           "       usergroup_members ug " .
           " WHERE ug.usergroupmember_usergroupid = 'developer' " .
           "   AND ug.usergroupmember_userid = u.user_id " .
           "   AND u.user_id NOT IN " .
           "       (SELECT m.map_roscms_userid " .
           "          FROM subsys_mappings m " .
           "         WHERE m.map_roscms_userid = u.user_id " .
           "           AND m.map_subsys_name = 'blogs') ";
  $query_set = mysql_query($query) or die("DB error (subsys_blogs #3)");
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_blogs_update_user($result_row['user_id']))
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

function subsys_blogs_fix_info()
{
  $fix_count = 0;
  $query = "SELECT u.user_id " .
           "  FROM users u, " .
           "       subsys_mappings m, " .
                   SUBSYS_BLOGS_DBNAME . ".ser_authors b " .
           " WHERE m.map_roscms_userid = u.user_id " .
           "   AND m.map_subsys_name = 'blogs' " .
           "   AND b.authorid = m.map_subsys_userid " .
           "   AND (u.user_name != b.username OR " .
           "        u.user_fullname != b.realname OR " .
           "        u.user_email != b.email) ";
  $query_set = mysql_query($query) or die("DB error (subsys_blogs #12)".$query);
  while ($result_row = mysql_fetch_array($query_set))
    {
      if (subsys_blogs_update_user($result_row['user_id']))
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

function subsys_blogs_fix_all()
{
?>
<h2>Blogs</h2>
<?php
  $fix_count = 0;
  $fix_count += subsys_blogs_fix_mappings();
  $fix_count += subsys_blogs_fix_info();
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
