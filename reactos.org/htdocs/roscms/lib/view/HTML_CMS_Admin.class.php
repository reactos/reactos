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
 * class HTML_CMS_Admin
 * 
 * @package html
 * @subpackage cms
 */
class HTML_CMS_Admin extends HTML_CMS
{


  /**
   * register css/js and check APL
   *
   * @access public
   */
  public function __construct( )
  {
    Login::required();

    $this->branch = 'admin';
    $this->register_js('cms_admin.js');
    $this->register_css('cms_admin.css');

    if (!ThisUser::getInstance()->hasAccess('admin')) {
      die('Not enough rights to get into this area');
    }

    parent::__construct();
  } // end of constructor



  protected function body( )
  {
    echo_strip('
      <ul>
        <li>Entry Access Control Lists
          <ul>
            <li><a href="#" onclick="'."showNew('acl')".'">new</a></li>
            <li><a href="#" onclick="'."showSearch('acl','edit')".'">edit</a></li>
            <li><a href="#" onclick="'."showSearch('acl','delete')".'">delete</a></li>
          </ul>
        </li>
        <li>Groups
          <ul>
            <li><a href="#" onclick="'."showNew('group')".'">new</a></li>
            <li><a href="#" onclick="'."showSearch('group','edit')".'">edit</a></li>
            <li><a href="#" onclick="'."showSearch('group','delete')".'">delete</a></li>
          </ul>
        </li>
        <li>Languages
          <ul>
            <li><a href="#" onclick="'."showNew('lang')".'">new</a></li>
            <li><a href="#" onclick="'."showSearch('lang','edit')".'">edit</a></li>
          </ul>
        </li>
        <li>System Settings
          <ul>
            <li><a href="#" onclick="'."showAreaProtection()".'">Area Protection</a></li>
          </ul>
        </li>
      </ul>
      <div id="adminarea" style="border: 1px dashed red;">
      </div>');
  } // end of member function body


} // end of HTML_CMS_Admin
?>
