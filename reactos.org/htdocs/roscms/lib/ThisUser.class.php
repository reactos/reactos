<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2008  Danny Götte <dangerground@web.de>

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
 * class ThisUser
 * 
 */
class ThisUser
{
  private $security_level = 0;
  private $user = array('id'=>0,'name'=>'');
  private $groups = array();
  private $access = array();


  /**
   * adds a access area to the allowed list of this user
   *
   * @access public
   */
  public function addAccess( $access_area )
  {
    $this->access[$access_area] = true;
  } // end of member function setId



  /**
   * checks if the user has access to a requested area
   *
   * @param string access_area name of the area
   * @return bool
   * @access public
   */
  public function hasAccess( $access_area )
  {
    if (isset($this->access[$access_area])) {
      return $this->access[$access_area];
    }
    return false;
  } // end of member isGroupMember



  /**
   * returns the id of the user, which has requested the script
   *
   * @return int
   * @access public
   */
  public function id( )
  {
    return $this->user['id'];
  } // end of member function securityLevel


  /**
   * returns the name of the user, which has requested the script
   *
   * @return string
   * @access public
   */
  public function name( )
  {
    return $this->user['name'];
  } // end of member function securityLevel


  /**
   * set the current user data, of the user which has requested the script
   *
   * @access public
   */
  public function setData( $user )
  {
    if($user !== false){
      $this->user['id'] = $user['id'];
      $this->user['name'] = $user['name'];
    }
  } // end of member function setId


  /**
   * returns the instance
   *
   * @return object
   * @access public
   */
  public static function getInstance( )
  {
    static $instance;
    
    if (empty($instance)) {
      $instance = new ThisUser();
    }

    return $instance;
  } // end of member function check_lang

} // end of ThisUser
?>
