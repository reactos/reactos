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



function draw_award_icon($number) {

	switch ($number) {
		case "0":
		default:
			$RSDB_TEMP_return = 'questionmark';
			break;
		case "2":
			$RSDB_TEMP_return = 'fail';
			break;
		case "5":
			$RSDB_TEMP_return = 'honor';
			break;
		case "7":
			$RSDB_TEMP_return = 'bronze';
			break;
		case "8":
			$RSDB_TEMP_return = 'silver';
			break;
		case "9":
			$RSDB_TEMP_return = 'gold';
			break;
		case "10":
			$RSDB_TEMP_return = 'platinum';
			break;
	}

	return $RSDB_TEMP_return;
}

function draw_award_name($number) {

	switch ($number) {
		case "0":
		default:
			$RSDB_TEMP_return = 'Untested';
			break;
		case "2":
			$RSDB_TEMP_return = 'Known not to work';
			break;
		case "5":
			$RSDB_TEMP_return = 'Honorable Mention';
			break;
		case "7":
			$RSDB_TEMP_return = 'Bronze';
			break;
		case "8":
			$RSDB_TEMP_return = 'Silver';
			break;
		case "9":
			$RSDB_TEMP_return = 'Gold';
			break;
		case "10":
			$RSDB_TEMP_return = 'Platinum';
			break;
	}

	return $RSDB_TEMP_return;
}

?>