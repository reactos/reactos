<?php
    /*
    RSDB - ReactOS Support Database
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

/*
 *	ReactOS Support Database System - RSDB
 *	
 *	(c) by Klemens Friedl <frik85>
 *	
 *	2005 - 2006 
 */


	// To prevent hacking activity:
	if ( !defined('RSDB') )
	{
		die(" ");
	}




//error_reporting(E_ALL);
//ini_set('error_reporting', E_ALL);

	$file="";
	$file_name="";
	$file_type="";
	$file_size="";
	$file_error="";
	
	if (array_key_exists("file", $_FILES)) $file=$_FILES['file']['tmp_name'];
	if (array_key_exists("file", $_FILES)) $file_name=$_FILES['file']['name'];
	if (array_key_exists("file", $_FILES)) $file_type=$_FILES['file']['type'];
	if (array_key_exists("file", $_FILES)) $file_size=$_FILES['file']['size'];
	if (array_key_exists("file", $_FILES)) $file_error=$_FILES['file']['error'];
	
	$suoka="0";
	$infoExif="";
	
	
	if(!empty($file_name) || $file_error){
	
		$suoka="0";
		
		// Unique-Filename:
		$Utime = time();
		$Udate = getdate($Utime);
		$Ufile= date("Y-m-d") ."_".$Udate['seconds'];
		$file_name=$Ufile."_".$RSDB_intern_user_id."_".rawurlencode(htmlspecialchars($file_name));
		
		if(copy($file,"media/files/upload/$file_name")){   // File
			if(is_uploaded_file($file)){
				
				$image_info = getimagesize("media/files/upload/$file_name") ; // see EXIF for faster way
				
				switch ($image_info['mime']) {
					case 'image/gif':
						if (imagetypes() & IMG_GIF)  { // not the same as IMAGETYPE
							$picformat="gif";
						}
						else {
							$suoka="1";
						}
						break;
					case 'image/jpeg':
						if (imagetypes() & IMG_JPG)  {
							$picformat="jpg";
						}
						else {
							$suoka="1";
						}
						break;
					case 'image/png':
						if (imagetypes() & IMG_PNG)  {
							$picformat="png";
						}
						else {
							$suoka="1";
						}
						break;
					default:
						$suoka="1";
				}
				
				//echo "<p>mime-type: ".$image_info['mime']." resolution: ".$image_info[1]."x".$image_info[0]."</p>";
	
				if ($suoka == "0") {
					$picture="";
					$picture="media/files/upload/$file_name";
					echo "<font color='#000000'>";
					echo "<b>File upload successful!</b><br>";
					echo "Picturename: $file_name <br>";
					echo "Resolution: ".$image_info[1]."x".$image_info[0]."<br>";
					echo "Filetype: $picformat (".$image_info['mime']."/$file_type) <br>";
					echo "Filesize: $file_size Byte <br>";
					echo "</font>";
					if ($file_size >= "250000") {  // max. 250 KB
						@unlink("media/files/upload/$file_name");
						echo "<br />File size is too big! Picture size have to be < 250 KB (= 250000 Byte)!<br><br>File has been deleted!!!"; 
						$suoka="1";
					}
					else {
						$file_name_new = $file_name;
						$file_name_new = str_replace(".jpg","",$file_name_new);
						$file_name_new = str_replace(".jpeg","",$file_name_new);
						$file_name_new = str_replace(".png","",$file_name_new);
						$file_name_new = str_replace(".gif","",$file_name_new);
						switch ($picformat) {
							case 'gif':
								$Tdbfiletb = $file_name_new."_thumb.gif";
								$Tdbfile = $file_name_new.".gif";
								makeGifThumb("media/files/upload/$file_name","media/files/picture/".$Tdbfiletb,"90","250","www.ReactOS.org");
								makeGif("media/files/upload/$file_name","media/files/picture/".$Tdbfile,"90","www.ReactOS.org");
								break;
							case 'jpg':
								$Tdbfiletb = $file_name_new."_thumb.jpg";
								$Tdbfile = $file_name_new.".jpg";
								makeJpgThumb("media/files/upload/$file_name","media/files/picture/".$Tdbfiletb,"90","250","www.ReactOS.org");
								makeJpg("media/files/upload/$file_name","media/files/picture/".$Tdbfile,"90","www.ReactOS.org");
								$infoExif = read_exif_ex("media/files/upload/$file_name", "1");
								break;
							case 'png':
								$Tdbfiletb = $file_name_new."_thumb.png";
								$Tdbfile = $file_name_new.".png";
								makePngThumb("media/files/upload/$file_name","media/files/picture/".$Tdbfiletb,"90","250","www.ReactOS.org");
								makePng("media/files/upload/$file_name","media/files/picture/".$Tdbfile,"90","www.ReactOS.org");
								break;
							default:
								$suoka="1";
						}
						@unlink("media/files/upload/$file_name");
						$RSDB_TEMP_SUBMIT_valid = "yes";
					}
				}	
				else {
					@unlink("media/files/upload/$file_name");
					echo "<br>The filetype (file): $file_type isn't allowed! Please use only JPG, PNG and GIF files!<br><br>File has been deleted!!!";
					$suoka="1";
				}
			}	
		}
		else {
				$suoka="1";
		}	
	}
	

if ($suoka == "1") {
	echo "<p><b>The picture upload process was not successful!</b></p>";
	@unlink("media/files/upload/$file_name");
}

	function makeJpgThumb($img_sourse, $save_to, $quality, $width, $str) { 
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
	}
	
	function makePngThumb($img_sourse, $save_to, $quality, $width, $str) { 
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
	}
	
	function makeGifThumb($img_sourse, $save_to, $quality, $width, $str) { 
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
	}

	function makeJpg($img_sourse, $save_to, $quality, $str) { 
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
	}
	
	function makePng($img_sourse, $save_to, $quality, $str) { 
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
	}
	
	function makeGif($img_sourse, $save_to, $quality, $str) { 
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
		ImageDestroy($im_out);
	}

?>
