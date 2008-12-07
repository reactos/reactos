<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005-2008  Klemens Friedl <frik85@reactos.org>

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
 * class Date
 * 
 */
class Date
{


  /**
   * calculates date the user has set to
   * $date_org = "2008-03-23 10:03:42"; 
   * + user_timezone = -8;
   * + server_timezone = -1;
   * = "2008-03-23 01:03:42"
   *
   * @param string date if no datetime is set, current datetime is used

   * @return 
   * @access public
   */
  public static function getLocal( $date = null, $format = 'Y-m-d H:i' )
  {
    // calculate only for registered users
    if (ThisUser::getInstance()->id() > 0) {
      $basedate = strtotime($date);
      $date_new = strtotime((RosCMS::getInstance->siteTimeZone()).' hours', $basedate);
      return date($format, $date_new).' '.'UTC';
    }
    // guest visitors get UTC time
    elseif ($date != null) {

      // convert to timestamp first
      return date($format, strtotime($date));
    }
    else {
      return date($format);
    }
  } // end of member function getLocal


} // end of Date
?>
