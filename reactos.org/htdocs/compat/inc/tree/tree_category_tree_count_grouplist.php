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
	
			global $RSDB_intern_link_category_cat;
			global $RSDB_TEMP_cat_id_grouplist;
			global $RSDB_TEMP_counter_group;
			global $RSDB_TEMP_counter_group_sum;
	
			
		$RSDB_TEMP_counter_group=0;
		count_group_and_category($RSDB_TEMP_cat_id_grouplista);
	
		return count_group_and_category_equal();
	}

	function count_group_and_category($RSDB_TEMP_cat_id_group) {
	
		global $RSDB_TEMP_counter_group;

		$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_category = :category AND grpentr_comp = '1'");
    $stmt->bindParam('category',$RSDB_TEMP_cat_id_group,PDO::PARAM_STR);
    $stmt->execute();
		$result_count_group_and_category = $stmt->fetch(PDO::FETCH_NUM);
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

		
?>
