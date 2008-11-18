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
   *
   *
   * @access public
   */
  public static function isValid( $email )
  {
    return ($email != '' && preg_match('/^[\\w\\.\\+\\-=]+@[\\w\\.-]+\\.[\\w\\-]+$/', $email));
  }


  /**
   *
   *
   * @access public
   */
  public static function send( $receiver, $subject, $message )
  {
    global $rdf_system_email_str;
    global $rdf_system_brand;
  
    // email addresses
    $receiver = htmlentities($receiver, ENT_NOQUOTES, 'UTF-8');

    // header
    $headers = "";
    $headers .= "From:" .$rdf_system_email_str."\n";
    $headers .= "Reply-To:" . $rdf_system_email_str."\n"; 
    $headers .= "X-Mailer: ".$rdf_system_brand."\n"; 
    $headers .= "X-Sender-IP: ".$_SERVER['REMOTE_ADDR']."\n"; 
    $headers .= "Content-type: text/plain\n";

    // send the mail
    return @mail($receiver, $subject, $message, $headers);
  }


} // end of HTML
?>
