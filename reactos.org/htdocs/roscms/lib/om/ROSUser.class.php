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
//@TODO Wrap up into namespace and rename to User again, if 5.3 is final


/**
 * class ROSUser
 * 
 */
class ROSUser
{



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
   * checks if a new password was requested
   *
   * @param string code code which was delivered to activate the new password
   * @return bool
   * @access public
   */
  public static function hasPasswordReset( $code )
  {
    // check if an account with the password activation exists
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_USERS." WHERE activation_password = :getpwd_id LIMIT 1");
    $stmt->bindParam('getpwd_id',$code,PDO::PARAM_STR);
    $stmt->execute();
    return ($stmt->fetchColumn() !== false);
  } // end of member function hasPasswordReset



  /**
   * creates a new activation code
   *
   * @return string
   * @access public
   */
  public static function makeActivationCode( )
  {
    $code = '';
    for ($n = 0; $n < 20; ++$n) {
      $code .= chr(rand(0, 255));
    }

    $code = base64_encode($code);   // base64-set, but filter out unwanted chars
    $code = preg_replace('/[+\/=IG0ODQRtl]/i', '', $code);  // strips hard to discern letters, depends on used font type
    $code = substr($code, 0, rand(10, 15));
    return $code;
  } // end of member function makeActivationCode



  /**
   * adds a new account membership
   *
   * @param int user
   * @param int group
   * @access private
   */
  public static function addMembership( $user, $group )
  {
    $thisuser=&ThisUser::getInstance();

    // check if user is already member, so we don't add him twice
    // also check that you don't give accounts a higher seclevel
    $stmt=&DBConnection::getInstance()->prepare("SELECT DISTINCT g.security_level FROM ".ROSCMST_GROUPS." g JOIN ".ROSCMST_MEMBERSHIPS." m ON m.group_id=g.id WHERE g.id = :group_id AND m.user_id != :user_id LIMIT 1");
    $stmt->bindParam('group_id',$group,PDO::PARAM_INT);
    $stmt->bindParam('user_id',$user,PDO::PARAM_INT);
    $stmt->execute();
    $level = $stmt->fetchColumn();

    // is user able to add groups of this level
    if ($level !== false && $thisuser->hasAccess('addlvl'.$level.'group')) {

      // insert new membership
      $stmt=&DBConnection::getInstance()->prepare("INSERT INTO ".ROSCMST_MEMBERSHIPS." ( user_id , group_id ) VALUES ( :user_id, :group_id )");
      $stmt->bindParam('user_id',$user,PDO::PARAM_INT);
      $stmt->bindParam('group_id',$group,PDO::PARAM_INT);
      if ($stmt->execute()) {
        Log::writeLow('add user account membership: user-id='.$user.', group-id='.$group);
      }
    }
  } // end of member function addMembership



  /**
   * delete group membership
   *
   * @param int user
   * @param int group
   * @access private
   */
  public static function deleteMembership( $user, $group )
  {
    $stmt=&DBConnection::getInstance()->prepare("DELETE FROM ".ROSCMST_MEMBERSHIPS." WHERE user_id = :user_id AND group_id = :group_id LIMIT 1");
    $stmt->bindParam('user_id',$user,PDO::PARAM_INT);
    $stmt->bindParam('group_id',$group,PDO::PARAM_INT);
    if ($stmt->execute()) {
      Log::writeLow('delete user account membership: user-id='.$user.', group-id='.$group);
    }
  } // end of member function deleteMembership



  /**
   * disable user account
   *
   * @param int user
   * @access private
   */
  public static function disableAccount( $user )
  {
    // only with admin rights
    if ($thisuser->hasAccess('disableaccount')) {
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET disabled = TRUE WHERE id = :user_id");
      $stmt->bindParam('user_id',$user,PDO::PARAM_INT);
      if ($stmt->execute()) {
        Log::writeMedium('account '.$user.' disabled');
      }
    }
  } // end of member function disableAccount




  /**
   * reenable user account
   *
   * @param int user
   * @access private
   */
  public static function enableAccount( $user )
  {
    // enable account only with admin rights
    if ($thisuser->hasAccess('disableaccount')) {
      // enable account only, if he has already activated his account
      $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET disabled = FALSE WHERE activation = '' AND id = :user_id");
      $stmt->bindParam('user_id',$user,PDO::PARAM_INT);
      if ($stmt->execute()) {
        Log::writeMedium('account '.$user.' enabled');
      }
    }
  } // end of member function enableAccount




  /**
   * reenable user account
   *
   * @param int user
   * @access private
   */
  public static function changeLanguage( $user, $lang )
  {
    $stmt=&DBConnection::getInstance()->prepare("UPDATE ".ROSCMST_USERS." SET lang_id = :lang WHERE id = :user_id");
    $stmt->bindParam('lang',$lang);
    $stmt->bindParam('user_id',$user);
    if ($stmt->execute()) {
      Log::writeLow('change user account language: user-id='.$user.', lang-id='.$lang);
    }
  } // end of member function changeLanguage



} // end of ROSUser
?>
