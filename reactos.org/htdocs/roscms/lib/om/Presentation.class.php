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
   * prints text rotated by 90°
   *
   * @param string text
   * @access public
   */
  public static function verticalText( $text, $bgcolor = 'FFFFFF', $textc = '000000' )
  {
    // select font (1-5 as possible values, where 1 is smallest)
    $font = 2;
  
    // initial values
    $width = 20;
    $height = strlen($text)*imagefontwidth($font);

    // create new image
    $image = @imagecreatetruecolor($width, $height+5) or die('Cannot initialize new GD image stream');

    // deconstruct bg color
    $bg_r = hexdec(substr($bgcolor,0,2));
    $bg_g = hexdec(substr($bgcolor,2,2));
    $bg_b = hexdec(substr($bgcolor,4,2));

    // background color
    $bgcolor = imagecolorallocate($image, $bg_r, $bg_g, $bg_b);
    //$bgcolor = imagecolorallocate($image, 255, 255, 255);
    imagefill($image, 0, 0, $bgcolor);

    // deconstruct text color
    $text_r = hexdec(substr($textc,0,2));
    $text_g = hexdec(substr($textc,2,2));
    $text_b = hexdec(substr($textc,4,2));

    // Set text color
    $textcolor = imagecolorallocate($image, $text_r, $text_g, $text_b);
    //$textcolor = imagecolorallocate($image, 0, 0, 0);
    imagestringup($image, $font, 0, $height-1, $text, $textcolor);

    // output captcha image to browser
    header('Content-Type: image/png');
    header('Cache-control: no-cache, no-store');
    imagepng($image);
    imagedestroy($image);
  } // end of member function verticalText

} // end of Presentation
?>
