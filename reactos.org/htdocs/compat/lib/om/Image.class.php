<?php
    /*
    CompatDB - ReactOS Compatability Database
    Copyright (C) 2005-2006  Klemens Friedl <frik85@reactos.org>

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
 * class Star
 * 
 */
class Star
{



  /**
   * @FILLME
   *
   * @access public
   */
  private static function toGIF($img_sourse, $save_to, $quality, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromGIF($img_sourse);
		
		$im_out = imagecreatetruecolor($size[0], $size[1]);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $size[0], $size[1], $size[0], $size[1]);
		   
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImageGIF($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);() {

  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  private static function toPNG($img_sourse, $save_to, $quality, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromPNG($img_sourse);
		
		$im_out = imagecreatetruecolor($size[0], $size[1]);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $size[0], $size[1], $size[0], $size[1]);
		   
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImagePNG($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  private static function toJPG($img_sourse, $save_to, $quality, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromJPEG($img_sourse);
		
		$im_out = imagecreatetruecolor($size[0], $size[1]);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $size[0], $size[1], $size[0], $size[1]);
		   
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImageJPEG($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);
  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  private static function thumbGIF($img_sourse, $save_to, $quality, $width, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromGIF($img_sourse);
		
		$new_height = ($width * $size[1]) / $size[0]; // Generate new height for image
		$im_out = imagecreatetruecolor($width, $new_height);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $width, $new_height, $size[0], $size[1]);
		   
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImageGIF($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);

  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  private static function thumbPNG($img_sourse, $save_to, $quality, $width, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromPNG($img_sourse);
		
		$new_height = ($width * $size[1]) / $size[0]; // Generate new height for image
		$im_out = imagecreatetruecolor($width, $new_height);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $width, $new_height, $size[0], $size[1]);
		   
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImagePNG($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);

  } // end of member function icon



  /**
   * @FILLME
   *
   * @access public
   */
  private static function thumbJPG($img_sourse, $save_to, $quality, $width, $str) { 
		$size = GetImageSize($img_sourse);
		$im_in = ImageCreateFromJPEG($img_sourse);
		
		$new_height = ($width * $size[1]) / $size[0]; // Generate new height for image
		$im_out = imagecreatetruecolor($width, $new_height);
		
		ImageCopyResampled($im_out, $im_in, 0, 0, 0, 0, $width, $new_height, $size[0], $size[1]);
		
		#Find X & Y for note
		$X_var = ImageSX($im_out); 
		$X_var = $X_var - 130;
		$Y_var = ImageSY($im_out); 
		$Y_var = $Y_var - 25;
		
		#Color
		$white = ImageColorAllocate($im_out, 0, 0, 0);
		
		#Add note(simple: site address)
		ImageString($im_out,2,$X_var,$Y_var,$str,$white);
		
		ImageJPEG($im_out, $save_to, $quality); // Create image
		ImageDestroy($im_in);
		ImageDestroy($im_out);

  } // end of member function icon



} // end of Award
?>
