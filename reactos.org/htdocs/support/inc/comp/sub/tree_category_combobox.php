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


								

$query_count_cat=mysql_query("SELECT COUNT('cat_id')
						FROM `rsdb_categories`
						WHERE `cat_visible` = '1'
						AND `cat_path` = '0'
						" . $RSDB_intern_code_db_rsdb_categories . " ;");	
$result_count_cat = mysql_fetch_row($query_count_cat);

// Update the ViewCounter:
if ($RSDB_SET_cat != "" || $RSDB_SET_cat != "0") {
	$query_update_viewcounter = "UPDATE `rsdb_categories` SET `cat_viewcounter` = (cat_viewcounter + 1) WHERE `cat_id` = '" . $RSDB_SET_cat . "' LIMIT 1 ;";
	@mysql_query($query_update_viewcounter);
}

if ($result_count_cat[0]) {


	if ($RSDB_intern_code_view_shortname == "devnet") {
		$RSDB_TEMP_sortby = "cat_order";
	}
	else {
		$RSDB_TEMP_sortby = "cat_name";
	}


	

		$query_treeview = mysql_query("SELECT * 
									FROM `rsdb_categories` 
									WHERE `cat_visible` = '1'
									AND `cat_path` = " . $RSDB_SET_cat . "
									" . $RSDB_intern_code_db_rsdb_categories . "
									ORDER BY `".$RSDB_TEMP_sortby."` ASC") ;
		
		
			$cellcolor1="#E2E2E2";
			$cellcolor2="#EEEEEE";
			$cellcolorcounter="0";
			
			include('inc/tree/tree_category_tree_count_grouplist.php');
			
		while($result_treeview = mysql_fetch_array($query_treeview)) { // TreeView
			echo "<option value=\"". $result_treeview['cat_id']. "\"";
			if ($RSDB_intern_selected != "" && $RSDB_intern_selected == $result_treeview['cat_id']) {
				echo " selected "; 
			}		
			echo ">+ ". $result_treeview['cat_name'] ."</option>\n\n";
	
			$RSDB_TEMP_cat_path = $result_treeview['cat_path'];
			$RSDB_TEMP_cat_id = $result_treeview['cat_id'];
			$RSDB_TEMP_cat_level=0;
			
			$RSDB_TEMP_cat_current_id_guess=$RSDB_TEMP_cat_id;
	
			create_treeview($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level);
	
		}	// end while
}
?>

<?php

		
	function create_treeview($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain) {
		global $RSDB_intern_link_category_cat;
		global $RSDB_intern_code_db_rsdb_categories;
		global $RSDB_TEMP_sortby;

		$query_create_historybar=mysql_query("SELECT * 
								FROM `rsdb_categories` 
								WHERE `cat_path` = " . $RSDB_TEMP_cat_id ."
								AND `cat_visible` = '1' 
								" . $RSDB_intern_code_db_rsdb_categories . "
								ORDER BY `".$RSDB_TEMP_sortby."` ASC ;");
					
		while($result_create_historybar=mysql_fetch_array($query_create_historybar)) { 
				create_tree_entry($result_create_historybar['cat_id'], $RSDB_TEMP_cat_level_newmain);
				create_treeview($result_create_historybar['cat_path'], $result_create_historybar['cat_id'], $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain);
		}
	}
?>

<?php
	function create_tree_entry($RSDB_TEMP_entry_id, $RSDB_TEMP_cat_level_newmain) {
	global $RSDB_intern_selected;
		
		global $RSDB_intern_link_category_cat;
		global $cellcolor2;
		global $RSDB_intern_code_db_rsdb_categories;
		$cellcolor=$cellcolor2;
		

		
		$query_create_tree_entry=mysql_query("SELECT * 
												FROM `rsdb_categories` 
												WHERE `cat_id` = " . $RSDB_TEMP_entry_id ."
												AND `cat_visible` = '1'
												" . $RSDB_intern_code_db_rsdb_categories . " ;");
					
		$result_create_tree_entry=mysql_fetch_array($query_create_tree_entry);

		
		
		
		$RSDB_TEMP_cat_current_id_guess = $result_create_tree_entry['cat_id'];

		for ($guesslevel=1; ; $guesslevel++) {
				$query_category_tree_guesslevel= mysql_query("SELECT * 
															FROM `rsdb_categories` 
															WHERE `cat_id` = " . $RSDB_TEMP_cat_current_id_guess ."
															AND `cat_visible` = '1'
															" . $RSDB_intern_code_db_rsdb_categories . " ;");
				$result_category_tree_guesslevel=mysql_fetch_array($query_category_tree_guesslevel);
				$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
				
				if (!$result_category_tree_guesslevel['cat_name']) {
					$RSDB_intern_catlevel = ($guesslevel-1);
					break;
				}
		}

		echo "<option value=\"". $result_create_tree_entry['cat_id']. "\"";
		if ($RSDB_intern_selected != "" && $RSDB_intern_selected == $result_create_tree_entry['cat_id']) {
			echo " selected "; 
		}		
		echo ">\n\n";

		for ($n=$RSDB_TEMP_cat_level_newmain;$n<$RSDB_intern_catlevel;$n++) {
			echo "&nbsp;&nbsp;&nbsp;&nbsp;";
		}

		echo $result_create_tree_entry['cat_name'];

		//echo "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(".$result_create_tree_entry['cat_description'] .")";
		
		echo "</option>";
		
	}
?>


