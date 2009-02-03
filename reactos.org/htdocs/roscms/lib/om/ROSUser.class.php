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



} // end of ROSUser
?>
