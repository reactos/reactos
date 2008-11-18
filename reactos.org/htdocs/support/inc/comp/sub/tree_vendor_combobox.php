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

 
	$query_page = mysql_query("SELECT * 
								FROM `rsdb_item_vendor` 
								WHERE `vendor_name` LIKE  '%' 
								ORDER BY `vendor_name` ASC ;") ;

	$zaehler="0";
	
	
	
	while($result_page = mysql_fetch_array($query_page)) { // Pages
		echo "<option value=\"". $result_page['vendor_id'] ."\"";
		if ($RSDB_intern_selected != "" && $RSDB_intern_selected == $result_page['vendor_id']) {
			echo " selected "; 
		}		
		echo ">". $result_page['vendor_name']."</option>"; 
	}	// end while
?>


