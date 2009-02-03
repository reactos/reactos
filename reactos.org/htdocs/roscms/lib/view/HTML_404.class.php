<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Ge van Geldorp <gvg@reactos.org>
                        Klemens Friedl <frik85@reactos.org>

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
 * class HTML_404
 * 
 * @package html
 * @subpackage default
 */
class HTML_404 extends HTML
{



  public function body( )
  {
    // output
    echo_strip('
      <h1>404 - Page not found</h1>
      <p>Our Web server cannot find the page or file you asked for.</p>
      <p>The link you followed may be broken or expired. </p>
      <p>Please use one of the following links to find the information you are looking for:</p>
      <ul>
        <li><a href="'.RosCMS::getInstance()->pathGenerated().'">'.RosCMS::getInstance()->siteName().' Website</a> </li>
        <li><a href="'.RosCMS::getInstance()->pathGenerated().'?page=sitemap">'.RosCMS::getInstance()->siteName().' Sitemap</a> 
        </li>
      </ul>');
  }


} // end of HTML_404
?>
