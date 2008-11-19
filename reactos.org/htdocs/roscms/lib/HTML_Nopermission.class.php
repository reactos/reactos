<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
 * class HTML_Nopermission
 * 
 */
class HTML_Nopermission extends HTML
{

  public function body( )
  {
    echo_strip('
      <div>
        <h1>No Permission</h1>
        <h2>No Permission</h2> 
        <p>You have <strong>no permission</strong> to use this part of the homepage!</p>
        <p>You <strong>need a higher account level</strong> to access this page. Please contact a member of the administrator group if you want more information.</p>
        <a href="');echo @$_SERVER['HTTP_REFERER'];echo_strip('">Back</a>
      </div>');
  }


} // end of HTML_Nopermissions
?>
