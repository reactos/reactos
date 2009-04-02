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


								

$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_categories WHERE cat_visible = '1' AND cat_path = :path " . $RSDB_intern_code_db_rsdb_categories . "");
$stmt->bindParam('path',$RSDB_SET_cat,PDO::PARAM_STR);
$stmt->execute();
$result_count_cat = $stmt->fetch(PDO::FETCH_NUM);

// Update the ViewCounter:
if ($RSDB_SET_cat != "" || $RSDB_SET_cat != "0") {
  $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_categories SET cat_viewcounter = (cat_viewcounter + 1) WHERE cat_id = :cat_id");
  $stmt->bindParam('cat_id',$RSDB_SET_cat,PDO::PARAM_STR);
  $stmt->execute();
}

if ($result_count_cat[0]) {


		$RSDB_TEMP_sortby = "cat_name";

?>
	 
<table width="100%" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3"> 
		
    <td width="45%" bgcolor="#5984C3"> 
    <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Category-Tree</strong></font></div></td>
		
    <td width="45%"> 
      <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
		<td width="10%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>No.</strong></font></div></td>
  </tr>
	  <?php
	

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_visible = '1' AND cat_path = :path " . $RSDB_intern_code_db_rsdb_categories . " ORDER BY ".$RSDB_TEMP_sortby." ASC");
    $stmt->bindParam('path',$RSDB_SET_cat,PDO::PARAM_STR);
    $stmt->execute();
		
		
			$cellcolor1="#E2E2E2";
			$cellcolor2="#EEEEEE";
			$cellcolorcounter="0";
			
			include('inc/tree/tree_category_tree_count_grouplist.php');
			
		while($result_treeview = $stmt->fetch(PDO::FETCH_ASSOC)) { // TreeView
	?>
	  <tr> 
		
    <td width="45%" valign="top" bgcolor="<?php 
										echo $cellcolor1;
										$cellcolor = $cellcolor1;
								 ?>" > 
      <div align="left"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
	  
//		echo "<img src='media/icons/categories/".$result_treeview['cat_icon']."' width='16' height='16'>";
		
		echo "&nbsp;<b><a href='".$RSDB_intern_link_cat2_EX.$result_treeview['cat_id'].$RSDB_URI_slash2."&amp;cat2=".$RSDB_SET_cat2."'>".$result_treeview['cat_name']."</a></b>";
//		$RSDB_TEMP_cat_icon = $result_treeview['cat_icon'];
		$RSDB_TEMP_cat_path = $result_treeview['cat_path'];
		$RSDB_TEMP_cat_id = $result_treeview['cat_id'];
		//$RSDB_TEMP_cat_level = $result_treeview['cat_level'];
		$RSDB_TEMP_cat_level=0;
		
		$RSDB_TEMP_cat_current_id_guess=$RSDB_TEMP_cat_id;
		
		for ($guesslevel=1; ; $guesslevel++) {
//				echo $guesslevel."#";
				$stmt_cat=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' " . $RSDB_intern_code_db_rsdb_categories . "");
        $stmt_cat->bindParam('cat_id',$RSDB_TEMP_cat_current_id_guess,PDO::PARAM_STR);
        $stmt_cat->execute();
				$result_category_tree_guesslevel=$stmt_cat->fetch(PDO::FETCH_ASSOC);
//					echo $result_category_tree_guesslevel['cat_name'];
				$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
				
				if (!$result_category_tree_guesslevel['cat_name']) {
					//echo "ENDE:".($guesslevel-1);
					$RSDB_intern_catlevel = ($guesslevel-1);
					break;
				}
		}
		$RSDB_TEMP_cat_level = $RSDB_intern_catlevel;
	  
	  ?>
        </font></div></td>
		
    <td width="45%" valign="top" bgcolor="<?php echo $cellcolor; ?>"> 
      <div align="left"><font face="Arial, Helvetica, sans-serif"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_treeview['cat_description']; ?></font><font size="2"></font> 
        </font></div></td>
		
    <td width="10%" valign="top" bgcolor="<?php echo $cellcolor; ?>"><font size="2">
      <?php

		echo count_tree_grouplist($result_treeview['cat_id']);
	
	?>
      </font></td>
	  </tr>
	  <?php
	  		Category::showTree($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level, false);
	
		}	// end while
	?>
	</table>
	
<p>&nbsp;</p>
<?php
}
?>



