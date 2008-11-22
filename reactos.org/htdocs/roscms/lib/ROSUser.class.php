<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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


// this Class name is a workaround to get WIKI working


/**
 * class User
 * 
 */
class ROSUser
{

  /** Aggregations: */

  /** Compositions: */

   /*** Attributes: ***/


  /**
   * checks if this timezone is in our Database
   *
   * @param string tz_code timezone code
   * @return bool
   * @access public
   */
  public static function checkTimezone( $tz_code )
  {
    $stmt=DBConnection::getInstance()->prepare("SELECT 1 FROM user_timezone WHERE tz_code = :tz_code LIMIT 1");
    $stmt->bindparam('tz_code',$tz_code,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchOnce();
  }// end of member function timezone_string


  /**
   * gives Language name of user settings (if language is unknown return language_id)
   *
   * @param string id language id
   * @return bool
   * @access public
   */
  public static function getLanguage( $user_id, $short = false)
  {
    $stmt=DBConnection::getInstance()->prepare("SELECT lang_name FROM user_language l JOIN users u ON u.user_language=l.lang_id WHERE user_id = :user_id LIMIT 1");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_STR);
    $stmt->execute();
    $ret = $stmt->fetchColumn();
    if ($ret===false || $short){
      $stmt=DBConnection::getInstance()->prepare("SELECT user_language FROM users WHERE user_id = :user_id LIMIT 1");
      $stmt->bindparam('user_id',$user_id,PDO::PARAM_STR);
      $stmt->execute();
      return $stmt->fetchColumn();
    }
    return $ret;
  } // end of member function getLanguage



  /**
   * gives Country name of user settings
   *
   * @param string id country id
   * @return bool
   * @access public
   */
  public static function getCountry( $user_id )
  {
    $stmt=DBConnection::getInstance()->prepare("SELECT c.coun_name FROM user_countries c JOIN users u ON u.user_country = c.coun_id  WHERE user_id = :user_id LIMIT 1");
    $stmt->bindparam('user_id',$user_id,PDO::PARAM_INT);
    $stmt->execute();
    return $stmt->fetchColumn();
    
  } // end of member function getCountry


  /**
   * returns an array with name, email, fullname, register timestamp
   *
   * @param int id
   * @return array
   * @access public
   */
  public static function getDetailsById( $id )
  {

    $stmt=DBConnection::getInstance()->prepare("SELECT user_name AS name, user_email AS email, user_fullname AS fullname, UNIX_TIMESTAMP(user_register) AS register FROM users WHERE user_id = :user_id LIMIT 1");
    $stmt->bindParam('user_id',$id,PDO::PARAM_INT);
    $stmt->execute() or die("DB error (subsys_utils #4)");

    // check if a user was found
    $user = $stmt->fetchOnce(PDO::FETCH_ASSOC);
    if ($user === false) {
      echo 'Can\'t find roscms user details for user id '.$id.'<br />';
      return false;
    }

  // We need a valid username and email address in roscms
    if ($user['name'] == '') {
      echo 'No valid roscms user name found for user id '.$id.'<br />';
      return false;
    }
    if ($user['email'] == '') {
      echo 'No valid roscms email address found for user id '.$id.'<br />';
      return false;
    }

    return $user;
  }


  /**
   * does sync the user to subsystems user tables and adds mapping if necessary
   *
   * @param int id user_id
   * @access public
   */
  public static function syncSubsystems( $id )
  {
    // wiki
    $wiki=new Subsystem_Wiki();
    $wiki->addOrUpdateMapping( $id );

    // forum
    $forum=new Subsystem_PHPBB();
    $forum->addOrUpdateMapping( $id );

    // bugzilla
    $bugzilla=new Subsystem_Bugzilla();
    $bugzilla->addOrUpdateMapping( $id );
  } // end of member function syncSubsystems


  /**
   * 
   *
   * @param string email
   * @return bool
   * @access public
   */
  public static function hasPasswordReset( $code )
  {
    // check if an account with the password activation exists
    $stmt=DBConnection::getInstance()->prepare("SELECT user_id FROM users WHERE user_roscms_getpwd_id = :getpwd_id LIMIT 1");
    $stmt->bindParam('getpwd_id',$code,PDO::PARAM_STR);
    $stmt->execute();
    $password_id_exists = ($stmt->fetchColumn() !== false);
  } // end of member isGroupMember


  /**
   * 
   *
   * @param string email
   * @return bool
   * @access public
   */
  public static function hasEmail( $email )
  {
    // check if another account with the same email address already exists
    $stmt=DBConnection::getInstance()->prepare("SELECT user_email FROM users WHERE user_email = :email LIMIT 1");
    $stmt->bindParam('email',$email,PDO::PARAM_STR);
    $stmt->execute();
    
    return ($stmt->fetchColumn() !== false);
  } // end of member isGroupMember


  /**
   * 
   *
   * @param string email
   * @return bool
   * @access public
   */
  public static function makeActivationCode( )
  {
    $code = '';
    for ($n = 0; $n < 20; $n++) {
      $code .= chr(rand(0, 255));
    }

    $code = base64_encode($code);   // base64-set, but filter out unwanted chars
    $code = preg_replace('/[+\/=IG0ODQRtl]/i', '', $code);  // strips hard to discern letters, depends on used font type
    $code = substr($code, 0, rand(10, 15));
    return $code;
  } // end of member makeActivationCode


} // end of ROSUser
?>
