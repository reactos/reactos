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
 * class Subsystem_Bugzilla
 * 
 */
class Subsystem_Bugzilla extends Subsystem
{

  /** Aggregations: */

  /** Compositions: */
  const DB_NAME = 'bugs'; // change this to your wiki-DB name

  // relationships
  const REL_ASSIGNEE = 0;
  const REL_QA       = 1;
  const REL_REPORTER = 2;
  const REL_CC       = 3;
  const REL_VOTER    = 4;
  const REL_ANY      = 100;
  private $relationships = array(self::REL_ASSIGNEE, self::REL_QA, self::REL_REPORTER, self::REL_CC, self::REL_VOTER);

  // positive events
  const EVT_OTHER           = 0;
  const EVT_ADDED_REMOVED   = 1;
  const EVT_COMMENT         = 2;
  const EVT_ATTACHMENT      = 3;
  const EVT_ATTACHMENT_DATA = 4;
  const EVT_PROJ_MANAGEMENT = 5;
  const EVT_OPENED_CLOSED   = 6;
  const EVT_KEYWORD         = 7;
  const EVT_CC              = 8;
  private $pos_events = array(self::EVT_OTHER, self::EVT_ADDED_REMOVED, self::EVT_COMMENT, self::EVT_ATTACHMENT, self::EVT_ATTACHMENT_DATA, self::EVT_PROJ_MANAGEMENT, self::EVT_OPENED_CLOSED, self::EVT_KEYWORD, self::EVT_CC);

  // negative events
  const EVT_UNCONFIRMED   = 50;
  const EVT_CHANGED_BY_ME = 51;
  private $neg_events = array(self::EVT_UNCONFIRMED, self::EVT_CHANGED_BY_ME);

  // global events
  const EVT_FLAG_REQUESTED = 100; # Flag has been requested of me
  const EVT_REQUESTED_FLAG = 101; # I have requested a flag
  private $global_events = array(self::EVT_FLAG_REQUESTED, self::EVT_REQUESTED_FLAG);


  /**
   *
   *
   * @param string path 
   * @access public
   */
  public function __construct( )
  {

    // set subsystem specific
    $this->name = 'bugzilla';
    $this->user_table = self::DB_NAME.'.profiles';
    $this->userid_column = 'userid';
  } // end of constructor


