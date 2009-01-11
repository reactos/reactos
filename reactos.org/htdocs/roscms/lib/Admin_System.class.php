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
 * class Admin_System
 * 
 */
class Admin_System
{



  /**
   *
   *
   * @access protected
   */
  public function __construct()
  {
    // check if user has enough rights
    Login::required();
    if (!ThisUser::getInstance()->hasAccess('admin')) {
      die('Not enough rights to access this Area.');
    }

    // is an action given ?
    if (empty($_GET['action'])) {
      die('missing param');
    }

    // show requested form
    if (empty($_GET['submit'])) {
      switch ($_GET['action']) {
        case 'apl':
          $this->showAPL();
          break;
      }
    }

    // submit form data
    else {
      switch ($_GET['action']) {
        case 'apl':
          $this->submitAPL();
          break;
      }
    }
  }



  /**
   *
   *
   * @access protected
   */
  protected function showAPL( )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name_short, description FROM ".ROSCMST_AREA." ORDER BY name_short ASC");
    $stmt->execute();
    $areas=$stmt->fetchAll(PDO::FETCH_ASSOC);

    echo_strip('
      <h2>Create new Area Protection List (APL)</h2>
      <form onsubmit="return false;">
        <fieldset>
          <legend>Groups Access Rights</legend>
          <table>
            <tr>
              <th style="vertical-align:bottom;" title="Security Level">SecLvl</th>
              <th style="vertical-align:bottom;">Group Name</th>');
    foreach ($areas as $area) {
      echo '<th style="width: 1em;font-family:monospace;vertical-align:bottom;line-height: 0.8em;" title="'.$area['name_short'].': '.$area['description'].'">';
      for ($i=0; $i<strlen($area['name_short']);$i++) {
        echo substr($area['name_short'], $i, 1).'<br />';
      }
      echo '</th>';
    }
    echo '</tr>';

    $stmt_is=&DBConnection::getInstance()->prepare("SELECT TRUE FROM ".ROSCMST_AREA." a JOIN ".ROSCMST_AREA_ACCESS." b ON a.id=b.area_id WHERE b.group_id=:group_id AND a.id=:area_id ORDER BY a.name_short LIMIT 1");

    $stmt=&DBConnection::getInstance()->prepare("SELECT id, name, security_level, description FROM ".ROSCMST_GROUPS." ORDER BY security_level ASC, name ASC");
    $stmt->execute();
    while ($group = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $stmt_is->bindParam('group_id',$group['id'],PDO::PARAM_INT);
      echo_strip('
        <tr title="'.htmlspecialchars($group['description']).'">
          <td style="text-align:center;">'.$group['security_level'].'</td>
          <td>'.htmlspecialchars($group['name']).'</td>');
      foreach ($areas as $area) {
        $stmt_is->bindParam('area_id',$area['id'],PDO::PARAM_INT);
        $stmt_is->execute();
        $is = $stmt_is->fetchColumn();

        echo '<td title="'.$group['name'].'--'.$area['name_short'].': '.$area['description'].'"><input type="checkbox" value="1" name="valid'.$group['id'].'_'.$area['id'].'" '.($is ? 'checked="checked"' : '').' /></td>';
      }
      echo '</tr>';
    }

    echo_strip('
          </table>
        </fieldset>
        <button onclick="'."submitAreaProtection()".'">update APL</button>
      </form>
    ');
  } // end of member function showNew



  /**
   *
   *
   * @access protected
   */
  protected function submitAPL( )
  {
    $success = true;

    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_AREA_ACCESS);
    $success = $success && $stmt->execute();

    if ($success) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_AREA_ACCESS." (area_id, group_id) VALUES (:area_id,:group_id)");
      foreach ($_POST as $item=>$val) {
        if (strpos($item,'valid')===0) {
          $item = substr($item, 5);
          $id = explode('_',$item);
          if($id[0] > 0 && $id[1] > 0 && $val=='true') {
            $stmt->bindParam('area_id',$id[1],PDO::PARAM_INT);
            $stmt->bindParam('group_id',$id[0],PDO::PARAM_INT);
            $success = $success && $stmt->execute();
          }
        }
      }
    }

    // give the user success message
    if ($success) {
      echo 'Success, Area Protection List was updated successfully';
    }
    else {
      echo 'Error, while updating Area Protection list';
    }
  } // end of member function submitNew

} // end of Admin_System
?>
