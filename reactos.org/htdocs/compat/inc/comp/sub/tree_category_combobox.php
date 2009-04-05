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


								

$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_categories WHERE cat_visible = '1' AND cat_path = '0' AND cat_comp = '1'");
$stmt->execute();
$result_count_cat = $stmt->fetchOnce(PDO::FETCH_ASSOC);

// Update the ViewCounter:
if ($RSDB_SET_cat != "" || $RSDB_SET_cat != "0") {
  $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_categories SET cat_viewcounter = (cat_viewcounter + 1) WHERE cat_id = :cat_id");
  $stmt->bindParam('cat_id',$RSDB_SET_cat,PDO::PARAM_STR);
  $stmt->execute();
}

if ($result_count_cat[0]) {


		$RSDB_TEMP_sortby = "cat_name";


	

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_visible = '1' AND cat_path = :cat_path AND cat_comp = '1' ORDER BY cat_name ASC");
    $stmt->bindParam('cat_path',$RSDB_SET_cat,PDO::PARAM_STR);
    $stmt->execute();
		
		
			$cellcolor1="#E2E2E2";
			$cellcolor2="#EEEEEE";
			$cellcolorcounter="0";
			
			include('inc/tree/tree_category_tree_count_grouplist.php');
			
		while($result_treeview = $stmt->fetch(PDO::FETCH_ASSOC)) { // TreeView
			echo "<option value=\"". $result_treeview['cat_id']. "\"";
			if ($RSDB_intern_selected != "" && $RSDB_intern_selected == $result_treeview['cat_id']) {
				echo " selected "; 
			}		
			echo ">+ ". $result_treeview['cat_name'] ."</option>\n\n";
	
			$RSDB_TEMP_cat_path = $result_treeview['cat_path'];
			$RSDB_TEMP_cat_id = $result_treeview['cat_id'];
			$RSDB_TEMP_cat_level=0;
			
			$RSDB_TEMP_cat_current_id_guess=$RSDB_TEMP_cat_id;
	
			Category::showTree($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level, true);
	
		}	// end while
}
?>


