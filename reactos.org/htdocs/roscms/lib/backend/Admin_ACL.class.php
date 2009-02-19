<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2009  Danny Götte <dangerground@web.de>

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
 * class Admin_ACL
 * 
 * @package Branch_Admin
 */
class Admin_ACL extends Admin
{



  /**
   * Interface to add new ACL
   *
   * @access protected
   */
  protected function showNew( )
  {
    // get list of access rights
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, description FROM ".ROSCMST_RIGHTS." ORDER BY name ASC");
    $stmt->execute();
    $rights=$stmt->fetchAll(PDO::FETCH_ASSOC);

    echo_strip('
      <h2>Create new Access Control List (ACL)</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Access Control List Options</legend>
          <label for="access_name">Name</label>
          <input id="access_name" name="access_name" maxlength="100" value="" />
          <br />

          <label for="access_desc">Description</label>
          <input id="access_desc" name="access_desc" maxlength="255" value="" />
        </fieldset>
        <br />
        <fieldset>
          <legend>Groups Access Rights</legend>
          <table class="roscmsTable">
            <thead>
              <tr>
                <th title="Security Level">SecLvl</th>
                <th>Group Name</th>');

    // access rights in head
    foreach ($rights as $right) {
      echo '<th style="vertical-align:bottom;" title="'.$right['name'].': '.$right['description'].'"><img src="'.RosCMS::getInstance()->pathInstance().'?page=presentation&amp;type=vtext&amp;text='.$right['name'].'&bgcolor=5984C3&textc=ffffff" alt="'.$right['name'].'" /></th>';
    }
    echo_strip('
        </tr>
      </thead>
      <tbody>');

    // list of groups
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level, description FROM ".ROSCMST_GROUPS." ORDER BY security_level ASC, name ASC");
    $stmt->execute();
    $x=0;
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <tr title="'.htmlspecialchars($group['description']).'" class="'.(++$x%2 ? 'even' : 'odd').'">
          <td>'.$group['security_level'].'</td>
          <td>'.htmlspecialchars($group['name']).'</td>');

      // access rights per group
      foreach ($rights as $right) {
        echo '<td title="'.$group['name'].'--'.$right['name'].': '.$right['description'].'"><input type="checkbox" value="1" name="valid'.$group['id'].'_'.$right['id'].'" /></td>';
      }
      echo '</tr>';
    }

    echo_strip('
            </tbody>
          </table>
        </fieldset>
        <button onclick="'."submitAccessAdd()".'">Create new ACL</button>
      </form>
    ');
  } // end of member function showNew



  /**
   * Backend to create a new ACL
   *
   * @access protected
   */
  protected function submitNew( )
  {
    $success = true;
  
    // try to insert new access list
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ACCESS." (name, description) VALUES (:name, :description)");
    $stmt->bindParam('name',$_POST['access_name'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['access_desc'],PDO::PARAM_STR);
    if ($stmt->execute()) {
    
      // check for new access list id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS." WHERE name=:name");
      $stmt->bindParam('name',$_POST['access_name'],PDO::PARAM_STR);
      $stmt->execute();
      $access_id = $stmt->fetchColumn();
      if ($access_id !== false) {

        // insert all
        $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ACL." (access_id, group_id, right_id) VALUES (:access_id,:group_id,:right_id)");
        $stmt->bindParam('access_id',$access_id,PDO::PARAM_INT);
        foreach ($_POST as $item=>$val) {
          if (strpos($item,'valid')===0) {
            $item = substr($item, 5);
            $id = explode('_',$item);
            if($id[0] > 0 && $id[1] > 0 && $val=='true') {
              $stmt->bindParam('right_id',$id[1],PDO::PARAM_INT);
              $stmt->bindParam('group_id',$id[0],PDO::PARAM_INT);
              $success = $success && $stmt->execute();
            }
          }
        } // end foreach
      } // end got list id
      else {
        $success = false;
      }
    } // end list inserted
    else {
      $success = false;
    }

    // give the user a success or failure message
    if ($success) {
      echo 'New ACL was created successfully';
    }
    else {
      echo 'Error, while creating new ACL';
    }
  } // end of member function submitNew



  /**
   * Interface to search(within a list) ACLs
   *
   * @access protected
   */
  protected function showSearch( )
  {
    // list all ALCs
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, standard, name, description FROM ".ROSCMST_ACCESS." ORDER BY name ASC");
    $stmt->execute();
    $x=0;
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$x;
      echo_strip ('
        <tr id="tra'.($x).'" class="'.($x%2 ? 'odd' : 'even').'" onclick="'."editAccess(".$access['id'].")".'" onmouseover="'."hlRow(this.id,1)".'" onmouseout="'."hlRow(this.id,2)".'">
          <td>'.$access['standard'].'</td>
          <td>'.$access['name'].'</td>
          <td>'.htmlentities($access['description']).'</td>
        </tr>');
    }

  } // end of member function showSearch



  /**
   * Backend for search ACLs
   *
   * @access protected
   */
  protected function submitSearch( )
  {
    // show edit / delete form, if entry was selected
    if ($_POST['access'] > 0) {
      if ($_GET['for'] == 'edit') {
        self::showEdit();
      }
      elseif ($_GET['for'] == 'delete') {
        self::showDelete();
      }
    }

    // show search again
    else {
      self::showSearch();
    }
  } // end of member function submitSearch



  /**
   * Interface to Edit ACLs
   *
   * @access protected
   */
  protected function showEdit( )
  {
    // list of access rights
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, description FROM ".ROSCMST_RIGHTS." ORDER BY name ASC");
    $stmt->execute();
    $rights=$stmt->fetchAll(PDO::FETCH_ASSOC);

    // information about current Access
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, description, id FROM ".ROSCMST_ACCESS." WHERE id=:access_id");
    $stmt->bindParam('access_id',$_REQUEST['access'],PDO::PARAM_INT);
    $stmt->execute();
    $access = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <h2>Edit Access Control List (ACL)</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Access Control List Options</legend>
          <input type="hidden" name="access_id" id="access_id" value="'.$access['id'].'" />
          
          <label for="access_name">Name</label>
          <input id="access_name" name="access_name" maxlength="100" value="'.$access['name'].'" />
          <br />

          <label for="access_desc">Description</label>
          <input id="access_desc" name="access_desc" maxlength="255" value="'.$access['description'].'" />
        </fieldset>
        <br />
        <fieldset>
          <legend>Groups Access Rights</legend>
          <table class="roscmsTable">
            <thead>
              <tr>
                <th title="Security Level">SecLvl</th>
                <th>Group Name</th>');

    // list rights in table head
    foreach ($rights as $right) {
      echo '<th style="vertical-align:bottom;" title="'.$right['name'].': '.$right['description'].'"><img src="'.RosCMS::getInstance()->pathInstance().'?page=presentation&amp;type=vtext&amp;text='.$right['name'].'&bgcolor=5984C3&textc=ffffff" alt="'.$right['name'].'" /></th>';
    }
    echo '</tr></thead><tbody>';

    // for usage in loop
    $stmt_is=&DBConnection::getInstance()->prepare("SELECT TRUE FROM ".ROSCMST_ACL." WHERE group_id=:group_id AND right_id=:right_id AND access_id=:access_id LIMIT 1");
    $stmt_is->bindParam('access_id',$access['id'],PDO::PARAM_INT);

    // go through groups
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level, description FROM ".ROSCMST_GROUPS." ORDER BY security_level ASC, name ASC");
    $stmt->execute();
    $x=0;
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      ++$x;
      $stmt_is->bindParam('group_id',$group['id'],PDO::PARAM_INT);
      echo_strip('
        <tr id="tr'.$x.'" title="'.htmlspecialchars($group['description']).'" class="'.($x%2 ? 'even' : 'odd').'" onmouseover="'."hlRow(this.id,1)".'" onmouseout="'."hlRow(this.id,2)".'">
          <td>'.$group['security_level'].'</td>
          <td>'.htmlspecialchars($group['name']).'</td>');

      // list rights per group
      foreach ($rights as $right) {
        $stmt_is->bindParam('right_id',$right['id'],PDO::PARAM_INT);
        $stmt_is->execute();
        $is = $stmt_is->fetchColumn();

        echo '<td title="'.$group['name'].'--'.$right['name'].': '.$right['description'].'"><input type="checkbox" value="1" name="valid'.$group['id'].'_'.$right['id'].'" '.($is ? 'checked="checked"' : '').' /></td>';
      }
      echo '</tr>';
    }

    echo_strip('
            </tbody>
          </table>
        </fieldset>
        <button onclick="'."submitAccessEdit()".'">edit ACL</button>
      </form>
    ');
  } // end of member function showEdit



  /**
   * Backend for edit ACLs
   *
   * @access protected
   */
  protected function submitEdit( )
  {
    $success = true;

    // try to insert new access list
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ACCESS." SET name=:name, description=:description WHERE id=:access_id");
    $stmt->bindParam('name',$_POST['access_name'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['access_desc'],PDO::PARAM_STR);
    $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    // delete old ACL connections
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ACL." WHERE access_id=:access_id");
    $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    if ($success) {

      // insert new ACL connections
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ACL." (access_id, group_id, right_id) VALUES (:access_id,:group_id,:right_id)");
      $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
      foreach ($_POST as $item=>$val) {
        if (strpos($item,'valid')===0) {
          $item = substr($item, 5);
          $id = explode('_',$item);
          if($id[0] > 0 && $id[1] > 0 && $val=='true') {
            $stmt->bindParam('right_id',$id[1],PDO::PARAM_INT);
            $stmt->bindParam('group_id',$id[0],PDO::PARAM_INT);
            $success = $success && $stmt->execute();
          }
        }
      }
    }

    // give the user a success or failure message
    if ($success) {
      echo 'ACL was edited successfully';
    }
    else {
      echo 'Error, while editing ACL';
    }
  } // end of member function submitEdit



