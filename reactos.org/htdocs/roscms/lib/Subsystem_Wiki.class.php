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
 * class Subsystem_Wiki
 * 
 */
class Subsystem_Wiki extends Subsystem
{

  /** Aggregations: */

  /** Compositions: */
  const DB_NAME = 'wiki'; // change this to your wiki-DB name

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
    $this->name = 'wiki';
    $this->user_table = self::DB_NAME.'.user';
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
    return parent::in( $login_type, $target, 'wiki' );
  } // end of member function login


  /**
   *  checks if user Details are matching in roscms with wiki
   *
   * @access protected
   */
  protected function checkUser( )
  {
    $inconsistencies = 0;
    $stmt=DBConnection::getInstance()->prepare("SELECT u.id AS user_id, u.name AS user_name, u.email, fullname, p.user_name AS subsys_name, p.user_email AS subsys_email, p.user_real_name AS subsys_fullname FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_SUBSYS." m ON m.user_id = u.id JOIN ".$this->user_table." p ON  p.user_id = m.subsys_user_id WHERE m.subsys = 'wiki' AND (REPLACE(u.name, '_', ' ') != p.user_name OR u.email != p.user_email OR u.fullname != p.user_real_name) ");
    $stmt->execute() or die('DB error (subsys_wiki #1)');

    while ($mapping = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo 'Info mismatch for RosCMS userid '.$mapping['user_id'].': ';

      if ($mapping['user_name'] != $mapping['subsys_name']) {
        echo 'user_name '.$mapping['user_name'].'/'.$mapping['subsys_name'].' ';
      }

      if ($mapping['email'] != $mapping['subsys_email']) {
        echo 'user_email '.$mapping['user_email'].'/'.$mapping['subsys_email'].' ';
      }

      if ($mapping['fullname'] != $mapping['subsys_fullname']) {
        echo 'user_fullname '.$mapping['fullname'].'/'.$mapping['subsys_fullname'].' ';
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
  protected function updateUserPrivate( $user_id, $user_name, $user_email, $user_fullname, $subsys_user )
  {
    $wiki_user_name = str_replace('_', ' ', $user_name);

    // Make sure that the email address and/or user name are not already in use in wiki from another user_id
    $stmt=DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".$this->user_table." WHERE (LOWER(user_name) = LOWER(:user_name) OR  LOWER(user_email) = LOWER(:user_email)) AND user_id <> :user_id");
    $stmt->bindParam('user_name',$wiki_user_name,PDO::PARAM_STR);
    $stmt->bindParam('user_email',$user_email,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_wiki #7)');

    // stop if one of both already exists
    if ($stmt->fetchColumn() > 0) {
      echo 'User name ('.$user_name.') and/or email address ('.$user_email.') collision<br />';
      return false;
    }

    // Now, make sure that info in wiki matches info in roscms
    $stmt=DBConnection::getInstance()->prepare("UPDATE ".$this->user_table." SET user_name = :user_name, user_email = :user_email, user_real_name = :user_fullname WHERE user_id = :user_id");
    $stmt->bindParam('user_name',$wiki_sql_user_name,PDO::PARAM_STR);
    $stmt->bindParam('user_email',$roscms_user_email,PDO::PARAM_STR);
    $stmt->bindParam('user_fullname',$roscms_user_fullname,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$wiki_user_id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_wiki #8)');

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
  protected function addUser( $id, $name, $email, $fullname )
  {
    // add new user to wiki user table
    $stmt=DBConnection::getInstance()->prepare("INSERT INTO ".$this->user_table." (user_name, user_real_name, user_password, user_newpassword, user_email, user_options, user_touched) VALUES (REPLACE(:user_name, '_', ' '), :user_fullname, '', '', :user_email, '', DATE_FORMAT(NOW(), '%Y%m%d%H%i%s'))");
    $stmt->bindParam('user_name',$name,PDO::PARAM_STR);
    $stmt->bindParam('user_fullname',$fullname,PDO::PARAM_STR);
    $stmt->bindParam('user_email',$email,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_wiki #10)');

    // Finally, insert a row in the mapping table
    $stmt=DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsys_user_id) VALUES(:user_id, 'wiki', LAST_INSERT_ID())");
    $stmt->bindParam('user_id',$id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_wiki #11)');

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
    $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM ".$this->user_table." WHERE LOWER(user_email) = LOWER(:user_email) LIMIT 1");
    $stmt->bindParam('user_email',$user['email'],PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_wiki #5)');
    $wiki_user_id = $stmt->fetchColumn();
    if ($wiki_user_id === false) {

      // That failed. Let's try to match on user name then
      $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM ".$this->user_table." WHERE LOWER(user_name) = LOWER(REPLACE(:user_name, '_', ' '))");
      $stmt->bindParam('user_name',$user['name'],PDO::PARAM_STR);
      $stmt->execute() or die('DB error (subsys_wiki #6)');
      $wiki_user_id = $stmt->fetchColumn();
    }

    if ($wiki_user_id === false){

    // We haven't found a match, so we need to add a new wiki user
      return self::addUser($user_id, $user['name'], $user['email'], $user['fullname']);
    }
    else {

      // Synchronize the info in wiki
      if (false === self::updateUserPrivate($user_id, $user['name'], $user['email'], $user['fullname'], $wiki_user_id)) {
        return false;
      }

      // Insert a row in the mapping table
      $stmt=DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsys_user_id) VALUES(:roscms_user, 'wiki', :wiki_user)");
      $stmt->bindParam('roscms_user',$user_id,PDO::PARAM_INT);
      $stmt->bindParam('wiki_user',$wiki_user_id,PDO::PARAM_INT);
      $stmt->execute() or die('DB error (subsys_wiki #9)');

      return true;
    }
  } // end of member function addUser


} // end of Subsystem_Wiki
?>
