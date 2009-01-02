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

    // only if user has rights to access the interface
    if ($thisuser->securityLevel() > 0) {

      // go through acl's
      $stmt=&DBConnection::getInstance()->prepare("SELECT a.id, b.can_read, b.can_add, b.can_write, b.can_delete, b.can_publish, b.can_translate FROM ".ROSCMST_ACCESS." a JOIN ".ROSCMST_ACL." b ON a.id=b.acl_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id = b.group_id WHERE m.user_id = :user_id");
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
    }

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
   *
   *
   * @access public
   */
  public static function getAccessId( $name_short )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT id FROM ".ROSCMST_ACCESS." WHERE name_short=:name_short LIMIT 1");
    $stmt->bindParam('name_short',$name_short,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  }


  /**
   * Constructs a list of things the user can do
   *
   * @param int data_id 
   * @return rights list
   * @access private
   */
  private function getRightsList( $rev_id, $is_rev = true )
  {
    $thisuser = &ThisUser::getInstance();

    // roscms interface access ?
    if ($thisuser->securityLevel() < 1) {
      return;
    }

    // contains list with granted rights
    $rights = array('read'=>false,'write'=>false,'add'=>false,'delete'=>false,'publish'=>false,'translate'=>false,);

    // get rights
    if ($is_rev) {
      $stmt=&DBConnection::getInstance()->prepare("SELECT b.can_read, b.can_add, b.can_delete, b.can_translate, b.can_publish, b.can_write FROM ".ROSCMST_REVISIONS." r JOIN ".ROSCMST_ENTRIES." d ON r.data_id=d.id JOIN ".ROSCMST_ACCESS." a ON d.acl_id=a.id JOIN ".ROSCMST_ACL." b ON a.id=b.acl_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=b.group_id WHERE r.id = :rev_id AND m.user_id=:user_id");
      $stmt->bindParam('rev_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    }
    else {
      $stmt=&DBConnection::getInstance()->prepare("SELECT b.can_read, b.can_add, b.can_delete, b.can_translate, b.can_publish, b.can_write FROM ".ROSCMST_ENTRIES." d JOIN ".ROSCMST_ACCESS." a ON d.acl_id=a.id JOIN ".ROSCMST_ACL." b ON a.id=b.acl_id JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=b.group_id WHERE d.id = :data_id AND m.user_id=:user_id");
      $stmt->bindParam('data_id',$rev_id,PDO::PARAM_INT);
      $stmt->bindParam('user_id',$thisuser->id(),PDO::PARAM_INT);
    }
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
  public function hasRight( $data_id, $kind )
  {
    // only if roscms interface access is granted
    if (ThisUser::getInstance()->securityLevel() < 1) {
      return false;
    }

    // return if the requested kind of right is in the rights list for the user
    $rights = self::getRightsList($data_id, false);
    return $rights[$kind];
  } // end of member function hasRight


  /**
   * gives a short overview about user rights
   *
   * @param int data_id 
   * @return explanation
   * @access public
   */
  public function rightsOverview( $data_id )
  {
    // only if roscms interface access is granted
    if (ThisUser::getInstance()->securityLevel() < 1) {
      return;
    }

    $rights = self::getRightsList($data_id, false);  // so we don't need to call the same function several times
    $explanation = ''; // contains abbreviations for each right or a - (if missing) symbol instead

    // start to construct list
    $explanation .= $rights['read'] ? '-' : 'r';
    $explanation .= $rights['write'] ? '-' : 'w';
    $explanation .= $rights['add'] ? '-' : 'a';
    $explanation .= $rights['delete'] ? '-' : 'd';
    $explanation .= $rights['publish'] ? '-' : 'p';
    $explanation .= $rights['translate'] ? '-' : 't';

    // add also security level
    $explanation .= ' '.ThisUser::getInstance()->securityLevel();

    return $explanation;
  } // end of member function rightsOverview


} // end of Security
?>
