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
 * should be called using getInstance() to refer every time to the same user, preferable the current user
 * - holds user id, name
 * - can be used to ask for access to specific areas of the CMS
 * 
 */
class ThisUser
{

  // standard user values
  private $user = array(
    'id'=>false,
    'name'=>false,
    'language'=>false,
    'timezone'=>false);
  private $access = array();



  /**
   * adds a access area to the allowed list of this user
   *
   * @access public
   */
  public function addAccess( $access_area )
  {
    $this->access[$access_area] = true;
  } // end of member function addAccess



  /**
   * checks if the user has access to a requested area
   *
   * @param string access_area name of the area
   * @return bool
   * @access public
   */
  public function hasAccess( $access_area )
  {
    if (isset($access_area) && isset($this->access[$access_area])) {
      return $this->access[$access_area];
    }
    return false;
  } // end of member hasAccess



  /**
   * returns the id of current user
   * be sure to have setData called before
   *
   * @return int
   * @access public
   */
  public function id( )
  {
    return (int)$this->user['id'];
  } // end of member function id



  /**
   * returns the name of current user
   * be sure to have setData called before
   *
   * @return string
   * @access public
   */
  public function name( )
  {
    return $this->user['name'];
  } // end of member function name



  /**
   * returns the language of current user
   * be sure to have setData called before
   *
   * @return int
   * @access public
   */
  public function language( )
  {
    return (int)$this->user['language'];
  } // end of member function language



  /**
   * returns the timezone of current user
   * be sure to have setData called before
   *
   * @return int
   * @access public
   */
  public function timezone( )
  {
    return (int)$this->user['timezone'];
  } // end of member function timezone



  /**
   * set the current user data
   *
   * @param mixed[] user
   * @access public
   */
  public function setData( $user )
  {
    if($user !== false){
      $this->user['id'] = $user['id'];
      $this->user['name'] = $user['name'];
      $this->user['language'] = $user['lang_id'];
      $this->user['timezone'] = $user['timezone_id'];
    }
  } // end of member function setData



  /**
   * returns the instance
   *
   * @return object
   * @access public
   */
  public static function getInstance( )
  {
    static $instance;

    // check if we already have an instance
    if (empty($instance)) {
      $instance = new ThisUser();
    }

    return $instance;
  } // end of member function getInstance



} // end of ThisUser
?>
