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


$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_category = :category " . $RSDB_intern_code_db_rsdb_groups . "");
$stmt->bindParam('category',$RSDB_SET_cat,PDO::PARAM_STR);
$stmt->execute();
$result_count_groups = $stmt->fetch(PDO::FETCH_NUM);
if ($result_count_groups[0]) {

?>
	<table width="100%" border="0" cellpadding="1" cellspacing="1">
	  <tr bgcolor="#5984C3"> 
		<td width="20%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Name</strong></font></div></td>
		<td width="50%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
		<td width="10%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Pages</strong></font></div></td>
		<td width="20%"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Rating &Oslash; </strong></font></div></td>
	  </tr>
	  <?php
	
    $stmt=CDBConnecion::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_category = :category " . $RSDB_intern_code_db_rsdb_groups . " ORDER BY grpentr_order ASC");
    $stmt->bindParam('category',$RSDB_SET_cat,PDO::PARAM_STR);
    $stmt->execute();
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		
		while($result_page = $stmt->fetch(PDO::FETCH_ASSOC)) { // Pages
	?>
	  <tr> 
		<td valign="top" bgcolor="<?php
									$zaehler++;
									if ($zaehler == "1") {
										echo $farbe1;
										$farbe = $farbe1;
									}
									elseif ($zaehler == "2") {
										$zaehler="0";
										echo $farbe2;
										$farbe = $farbe2;
									}
								 ?>" > <div align="left"><font size="2" face="Arial, Helvetica, sans-serif"><a href="<?php echo $RSDB_intern_link_group.$result_page['grpentr_id']; ?>"><?php echo $result_page['grpentr_name']; ?></a></font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><font size="2"><?php
	
	if (strlen(htmlentities($result_page['grpentr_description'], ENT_QUOTES)) <= 30) {
		echo $result_page['grpentr_description'];
	}
	else {
		echo substr(htmlentities($result_page['grpentr_description'], ENT_QUOTES), 0, 30)."...";
	}
	 
	  ?></font></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="center"><font size="2">
		  <?php 
			
			$counter_stars_install_sum = 0;
			$counter_stars_function_sum = 0;
			$counter_stars_user_sum = 0;
			
			$counter_items = 0;

      $stmt_item=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_".$RSDB_intern_code_view_shortname." WHERE ".$RSDB_intern_code_view_shortname."_groupid = :group_id AND ".$RSDB_intern_code_view_shortname."_visible` = '1' ORDER BY ".$RSDB_intern_code_view_shortname."_groupid DESC");
      $stmt_item->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
      $stmt_item->execute();
			while($result_group_sum_items = $stmt_item->fetch(PDO::FETCH_ASSOC)) { 
				$counter_items++;
        $stmt_tests=CDBConnection::getInstance()->prepare("SELECT SUM(test_result_install) AS install_sum, SUM(test_result_function) AS function_sum, COUNT(*) AS user_sum FROM ".$RSDB_intern_code_view_shortname."_item_".$RSDB_intern_code_view_shortname."_testresults  WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
        $stmt_tests->bindParam('comp_id',$result_group_sum_items[$RSDB_intern_code_view_shortname.'_id'],PDO::PARAM_STR);
        $stmt_tests->execute();
        $tmp=$stmt_tests->fetch(PDO::FETCH_ASSOC);

        $counter_stars_install_sum += $tmp['install_sum'];
        $counter_stars_function_sum += $tmp['function_sum'];
        $counter_stars_user_sum += $tmp['user_sum'];
			}
			echo $counter_items;
			
			?>
		</font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2">
	    <?php 
			
			echo draw_stars_small($counter_stars_function_sum, $counter_stars_user_sum, 5, "") . " (".$counter_stars_user_sum.")";
			
			?>
		  </font></div></td>
	  </tr>
	  <?php	
		}	// end while
	?>
	</table>
<?php
}
?>

