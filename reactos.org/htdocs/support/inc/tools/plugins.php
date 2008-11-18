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



	// Exif reader v 1.2
    // By Richard James Kendall 
		require_once("inc/tools/plugins/exif.php");
	
		function read_exif($RSDB_TEMP_filename, $spacechar) { 
			global $exif_data;
			
			if ($spacechar == 1) {
				$spacechar = "\n";
			}
			else {
				$spacechar = "<br />";
			}
			
			exif($RSDB_TEMP_filename);
			while(list($key, $val) = each($exif_data)) {
				if ($key == "Flash") {
					echo 'Flash:';
					while(list($key, $val) = each($exif_data['Flash'])) {
						if ($key != 0) {
							echo ' ' . $val. ',';
						}
					}
					echo $spacechar;
				}
				else {
					echo $key . ": " . $val . $spacechar;
				}
			}
		}


		function read_exif_ex($RSDB_TEMP_filename, $spacechar) { 
			global $exif_data;
			
			$tinfo="";
			
			if ($spacechar == 1) {
				$spacechar = "\n";
			}
			else {
				$spacechar = "<br />";
			}
			
			exif($RSDB_TEMP_filename);
			while(list($key, $val) = each($exif_data)) {
				if ($key == "Flash") {
					$tinfo .= 'Flash:';
					while(list($key, $val) = each($exif_data['Flash'])) {
						if ($key != 0) {
							$tinfo .= ' ' . $val. ',';
						}
					}
					$tinfo .= $spacechar;
				}
				else {
					$tinfo .= $key . ": " . $val . $spacechar;
				}
			}
			return $tinfo;
		}

?>