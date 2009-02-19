<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2009  Danny Götte <dangerground@web.de>

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
 * class Backend_UserDetails
 *
 * @package Branch_User
 */
class Backend_UserDetails extends Backend
{


  /**
   * checks if the user needs to be updated
   * checks what has to be displayed
   *
   * @access public
   */
  public function __construct( )
  {
    parent::__construct();

    // some userdata has to be updated ?
    if (isset($_GET['action'])) {
      switch ($_GET['action']) {
        case 'addmembership':
          ROSUser::addMembership($_GET['user'], $_GET['group']);
          break;
        case 'delmembership':
          ROSUser::deleteMembership($_GET['user'], $_GET['group']);
          break;
        case 'accountdisable':
          ROSUser::disableAccount($_GET['user']);
          break;
        case 'accountenable':
          ROSUser::disableAccount($_GET['user']);
          break;
        case 'upateusrlang':
          ROSUser::changeLanguage($_GET['user'], $_GET['lang']);
          break;
      }

      // needs $_GET['user']
      new Backend_ViewUserDetails();
    }

  } // end of constructor



} // end of Backend_User
?>
