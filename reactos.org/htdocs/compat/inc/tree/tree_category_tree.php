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


	if ($RSDB_intern_code_view_shortname == "devnet") {
		$RSDB_TEMP_sortby = "cat_order";
	}
	else {
		$RSDB_TEMP_sortby = "cat_name";
	}

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
	  		create_treeview($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level);
	
		}	// end while
	?>
	</table>
	
<p>&nbsp;</p>
<?php
}
?>

<?php

		
	function create_treeview($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain) {
		//echo "&nbsp;&nbsp; path: ".$RSDB_TEMP_cat_path." | id: ".$RSDB_TEMP_cat_id." | level: ".$RSDB_TEMP_cat_level."<br>";
		
		global $RSDB_intern_link_category_cat;
		global $RSDB_intern_code_db_rsdb_categories;
		global $RSDB_TEMP_sortby;

    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_path = :path AND cat_visible = '1' " . $RSDB_intern_code_db_rsdb_categories . " ORDER BY ".$RSDB_TEMP_sortby." ASC");
    $stmt->bindParam('path',$RSDB_TEMP_cat_id,PDO::PARAM_STR);
    $stmt->execute();
					
		//$result_create_historybar=mysql_fetch_array($query_create_historybar);
		while($result_create_historybar=$stmt->fetch(PDO::FETCH_ASSOC)) { 
			//echo "&nbsp;&nbsp; catlev: ".$result_create_historybar['cat_level']." | curlev: ".$RSDB_TEMP_cat_level."<br>";
			
/*			if ($result_create_historybar['cat_level'] > $RSDB_TEMP_cat_level) {
*/
	//				echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_category_cat.$result_create_historybar['cat_id']."'>".$result_create_historybar['cat_name']."</a>";
				create_tree_entry($result_create_historybar['cat_id'], $RSDB_TEMP_cat_level_newmain);
				//echo "<p>name: ".$result_create_historybar['cat_name']." | path: ".$RSDB_TEMP_cat_path." | id: ".$RSDB_TEMP_cat_id." | lev: ".$RSDB_TEMP_cat_level."</p>";
				create_treeview($result_create_historybar['cat_path'], $result_create_historybar['cat_id'], $RSDB_TEMP_cat_level, $RSDB_TEMP_cat_level_newmain);
				//create_treeview($RSDB_TEMP_cat_path, $RSDB_TEMP_cat_id, $RSDB_TEMP_cat_level);
/*			}
			else {
				echo "<p>############## ".$result_create_historybar['cat_id']." ##############</p>";
				//echo " <font size='2'>&gt;</font> <a href='".$RSDB_intern_link_category_cat.$result_create_historybar['cat_id']."'>".$result_create_historybar['cat_name']."</a>";
			}
*/
		}
	}
?>

<?php
	function create_tree_entry($RSDB_TEMP_entry_id, $RSDB_TEMP_cat_level_newmain) {
		
		global $RSDB_intern_link_category_cat;
		global $cellcolor2;
		global $RSDB_intern_code_db_rsdb_categories;
		$cellcolor=$cellcolor2;
		
//		global $RSDB_TEMP_cat_icon;

		
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' " . $RSDB_intern_code_db_rsdb_categories . "");
    $stmt->bindParam('cat_id',$RSDB_TEMP_entry_id,PDO::PARAM_STR);
    $stmt->execute();
					
		$result_create_tree_entry=$stmt->fetch(PDO::FETCH_ASSOC);

/*		if ($result_create_tree_entry['cat_icon'] != "") {
			$RSDB_TEMP_cat_icon = $result_create_tree_entry['cat_icon'];
		}
*/
		
		echo "<tr><td width='45%' valign='top' bgcolor='".$cellcolor."'>"; 
		echo "<div align='left'><font size='2' face='Arial, Helvetica, sans-serif'>&nbsp;";
		
		
		$RSDB_TEMP_cat_current_id_guess = $result_create_tree_entry['cat_id'];

		// count the levels -> current category level
		for ($guesslevel=1; ; $guesslevel++) {
//				echo $guesslevel."#";
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_categories WHERE cat_id = :cat_id AND cat_visible = '1' " . $RSDB_intern_code_db_rsdb_categories . "");
        $stmt->bindParam('cat_id',$RSDB_TEMP_cat_current_id_guess,PDO::PARAM_STR);
        $stmt->execute();
				$result_category_tree_guesslevel=$stmt->fetch(PDO::FETCH_ASSOC);
//					echo $result_category_tree_guesslevel['cat_name'];
				$RSDB_TEMP_cat_current_id_guess = $result_category_tree_guesslevel['cat_path'];
				
				if (!$result_category_tree_guesslevel['cat_name']) {
//					echo "ENDE:".($guesslevel-1);
					$RSDB_intern_catlevel = ($guesslevel-1);
					break;
				}
		}



		for ($n=$RSDB_TEMP_cat_level_newmain;$n<$RSDB_intern_catlevel;$n++) {
			echo "&nbsp;&nbsp;&nbsp;&nbsp;";
		}

//		echo "<img src='media/icons/categories/".$RSDB_TEMP_cat_icon."' width='16' height='16'> ";

		echo "<a href='".$RSDB_intern_link_category_cat.$result_create_tree_entry['cat_id']."'>".$result_create_tree_entry['cat_name']."</a>";

		echo "</font></div></td>";
		echo "<td width='45%' valign='top' bgcolor='".$cellcolor."'>";
		echo "<div align='left'><font face='Arial, Helvetica, sans-serif'>";
		
		echo "<font size='2' face='Arial, Helvetica, sans-serif'>".$result_create_tree_entry['cat_description']."</font>";
		
		echo "</font></div></td><td width='10%' valign='top' bgcolor='".$cellcolor."'><font size='2'>".count_tree_grouplist($result_create_tree_entry['cat_id'])."</font></td></tr>";
		
	}
?>


