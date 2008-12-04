<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>

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
 * class Subsystem
 * 
 */
abstract class Subsystem extends Login
{

   /*** Attributes: ***/
   protected static $roscms_path;
   protected $name = '';          // name of subsystem
   protected $user_table = '';    // user table name
   protected $userid_column = ''; // user table id column name


  /**
   *
   *
   * @return int
   * @access protected
   */
  protected function checkMapping( )
  {
    $inconsistencies = 0;
  
    $stmt=DBConnection::getInstance()->prepare("SELECT u.id FROM ".ROSCMST_USERS." u WHERE u.id NOT IN (SELECT m.user_id FROM ".ROSCMST_SUBSYS." m WHERE m.user_id = u.id AND m.subsys = :subsys_name)");
    $stmt->bindParam('subsys_name',$this->name,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_utils #1)');
  
    while ($user = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo 'No mapping of RosCMS userid '.$user['id'].'<br />';
      $inconsistencies++;
    }

    return $inconsistencies;
  } // end of member function checkMapping


  /**
   * checks if user id and mappings with current subsystem is set
   *
   * @param string subsys
   * @param string date if no datetime is set, current datetime is used
   * @param string date if no datetime is set, current datetime is used

   * @return int
   * @access protected
   */
  protected function userIdsExists()
  {
    $inconsistencies = 0;

    $stmt=DBConnection::getInstance()->prepare("SELECT u.id AS user_id, m.subsys_user_id FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_SUBSYS." m ON m.user_id = u.id LEFT OUTER JOIN ".$this->user_table." ss ON ss.".$this->userid_column." = m.subsys_user_id WHERE m.subsys = :subsys_name AND ss.".$this->userid_column." IS NULL");
    $stmt->bindParam('subsys_name',$this->name,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_utils #2)');
    while ($mapping = $stmt->fetch(PDO::FETCH_ASSOC))
    {
      echo 'RosCMS userid '.$mapping['user_id'].' maps to subsys userid '.$mapping['subsys_user_id'].' but that subsys userid doesn\'t exist<br />';
      $inconsistencies++;
    }

    $stmt=DBConnection::getInstance()->prepare("SELECT ss.".$this->userid_column." AS user_id FROM ".$this->user_table." ss WHERE ss.".$this->userid_column." NOT IN (SELECT m.subsys_user_id FROM ".ROSCMST_SUBSYS." m WHERE m.subsys_user_id = ss.".$this->userid_column." AND m.subsys = :subsys_name)");
    $stmt->bindParam('subsys_name',$this->name,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_utils #3)');
    while ($subsys = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo 'No RosCMS userid for subsys userid '.$subsys['user_id'].'<br />';
      $inconsistencies++;
    }

    return $inconsistencies;
  } // end of member function userIdExists


  /**
   * 
   *
   * @access protected
   */
  protected function check( )
  {
    echo '<h2>'.$this->name.'</h2>';

    $fix_url = '?page=admin&amp;sec=subsys&amp;sec2=fix&amp;subsys='.$this->name;

    $inconsistencies = checkMapping() + userIdsExist() + checkUser();
    switch ($inconsistencies) {
      case 0:
        echo 'No problems found.<br />';
        break;

      case 1:
        echo '<br />one problem found. <a href="'.$fix_url.'">Fix this</a><br />';
        break;

      default:
        echo '<br />'.$inconsistencies.' problems found. <a href="'.$fix_url.'">Fix these</a><br />';
        break;
    }

    return $inconsistencies;
  }  // end of member function check


  /**
   * add or updates (if already exists) the mapping of the given user id 
   *
   * @param int user_id
   * @access protected
   */
  public function addOrUpdateMapping( $user_id )
  {
    $stmt=DBConnection::getInstance()->prepare("SELECT subsys_user_id FROM ".ROSCMST_SUBSYS." WHERE user_id = :user_id AND subsys = :subsys LIMIT 1");
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->bindParam('subsys',$this->name,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_wiki #2)');

    if ($subsys_user = $stmt->fetchColumn()) {
      return $this->updateUser($user_id, $subsys_user);
    }
    else {
      return $this->addMapping($user_id);
    }
  } // end of member check


  /**
   * add or updates (if already exists) the mapping of the given user id 
   *
   * @param int user_id
   * @param int subsys_user subsytem user id
   * @access protected
   */
  protected function updateUser( $user_id, $subsys_user )
  {
    $user = ROSUser::getDetailsById( $user_id );
    if ($user === false) {
      return false;
    }

    return $this->updateUserPrivate($user_id, $user['name'], $user['email'], $user['fullname'], $subsys_user);
  } // end of member function updateUser


  /**
   * update user details in the subsystem database
   *
   * @param int user_id
   * @param string user_name
   * @param string user_email
   * @param string user_fullname
   * @param int subsys_user subsytem user id
   * @access protected
   */
  abstract protected function updateUserPrivate( $user_id, $user_name, $user_email, $user_fullname, $subsys_user );


  /**
   * add a new user to the subsystem database
   *
   * @param int user_id
   * @param string user_name
   * @param string user_email
   * @param string user_fullname
   * @access protected
   */
  abstract protected function addUser( $user_id, $user_name, $user_email, $user_fullname );


  /**
   *
   *
   * @param int user_id
   * @access protected
   */
  abstract protected function addMapping( $user_id );


  /**
   * checks if user Details are matching in roscms with subsystem
   *
   * @access protected
   */
  abstract protected function checkUser( );

} // end of Subsys
?>
