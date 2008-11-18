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


	
	function show_osversion($osvernumber) { // intern function, don't call this function directly
		$tempabc = array();
		$tempd="";
		
		$tempabc = str_split($osvernumber, 1);
		
		if ($tempabc[0] != 0) {
			$tempd .= $tempabc[0];
		}
		if ($tempabc[1] != 0) {
			$tempd .= $tempabc[1];
		}
		//if ($tempabc[2] != 0) {
			$tempd .= $tempabc[2];
		//}
		//if ($tempabc[3] != 0) {
			$tempd .= ".".$tempabc[3];
		//}
		//if ($tempabc[4] != 0) {
			$tempd .= ".".$tempabc[4];
		//}
		if ($tempabc[5] != 0) {
			$tempd .= $tempabc[5];
		}
		/*else {
			$tempd = "unkown";
		}*/
		
		return $tempd;
	}
	
?>