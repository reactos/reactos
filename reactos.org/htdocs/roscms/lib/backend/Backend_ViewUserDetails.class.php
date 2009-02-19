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
 * class Backend_ViewUserDetails
 *
 * @package Branch_Maintain
 */
class Backend_ViewUserDetails extends Backend
{


  /**
   * checks if the user needs to be updated
   * checks what has to be displayed
   *
   * @access public
   */
  public function __construct( )
  {
    parent::__construct();
    $this->show();
  } // end of constructor



  /**
   * displays some informations about the user
   * - interface to add/delete group memberships
   * - interface to disable/enable the user
   * - interface to change users language
   *
   * @access private
   */
  private function show( )
  {
    $thisuser = &ThisUser::getInstance();

    // get user data
    $stmt=&DBConnection::getInstance()->prepare("SELECT u.id, u.name, u.modified, u.logins, u.created, u.fullname, u.email, u.lang_id, l.name AS language, u.disabled FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_LANGUAGES." l ON l.id=u.lang_id WHERE u.id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
    $stmt->execute();
    $user = $stmt->fetchOnce();

    // display user name (real name) and language
    echo_strip('
      <h2 style="margin: 0px;">'.$user['name'].' ['.$user['id'].']</h2>
      <div style="margin-bottom: 15px;">'.$user['fullname'].'</div>
      <fieldset>
        <p><strong>Lang:</strong> '.$user['language'].'</p>');

    // display some details, not for everyone
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

    // get group memberships
    $stmt=&DBConnection::getInstance()->prepare("SELECT g.name, m.group_id FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON g.id=m.group_id WHERE m.user_id = :user_id ORDER BY g.name ASC");
    $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
    $stmt->execute();

    // show group memberships
    while ($membership = $stmt->fetch(PDO::FETCH_ASSOC)) {

      echo '<li>'.$membership['name'].' ';

      // display deletion interface 
      if ($thisuser->hasAccess('delmembership')) {
        echo_strip('
          &nbsp;
          <span class="frmeditbutton" onclick="'."delMembership(".$_GET['user'].", '".$membership['group_id']."')".'">
            <img src="'.RosCMS::getInstance()->pathRosCMS().'images/remove.gif" alt="" style="width:11px; height:11px; border:0px;" />
            &nbsp;Delete
          </span>');
      }
      echo '</li>';
    } // end while
    echo '</ul>';

    // display group adding interface
    if ($thisuser->hasAccess('addmembership')) {
      echo '<select id="cbmmemb" name="cbmmemb">';
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level FROM ".ROSCMST_GROUPS." WHERE id NOT IN(SELECT group_id FROM ".ROSCMST_MEMBERSHIPS." WHERE user_id=:user_id) ORDER BY name ASC");
      $stmt->bindParam('user_id',$_GET['user'],PDO::PARAM_INT);
      $stmt->execute();

      // show available groups
      while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {

        // permit to add groups allowed via groups security level
        if ($thisuser->hasAccess('addlvl'.$group['security_level'].'group')) {
          echo '<option value="'.$group['id'].'">'.$group['name'].'</option>';
        }
      }

      // add membership button
      echo_strip('</select>
        <button name="addmemb" id="addmemb" onclick="'."addMembership(".$_GET['user'].", document.getElementById('cbmmemb').value)".'">Add Membership</button>
        <br />
        <br />
        <select id="cbmusrlang" name="cbmusrlang">');

      // show available languages, the user can changed to
      $stmt=&DBConnection::getInstance()->prepare("SELECT id, name FROM ".ROSCMST_LANGUAGES." ORDER BY name ASC");
      $stmt->execute();
      while ($lang = $stmt->fetch(PDO::FETCH_ASSOC)) {
        echo '<option value="'.$lang['id'].'"'.($lang['id'] == $user['lang_id'] ? ' selected="selected"' : '').'>'.$lang['name'].'</option>';
      }

      // change language button
      echo_strip('</select>
        <input type="button" name="addusrlang" id="addusrlang" value="Update User language" onclick="'."updateUserLang(".$_GET['user'].", document.getElementById('cbmusrlang').value)".'" /><br />');
    }

    // show interface to add translators
    elseif ($thisuser->hasAccess('addtransl')) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROm ".ROSCMST_GROUPS." WHERE name_short='translator'");
      $stmt->execute();

      // make translator button
      echo '<input type="button" name="addmemb" id="addmemb" value="Make this User a Translator" onclick="'."addMembership(".$_GET['user'].", '".$stmt->fetchColumn()."')".'" />';

    }
    echo '</fieldset><br />';
  } // end of member function show



} // end of Backend_ViewUserDetails
?>
