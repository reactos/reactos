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


  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :comp_id ORDER BY comp_name ASC");
  $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
  $stmt->execute();
	$result_page = $stmt->fetch(PDO::FETCH_ASSOC);
	
if ($result_page['comp_id']) {
	echo "<h2>".$result_page['comp_name'] ." [". "ReactOS ".@show_osversion($result_page['comp_osversion']) ."]</h2>"; 
	
	include("inc/comp/comp_item_menubar.php");
	
  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_id = :group_id");
  $stmt->bindParam('group_id',$result_page['comp_groupid'],PDO::PARAM_STR);
  $stmt->execute();
	$result_entry_vendor2 = $stmt->fetch(PDO::FETCH_ASSOC);

  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id");
  $stmt->bindParam('vendor_id',$result_entry_vendor2['grpentr_vendor'],PDO::PARAM_STR);
  $stmt->execute();
	$result_entry_vendor = $stmt->fetch(PDO::FETCH_ASSOC);
	
?>
	<table width="100%" border="0" cellpadding="1" cellspacing="5">
      <tr>
        <td width="40%" valign="top">
			<h3>Details</h3>
		  <p><span class="simple"><strong>Application</strong></span> </p>
			<ul class=simple>
              <li><strong>Name:</strong> <?php echo htmlentities($result_page['comp_name']); ?></li>
              <li><strong>Version:</strong> <?php echo htmlentities($result_page['comp_appversion']); ?></li>
              <li><strong>Company:</strong> <?php echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_entry_vendor['vendor_id'].'">'.htmlentities($result_entry_vendor['vendor_name']).'</a>'; ?></li>
              <li><strong>Description:</strong> <?php echo wordwrap(nl2br(htmlentities($result_page['comp_description'], ENT_QUOTES))); ?></li>
		  </ul>
			<span class="simple"><strong>ReactOS</strong></span>
            <ul class=simple>
              <li><strong>Version:</strong> <?php echo "ReactOS ". @show_osversion($result_page['comp_osversion']); ?></li>
			  <li><strong>Other tested versions:</strong><ul class=simple>
			  <?php
		
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_name = :name AND comp_visible = '1' AND comp_groupid = :group_id ORDER BY comp_osversion DESC");
      $stmt->bindParam('name',$result_page['comp_name'],PDO::PARAM_STR);
      $stmt->bindParam('group_id',$result_page['comp_groupid'],PDO::PARAM_STR);
      $stmt->execute();
			while($result_entry_osver = $stmt->fetch(PDO::FETCH_ASSOC)) {
				if ($result_entry_osver['comp_osversion'] != $result_page['comp_osversion']) {
					echo "<li><a href=\"".$RSDB_intern_link_item.$result_entry_osver['comp_id']."\">"."ReactOS ". @show_osversion($result_entry_osver['comp_osversion'])."</a></li>";
				}
			}
		
		?>			  </ul></li>
			 
            </ul>
            <span class="simple"><strong>Compatibility</strong></span>
            <ul class=simple>
              <li><strong>Award:</strong> <img src="media/icons/awards/<?php echo draw_award_icon($result_page['comp_award']); ?>_32.gif" alt="<?php echo draw_award_name($result_page['comp_award']); ?>" width="32" height="32" />
			  <?php echo draw_award_name($result_page['comp_award']); ?></li>
              <li><strong>Function:</strong> <?php
			
			$counter_stars_install = 0;
			$counter_stars_function = 0;
			$counter_stars_user = 0;

      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
      $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
      $stmt->execute();

			while($result_count_stars = $stmt->fetch(PDO::FETCH_ASSOC)) {
				$counter_stars_install += $result_count_stars['test_result_install'];
				$counter_stars_function += $result_count_stars['test_result_function'];
				$counter_stars_user++;
			}
			
			echo draw_stars($counter_stars_function, $counter_stars_user, 5, "tests");

			
			?></li>
              <li><strong>Install:</strong> <?php
			
			echo draw_stars($counter_stars_install, $counter_stars_user, 5, "tests");
			
			?></li>
            </ul>
            <span class="simple"><strong>Further Information</strong></span>
            <ul class=simple>
<?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_comp_id = :comp_id AND test_visible = '1'");
          $stmt->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt->execute();
					$result_count_testentries = $stmt->fetch();
					
					echo '<b><li><a href="'. $RSDB_intern_link_item.$result_page['comp_id'] .'&amp;item2=tests">Compatibility Tests</b>';
					
					if ($result_count_testentries[0] > 0) {
						echo " (". $result_count_testentries[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
<?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_forum WHERE fmsg_comp_id = :comp_id AND fmsg_visible = '1'");
          $stmt->bindParam('comp_id',$result_page['comp_id'],PDO::PARAM_STR);
          $stmt->execute();
					$result_count_forumentries = $stmt->fetch();
					
					if ($result_count_forumentries[0] > 0) {
						echo "<b>";
					}
			  		
					echo '<li><a href="'. $RSDB_intern_link_item.$result_page['comp_id'] .'&amp;item2=forum">Forum';
					
					if ($result_count_forumentries[0] > 0) {
						echo "</b> (". $result_count_forumentries[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
<?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE media_groupid = :group_id AND media_visible = '1'");
          $stmt->bindParam('group_id',$result_page['comp_media'],PDO::PARAM_STR);
          $stmt->execute();
					$result_count_screenshots = $stmt->fetch();
					
					if ($result_count_screenshots[0] > 0) {
						echo "<b>";
					}
			  		
					echo '<li><a href="'. $RSDB_intern_link_item.$result_page['comp_id'] .'&amp;item2=screens">Screenshots';
					
					if ($result_count_screenshots[0] > 0) {
						echo "</b> (". $result_count_screenshots[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
			  <li><a href="<?php echo "http://www.reactos.org/bugzilla/buglist.cgi?bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=product&type0-0-0=substring&value0-0-0=".$result_page['comp_name']."&field0-0-1=component&type0-0-1=substring&value0-0-1=".$result_page['comp_name']."&field0-0-2=short_desc&type0-0-2=substring&value0-0-2=".$result_page['comp_name']."&field0-0-3=status_whiteboard&type0-0-3=substring&value0-0-3=".$result_page['comp_name']; ?>" target="_blank">Bugs</a></li>
          </ul>
		</td>
        <td width="10%" align="center" valign="top"></td>
        <td width="40%" valign="top">
        <h3 align="right">Screenshot</h3>
        <p align="center"><?php
		
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE media_groupid = :group_id AND (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5) ORDER BY media_useful_vote_value DESC LIMIT 1");
      $stmt->bindParam('group_id',$result_page['comp_media'],PDO::PARAM_STR);
      $stmt->execute();
			$result_screenshots= $stmt->fetch(PDO::FETCH_ASSOC);
	
			if ($result_screenshots['media_thumbnail']=="") {
				echo '<img src="media/screenshots/comp_default.jpg" width="250" height="188" border="0" />';
			}
			else {
				echo '<a href="'.$RSDB_intern_link_item.$result_page['comp_id'].'&amp;item2=screens"><img src="media/files/picture/'.urlencode($result_screenshots['media_thumbnail']).'" width="250" height="188" border="0" alt="'.htmlentities($result_screenshots['media_description']).'" /></a>';
			}
		
		?></p>
		<p>&nbsp;</p>
		<?php
			if ($result_page['comp_infotext']) {
		?>
				<h4 align="left">Information:</h4>
				<p align="left"><font face="Arial, Helvetica, sans-serif" size="2"><?php echo wordwrap(nl2br(htmlentities($result_page['comp_infotext'], ENT_QUOTES))); ?></font></p></td>
     	<?php
			}
		?>
	  </tr>
    </table>
<?php
	include("inc/comp/comp_item_details_maintainer.php");
}
?>
