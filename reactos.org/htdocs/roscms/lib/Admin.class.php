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
 * class Admin
 * 
 */
abstract class Admin
{



  public function __construct()
  {
  
    // check if user has enough rights
    Login::required();
    if (ThisUser::getInstance()->securityLevel() < 3) {
      die('Not enough rights to access this Area.');
    }

    // is an action given ?
    if (empty($_GET['action'])) {
      die('missing param');
    }

    // show requested form
    if (empty($_GET['submit'])) {
      switch ($_GET['action']) {
        case 'new':
          $this->showNew();
          break;
        case 'search':
          $this->showSearch();
          break;
        case 'edit':
          $this->showEdit();
          break;
        case 'delete':
          $this->showDelete();
          break;
      }
    }

    // submit form data
    else {
      switch ($_GET['action']) {
        case 'new':
          $this->submitNew();
          break;
        case 'search':
          $this->submitSearch();
          break;
        case 'edit':
          $this->submitEdit();
          break;
        case 'delete':
          $this->submitDelete();
          break;
      }
    }
  }



  abstract protected function showNew();
  abstract protected function showSearch();
  abstract protected function showEdit();
  abstract protected function showDelete();
  
  abstract protected function submitNew();
  abstract protected function submitSearch();
  abstract protected function submitEdit();
  abstract protected function submitDelete();
} // end of Admin
?>