  /**
   * Interface to delete ACLs
   *
   * @access protected
   */
  protected function showDelete( )
  {
    // check how many entries are depend on this ACL
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(id) FROM ".ROSCMST_ENTRIES." WHERE access_id=:access_id");
    $stmt->bindParam('access_id',$_POST['access'],PDO::PARAM_INT);
    $stmt->execute();
    $data_count = $stmt->fetchColumn();

    // check if
    if ($data_count > 0) {
      echo '<div>Can\'t delete entry: It\'s used in '.$data_count.' entries. Remove usage first, and try again later.</div>';
    }
    else {

      // ACL information
      $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short, description, id FROM ".ROSCMST_ACCESS." WHERE id=:access_id");
      $stmt->bindParam('access_id',$_POST['access'],PDO::PARAM_INT);
      $stmt->execute();
      $access = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      echo_strip('
        <form onsubmit="return false;">
          <div>
            <input type="hidden" name="access_id" id="access_id" value="'.$access['id'].'" />

            Do you really want to delete the access &quot;<span title="'.$access['description'].'">'.$access['name'].'</span>&quot; ?
            <button style="color: red;" onclick="'."submitDelete('acl')".'" name="uaq" value="yes">Yes, Delete it.</button>
            <button style="color: green;" name="uaq" value="no">No</button>
          </div>
        </form>');
    }
  } // end of member function showDelete



  /**
   * Backend for delete ACLs
   *
   * @access protected
   */
  protected function submitDelete( )
  {
    $success = true;

    // check if it is used anywhere
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(id) FROM ".ROSCMST_ENTRIES." WHERE access_id=:access_id");
    $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
    $stmt->execute();
    $data_count = $stmt->fetchColumn();
    if ($data_count > 0) {
      echo '<div>Can\'t delete entry: It\'s used in '.$data_count.' entries. Remove usage first, and try again later.</div>';
    }
    else {

      // delete acl
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ACCESS." WHERE id=:access_id");
      $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
      $success = $success && $stmt->execute();

      // delete rights list
      if ($success) {
        $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ACL." WHERE access_id=:access_id");
        $stmt->bindParam('access_id',$_POST['access_id'],PDO::PARAM_INT);
        $success = $success && $stmt->execute();
      }

      // status message
      if ($success) {
        echo 'ACL was deleted successfully';
      }
      else {
        echo 'Error, while deleting ACL';
      }
    }
  } // end of member function submitDelete



} // end of Admin_ACL
?>
