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
 */
class HTML_404 extends HTML
{

  public function body( )
  {
    global $roscms_langres;
    global $roscms_intern_webserver_pages, $roscms_page_url;

    // output
    echo_strip('
      <h1>404 - '.$roscms_langres['Page_not_found'].'</h1>
      <h2>'.$roscms_langres['Page_not_found'].'</h2>
      <p>Our Web server cannot find the page or file you asked for.</p>
      <p>The link you followed may be broken or expired. </p>
      <p>Please use one of the following links to find the information you are looking for:</p>
      <ul>
        <li><a href="'.$roscms_intern_webserver_pages.'">'.$roscms_page_url.' Website</a> </li>
        <li><a href="'.$roscms_intern_webserver_pages.'?page=sitemap">'.$roscms_page_url.' Sitemap</a> 
        </li>
      </ul>');
  }


} // end of HTML_404
?>
