<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>
                  2005  Klemens Friedl <frik85@reactos.org>

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
 * class Cookie
 * 
 */
class Cookie
{



  /**
   * This will return the last two components of the server name, with a leading
   * dot (i.e. usually .reactos.com or .reactos.org for us). See the PHP docs
   * on setcookie() why we need the leading dot.
   * 
   * DEPENDS on usage of www. or atleast one subdomain
   *
   * @return 
   * @access public
   */
  private static function getDomain( )
  {
    // Server name might be just an IP address
    if (preg_match('#[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}#', $_SERVER['SERVER_NAME'])) {
      return $_SERVER["SERVER_NAME"];
    }

    /* If it' a DNS address, return the domain name along with the suffix */
    if (preg_match('#(\.[^.]+\.[^.]+$)#', $_SERVER['SERVER_NAME'], $matches)) {
      return $matches[1];
    }

    // no domain was found
    return '';
  } // end of member function getCookieDomain



  /**
   * sets a new cookie
   *
   * @param string name cookie name
   * @param string value cookie value
   * @param int time expire time
   * @return bool
   * @access public
   */
  public static function write( $name, $value, $time )
  {
    return setcookie($name, $value, $time, '/', self::getDomain());
  } // end of member function write



} // end of Cookie
?>
