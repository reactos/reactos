<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007      Klemens Friedl <frik85@reactos.org>
                  2008-2009 Danny Götte <dangerground@web.de>

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
 * class CMSWebsitePreview
 * 
 */
class CMSWebsitePreview
{


  /**
   *
   * @access public
   */
  public function __construct( )
  {
    Login::required();

    $this->show($_GET['rev_id']);
  }


  /**
   *
   *
   * @access private
   */
  private function show( $rev_id )
  {  
    echo_strip('
      <iframe style="width:100%;height:600px;background-color: white;border: 1px solid black;" src="?page=data_out&amp;d_f=page&amp;d_val='.$rev_id.'"></iframe>');
  } // end of member function show

} // end of CMSWebsitePreview
?>
