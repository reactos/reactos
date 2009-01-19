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
    $thisuser = &ThisUser::getInstance();
    $display = ''; // list / user details

    if (!$thisuser->hasAccess('user')) {
      return;
    }

    if (!$thisuser->hasAccess('more_lang')) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT lang_id FROM ".ROSCMST_USERS." WHERE id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',$thisuser->id());
      $stmt->execute();
      $user_lang = $stmt->fetchColumn();

      if ($user_lang === false ) {
        die('Please set a valid language in the Account settings!');
      }
    }
    else {
      $user_lang = false;
    }

    if (isset($_GET['d_fl'])) {
      // do some actions
      switch ($_GET['d_fl']) {
        case 'addmembership':
          // check if user is already member, so we don't add him twice
          // also check that you don't give accounts a higher seclevel
          $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT g.security_level FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE g.id = :group_id AND m.user_id != :user_id LIMIT 1");
          $stmt->bindParam('group_id',$_GET['group'],PDO::PARAM_INT);
          $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
          $stmt->execute();
          $level = $stmt->fetchColumn();

          if ($level !== false && $thisuser->hasAccess('addlvl'.$level.'group')) {

            // insert new membership
            $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_MEMBERSHIPS." ( user_id , group_id ) VALUES ( :user_id, :group_id )");
            $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
            $stmt->bindParam('group_id',$_GET['group'],PDO::PARAM_INT);
            $stmt->execute();
            if ($user_lang !== false) {
              Log::writeLangMedium('add user account membership: user-id='.$_GET['user'].', group-id='.$_GET['group'].' done by '.$thisuser->id().' {data_user_out}', $user_lang);
            }
            Log::writeMedium('add user account membership: user-id='.$_GET['user'].', group-id='.$_GET['group'].' done by '.$thisuser->id().' {data_user_out}');
          }
          // preselect displayed content
          $display = 'detail';
          break;

        case 'delmembership':
          $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_MEMBERSHIPS." WHERE user_id = :user_id AND group_id = :group_id LIMIT 1");
          $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
          $stmt->bindParam('group_id',$_GET['group'],PDO::PARAM_INT);
          $stmt->execute();
          if ($user_lang !== false) {
            Log::writeLangMedium('delete user account membership: user-id='.$_GET['user'].', group-id='.$_GET['group'].' done by '.$thisuser->id().' {data_user_out}', $user_lang);
          }
          Log::writeMedium('delete user account membership: user-id='.$_GET['user'].', group-id='.$_GET['group'].' done by '.$thisuser->id().' {data_user_out}');
          // preselect displayed content
          $display = 'detail';
          break;

        case 'accountdisable':
          // only with admin rights
          if ($thisuser->hasAccess('disableaccount')) {
            $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET disabled = TRUE WHERE id = :user_id");
            $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
            $stmt->execute();
          }
          // preselect displayed content
          $display = 'detail';
          break;

        case 'accountenable':
          // enable account only with admin rights
          if ($thisuser->hasAccess('disableaccount')) {
            // enable account only, if he has already activated his account
            $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET disabled = FALSE WHERE activation = '' AND id = :user_id");
            $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
            $stmt->execute();
          }
          // preselect displayed content
          $display = 'detail';
          break;

        case 'upateusrlang':
          $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET lang_id = :lang WHERE id = :user_id");
          $stmt->bindParam('lang',$_GET['lang']);
          $stmt->bindParam('user_id',$_GET['user']);
          $stmt->execute();
          if ($user_lang) {
            Log::writeLangMedium('change user account language: user-id='.$_GET['user'].', lang-id='.$_GET['lang'].' done by '.$thisuser->id().' {data_user_out}', $user_lang);
          }
          Log::writeMedium('change user account language: user-id='.$_GET['user'].', lang-id='.$_GET['lang'].' done by '.$thisuser->id().' {data_user_out}');
          // preselect displayed content
          $display = 'detail';
          break;

        default:
          $display = $_GET['d_fl'];
          break;
      }
    }

    // list / details
    if($display == 'list') {
      if (isset($_GET['phrase']) && strlen($_GET['phrase']) > 2) {
        echo_strip('
          <fieldset>
            <legend>Results</legend>
            <ul>');

        switch ($_GET['option']) {
          case 'fullname':
            $sql_search = "u.fullname";
            break;
          case 'email':
            $sql_search = "u.email";
            break;
          case 'website':
            $sql_search = "u.homepage";
            break;
          case 'language':
            if ($thisuser->hasAccess('more_lang')) {
              $sql_search = "l.name";
              break;
            }
          case 'accountname':
          default:
            $sql_search = "u.name";
            break;
        }

        if ($user_lang === false) {
          $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.fullname, l.name AS language FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_LANGUAGES." l ON u.lang_id=l.id WHERE ". $sql_search ." LIKE :value ORDER BY u.name ASC LIMIT 25");
        }
        else {
          $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.fullname, l.name AS language FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_LANGUAGES." l ON u.lang_id=l.id WHERE ". $sql_search ." LIKE :value AND l.id=:user_lang ORDER BY u.name ASC LIMIT 25");
          $stmt->bindValue('user_lang',$user_lang,PDO::PARAM_INT);
        }
        $stmt->bindValue('value','%'.$_GET['phrase'].'%',PDO::PARAM_INT);
        $stmt->execute();
        $users = $stmt->fetchAll(PDO::FETCH_ASSOC);
        foreach ($users as $user) {
          echo_strip('
            <li>
              <a href="'."javascript:getUserDetails('".$user['id']."')".'">'.$user['name'].'</a>
              ('.$user['fullname'].'; '.$user['language'].', )
            </li>');
        }
        echo '</ul>';

        if (count($users) == 25) {
          echo '<p>... more than 25 users</p>';
        }

        echo '</fieldset><br />';
      }
      else {
        echo '<p>more than 2 characters required</p>';
      }
    }

    elseif ($display == 'detail') {
      $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.modified, u.logins, u.created, u.fullname, u.email, l.name AS language, u.disabled FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_LANGUAGES." l ON l.id=u.lang_id WHERE u.id = :user_id LIMIT 1");
      $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
      $stmt->execute();
      $user = $stmt->fetchOnce();

      echo_strip('
        <fieldset>
          <legend>Details for \''.$user['name'].'\'</legend>
          <p><strong>Name:</strong> '.$user['name'].' ('.$user['fullname'].') ['.$user['id'].']</p>
          <p><strong>Lang:</strong> '.$user['language'].'</p>');
      if ($thisuser->hasAccess('user_details')) {
        echo_strip('
          <p><strong>E-Mail:</strong> '.$user['email'].'</p>
          <p><strong>Latest Login:</strong> '.$user['modified'].'; '.$user['logins'].' logins</p>
          <p><strong>Registered:</strong> '.$user['created'].'</p>
          <p>Account is '.($user['disabled']==false?'enabled':'disabled').'
            &nbsp;(
            <span class="frmeditbutton" onclick="'."setAccount(".$user['id'].", '".($user['disabled']==false?'disable':'enable')."')".'">&nbsp;'.($user['disabled']==false?'disable':'enable').'</span> 
            it)
          </p>');
      }
      echo_strip('
          <fieldset>
            <legend>Usergroup Memberships</legend>
            <ul>');

      $stmt=&DBConnection::getInstance()->prepare("SELECT g.name, m.group_id FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON g.id=m.group_id WHERE m.user_id = :user_id ORDER BY g.name ASC");
      $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
      $stmt->execute();
      while ($membership = $stmt->fetch(PDO::FETCH_ASSOC)) {

        echo '<li>'.$membership['name'].' ';
        if ($thisuser->hasAccess('delmembership')) {
          echo_strip('
            &nbsp;
            <span class="frmeditbutton" onclick="'."delMembership(".$_GET['user'].", '".$membership['group_id']."')".'">
              <img src="images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
              &nbsp;Delete
            </span>');
        }
        echo '</li>';
      } // end while
      echo '</ul>';

      if ($thisuser->hasAccess('addmembership')) {
        echo '<select id="cbmmemb" name="cbmmemb">';
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level FROM ".ROSCMST_GROUPS." WHERE id NOT IN(SELECT group_id FROM ".ROSCMST_MEMBERSHIPS." WHERE user_id=:user_id) ORDER BY name ASC");
        $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
        $stmt->execute();
        while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {

          // only super admin can give super admin rights
          if ($thisuser->hasAccess('addlvl'.$group['security_level'].'group')) {
            echo '<option value="'.$group['id'].'">'.$group['name'].'</option>';
          }
        }
        echo_strip('</select>
          <button name="addmemb" id="addmemb" onclick="'."addMembership(".$_GET['user'].", document.getElementById('cbmmemb').value)".'">Add Membership</button>
          <br />
          <br />
          <select id="cbmusrlang" name="cbmusrlang">');
        $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
        $stmt->execute();
        while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {
          echo '<option value="'.$lang['id'].'">'.$lang['name'].'</option>';
        }
        echo_strip('</select>
          <input type="button" name="addusrlang" id="addusrlang" value="Update User language" onclick="'."updateUserLang(".$_GET['user'].", document.getElementById('cbmusrlang').value)".'" /><br />');
      }
      elseif ($thisuser->hasAccess('addtransl')) {
        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROm ".ROSCMST_GROUPS." WHERE name_short='translator'");
        $stmt->execute();

        echo '<input type="button" name="addmemb" id="addmemb" value="Make this User a Translator" onclick="'."addMembership(".$_GET['user'].", '".$stmt->fetchColumn()."')".'" />';

      }
      echo '</fieldset><br />';
    }
  }


} // end of Export_User
?>
