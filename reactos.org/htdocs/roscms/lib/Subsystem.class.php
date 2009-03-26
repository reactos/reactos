<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2009  Danny Gštte <dangerground@web.de>

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
 * @package subsystems
 */
class Subsystem extends Login
{



  /**
   * returns the roscms username
   *
   * @param int id user_id
   * @return string
   * @access public
   */
  public static function getUserName( $user_id )
  {
    $stmt=&DBConnection::getInstance()->prepare("SELECT name FROM ".ROSCMST_USERS." WHERE id = :user_id");
    $stmt->bindParam('user_id',$user_id,PDO::PARAM_STR);
    $stmt->execute();
    return $stmt->fetchColumn();
  } // end of member function getUserName



} // end of Subsystem
?>
