<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>
                  2008  Danny Götte <dangerground@web.de>

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
 * class Security
 * 
 */
class Security
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * 
   *
   * @param string kind kind of access is asked for
   * @return ACL
   * @access public
   */
  public function getACL( $kind )
  {
    $thisuser = &ThisUser::getInstance();

    $acl = '';
    $sec_access = false;  // security access already granted ?

    // go through acl's
    $stmt=&DBConnection::getInstance()->prepare("SELECT a.id, b.can_read, b.can_add, b.can_write, b.can_delete, b.can_publish, b.can_translate FROM ".ROSCMST_ACCESS." a JOIN ".ROSCMST_ENTRY_AREA." b ON a.id=b.acl_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id = b.group_id WHERE m.user_id = :user_id");
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute();
    while ($access = $stmt->fetch(PDO::FETCH_ASSOC)) {

      // add entries, remove them if they're on the deny list
      if ($access['can_'.$kind] == true) {
        if ($sec_access) {
          $acl .= " , ";
        }
        $acl .= DBConnection::getInstance()->quote($access['id'],PDO::PARAM_INT);
        $sec_access = true;
      }
    } // while

    // group our acl list, or fail because no rights to access
    if ($sec_access > 0) {
      $acl = " AND d.acl_id IN(". $acl .", NULL) ";
    }
    else {
      $acl = " AND FALSE ";
    }

    return $acl;
  } // end of member function getACL



  /**
   * Constructs a list of things the user can do
   *
   * @param int data_id 
   * @return rights list
   * @access private
   */
  private function getRightsList( $data_id )
  {
    $thisuser = &ThisUser::getInstance();

    // get rights
    $stmt=&DBConnection::getInstance()->prepare("SELECT name_short FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_ACCESS." a ON d.acl_id=a.id JOIN ".ROSCMST_ENTRY_AREA." b ON a.id=b.acl_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=b.group_id WHERE d.id = :data_id AND m.user_id=:user_id");
    $stmt->bindParam('data_id',$rev_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    $stmt->execute() or die('Rev-Entry "'.$rev_id.'" not found [usergroups].');

    // create a list with rights
    while($list = $stmt->fetch(PDO::FETCH_ASSOC)) {
      $rights['read'] |= ($list['can_read'] == true);
      $rights['write'] |= ($list['can_write'] == true);
      $rights['add'] |= ($list['can_add'] == true);
      $rights['delete'] |= ($list['can_delete'] == true);
      $rights['publish'] |= ($list['can_publish'] == true);
      $rights['translate'] |= ($list['can_translate'] == true);
    }



    return $rights;
  } // end of member function getRightsList


  /**
   * checks if the user has the given right to do things
   *
   * @param int data_id 
   * @param string kind kind of rights e.g. 'read' 
   * @return 
   * @access public
   */
  public function hasRight( $data_id, $area )
  {
return true;
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_ACL." a ON a.acl_id=d.acl_id JOIN ".ROSCMST_ENTRY_AREA." e ON e.acl_id=a.id JOIN ".ROSCMST_RIGHTS." r ON r.id=e.right_id JOIN ".ROSCMST_GROUPS." g ON g.id=e.group_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE r.name_short=:area AND m.user_id=:user_id AND d.id=:data_id LIMIT 1");
    $stmt->bindParam('data_id',$data_id,PDO::PARAM_INT);
    $stmt->bindParam('area',$area,PDO::PARAM_STR);
    $stmt->bindParam('user_id',ThisUser::getInstance()->id(),PDO::PARAM_INT);
    return $stmt->execute();
  } // end of member function hasRight



} // end of Security
?>