  /**
   *  checks if user Details are matching in roscms with wiki
   *
   * @access protected
   */
  protected function checkUser( )
  {
    $inconsistencies = 0;
    $stmt=&DBConnection::getInstance()->prepare("SELECT u.id AS user_id, u.name AS user_name, u.email, p.realname AS subsys_user, p.login_name AS subsys_email FROM ".ROSCMST_USERS." u JOIN ".ROSCMST_SUBSYS." m ON m.user_id=u.id JOIN ".$this->user_table." p ON p.userid=m.subsys_user_id WHERE m.map_subsys_name = 'bugzilla' AND (u.name != p.realname OR p.realname IS NULL OR u.email != p.login_name) ");
    $stmt->execute() or die('DB error (subsys_bugzilla #1)');
    while ($mapping = $stmt->fetch(PDO::FETCH_ASSOC)) {
      echo 'Info mismatch for RosCMS userid '.$mapping['user_id'].': ';
      
      // user name
      if ($mapping['user_name'] != $mapping['subsys_user']) {
        echo 'user_name '.$mapping['user_name'].'/'.$mapping['subsys_user'].' ';
      }
      
      // email
      if ($mapping['email'] != $mapping['subsys_email']){
        echo 'user_email '.$mapping['email'] . "/" .$mapping['subsys_email'];
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
    // Make sure that the email address and/or user name are not already in use in bugzilla
    $stmt=&DBConnection::getInstance()->prepare("SELECT COUNT(*) FROM ".$this->user_table." WHERE (LOWER(login_name) = LOWER(:email) OR LOWER(realname) = LOWER(:name)) AND userid != :user_id ");
    $stmt->bindParam('email',$user_email,PDO::PARAM_STR);
    $stmt->bindParam('name',$user_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$subsys_user,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_bugzilla #7)');
    $exists = $stmt->fetchColumn();
    
    if ($exists > 0) {
      echo 'User name ('.$user_name.') and/or email address ('.$user_email.') collision<br />';
      return false;
    }

    // Now, make sure that info in bugzilla matches info in roscms
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".$this->user_table." SET login_name = :email, realname = :name WHERE userid = :user_id");
    $stmt->bindParam('email',$user_email,PDO::PARAM_STR);
    $stmt->bindParam('name',$user_name,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$subsys_user,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_bugzilla #8)');

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

    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".$this->user_table." (login_name, cryptpassword, realname) VALUES (:email, '*', :name)");
    $stmt->bindParam('email',$email,PDO::PARAM_STR);
    $stmt->bindParam('name',$name,PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_bugzilla #10)');

    // The default email_setting was copied from bugzilla/Bugzilla/User.pm function insert_new_user
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".self::DB_NAME.".email_setting (user_id, relationship, event) VALUES (LAST_INSERT_ID(), :relation, :event)");
    foreach ($this->relationships as $rel) {
      foreach (array_merge($this->pos_events, $this->neg_events) as $event) {
        if ($event != self::EVT_CHANGED_BY_ME && ($event != self::EVT_CC || $rel == self::REL_REPORTER)) {
          $stmt->bindParam('relation',$rel,PDO::PARAM_INT);
          $stmt->bindParam('event',$event,PDO::PARAM_INT);
          $stmt->execute() or die('DB error (subsys_bugzilla #14)');
        }
      }
    }
    
    foreach ($this->global_events as $event) {
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".self::DB_NAME.".email_setting (user_id, relationship, event) VALUES (LAST_INSERT_ID(), :relation, :event)");
      $stmt->bindValue('relation',self::REL_ANY,PDO::PARAM_INT);
      $stmt->bindParam('event',$event,PDO::PARAM_INT);
      $stmt->execute() or die('DB error (subsys_bugzilla #15)');
    }

    // Finally, insert a row in the mapping table
    $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsy_user_id) VALUES(:user_id, 'bugzilla', LAST_INSERT_ID())");
    $stmt->bindParam('user_id',$id,PDO::PARAM_INT);
    $stmt->execute() or die('DB error (subsys_bugzilla #11)');

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
    $stmt=&DBConnection::getInstance()->prepare("SELECT userid FROM ".$this->user_table." WHERE LOWER(login_name) = LOWER(:user_email) LIMIT 1");
    $stmt->bindParam('user_email',$user['email'],PDO::PARAM_STR);
    $stmt->execute() or die('DB error (subsys_bugzilla #5)');

    $bz_user_id = $stmt->fetchColumn();
    if ($bz_user_id === false) {

      // That failed. Let's try to match on user name then
      $stmt=&DBConnection::getInstance()->prepare("SELECT userid FROM ".$this->user_table." WHERE LOWER(realname) = LOWER(:user_name)");
      $stmt->bindParam('user_name',$roscms_user_name,PDO::PARAM_STR);
      $stmt->execute() or die('DB error (subsys_bugzilla #6)');
      $bz_user_id = $stmt->fetchColumn();
    }

    if ($bz_user_id === false){
      // We haven't found a match, so we need to add a new bugzilla user
        return self::addUser($user_id, $user['name'], $user['email'],$user['register']);
    }
    else {
      // Synchronize the info in bugzilla
      if (false === self::updateUserPrivate($user_id, $user['name'], $user['email'],$user['register'], $bz_user_id)){
        return false;
      }

      // Insert a row in the mapping table
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_SUBSYS." (user_id, subsys, subsys_user_id) VALUES(:roscms_user, 'bugzilla', :bugzilla_user)");
      $stmt->bindParam('roscms_user',$user_id,PDO::PARAM_INT);
      $stmt->bindParam('bugzilla_user',$bz_user_id,PDO::PARAM_INT);
      $stmt->execute() or die('DB error (subsys_bugzilla #9)');

      return true;
    }

  } // end of member function addUser


} // end of Subsystem_Bugzilla
?>
