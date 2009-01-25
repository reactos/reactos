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
 * class Presentation
 * 
 */
class Presentation
{



  /**
   * 
   *
   * @param string text
   * @return 
   * @access private
   */
  public static function verticalText( $text )
  {
    $font = 2;
  
    // initial values
    $width = 20;
    $height = strlen($text)*imagefontwidth($font);

    $image = @imagecreatetruecolor($width, $height+5) or die('Cannot initialize new GD image stream');

    // background
    $bgcolor = imagecolorallocate($image, 255, 255, 255);
    imagefill($image, 0, 0, $bgcolor);

    // Set text
    $textcolor = imagecolorallocate($image, 0, 0, 0);
    imagestringup($image, $font, 0, $height-1, $text, $textcolor);

    // output captcha image to browser
    header('Content-Type: image/png');
    header('Cache-control: no-cache, no-store');
    imagepng($image);
    imagedestroy($image);
  }

} // end of Presentation
?>
