<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008  Klemens Friedl <frik85@reactos.org>

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
 * class EMail
 * 
 */
class EMail
{


  /**
   * checks if an email is basicly (no full check) valid adress
   *
   * @access public
   */
  public static function isValid( $email )
  {
    // check if email is not empty and basicly a valid form of email adress
    return preg_match('/^[\\w\\.\\+\\-=]+@[\\w\\.-]+\\.[\\w\\-]+$/', $email);
  } // end of member function isValid


  /**
   * send an email, using our defined headers
   *
   * @param string receiver valid email adress
   * @param string subject email subject
   * @param string message email text
   * @return bool
   * @access public
   */
  public static function send( $receiver, $subject, $message )
  {
    // check for valid receiver
    if (!self::isValid($receiver)) {
      return false;
    }

    // email addresses
    $receiver = htmlentities($receiver, ENT_NOQUOTES, 'UTF-8');

    // header
    $headers = "";
    $headers .= "From:" .RosCMS::getInstance()->emailSystem()."\n";
    $headers .= "Reply-To:" . RosCMS::getInstance()->emailSystem()."\n"; 
    $headers .= "X-Mailer: ".RosCMS::getInstance()->systemBrand()."\n"; 
    $headers .= "X-Sender-IP: ".$_SERVER['REMOTE_ADDR']."\n"; 
    $headers .= "Content-type: text/plain\n";

    // send the mail
    return @mail($receiver, $subject, $message, $headers);
  } // end of member function send



  /**
   * checks if the email is reserved by any user
   *
   * @param string email
   * @return bool
   * @access public
   */
  public static function isUsed( $email )
  {
    // check if another account with the same email address already exists
    $stmt=&DBConnection::getInstance()->prepare("SELECT 1 FROM ".ROSCMST_USERS." WHERE email = :email LIMIT 1");
    $stmt->bindParam('email',$email,PDO::PARAM_STR);
    $stmt->execute();
    
    return ($stmt->fetchColumn() !== false);
  } // end of member function isUsed


} // end of HTML
?>
