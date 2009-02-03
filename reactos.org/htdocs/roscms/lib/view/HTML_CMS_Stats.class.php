<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2007  Klemens Friedl <frik85@reactos.org>

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
 * class HTML_CMS_Stats
 * 
 * @package html
 * @subpackage cms
 */
class HTML_CMS_Stats extends HTML_CMS
{



  /**
   * setup branch info
   *
   * @access public
   */
  public function __construct( )
  {
    $this->branch = 'stats';

    parent::__construct();
  } // end of constructor



  protected function body( )
  {
    include('../stats/admin/view_stats.php');
  } // end of member function body


} // end of HTML_CMS_Stats
?>
