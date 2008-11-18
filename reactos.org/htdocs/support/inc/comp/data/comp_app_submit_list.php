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


	$query_count_groups=mysql_query("SELECT COUNT('grpentr_id')
							FROM `rsdb_groups`
							WHERE `grpentr_visible` = '1'
							AND `grpentr_name` LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%' ;");	
	$result_count_groups = mysql_fetch_row($query_count_groups);

header( 'Content-type: text/xml' );
echo '<?xml version="1.0" encoding="UTF-8"?>
<root>
';

	if (!$result_count_groups[0]) {
		echo "    #none#\n";
	}
	else {
		echo "    ".$result_count_groups[0]."\n";
	}

	$query_page = mysql_query("SELECT * 
								FROM `rsdb_groups` 
								WHERE `grpentr_visible` = '1'
								AND `grpentr_name` LIKE '%" . mysql_real_escape_string($RSDB_SET_search) . "%' 
								ORDER BY `grpentr_name` ASC ;") ;
	
	while($result_page = mysql_fetch_array($query_page)) { // Pages
?>
	<dbentry>
		<item id="<?php echo $result_page['grpentr_id']; ?>"><?php echo $result_page['grpentr_name']; ?></item>
		<desc><?php
				if (!strlen($result_page['grpentr_description']) == "0") {
					if (strlen(htmlentities($result_page['grpentr_description'], ENT_QUOTES)) <= 30) {
						echo $result_page['grpentr_description'];
					}
					else {
						echo substr(htmlentities($result_page['grpentr_description'], ENT_QUOTES), 0, 30)."...";
					}
				}
				else {
					echo ".";
				}
		  ?></desc>
		<vendor id="<?php echo $result_page['grpentr_vendor']; ?>"><?php
				if ($result_page['grpentr_vendor'] == "0") {
					echo ".";
				}
				else {
					$query_entry_vendor = mysql_query("SELECT * 
														FROM `rsdb_item_vendor` 
														WHERE `vendor_id` = " .  $result_page['grpentr_vendor'] ." ;") ;
					$result_entry_vendor = mysql_fetch_array($query_entry_vendor);
					echo $result_entry_vendor['vendor_name'];
				}
		  ?> </vendor>
		<comp><?php echo $result_page['grpentr_comp']; ?></comp>
		<devnet><?php echo $result_page['grpentr_devnet']; ?></devnet>
	</dbentry>
<?
	}
?>
</root>