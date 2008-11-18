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
						AND `grpentr_category` = " . $RSDB_SET_cat . "
						" . $RSDB_intern_code_db_rsdb_groups . " ;");	
$result_count_groups = mysql_fetch_row($query_count_groups);
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
	
		$query_page = mysql_query("SELECT * 
									FROM `rsdb_groups` 
									WHERE `grpentr_visible` = '1'
									AND `grpentr_category` = " . $RSDB_SET_cat . "
									" . $RSDB_intern_code_db_rsdb_groups . "
									ORDER BY `grpentr_order` ASC") ;
	
		$farbe1="#E2E2E2";
		$farbe2="#EEEEEE";
		$zaehler="0";
		
		while($result_page = mysql_fetch_array($query_page)) { // Pages
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

			$query_group_sum_items = @mysql_query("SELECT * 
													FROM `rsdb_item_".mysql_escape_string($RSDB_intern_code_view_shortname)."` 
													WHERE `".mysql_escape_string($RSDB_intern_code_view_shortname)."_groupid` = " . $result_page['grpentr_id'] . "
													AND `".mysql_escape_string($RSDB_intern_code_view_shortname)."_visible` = '1'
													ORDER BY `".mysql_escape_string($RSDB_intern_code_view_shortname)."_groupid` DESC ;") ;
			while($result_group_sum_items = @mysql_fetch_array($query_group_sum_items)) { 
				$counter_items++;
				$query_count_stars_sum = @mysql_query("SELECT * 
								FROM `".mysql_escape_string($RSDB_intern_code_view_shortname)."_item_".mysql_escape_string($RSDB_intern_code_view_shortname)."_testresults` 
								WHERE `test_visible` = '1'
								AND `test_comp_id` = " . $result_group_sum_items[$RSDB_intern_code_view_shortname.'_id'] . "
								ORDER BY `test_comp_id` ASC") ;
								
				while($result_count_stars_sum = @mysql_fetch_array($query_count_stars_sum)) {
					$counter_stars_install_sum += $result_count_stars_sum['test_result_install'];
					$counter_stars_function_sum += $result_count_stars_sum['test_result_function'];
					$counter_stars_user_sum++;
				}
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

