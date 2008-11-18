<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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

/**
 * class Export_User
 * 
 */
class Export_User extends Export
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   *
   * @return 
   * @access public
   */
  public function __construct( )
  {
    parent::__construct();
    $this->search();
  }


  /**
   *
   * @return 
   * @access public
   */
  public function search( )
  {
    global $roscms_security_level;
    global $roscms_intern_account_id;

    global $RosCMS_GET_d_use;
    global $RosCMS_GET_d_flag;
    global $RosCMS_GET_d_value;
    global $RosCMS_GET_d_value2;

    $usage = $RosCMS_GET_d_use;
    $flag = $RosCMS_GET_d_flag;
    $user_id = $RosCMS_GET_d_value;
    $search_string = $RosCMS_GET_d_value;
    $group_id = $RosCMS_GET_d_value2;
    $new_lang = $RosCMS_GET_d_value2;
    $search_type = $RosCMS_GET_d_value2;

    if (ROSUser::isMemberOfGroup('transmaint') || $roscms_security_level == 3) {
      if ($usage == 'usrtbl') {

        if (ROSUser::isMemberOfGroup('transmaint')) {
          $stmt=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
          $stmt->bindParam('user_id',$roscms_intern_account_id);
          $stmt->execute();
          $user_lang = $stmt->fetchColumn();

          if ($user_lang === false ) {
            die('Please set a valid language in the myReactOS settings!');
          }
        }
        else {
          $user_lang = false;
        }

        switch ($flag) {
          case 'addmembership':
            $stmt=DBConnection::getInstance()->prepare("INSERT INTO usergroup_members ( usergroupmember_userid , usergroupmember_usergroupid ) VALUES ( :user_id, :group_id )");
            $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
            $stmt->bindParam('group_id',$group_id,PDO::PARAM_INT);
            $stmt->execute();
            if ($user_lang !== false) {
              Log::writeLangMedium("add user account membership: user-id=".$user_id.", group-id=".$RosCMS_GET_d_value2." done by ".$roscms_intern_account_id." {data_user_out}", $user_lang);
            }
            Log::writeMedium('add user account membership: user-id='.$user_id.', group-id='.$group_id.' done by '.$roscms_intern_account_id.' {data_user_out}');
            $flag = 'detail';
            break;

          case 'delmembership':
            $stmt=DBConnection::getInstance()->prepare("DELETE FROM usergroup_members WHERE usergroupmember_userid = :user_id AND usergroupmember_usergroupid = :group_id LIMIT 1");
            $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
            $stmt->bindParam('group_id',$group_id,PDO::PARAM_INT);
            $stmt->execute();
            if ($user_lang !== false) {
              Log::writeLangMedium('delete user account membership: user-id='.$user_id.', group-id='.$group_id.' done by '.$roscms_intern_account_id.' {data_user_out}', $user_lang);
            }
            Log::writeMedium('delete user account membership: user-id='.$user_id.', group-id='.$group_id.' done by '.$roscms_intern_account_id.' {data_user_out}');
            $flag = 'detail';
            break;

          case 'upateusrlang':
            $stmt=DBConnection::getInstance()->prepare("UPDATE users SET user_timestamp_touch2 = NOW(), user_language = :lang WHERE user_id = :user_id LIMIT 1");
            $stmt->bindParam('lang',$group_id);
            $stmt->bindParam('user_id',$user_id);
            $stmt->execute();
            if ($user_lang) {
              Log::writeLangMedium('change user account language: user-id='.$user_id.', lang-id='.$group_id.' done by '.$roscms_intern_account_id.' {data_user_out}', $user_lang);
            }
            Log::writeMedium('change user account language: user-id='.$user_id.', lang-id='.$group_id.' done by '.$roscms_intern_account_id.' {data_user_out}');
            $flag = "detail";
            break;
        }

        // list / details
        switch ($flag) {
          case 'list':
            if (strlen($RosCMS_GET_d_value) > 2) {
              echo_strip('
                <fieldset>
                  <legend>Results</legend>
                  <ul>');

              switch ($search_type) {
                case 'fullname':
                  $sql_search = "u.user_fullname";
                  break;
                case 'email':
                  $sql_search = "u.user_email";
                  break;
                case 'website':
                  $sql_search = "u.user_website";
                  break;
                case 'language':
                  $sql_search = "u.user_language";
                  break;
                case 'accountname':
                default:
                  $sql_search = "u.user_name";
                  break;
              }

              $stmt=DBConnection::getInstance()->prepare("SELECT u.user_id, u.user_name, u.user_fullname, u.user_language FROM users u WHERE ". $sql_search ." LIKE :value ORDER BY u.user_name ASC LIMIT 25");
              $stmt->bindValue('value','%'.$search_string.'%',PDO::PARAM_INT);
              $stmt->execute();
              $users = $stmt->fetchAll(PDO::FETCH_ASSOC);
              foreach ( $users as $user) {
                echo_strip('
                  <li>
                    <a href="'."javascript:getuserdetails('".$user['user_id']."')".'">'.$user['user_name'].'</a>
                    ('.$user['user_language'].', '.$user['user_fullname'].')
                  </li>');
              }
              echo '</ul>';

              if (count($users) == 25) {
                echo '<p>... more than 25 users</p>';
              }

              echo '</fieldset><br />';
            }
            else {
              echo "<p>more than 2 characters requiered</p>";
            }
            break;

          case 'detail':
            $stmt=DBConnection::getInstance()->prepare("SELECT user_id, user_name, user_timestamp_touch2 AS visit, user_login_counter AS visitcount, user_register, user_fullname, user_email, user_language FROM users WHERE user_id = :user_id LIMIT 1");
            $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
            $stmt->execute();
            $user = $stmt->fetchOnce();

            echo_strip('
              <fieldset>
                <legend>Details for \''.$user['user_name'].'\'</legend>
                <p><strong>Name:</strong> '.$user['user_name'].' ('.$user['user_fullname'].') ['.$user['user_id'].']</p>
                <p><strong>Lang:</strong> '.$user['user_language'].'</p>');
            if ($roscms_security_level == 3) {
              echo_strip('
                <p><strong>E-Mail:</strong> '.$user['user_email'].'</p>
                <p><strong>Latest Login:</strong> '.$user['visit'].'; '.$user['visitcount'].' logins</p>
                <p><strong>Registered:</strong> '.$user['user_register'].'</p>');
            }
            echo_strip('
                <fieldset>
                  <legend>Usergroup Memberships</legend>
                  <ul>');

            $stmt=DBConnection::getInstance()->prepare("SELECT g.usrgroup_name_id, g.usrgroup_name FROM users u, usergroups g, usergroup_members m WHERE user_id = :user_id AND u.user_id = m.usergroupmember_userid AND g.usrgroup_name_id = m.usergroupmember_usergroupid ORDER BY g.usrgroup_name ASC");
            $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
            $stmt->execute();
            while ($user = $stmt->fetch(PDO::FETCH_ASSOC)) {

              echo '<li>'.$user['usrgroup_name'].' ';
              if ($roscms_security_level == 3) {
                echo_strip('
                  &nbsp;
                  <span class="frmeditbutton" onclick="'."delmembership(".$user_id.", '".$user['usrgroup_name_id']."')".'">
                    <img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
                    &nbsp;Delete
                  </span>');
              }
              echo '</li>';
            } // end while
            echo '</ul>';

            if ($roscms_security_level == 3) {
              echo '<select id="cbmmemb" name="cbmmemb">';
              $stmt=DBConnection::getInstance()->prepare("SELECT usrgroup_name_id, usrgroup_name FROM usergroups WHERE usrgroup_seclev  <= :sec_level ORDER BY usrgroup_name ASC");
              $stmt->bindParam('sec_level',$roscms_security_level,PDO::PARAM_INT);
              $stmt->execute();
              while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {

                // only super admin can give super admin rights
                if (ROSUser::isMemberOfGroup('ros_sadmin') || $group['usrgroup_name_id'] != 'ros_sadmin') {
                  echo '<option value="'.$group['usrgroup_name_id'].'">'.$group['usrgroup_name'].'</option>';
                }
              }
              echo_strip('</select>
                <input type="button" name="addmemb" id="addmemb" value="Add Membership" onclick="'."addmembership(".$user_id.", document.getElementById('cbmmemb').value)".'" />
                <br />
                <br />
                <select id="cbmusrlang" name="cbmusrlang">');
              $stmt=DBConnection::getInstance()->prepare("SELECT lang_id , lang_name FROM languages ORDER BY lang_name ASC");
              $stmt->execute();
              while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {
                echo '<option value="'.$lang['lang_id'].'">'.$lang['lang_name'].'</option>';
              }
              echo_strip('</select>
              <input type="button" name="addusrlang" id="addusrlang" value="Update User language" onclick="'."updateusrlang(".$user_id.", document.getElementById('cbmusrlang').value)".'" /><br />');
            }
            elseif (ROSUser::isMemberOfGroup('transmaint')) {
              echo_strip('<input type="button" name="addmemb" id="addmemb" value="Make this User a Translator" onclick="'."addmembership(".$user_id.", 'translator')".'" />
                <br />
                <br />');
              $stmt=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
              $stmt->bindParam('user_id',$roscms_intern_account_id,PDO::PARAM_INT);
              $stmt->execute();
              $user_lang = $stmt->fetchColumn();

              if ($user_lang != '') {
                echo '<input type="button" name="addusrlang" id="addusrlang" value="Switch User language to \''.$user_lang.'\'" onclick="'."updateusrlang(".$user_id.", '".$user_lang.")".'" /><br />';
              }

            }
            echo '</fieldset><br />';
            break;
        }
      }
    }
  }


} // end of Export_User
?>
