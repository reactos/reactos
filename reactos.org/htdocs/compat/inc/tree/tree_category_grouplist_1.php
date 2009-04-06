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


$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_category = :category AND grpentr_comp = '1'");
$stmt->bindParam('category',@$_GET['cat'],PDO::PARAM_STR);
$stmt->execute();
$result_count_groups = $stmt->fetch(PDO::FETCH_NUM);
if ($result_count_groups[0]) {

?>
	<table width="100%" border="0" cellpadding="1" cellspacing="1">
	  <tr bgcolor="#5984C3"> 
		<td width="20%" title="Item"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong><?php
		

				echo "Application";

		?></strong></font></div></td>
		<td width="15%" title="Company/Vendor/Project"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Vendor</strong></font></div></td>
		<td width="25%" title="Description"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
		<td width="10%" title="Award/Medal (best)"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Award</strong></font></div></td>
		<td width="6%" title="Version"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Ver.</strong></font></div></td>
		<td width="17%" title="Compatibility &Oslash;"> <div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Compatibility &Oslash;</strong></font></div></td>
	    <td width="7%" title="Status"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Stat.</strong></font></div></td>
	  </tr>
	  <?php
	
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_category = :category AND grpentr_comp = '1' ORDER BY grpentr_name ASC");
    $stmt->bindParam('category',@$_GET['cat'],PDO::PARAM_STR);
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
								 ?>" > <div align="left"><font size="2" face="Arial, Helvetica, sans-serif"><b><a href="<?php echo $RSDB_intern_link_group.$result_page['grpentr_id']; ?>">
		  <?php
			$stmt_vendor=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id");
      $stmt_vendor->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
      $stmt_vendor->execute();
			$result_entry_vendor = $stmt_vendor->fetchOnce(PDO::FETCH_ASSOC);
	/*	
			echo $result_entry_vendor['vendor_name']."&nbsp;";
	*/
		  ?>
		  <?php echo $result_page['grpentr_name']; ?></a></b><?php
			echo " &nbsp;<i>";
			$stmt_comp=CDBConnection::getInstance()->prepare("SELECT DISTINCT(comp_appversion), comp_osversion, comp_id, comp_name FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_groupid = :group_id GROUP BY comp_appversion ORDER BY comp_appversion ASC  LIMIT 15");
      $stmt_comp->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
      $stmt_comp->execute();
			while($result_entry_appver = $stmt_comp->fetch(PDO::FETCH_ASSOC)) {
				if ($result_entry_appver['comp_name'] > $result_page['grpentr_name']) {
					echo "<a href=\"".$RSDB_intern_link_group.$result_page['grpentr_id']."&amp;group2=".$result_entry_appver['comp_appversion']."\">".substr($result_entry_appver['comp_name'], strlen($result_page['grpentr_name'])+1 )."</a>, ";
				}
			}
			echo "</i>";
		?></font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"> <div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<?php
		
			echo '<a href="'.$RSDB_intern_link_vendor_id_EX.$result_entry_vendor['vendor_id'].'">'.$result_entry_vendor['vendor_name'].'</a>';

		  ?></font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><font size="2"><?php
	
	if (strlen(htmlentities($result_page['grpentr_description'], ENT_QUOTES)) <= 30) {
		echo $result_page['grpentr_description'];
	}
	else {
		echo substr(htmlentities($result_page['grpentr_description'], ENT_QUOTES), 0, 30)."...";
	}
	 
	  ?></font></td>
<?php
			$counter_stars_install_sum = 0;
			$counter_stars_function_sum = 0;
			$counter_stars_user_sum = 0;
			$counter_awards_best = 0;
			
			$counter_items = 0;

			$counter_testentries = 0;
			$counter_forumentries = 0;
			$counter_screenshots = 0;

			$stmt_item=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' ORDER BY comp_groupid DESC");
      $stmt_item->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
      $stmt_item->execute();
			while($result_group_sum_items = $stmt_item->fetch(PDO::FETCH_ASSOC)) { 
				$counter_items++;
				if ($counter_awards_best < $result_group_sum_items['comp_award']) {
					$counter_awards_best = $result_group_sum_items['comp_award'];
				}
        $stmt_tests=CDBConnection::getInstance()->prepare("SELECT SUM(test_result_install) AS install_sum, SUM(test_result_function) AS function_sum, COUNT(*) AS user_sum FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
        $stmt_tests->bindParam('comp_id',$result_group_sum_items['comp_id'],PDO::PARAM_STR);
        $stmt_tests->execute();
        $tmp=$stmt_tests->fetch(PDO::FETCH_ASSOC);

        $counter_stars_install_sum += $tmp['install_sum'];
        $counter_stars_function_sum += $tmp['function_sum'];
        $counter_stars_user_sum += $tmp['user_sum'];
				
        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
        $stmt_count->bindParam('comp_id',$result_group_sum_items['comp_id'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_testentries = $stmt_count->fetch(PDO::FETCH_NUM);
				$counter_testentries += $result_count_testentries[0];
				
				// Forum entries:
        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' AND fmsg_comp_id = :comp_id");
        $stmt_count->bindParam('comp_id',$result_group_sum_items['comp_id'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_forumentries = $stmt_count->fetch(PDO::FETCH_NUM);
				$counter_forumentries += $result_count_forumentries[0];

				// Screenshots:
        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE media_visible = '1' AND media_groupid = :group_id");
        $stmt_count->bindParam('group_id',$result_group_sum_items['comp_media'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_screenshots = $stmt_count->fetch(PDO::FETCH_NUM);
				$counter_screenshots += $result_count_screenshots[0];
			}
?>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="1">&nbsp;<img src="media/icons/awards/<?php echo Award::icon($counter_awards_best); ?>.gif" alt="<?php echo Award::name($counter_awards_best); ?>" width="16" height="16" /> <?php echo Award::name($counter_awards_best); ?></font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="center"><font size="2">
		  <?php 
			
			echo $counter_items;
			
			?>
		</font></div></td>
		<td valign="top" bgcolor="<?php echo $farbe; ?>"><div align="left"><font size="2">
	    <?php 
			
			echo Star::drawSmall($counter_stars_function_sum, $counter_stars_user_sum, 5, "") . " (".$counter_stars_user_sum.")";
			
			?>
		  </font></div></td>
	    <td valign="top" bgcolor="<?php echo $farbe; ?>" title="<?php echo "Tests: ".$counter_testentries.", Forum entries: ".$counter_forumentries.", Screenshots: ".$counter_screenshots; ?>"><div align="center">
	      <table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr>
              <td width="33%"><div align="center"><?php if ($counter_testentries > 0) { ?><img src="media/icons/info/test.gif" alt="Compatibility Test Report entries" width="13" height="13"><?php } else { echo "&nbsp;"; } ?></div></td>
              <td width="33%"><div align="center"><?php if ($counter_forumentries > 0) { ?><img src="media/icons/info/forum.gif" alt="Forum entries" width="13" height="13"><?php } else { echo "&nbsp;"; } ?></div></td>
              <td width="33%"><div align="center"><?php if ($counter_screenshots > 0) { ?><img src="media/icons/info/screenshot.gif" alt="Screenshots" width="13" height="13"><?php } else { echo "&nbsp;"; } ?></div></td>
            </tr>
          </table>
	        </div></td>
	  </tr>
	  <?php	
		}	// end while
	?>
	</table>
<?php
}
?>
