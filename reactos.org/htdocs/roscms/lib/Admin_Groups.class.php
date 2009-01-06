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
 * class Admin_Groups
 * 
 */
class Admin_Groups extends Admin
{



  /**
   *
   *
   * @access protected
   */
  protected function showNew( )
  {
    echo_strip('
      <h2>Create new Group</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Group Data</legend>
          <label for="group_sec">Security Level</label>
          <select id="group_sec" name="group_sec">
            <option value="0">0 (no access to CMS, only that myRosCMS stuff)</option>
            <option value="1">1 (simple users (e.g. translator))</option>
            <option value="2">2 (advanced rights, e.g. developers)</option>
            <option value="3">3 (some admin functions)</option>
          </select>
          <br />

          <label for="group_name">Name</label>
          <input id="group_name" name="group_name" maxlength="100" />
          <br />

          <label for="group_short">Short Name (Identifier)</label>
          <input id="group_short" name="group_short" maxlength="50" />
          <br />

          <label for="group_desc">Description</label>
          <input id="group_desc" name="group_desc" maxlength="255" />
        </fieldset>
        <br />
        <fieldset>
          <legend>configure group access rights</legend>
          <table>
            <tr>
              <th>ACL Name</th>
              <th title="read">R</th>
              <th title="write">W</th>
              <th title="add">A</th>
              <th title="delete">D</th>
              <th title="publish">P</th>
              <th title="translate">T</th>
            </tr>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, description FROM ".ROSCMST_ACCESS." ORDER BY name ASC");
    $stmt->execute();
    while ($acl = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <tr title="'.htmlspecialchars($acl['description']).'">
          <td>'.htmlspecialchars($acl['name']).'</td>
          <td title="read entries"><input type="checkbox" name="read'.$acl['id'].'" id="read'.$acl['id'].'" value="1" /></td>
          <td title="edit entries"><input type="checkbox" name="write'.$acl['id'].'" id="write'.$acl['id'].'" value="1" /></td>
          <td title="add new entries"><input type="checkbox" name="add'.$acl['id'].'" id="add'.$acl['id'].'" value="1" /></td>
          <td title="delete entries"><input type="checkbox" name="del'.$acl['id'].'" id="del'.$acl['id'].'" value="1" /></td>
          <td title="make entries stable"><input type="checkbox" name="pub'.$acl['id'].'" id="pub'.$acl['id'].'" value="1" /></td>
          <td title="translate entries"><input type="checkbox" name="trans'.$acl['id'].'" id="trans'.$acl['id'].'" value="1" /></td>
        </tr>');
    }

    echo_strip('
          </table>
        </fieldset>
        <button onclick="'."submitNew('group')".'">Create new Group</button>
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
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_GROUPS." (name, name_short, description, security_level, visible) VALUES (:name, :short, :description, :security_level, FALSE)");
    $stmt->bindParam('name',$_POST['group_name'],PDO::PARAM_STR);
    $stmt->bindParam('short',$_POST['group_short'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['group_desc'],PDO::PARAM_STR);
    $stmt->bindParam('security_level',$_POST['group_sec'],PDO::PARAM_INT);
    if ($stmt->execute()) {
    
      // check for new access list id
      $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_GROUPS." WHERE name=:name");
      $stmt->bindParam('name',$_POST['group_name'],PDO::PARAM_STR);
      $stmt->execute();
      $group_id = $stmt->fetchColumn();
      if ($group_id !== false) {

        // prepare for usage in loop
        $stmt_ins=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_ACL." (acl_id, group_id, can_read, can_write, can_add, can_delete, can_publish, can_translate) VALUES (:acl_id, :group_id, :read, :write, :add, :delete, :publish, :translate)");
        $stmt_ins->bindParam('group_id',$group_id,PDO::PARAM_INT);

        // insert access rights for each group
        $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS);
        $stmt->execute();
        while ($acl = $stmt->fetch(PDO::FETCH_ASSOC)) {
          $stmt_ins->bindParam('acl_id',$acl['id'],PDO::PARAM_INT);
          $stmt_ins->bindValue('read',$_POST['read'.$acl['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('write',$_POST['write'.$acl['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('add',$_POST['add'.$acl['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('delete',$_POST['del'.$acl['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('publish',$_POST['pub'.$acl['id']]=='true',PDO::PARAM_BOOL);
          $stmt_ins->bindValue('translate',$_POST['trans'.$acl['id']]=='true',PDO::PARAM_BOOL);
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
      echo_strip('New Group was created successfully');
    }
    else {
      echo_strip('Error, while creating new Group');
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
      <h2>Select Group to '.($_GET['for']=='edit' ? 'edit' : 'delete').'</h2>
      <form onsubmit="return false;">
        <select name="group" id="group">
          <option value="0">&nbsp;</option>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, description FROM ".ROSCMST_GROUPS." ORDER BY name ASC");
    $stmt->execute();
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo '<option value="'.$group['id'].'" title="'.$group['description'].'">'.$group['name'].'</option>';
    }

    echo_strip('
        </select>
        <button onclick="'."submitSearch('group','".($_GET['for'] == 'edit' ? 'edit' : 'delete')."')".'">go on</button>
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
    if ($_POST['group'] > 0) {
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
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, name_short, description, id, security_level FROM ".ROSCMST_GROUPS." WHERE id=:acl_id");
    $stmt->bindParam('acl_id',$_POST['group'],PDO::PARAM_INT);
    $stmt->execute();
    $group = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <h2>Edit Group</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Group Data</legend>
          <input type="hidden" name="group_id" id="group_id" value="'.$group['id'].'" />

          <label for="group_sec">Security Level</label>
          <select id="group_sec" name="group_sec">
            <option value="0"'.($group['security_level'] == 0 ? ' selected="selected"' : '').'>0 (no access to CMS, only that myRosCMS stuff)</option>
            <option value="1"'.($group['security_level'] == 1 ? ' selected="selected"' : '').'>1 (simple users (e.g. translator))</option>
            <option value="2"'.($group['security_level'] == 2 ? ' selected="selected"' : '').'>2 (advanced rights, e.g. developers)</option>
            <option value="3"'.($group['security_level'] == 3 ? ' selected="selected"' : '').'>3 (some admin functions)</option>
          </select>
          <br />

          <label for="group_name">Name</label>
          <input id="group_name" name="group_name" maxlength="100" value="'.htmlspecialchars($group['name']).'" />
          <br />

          <label for="group_short">Short Name (Identifier)</label>
          <input id="group_short" name="group_short" maxlength="50" value="'.htmlspecialchars($group['name_short']).'" />
          <br />

          <label for="group_desc">Description</label>
          <input id="group_desc" name="group_desc" maxlength="255" value="'.htmlspecialchars($group['description']).'"  />
        </fieldset>
        <br />
        <fieldset>
          <legend>configure group access rights</legend>
          <table>
            <tr>
              <th>ACL Name</th>
              <th title="read">R</th>
              <th title="write">W</th>
              <th title="add">A</th>
              <th title="delete">D</th>
              <th title="publish">P</th>
              <th title="translate">T</th>
            </tr>');

    $stmt=&DBConnection::getInstance()->prepare("SELECT s.id, s.name, s.description, a.can_read, a.can_write, a.can_add, a.can_delete, a.can_publish, a.can_translate FROM ".ROSCMST_ACCESS." s JOIN ".ROSCMST_ACL." a ON a.acl_id=s.id WHERE a.group_id=:group_id ORDER BY name ASC");
    $stmt->bindParam('group_id',$group['id'],PDO::PARAM_INT);
    $stmt->execute();
    while ($acl = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo_strip('
        <tr title="'.htmlspecialchars($acl['description']).'">
          <td>'.htmlspecialchars($acl['name']).'</td>
          <td title="read entries"><input type="checkbox" name="read'.$acl['id'].'" id="read'.$acl['id'].'" '.($acl['can_read'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="edit entries"><input type="checkbox" name="write'.$acl['id'].'" id="write'.$acl['id'].'" '.($acl['can_write'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="add new entries"><input type="checkbox" name="add'.$acl['id'].'" id="add'.$acl['id'].'" '.($acl['can_add'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="delete entries"><input type="checkbox" name="del'.$acl['id'].'" id="del'.$acl['id'].'" '.($acl['can_delete'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="make entries stable"><input type="checkbox" name="pub'.$acl['id'].'" id="pub'.$acl['id'].'" '.($acl['can_publish'] == true ? 'checked="checked"' : '').' value="1" /></td>
          <td title="translate entries"><input type="checkbox" name="trans'.$acl['id'].'" id="trans'.$acl['id'].'" '.($acl['can_translate'] == true ? 'checked="checked"' : '').' value="1" /></td>
        </tr>');
    }

    echo_strip('
          </table>
        </fieldset>
        <button onclick="'."submitEdit('group')".'">Edit Group</button>
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
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_GROUPS." SET name=:name, name_short=:short, description=:description, security_level=:sec_level WHERE id=:group_id");
    $stmt->bindParam('name',$_POST['group_name'],PDO::PARAM_STR);
    $stmt->bindParam('short',$_POST['group_short'],PDO::PARAM_STR);
    $stmt->bindParam('description',$_POST['group_desc'],PDO::PARAM_STR);
    $stmt->bindParam('sec_level',$_POST['group_sec'],PDO::PARAM_STR);
    $stmt->bindParam('group_id',$_POST['group_id'],PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    // prepare for usage in loop
    $stmt_ins=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_ACL." SET can_read=:read, can_write=:write, can_add=:add, can_delete=:delete, can_publish=:publish, can_translate=:translate WHERE acl_id=:acl_id AND group_id=:group_id");
    $stmt_ins->bindParam('group_id',$_POST['group_id'],PDO::PARAM_INT);

    // insert access rights for each group
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS);
    $success = $success && $stmt->execute();
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $stmt_ins->bindParam('acl_id',$access['id'],PDO::PARAM_INT);
      $stmt_ins->bindValue('read',$_POST['read'.$access['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('write',$_POST['write'.$access['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('add',$_POST['add'.$access['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('delete',$_POST['del'.$access['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('publish',$_POST['pub'.$access['id']]=='true',PDO::PARAM_BOOL);
      $stmt_ins->bindValue('translate',$_POST['trans'.$access['id']]=='true',PDO::PARAM_BOOL);
      $success = $success && $stmt_ins->execute();
    }

    // give the user a success or failure message
    if ($success) {
      echo 'Group was edited successfully';
    }
    else {
      echo 'Error, while editing Group';
    }
  }



  /**
   *
   *
   * @access protected
   */
  protected function showDelete( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT name, description, id FROM ".ROSCMST_GROUPS." WHERE id=:group_id");
    $stmt->bindParam('group_id',$_POST['group'],PDO::PARAM_INT);
    $stmt->execute();
    $group = $stmt->fetchOnce(PDO::FETCH_ASSOC);

    echo_strip('
      <form onsubmit="return false;">
        <div>
          <input type="hidden" name="group_id" id="group_id" value="'.$group['id'].'" />

          Do you really want to delete the Group &quot;<span title="'.$group['description'].'">'.$group['name'].'</span>&quot; ?
          <button style="color: red;" onclick="'."submitDelete('group')".'" name="uaq" value="yes">Yes, Delete it.</button>
          <button style="color: green;" name="uaq" value="no">No</button>
        </div>
      </form>');
  }



  /**
   *
   *
   * @access protected
   */
  protected function submitDelete( )
  {
    $success = true;

    // delete acl
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_GROUPS." WHERE id=:group_id");
    $stmt->bindParam('group_id',$_POST['group_id'],PDO::PARAM_INT);
    $success = $success && $stmt->execute();

    // delete rights list
    if ($success) {
      $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_ACL." WHERE group_id=:group_id");
      $stmt->bindParam('group_id',$_POST['group_id'],PDO::PARAM_INT);
      $success = $success && $stmt->execute();
    }

    // status message
    if ($success) {
      echo 'Group was deleted successfully';
    }
    else {
      echo 'Error, while deleting Group';
    }
  }



} // end of Admin_Groups
?>
