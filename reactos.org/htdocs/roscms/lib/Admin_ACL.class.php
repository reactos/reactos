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
 */
class Admin_ACL extends Admin
{



  /**
   *
   *
   * @access protected
   */
  protected function showNew( )
  {
    echo_strip('
      <h2>Create new Access Control List (ACL)</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Access Control List Options</legend>
          <label for="acl_name">Name</label>
          <input id="acl_name" name="acl_name" maxlength="100" value="" />
          <br />

          <label for="acl_short">Short Name (Identifier)</label>
          <input id="acl_short" name="acl_short" maxlength="50" value="" />
          <br />

          <label for="acl_desc">Description</label>
          <input id="acl_desc" name="acl_desc" maxlength="255" value="" />
        </fieldset>
        <br />
        <fieldset>
          <legend>Groups Access Rights</legend>
          <table>
            <tr>
              <th title="Security Level">SecLvl</th>
              <th>Group Name</th>
              <th title="read">R</th>
              <th title="write">W</th>
              <th title="add">A</th>
              <th title="delete">D</th>
              <th title="publish">P</th>
              <th title="translate">T</th>
            </tr>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level, description FROM ".ROSCMST_GROUPS." ORDER BY security_level ASC, name ASC");
    $stmt->execute();
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <tr title="'.htmlspecialchars($group['description']).'">
          <td>'.$group['security_level'].'</td>
          <td>'.htmlspecialchars($group['name']).'</td>
          <td title="read entries"><input type="checkbox" name="read'.$group['id'].'" id="read'.$group['id'].'" checked="checked" value="1" /></td>
          <td title="edit entries"><input type="checkbox" name="write'.$group['id'].'" id="write'.$group['id'].'" value="1" /></td>
          <td title="add new entries"><input type="checkbox" name="add'.$group['id'].'" id="add'.$group['id'].'" value="1" /></td>
          <td title="delete entries"><input type="checkbox" name="del'.$group['id'].'" id="del'.$group['id'].'" value="1" /></td>
          <td title="make entries stable"><input type="checkbox" name="pub'.$group['id'].'" id="pub'.$group['id'].'" value="1" /></td>
          <td title="translate entries"><input type="checkbox" name="trans'.$group['id'].'" id="trans'.$group['id'].'" value="1" /></td>
        </tr>');
    }

    echo_strip('
          </table>
        </fieldset>
        <button onclick="'."submitNew('acl')".'">Create new ACL</button>
      </form>
    ');
  } // end of member function showNew



  /**
   *
   *
   * @access protected
   */
  protected function submitNew( )
  {
    $success = true;
  
    // try to insert new access list
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ACCESS." (name, name_short, description) VALUES (:name, :short, :description)");
    $stmt->bindParam('name',$_POST['acl_name'],PDO::PARAM_STR);
    $stmt->bindParam('short',$_POST['acl_short'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['acl_desc'],PDO::PARAM_STR);
    if ($stmt->execute()) {
    
      // check for new access list id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS." WHERE name=:name");
      $stmt->bindParam('name',$_POST['acl_name'],PDO::PARAM_STR);
      $stmt->execute();
      $access_id = $stmt->fetchColumn();
      if ($access_id !== false) {

        // prepare for usage in loop
        $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ENTRY_AREA." (acl_id, group_id, can_read, can_write, can_add, can_delete, can_publish, can_translate) VALUES (:acl_id, :group_id, :read, :write, :add, :delete, :publish, :translate)");
        $stmt_ins->bindParam('acl_id',$access_id,PDO::PARAM_INT);

        // insert access rights for each group
        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_GROUPS);
        $stmt->execute();
        while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
          $stmt_ins->bindParam('group_id',$group['id'],PDO::PARAM_INT);
          $stmt_ins->bindValue('read',$_POST['read'.$group['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('write',$_POST['write'.$group['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('add',$_POST['add'.$group['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('delete',$_POST['del'.$group['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('publish',$_POST['pub'.$group['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('translate',$_POST['trans'.$group['id']]=='true',PDO::PARAM_BOOL);
          $success = $success && $stmt_ins->execute();
        }
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
   *
   *
   * @access protected
   */
  protected function showSearch( )
  {
    echo_strip('
      <h2>Select ACL to '.($_GET['for']=='edit' ? 'edit' : 'delete').'</h2>
      <form onsubmit="return false;">
        <select name="acl" id="acl">
          <option value="0">&nbsp;</option>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, description FROM ".ROSCMST_ACCESS." ORDER BY name ASC");
    $stmt->execute();
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$access['id'].'" title="'.$access['description'].'">'.$access['name'].'</option>';
    }

    echo_strip('
        </select>
        <button onclick="'."submitSearch('acl','".($_GET['for'] == 'edit' ? 'edit' : 'delete')."')".'">go on</button>
      </form>');
  }



  /**
   *
   *
   * @access protected
   */
  protected function submitSearch( )
  {
    // show edit / delete form, if entry was selected
    if ($_POST['acl'] > 0) {
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
  }



  /**
   *
   *
   * @access protected
   */
  protected function showEdit( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short, description, id FROM ".ROSCMST_ACCESS." WHERE id=:acl_id");
    $stmt->bindParam('acl_id',$_POST['acl'],PDO::PARAM_INT);
    $stmt->execute();
    $access = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <h2>Edit Access Control List (ACL)</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Access Control List Options</legend>
          <input type="hidden" name="acl_id" id="acl_id" value="'.$access['id'].'" />
          
          <label for="acl_name">Name</label>
          <input id="acl_name" name="acl_name" maxlength="100" value="'.$access['name'].'" />
          <br />

          <label for="acl_short">Short Name (Identifier)</label>
          <input id="acl_short" name="acl_short" maxlength="50" value="'.$access['name_short'].'" />
          <br />

          <label for="acl_desc">Description</label>
          <input id="acl_desc" name="acl_desc" maxlength="255" value="'.$access['description'].'" />
        </fieldset>
        <br />
        <fieldset>
          <legend>Groups Access Rights</legend>
          <table>
            <tr>
              <th title="Security Level">SecLvl</th>
              <th>Group Name</th>
              <th title="read">R</th>
              <th title="write">W</th>
              <th title="add">A</th>
              <th title="delete">D</th>
              <th title="publish">P</th>
              <th title="translate">T</th>
            </tr>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT g.id, g.name, g.security_level, g.description, a.can_read, a.can_write, a.can_add, a.can_delete, a.can_publish, a.can_translate FROM ".ROSCMST_ENTRY_AREA." a JOIN ".ROSCMST_GROUPS." g ON g.id=a.group_id WHERE a.acl_id=:acl_id ORDER BY g.security_level ASC, g.name ASC");
    $stmt->bindParam('acl_id',$access['id'],PDO::PARAM_INT);
    $stmt->execute();
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <tr title="'.htmlspecialchars($group['description']).'">
          <td>'.$group['security_level'].'</td>
          <td>'.htmlspecialchars($group['name']).'</td>
          <td title="read entries"><input type="checkbox" name="read'.$group['id'].'" id="read'.$group['id'].'" '.($group['can_read'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="edit entries"><input type="checkbox" name="write'.$group['id'].'" id="write'.$group['id'].'" '.($group['can_write'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="add new entries"><input type="checkbox" name="add'.$group['id'].'" id="add'.$group['id'].'" '.($group['can_add'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="delete entries"><input type="checkbox" name="del'.$group['id'].'" id="del'.$group['id'].'" '.($group['can_delete'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="make entries stable"><input type="checkbox" name="pub'.$group['id'].'" id="pub'.$group['id'].'" '.($group['can_publish'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="translate entries"><input type="checkbox" name="trans'.$group['id'].'" id="trans'.$group['id'].'" '.($group['can_translate'] == true ? 'checked="checked"' : '').' value="1" /></td>
        </tr>');
    }

    echo_strip('
          </table>
        </fieldset>
        <button onclick="'."submitEdit('acl')".'">edit ACL</button>
      </form>
    ');
  }



  /**
   *
   *
   * @access protected
   */
  protected function submitEdit( )
  {
    $success = true;

    // try to insert new access list
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ACCESS." SET name=:name, name_short=:short, description=:description WHERE id=:acl_id");
    $stmt->bindParam('name',$_POST['acl_name'],PDO::PARAM_STR);
    $stmt->bindParam('short',$_POST['acl_short'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['acl_desc'],PDO::PARAM_STR);
    $stmt->bindParam('acl_id',$_POST['acl_id'],PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    // prepare for usage in loop
    $stmt_ins=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ENTRY_AREA." SET can_read=:read, can_write=:write, can_add=:add, can_delete=:delete, can_publish=:publish, can_translate=:translate WHERE acl_id=:acl_id AND group_id=:group_id");
    $stmt_ins->bindParam('acl_id',$_POST['acl_id'],PDO::PARAM_INT);

    // insert access rights for each group
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_GROUPS);
    $success = $success && $stmt->execute();
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $stmt_ins->bindParam('group_id',$group['id'],PDO::PARAM_INT);
      $stmt_ins->bindValue('read',$_POST['read'.$group['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('write',$_POST['write'.$group['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('add',$_POST['add'.$group['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('delete',$_POST['del'.$group['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('publish',$_POST['pub'.$group['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('translate',$_POST['trans'.$group['id']]=='true',PDO::PARAM_BOOL);
      $success = $success && $stmt_ins->execute();
    }

    // give the user a success or failure message
    if ($success) {
      echo 'ACL was edited successfully';
    }
    else {
      echo 'Error, while editing ACL';
    }
  }



  /**
   *
   *
   * @access protected
   */
  protected function showDelete( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(id) FROM ".ROSCMST_ENTRIES." WHERE acl_id=:acl_id");
    $stmt->bindParam('acl_id',$_POST['acl'],PDO::PARAM_INT);
    $stmt->execute();
    $data_count = $stmt->fetchColumn();

    // check if
    if ($data_count > 0) {
      echo '<div>Can\'t delete entry: It\'s used in '.$data_count.' entries. Remove usage first, and try again later.</div>';
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short, description, id FROM ".ROSCMST_ACCESS." WHERE id=:acl_id");
      $stmt->bindParam('acl_id',$_POST['acl'],PDO::PARAM_INT);
      $stmt->execute();
      $access = $stmt->fetchOnce(PDO::FETCH_ASSOC);

      echo_strip('
        <form onsubmit="return false;">
          <div>
            <input type="hidden" name="acl_id" id="acl_id" value="'.$access['id'].'" />

            Do you really want to delete the ACL &quot;<span title="'.$access['description'].'">'.$access['name'].'</span>&quot; ?
            <button style="color: red;" onclick="'."submitDelete('acl')".'" name="uaq" value="yes">Yes, Delete it.</button>
            <button style="color: green;" name="uaq" value="no">No</button>
          </div>
        </form>');
    }
  }



  /**
   *
   *
   * @access protected
   */
  protected function submitDelete( )
  {
    $success = true;

    // check if it is used anywhere
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(id) FROM ".ROSCMST_ENTRIES." WHERE acl_id=:acl_id");
    $stmt->bindParam('acl_id',$_POST['acl_id'],PDO::PARAM_INT);
    $stmt->execute();
    $data_count = $stmt->fetchColumn();
    if ($data_count > 0) {
      echo '<div>Can\'t delete entry: It\'s used in '.$data_count.' entries. Remove usage first, and try again later.</div>';
    }
    else {

      // delete acl
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ACCESS." WHERE id=:acl_id");
      $stmt->bindParam('acl_id',$_POST['acl_id'],PDO::PARAM_INT);
      $success = $success && $stmt->execute();

      // delete rights list
      if ($success) {
        $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ENTRY_AREA." WHERE acl_id=:acl_id");
        $stmt->bindParam('acl_id',$_POST['acl_id'],PDO::PARAM_INT);
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
  }

} // end of Admin_ACL
?>
