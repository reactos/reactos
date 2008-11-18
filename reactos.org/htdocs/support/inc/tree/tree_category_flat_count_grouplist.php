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



	function count_tree_grouplist ($RSDB_TEMP_cat_id_grouplista) {
	
		global $RSDB_SET_cat;
		global $RSDB_intern_link_category_cat;
		global $RSDB_TEMP_cat_id_grouplist;
		global $RSDB_TEMP_counter_group;
		global $RSDB_TEMP_counter_group_sum;
		global $RSDB_intern_code_db_rsdb_categories;

		
		if ($RSDB_TEMP_cat_id_grouplista) {
			$RSDB_TEMP_cat_id_grouplist = $RSDB_TEMP_cat_id_grouplista;
		}
		
		$RSDB_VAR_counter_tree_grouplist = 0;
	
		$query_count_groups=mysql_query("SELECT COUNT('cat_id')
								FROM `rsdb_categories`
								WHERE `cat_visible` = '1'
								AND `cat_path` = " . $RSDB_SET_cat . "
								" . $RSDB_intern_code_db_rsdb_categories . " ;");	
		$result_count_groups = mysql_fetch_row($query_count_groups);
		
		$RSDB_TEMP_counter_group=0;
		count_group_and_category($RSDB_TEMP_cat_id_grouplista);

		if ($result_count_groups[0]) {
			
			$query_treeview_count_groups = mysql_query("SELECT * 
										FROM `rsdb_categories` 
										WHERE `cat_visible` = '1'
										AND `cat_path` = " . $RSDB_TEMP_cat_id_grouplist . "
										" . $RSDB_intern_code_db_rsdb_categories . "
										ORDER BY `cat_name` ASC") ;
			
			
				$cellcolor1="#E2E2E2";
				$cellcolor2="#EEEEEE";
				$cellcolorcounter="0";
				
				
			while($result_treeview_count_groups = mysql_fetch_array($query_treeview_count_groups)) { // treeview_count_groups
		  
				
//				echo "<br><a href='".$RSDB_intern_link_category_cat.$result_treeview_count_groups['cat_id']."'>".$result_treeview_count_groups['cat_name']."</a>";
				$RSDB_TEMP_cat_path = $result_treeview_count_groups['cat_path'];
				$RSDB_TEMP_cat_id = $result_treeview_count_groups['cat_id'];
				//$RSDB_TEMP_cat_level = $result_treeview_count_groups['cat_level'];
				$RSDB_TEMP_cat_level=0;
		
				count_group_and_category($result_treeview_count_groups['cat_id']);
				
				create_counter_groups($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level);
		
			}	// end while
			
//			echo $RSDB_TEMP_counter_group_sum."|";
			return count_group_and_category_equal();
		}
	}

	function count_group_and_category($RSDB_TEMP_cat_id_group) {
	
		global $RSDB_TEMP_counter_group;
		global $RSDB_intern_code_db_rsdb_groups;

		$query_count_group_and_category=mysql_query("SELECT COUNT('grpentr_id')
						FROM `rsdb_groups`
						WHERE `grpentr_visible` = '1'
						AND `grpentr_category` = " . $RSDB_TEMP_cat_id_group . "
						" . $RSDB_intern_code_db_rsdb_groups . " ;");	
		$result_count_group_and_category = mysql_fetch_row($query_count_group_and_category);
//		echo "->".$result_count_group_and_category[0]."<-";
		
		if ($result_count_group_and_category[0]) {
			$RSDB_TEMP_counter_group = $RSDB_TEMP_counter_group + $result_count_group_and_category[0];
		}
	}

	function count_group_and_category_equal() {
		global $RSDB_TEMP_counter_group;
		global $RSDB_TEMP_counter_group_sum;
		$RSDB_TEMP_counter_group_sum = 0;
		$RSDB_TEMP_counter_group_sum = $RSDB_TEMP_counter_group;
		$RSDB_TEMP_counter_group = 0;
		return $RSDB_TEMP_counter_group_sum;
	}

		
	function create_counter_groups($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain) {
		
		global $RSDB_intern_link_category_cat;
		global $RSDB_TEMP_counter_group;
		global $RSDB_intern_code_db_rsdb_categories;


		$query_create_historybar=mysql_query("SELECT * 
								FROM `rsdb_categories` 
								WHERE `cat_path` = " . $RSDB_TEMP_cat_id ."
								" . $RSDB_intern_code_db_rsdb_categories . "
								AND `cat_visible` = '1' ;");
					
		while($result_create_historybar=mysql_fetch_array($query_create_historybar)) { 
			count_tree_groups_entry($result_create_historybar['cat_id'], $RSDB_TEMP_cat_level_newmain);
			create_counter_groups($result_create_historybar['cat_path'], $result_create_historybar['cat_id'], 0, $RSDB_TEMP_cat_level_newmain);
		}
	}


	function count_tree_groups_entry($RSDB_TEMP_entry_id, $RSDB_TEMP_cat_level_newmain) {
		
		global $RSDB_intern_link_category_cat;
		global $RSDB_TEMP_counter_group;
		global $RSDB_intern_code_db_rsdb_categories;
		

		
		$query_count_tree_groups_entry=mysql_query("SELECT * 
												FROM `rsdb_categories` 
												WHERE `cat_id` = " . $RSDB_TEMP_entry_id ."
												AND `cat_visible` = '1'
												" . $RSDB_intern_code_db_rsdb_categories . " ;");
					
		$result_count_tree_groups_entry=mysql_fetch_array($query_count_tree_groups_entry);

//		echo "<a href='".$RSDB_intern_link_category_cat.$result_count_tree_groups_entry['cat_id']."'>".$result_count_tree_groups_entry['cat_name']."</a>";

		count_group_and_category($result_count_tree_groups_entry['cat_id']);
		
	}
?>




