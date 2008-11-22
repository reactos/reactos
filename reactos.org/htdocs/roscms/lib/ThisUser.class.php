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


  /**
   * adds a new membership for this user and registers maximum security level
   *
   * @access public
   */
  public function addGroup( $group )
  {
    $this->groups[$group['name']] = $group['security_level'];
    if ($group['security_level'] > $this->security_level) $this->security_level = $group['security_level'];
  } // end of member function setId


  /**
   * adds a new membership for this user
   *
   * @return array
   * @access public
   */
  public function getGroups( )
  {
    return $this->groups;
  } // end of member function setId


  /**
   * checks if the user is member of at least in one of the groups
   *
   * @param string group_name 
   * @param string group_name2 
   * @param string group_name3 
   * @return bool
   * @access public
   */
  public function isMemberOfGroup( $group_name, $group_name2 = null, $group_name3 = null )
  {
    if (@$this->groups[$group_name] > -1 || @$this->groups[$group_name2] > -1 || @$this->groups[$group_name3] > -1) {
      return true;
    }
    return false;
  } // end of member isGroupMember


  /**
   * returns highest security level of users group memberships
   *
   * @return int
   * @access public
   */
  public function securityLevel( )
  {
    return $this->security_level;
  } // end of member function securityLevel


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
      $this->user['id'] = $user['user_id'];
      $this->user['name'] = $user['user_name'];
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
