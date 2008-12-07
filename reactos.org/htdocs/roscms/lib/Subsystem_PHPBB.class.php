<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Michael Wirth

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
 * class Subsystem_PHPBB
 * 
 */
class Subsystem_PHPBB extends Subsystem
{

  /** Aggregations: */

  /** Compositions: */
  const DB_NAME = 'forum'; // change this to your wiki-DB name

   /*** Attributes: ***/


  /**
   *
   *
   * @param string path 
   * @access public
   */
  public function __construct( )
  {

    // set subsystem specific
    $this->name = 'phpbb';
    $this->user_table = self::DB_NAME.'.phpbb_users';
    $this->userid_column = 'user_id';
  } // end of constructor


  /**
   *
   *
   * @param string target to jump back after login process
   * @param string subsystem name which is called
   * @return int
   * @access public
   */
  public static function in( $login_type, $target )
  {
    return parent::in( $login_type, $target, 'phpbb' );
  } // end of member function login


  /**
   *  checks if user Details are matching in roscms with wiki
   *
   * @access protected
   */
  protected function checkUser( )
  {
    $inconsistencies = 0;

    $stmt=&DBConnection::getInstance()->prepare("SELECT u.id AS user_id, u.name AS user_name, u.email, u.created, p.username AS subsys_name, p.user_email AS subsys_email, FROM_UNIXTIME(p.user_regdate) AS subsys_created FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_SUBSYS." m ON u.id = m.user_id JOIN ".$this->user_table." p ON p.user_id=m.subsys_user_id WHERE m.map_subsys_name = 'phpbb' AND (u.name != p.username OR u.email != p.user_email OR u.created != subsys_created) ");
    $stmt->execute() or die('DB error (subsys_phpbb #1)');
    while ($mapping = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo 'Info mismatch for RosCMS userid '.$mapping['user_id'].': ';

      if ($mapping['user_name'] != $mapping['subsys_name']) {
        echo 'user_name '.$mapping['user_name'].'/'.$mapping['subsys_name'].' ';
      }

      if ($mapping['email'] != $mapping['subsys_email']) {
        echo 'user_email '.$mapping['email'].'/'.$mapping['subsys_email'].' ';
      }

      if ($mapping['created'] != $mapping['subsys_created']) {
        echo 'user_register '.$mapping['created'].'/'.$mapping['subsys_created'];
      }

      echo '<br />';
      $inconsistencies++;
    }

    return $inconsistencies;
  } // end of member function checkUserInfo


  /**
   * update user details in the wiki database
   *
   * @param int user_id
   * @access protected
   */
  protected function updateUserPrivate( $user_id, $user_name, $user_email, $user_register, $subsys_user )
  {

    // Make sure that the email address and/or user name are not already in use in phpbb
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".$this->user_table." WHERE (LOWER(username) = LOWER(:user_name) OR LOWER(user_email) = LOWER(:user_email)) AND user_id <> :user_id");
    $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
    $stmt->bindParam('user_email',$user_email,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$subsys_user,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_phpbb #7)');
    if ($stmt->fetchColumn() > 0) {
        echo 'User name ('.$user_name.') and/or email address ('.$user_email.') collision<br />';
        return false;
    }

    // Now, make sure that info in phpbb matches info in roscms
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".$this->user_table." SET username = :user_name, user_email = :user_email, user_regdate = :reg_date WHERE user_id = :user_id");
    $stmt->bindParam('user_name',$user_name,PDO::PARAM_STR);
    $stmt->bindParam('reg_date',$user_register,PDO::PARAM_STR);
    $stmt->bindParam('user_email',$user_email,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_phpbb #8)');

    return true;
  } // end of member function updateUserPrivate


  /**
   * add a new user to the wiki database
   *
   * @param int user_id
   * @param string user_name
   * @param string user_email
   * @param string user_fullname
   * @access protected
   */
  protected function addUser( $id, $name, $email, $register )
  {
    // Determine the next available userid
    $stmt=&DBConnection::getInstance()->prepare("SELECT MAX(user_id) FROM ".$this->user_table);
    $stmt->execute() or die('DB error (subsys_phpbb #20)');
    $phpbb_user_id = $stmt->fetchColumn() + 1;

    $dbh = &DBConnection::getInstance();
    $stmt=$dbh->prepare("INSERT INTO ".$this->user_table." (user_id, username, username_clean, user_password, user_email, user_regdate, user_permissions, user_sig, user_occ,   	user_interests) VALUES (:user_id, :user_name, :user_clean_name, '*', :user_email, :reg_date, '','', '', '')");
    $stmt->bindParam('user_id',$phpbb_user_id,PDO::PARAM_INT);
    $stmt->bindParam('user_name',$name,PDO::PARAM_STR);
    $stmt->bindValue('user_clean_name',strtolower($name),PDO::PARAM_STR);
    $stmt->bindParam('user_email',$email,PDO::PARAM_STR);
    $stmt->bindParam('reg_date',$register,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_phpbb #10)');

    // Put the user in the REGISTERED group
    $stmt=&DBConnection::getInstance()->prepare("SELECT group_id FROM ".self::DB_NAME.".phpbb_groups WHERE group_name = 'REGISTERED' LIMIT 1");
    $stmt->execute() or die('DB error (subsys_phpbb #18)');
    $group_id = $stmt->fetchColumn();
    
    if($group_id === false){
      die('DB error (subsys_phpbb #20)');
    }
    
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".self::DB_NAME.".phpbb_user_group (group_id, user_id, user_pending) VALUES (:group_id, :user_id, 0)");
    $stmt->bindParam('group_id',$group_id,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$phpbb_user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_phpbb #19)');

    // Finally, insert a row in the mapping table
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsys_user_id) VALUES(:roscms_user, 'phpbb', :phpbb_user)");
    $stmt->bindParam('roscms_user',$id,PDO::PARAM_INT);
    $stmt->bindParam('phpbb_user',$phpbb_user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_phpbb #11)');

    return true;
  } // end of member function addUser


  /**
   * 
   *
   * @access protected
   */
  protected function addMapping( $user_id )
  {
    $user = ROSUser::getDetailsById($user_id);
    if ($user === false) {
      return false;
    }

    // First, try to match on email address
    $stmt=&DBConnection::getInstance()->prepare("SELECT user_id FROM ".$this->user_table." WHERE LOWER(user_email) = LOWER(:user_email)");
    $stmt->bindParam('user_email',$user['email'],PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_phpbb #5)');
    $phpbb_user_id = $stmt->fetchColumn();
    if ($phpbb_user_id === false) {
    
        // That failed. Let's try to match on user name then
        $stmt=&DBConnection::getInstance()->prepare("SELECT user_id FROM ".$this->user_table." WHERE LOWER(username) = LOWER(:user_name)");
        $stmt->bindParam('user_name',$user['name'],PDO::PARAM_STR);
        $stmt->execute() or die('DB error (subsys_phpbb #6)');
        $phpbb_user_id = $stmt->fetchColumn();
    }

    if ($phpbb_user_id === false) {

        // We haven't found a match, so we need to add a new phpbb user
        return self::addUser($user_id, $user['name'], $user['email'],$user['register']);
    }
    else {

      // Synchronize the info in phpbb
      if (false === self::updateUserPrivate($user_id, $user['name'], $user['email'],$user['register'], $phpbb_user_id)){
        return false;
      }

      // Insert a row in the mapping table
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsys_user_id) VALUES(:roscms_user, 'phpbb', :phpbb_user)");
      $stmt->bindParam('roscms_user',$user_id,PDO::PARAM_INT);
      $stmt->bindParam('phpbb_user',$phpbb_user_id,PDO::PARAM_INT);
      $stmt->execute() or die('DB error (subsys_phpbb #9)');

      return true;
    }
  } // end of member function addUser


} // end of Subsystem_PHPBB
?>
