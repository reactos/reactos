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


	
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_id = :group_id AND grpentr_comp = '1' ORDER BY grpentr_name ASC") ;
    $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
    $stmt->execute();
		$result_page = $stmt->fetch(PDO::FETCH_ASSOC);
		
		if ($result_page['grpentr_type'] == "default") {
		// Update the ViewCounter:
		if ($RSDB_SET_group != "" || $RSDB_SET_group != "0") {
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_groups SET grpentr_viewcounter = grpentr_viewcounter + 1 WHERE grpentr_id = '" . $RSDB_SET_group . "'");
      $stmt->execute();
		}
	
	?>
	
	<style type="text/css">
	<!--
	/* tab colors */
	.tab                { background-color : #ffffff; }
	.tab_s              { background-color : #5984C3; }
	.tab_u              { background-color : #A0B7C9; }
	
	/* tab link colors */
	a.tabLink           { text-decoration : none; }
	a.tabLink:link      { text-decoration : none; }
	a.tabLink:visited   { text-decoration : none; }
	a.tabLink:hover     { text-decoration : underline; }
	a.tabLink:active    { text-decoration : underline; }
	
	/* tab link size */
	p.tabLink_s         { color: navy; font-size : 10pt; font-weight : bold; padding : 0 8px 1px 2px; margin : 0; }
	p.tabLink_u         { color: black; font-size : 10pt; padding : 0 8px 1px 2px; margin : 0; }
	
	/* text styles */
	.strike 	       { text-decoration: line-through; }
	.bold              { font-weight: bold; }
	.newstitle         { font-weight: bold; color: purple; }
	.title_group       { font-size: 16px; font-weight: bold; color: #5984C3; text-decoration: none; }
	.bluetitle:visited { color: #323fa2; text-decoration: none; }
	
	.Stil1 {font-size: xx-small}
	.Stil2 {font-size: x-small}
	.Stil3 {color: #FFFFFF}
	.Stil4 {font-size: xx-small; color: #FFFFFF; }
	
	-->
	</style>
	<script src="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>rsdb.js"></script>
		<h2><?php echo $result_page['grpentr_name']; ?></h2>
	
		<table align="center" border="0" cellpadding="0" cellspacing="0" width="100%">
		  <tr align="left" valign="top">
			<!-- title -->
			<td valign="bottom" width="100%">
			  <table border="0" cellpadding="0" cellspacing="0" width="100%">
				<tr>
				  <td class="title_group" nowrap="nowrap"><?php 
				  
				  if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { 
				  	echo "Overview";
				  }
				  else { 
						$stmt=CDBConnection::getInstance()->prepare("SELECT comp_name, comp_appversion FROM rsdb_item_comp  WHERE comp_visible = '1' AND comp_groupid = :group_id AND comp_appversion = :version LIMIT 1");
            $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
            $stmt->bindParam('version',$RSDB_SET_group2,PDO::PARAM_STR);
            $stmt->execute();
						$result_entry_appname = $stmt->fetch(PDO::FETCH_ASSOC);
						echo $result_entry_appname['comp_name'];
				  }
			  
				  
				  ?></td>
				</tr>
				<tr valign="bottom">
				  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="1"></td>
				</tr>
			</table></td>

          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><a href="<?php echo $RSDB_intern_link_group_group2; ?>overview" class="tabLink">Overview</a></p></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == "overview" || $RSDB_SET_group2 == "") { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->

<?php

    $stmt=CDBConnection::getInstance()->prepare("SELECT DISTINCT (comp_appversion), comp_osversion, comp_id, comp_name FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_groupid = :group_id GROUP BY comp_appversion ORDER BY comp_appversion ASC LIMIT 15");
    $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
		while($result_entry_appver = $stmt->fetch()) {
?>
          <!-- start tab -->
          <td nowrap="nowrap">
            <table border="0" cellpadding="0" cellspacing="0">
                <tr align="left" valign="top">
                  <td width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr align="left" valign="top">
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/white_pixel.gif" alt="" height="4" width="1"></td>
                  <td width="4"><img src="images/tab_corner_<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "active"; } else { echo "inactive"; } ?>.gif" alt="" height="4" width="4"></td>
                  <td><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="middle">
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="4"><img src="images/blank.gif" alt="" height="1" width="4"></td>
                  <td nowrap="nowrap"><p class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tabLink_s"; } else { echo "tabLink_u"; } ?>"><?php echo "<a href=\"".$RSDB_intern_link_group_group2.$result_entry_appver['comp_appversion']."\" class=\"tabLink\">".$result_entry_appver['comp_name']."</a>"; ?></p></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab_s"; } else { echo "tab_u"; } ?>" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
                <tr valign="bottom">
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab"; } else { echo "tab_s"; } ?>" width="4"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="<?php if ($RSDB_SET_group2 == $result_entry_appver['comp_appversion']) { echo "tab"; } else { echo "tab_s"; } ?>"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="1"><img src="images/blank.gif" alt="" height="1" width="1"></td>
                  <td class="tab_s" width="2"><img src="images/blank.gif" alt="" height="1" width="2"></td>
                </tr>
          </table></td>
          <!-- end tab -->
<?php
		}
?>

			<!-- fill the remaining space -->
			<td valign="bottom" width="10">
			  <table border="0" cellpadding="0" cellspacing="0" width="100%">
				<tr valign="bottom">
				  <td class="tab_s"><img src="images/white_pixel.gif" alt="" height="1" width="10"></td>
				</tr>
			</table></td>
		  </tr>
		</table>
	<?php
				$counter_stars_install_sum = 0;
				$counter_stars_function_sum = 0;
				$counter_stars_user_sum = 0;
				$counter_awards_best = 0;
				
				$counter_items = 0;
	
				if ($RSDB_SET_group2 == "" || $RSDB_SET_group2 == "overview") {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' ORDER BY comp_groupid DESC");
          $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
          $stmt->execute();
				}
				else {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' AND `comp_appversion` = :version ORDER BY comp_groupid DESC");
          $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
          $stmt->bindParam('version',$RSDB_SET_group2,PDO::PARAM_STR);
          $stmt->execute();
				}
				while($result_group_sum_items = $stmt->fetch(PDO::FETCH_ASSOC)) { 
					$counter_items++;
					if ($counter_awards_best < $result_group_sum_items['comp_award']) {
						$counter_awards_best = $result_group_sum_items['comp_award'];
					}
          $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
          $stmt_sub->bindParam('comp_id',$result_group_sum_items['comp_id'],PDO::PARAM_STR);
          $stmt_sub->execute();
					while($result_count_stars_sum = $stmt_sub->fetch(PDO::FETCH_ASSOC)) {
						$counter_stars_install_sum += $result_count_stars_sum['test_result_install'];
						$counter_stars_function_sum += $result_count_stars_sum['test_result_function'];
						$counter_stars_user_sum++;
					}
				}
	
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id AND vendor_visible = '1'");
        $stmt->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
        $stmt->execute();
				$result_entry_vendor = $stmt->fetch(PDO::FETCH_ASSOC);
	

	
  $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(comp_id) FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1'");
  $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
  $stmt->execute();
	$result_item_entry_records = $stmt->fetch(PDO::FETCH_ASSOC);
	
	if ($result_item_entry_records[0] == 0) {
	
    $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_id = :vendor_id AND vendor_visible = '1'");
    $stmt->bindParam('vendor_id',$result_page['grpentr_vendor'],PDO::PARAM_STR);
    $stmt->execute();
		$result_vend = $stmt->fetch(PDO::FETCH_ASSOC);
?>
		<p>&nbsp;</p>
		<p><b>Vendor:</b> <?php echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_vend['vendor_id'].'">'.$result_vend['vendor_name'].'</a>'; ?></p>
		<p>&nbsp;</p>
		<p>No application or driver version stored.</p>
		<p>&nbsp;</p>
		<p><a href="<?php echo $RSDB_intern_link_submit_appver; ?>"><strong><font size="4">Submit new &quot;<?php echo $result_page['grpentr_name']; ?>&quot; version</font></strong></a></p>
		<p>&nbsp;</p>
<?php
	}
	else {
	?>
<br />
		<div id="moreinfo" style="display: none">	
			<div id="textversions" align="center"></div>
			<script language="JavaScript1.2">
				function show_Versions(lblVersions) {
					document.getElementById("textversions").innerHTML ='<b><a title=\"All Application and ReactOS versions\" href=\"javascript:showhideVersions()\">' + lblVersions + '</a></b>' ;
				}
			</script>
		</div>	
		<div id="versions" style="display: block">	
		<?php
		// Count the comp entries
		$stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(comp_id) FROM rsdb_item_comp WHERE comp_groupid = :group_id");
    $stmt->bindParam('group_id',$result_page["grpentr_id"],PDO::PARAM_STR);
    $stmt->execute();
		$result_count_comp = $stmt->fetch(PDO::FETCH_ASSOC);
		
		if ($result_count_comp[0]) {
	
	?>
		<a name="ver"></a>
		<h3>All Versions</h3>
		<?php
				include("inc/comp/data/group_item_list.php");
		?>
		<div id="group_version_list"></div>
		<br />
	<?php
		}
	?>
	</div>
		<table width="100%" border="0" cellpadding="1" cellspacing="5">
		  <tr>
			<td width="40%" valign="top">
	
			<?php
				if ($RSDB_SET_group2 == "" || $RSDB_SET_group2 == "overview") {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id  AND comp_visible = '1' ORDER BY comp_award DESC, comp_appversion DESC, comp_osversion DESC LIMIT 1");
				}
				else {
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_visible = '1' AND comp_appversion = :version ORDER BY comp_award DESC, comp_osversion DESC LIMIT 1");
          $stmt->bindParam('version',$RSDB_SET_group2,PDO::PARAM_STR);

				}
        $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
				$stmt->execute();
				$result_version_newest = $stmt->fetch(PDO::FETCH_ASSOC);;
				
				echo "<h3 align=\"center\"><font size=\"2\">".$result_entry_vendor['vendor_name']."</font> ".$result_version_newest['comp_name']."</h3>";
			?>
		  <p><font size="2">Most compatible entry.</font></p>
		  <span class="simple"><strong>Application</strong></span>
			<ul class=simple>
			  <li><strong>Name:</strong> <?php echo $result_version_newest['comp_name']; ?></li>
			  <li><strong>Version:</strong> <?php echo $result_version_newest['comp_appversion']; ?></li>
			  <li><strong>Vendor:</strong> <font size="2" face="Arial, Helvetica, sans-serif">
				<?php
			
				echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_entry_vendor['vendor_id'].'">'.$result_entry_vendor['vendor_name'].'</a>';
	
			  ?>
			  </font></li>
			  <li><strong>Description:</strong> <?php echo $result_version_newest['comp_description']; ?></li>
			  </ul>
			<span class="simple"><strong>ReactOS</strong></span>		<ul class=simple>
			  <li><strong>Version:</strong> <?php 
			  
			  echo "ReactOS ". @show_osversion($result_version_newest['comp_osversion']); ?></li>
			  <li><strong>Other tested versions:</strong><ul class=simple>
			  <?php
		
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_name = :name AND comp_visible = '1' AND `comp_groupid` = :group_id ORDER BY `comp_osversion` DESC");
      $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
      $stmt->bindParam('name',$result_version_newest['comp_name'],PDO::PARAM_STR);
      $stmt->execute();
			while($result_entry_osver = $stmt->fetch(PDO::FETCH_ASSOC)) {
				if ($result_entry_osver['comp_osversion'] != $result_version_newest['comp_osversion']) {
					echo "<li><a href=\"".$RSDB_intern_link_item.$result_entry_osver['comp_id']."\">"."ReactOS ". @show_osversion($result_entry_osver['comp_osversion'])."</a></li>";
				}
			}
		
		?>			  </ul></li>
			 
			  </ul>
			<span class="simple"><strong>Compatibility</strong></span>
			<ul class=simple>
			  <li><strong>Award:</strong> <font size="2"><img src="media/icons/awards/<?php echo Award::icon($result_version_newest['comp_award']); ?>.gif" alt="<?php echo Award::name($result_version_newest['comp_award']); ?>" width="16" height="16" /> <?php echo Award::name($result_version_newest['comp_award']); ?></font></li>
			  <li><strong>Function:</strong>
				  <?php 
				  
				$counter_stars_install = 0;
				$counter_stars_function = 0;
				$counter_stars_user = 0;
				
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
        $stmt->bindParam('comp_id',$result_version_newest['comp_id'],PDO::PARAM_STR);
        $stmt->execute();
				while($result_count_stars = $stmt->fetch(PDO::FETCH_ASSOC)) {
					$counter_stars_install += $result_count_stars['test_result_install'];
					$counter_stars_function += $result_count_stars['test_result_function'];
					$counter_stars_user++;
				}
				
				echo Star::drawNormal($counter_stars_function, $counter_stars_user, 5, "tests");
	
				
				?>
			  </li>
			  <li><strong>Install:</strong> <?php echo Star::drawNormal($counter_stars_install, $counter_stars_user, 5, "tests"); ?></li>
			</ul>
			<span class="simple"><strong>Further Information</strong></span>
			<ul class=simple>
			  <li><a href="<?php echo $RSDB_intern_link_item.$result_version_newest['comp_id']."&amp;item2="; ?>details">Details</a></li>
<?php
        $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_comp_id = :comp_id AND test_visible = '1'");
        $stmt->bindParam('comp_id',$result_version_newest['comp_id'],PDO::PARAM_STR);
        $stmt->execute();
					$result_count_testentries = $stmt->fetch(PDO::FETCH_ASSOC);
					
					echo '<b><li><a href="'. $RSDB_intern_link_item.$result_version_newest['comp_id'] .'&amp;item2=tests">Compatibility Tests</b>';
					
					if ($result_count_testentries[0] > 0) {
						echo " (". $result_count_testentries[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
<?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(fmsg_id) FROM rsdb_item_comp_forum WHERE fmsg_comp_id = :comp_id AND fmsg_visible = '1' ;");
          $stmt->bindParam('comp_id',$result_version_newest['comp_id'],PDO::PARAM_STR);
          $stmt->execute();
					$result_count_forumentries = $stmt->fetch(PDO::FETCH_ASSOC);
					
					if ($result_count_forumentries[0] > 0) {
						echo "<b>";
					}
			  		
					echo '<li><a href="'. $RSDB_intern_link_item.$result_version_newest['comp_id'] .'&amp;item2=forum">Forum';
					
					if ($result_count_forumentries[0] > 0) {
						echo "</b> (". $result_count_forumentries[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
<?php
          $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(media_id) FROM rsdb_object_media WHERE media_groupid = :group_id AND media_visible = '1'");
          $stmt->bindParam('group_id',$result_version_newest['comp_media'],PDO::PARAM_STR);
          $stmt->execute();
					$result_count_screenshots = $stmt->fetch();
					
					if ($result_count_screenshots[0] > 0) {
						echo "<b>";
					}
			  		
					echo '<li><a href="'. $RSDB_intern_link_item.$result_version_newest['comp_id'] .'&amp;item2=screens">Screenshots';
					
					if ($result_count_screenshots[0] > 0) {
						echo "</b> (". $result_count_screenshots[0] .")</a></li>";
					}
					else {
						echo "</a></li>";
					}
?>
			  <li><a href="<?php echo "http://www.reactos.org/bugzilla/buglist.cgi?bug_status=UNCONFIRMED&bug_status=NEW&bug_status=ASSIGNED&bug_status=REOPENED&field0-0-0=product&type0-0-0=substring&value0-0-0=".$result_version_newest['comp_name']."&field0-0-1=component&type0-0-1=substring&value0-0-1=".$result_version_newest['comp_name']."&field0-0-2=short_desc&type0-0-2=substring&value0-0-2=".$result_version_newest['comp_name']."&field0-0-3=status_whiteboard&type0-0-3=substring&value0-0-3=".$result_version_newest['comp_name']; ?>" target="_blank">Bugs</a></li>
			</ul>
		<br />
        <?php 
      $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(bundle_id) FROM rsdb_group_bundles WHERE bundle_groupid = :group_id");
      $stmt->bindParam('group_id',$result_page["grpentr_id"],PDO::PARAM_STR);
			$result_count_bundle = $stmt->fetch();

			if ($RSDB_SET_group2 != "" && $RSDB_SET_group2 != "overview" || $result_count_bundle[0] != 0) {
				$temp_pic = mt_rand(1,$result_count_screenshots[0]);
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE media_groupid = :group_id AND media_order = :order AND (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5) ORDER BY media_useful_vote_value DESC LIMIT 1");
        $stmt->bindParam('group_id',$result_version_newest['comp_media'],PDO::PARAM_STR);
        $stmt->bindParam('order',$temp_pic,PDO::PARAM_STR);
        $stmt->execute();
				$result_screenshots= $stmt->fetch($result_version_newest['comp_media']);
		
				if ($result_screenshots['media_thumbnail']=="") {
					echo '<img src="media/screenshots/comp_default.jpg" width="250" height="188" border="0" />';
				}
				else {
					echo '<a href="'.$RSDB_intern_link_item.$result_version_newest['comp_id'].'&amp;item2=screens"><img src="media/files/picture/'.urlencode($result_screenshots['media_thumbnail']).'" width="250" height="188" border="0" alt="'.htmlentities($result_screenshots['media_description']).'" /></a>';
				}
			}
		?>
</td>
			<td width="10%" align="center" valign="top"></td>
			<td width="40%" valign="top">
<?php 
	if ($RSDB_SET_group2 == "" || $RSDB_SET_group2 == "overview") {
?>
			  <h3 align="center">All Versions <font size="2">- Overview</font></h3>
			  <p><font size="2">A sum up of all tested versions.</font></p>
			  <span class="simple"><strong>Application</strong></span>
			  <ul class=simple>
				<li><strong>Name:</strong> <?php echo $result_page['grpentr_name']; ?></li>
				<li><strong>Vendor:</strong> <font size="2" face="Arial, Helvetica, sans-serif">
				  <?php
			
				echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_entry_vendor['vendor_id'].'">'.$result_entry_vendor['vendor_name'].'</a>';
	
			  ?>
				</font></li>
				<li><strong>Description:</strong> <?php echo htmlentities($result_page['grpentr_description']); ?></li>
			  </ul>
			  <span class="simple"><strong>Compatibility</strong></span>
              <strong>&Oslash;</strong>              <ul class=simple>
				<li><strong>Award (best):</strong> <font size="2"><img src="media/icons/awards/<?php echo Award::icon($counter_awards_best); ?>.gif" alt="<?php echo Award::name($counter_awards_best); ?>" width="16" height="16" /> <?php echo Award::name($counter_awards_best); ?></font></li>
				<li><strong>Function &Oslash;:</strong>
					<?php 
				
				echo Star::drawNormal($counter_stars_function_sum, $counter_stars_user_sum, 5, "tests");
				
				?>
				</li>
				<li><strong>Install &Oslash;:</strong> <?php echo Star::drawNormal($counter_stars_install_sum, $counter_stars_user_sum, 5, "tests"); ?></li>
		      </ul>
			  <span class="simple"><strong>Application versions</strong></span>
              <ul class=simple>
		<?php
		
      $stmt=CDBConnection::getInstance()->prepare("SELECT DISTINCT(comp_appversion), comp_osversion, comp_id, comp_name FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_groupid = :group_id GROUP BY comp_appversion ORDER BY comp_appversion ASC LIMIT 15") ;
      $stmt->bindParam('group_id',$result_page['grpentr_id'],PDO::PARAM_STR);
      $stmt->execute();
			while($result_entry_appver = $stmt->fetch(PDO::FETCH_ASSOC)) {
				echo "<li><b><a href=\"".$RSDB_intern_link_group_group2.$result_entry_appver['comp_appversion']."\">".$result_entry_appver['comp_name']."</a></b></li>";
			}
		
		?>
              </ul> 
		<?php 
		
			
			if ($result_count_bundle[0] != 0) {
			
		?>
			<span class="simple"><strong>Bundles</strong></span>
			<blockquote>
	            <?php  
		// Query Bundles	
		$stmt=CDBConnections::getInstance()->prepare("SELECT * FROM rsdb_group_bundles WHERE bundle_groupid = :group_id") ;
    $stmt->bindParam('group_id',$result_page["grpentr_id"],PDO::PARAM_STR);
    $stmt->execute();
		while($result_bundlelist = $stmt->fetch(PDO::FETCH_ASSOC)) {
			// Count the bundle entries for the current bundle
      $stmt_sub=CDBConnection::getInstance()->prepare("SELECT COUNT('bundle_id') FROM rsdb_group_bundles WHERE bundle_id = :bundle_id");
      $stmt_sub->bindParam('bundle_id',$result_bundlelist["bundle_id"],PDO::PARAM_STR);
      $stmt_sub->execute();
			$result_count_bundle = $stmt_sub->fetch();

			if ($result_count_bundle[0]) {

				$farbe1="#E2E2E2";
				$farbe2="#EEEEEE";
				$zaehler="0";
				echo "<table width='100%' border='0'>";
				echo "  <tr bgcolor='#5984C3'>";
				echo "	<td width='30%'><div align='center'><font color='#FFFFFF' size='2' face='Arial, Helvetica, sans-serif'><b>Name</b></font></div></td>";
				echo "	<td width='70%'><div align='center'><font color='#FFFFFF' size='2' face='Arial, Helvetica, sans-serif'><b>Description</b></font></div></td>";
				echo "  </tr>";
        $stmt_bundle=CDBConnnection::getInstance()->prepare("SELECT * FROM rsdb_group_bundles WHERE bundle_id = :bundle_id ORDER BY bundle_groupid ASC");
        $stmt_bundle->bindParam('bundle_id',$result_bundlelist["bundle_id"],PDO::PARAM_STR);
        $stmt_bundle->execute();
				while($result_bundlelist_groupitem = $stmt->fetch(PDO::FETCH_ASSOC)) {
          $stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_id = :group_id ORDER BY grpentr_name ASC");
          $stmt_sub->bindParam('group_id',$result_bundlelist_groupitem["bundle_groupid"],PDO::PARAM_STR);
          $stmt_sub->execute();
					$result_bundlelist_item = $stmt_sub->fetch(PDO::FETCH_ASSOC);
					echo "  <tr bgcolor='";
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
					echo "'>";
					echo "	<td width='30%'><font size='2' face='Arial, Helvetica, sans-serif'>&nbsp;<b><a href='". $RSDB_intern_link_group.$result_bundlelist_item['grpentr_id'] ."'>". $result_bundlelist_item["grpentr_name"] ."</b></font></td>";
					echo "	<td width='70%'><font size='2' face='Arial, Helvetica, sans-serif'>". $result_bundlelist_item["grpentr_description"] ."</font></td>";
					echo "  </tr>";
				}
				echo "</table>";
			}
		}
	?>			
			  </blockquote>
	<?php
		}
		else {
	?>
			  <br />
	          <p>
	            <?php
					$temp_pic = mt_rand(1,$result_count_screenshots[0]);
          $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_object_media WHERE media_groupid = :group_id AND media_order = :order AND (( media_useful_vote_value / media_useful_vote_user) > 2 OR  media_useful_vote_user < 5) ORDER BY media_useful_vote_value DESC LIMIT 1");
          $stmt->bindParam('group_id',$result_version_newest['comp_media'],PDO::PARAM_STR);
          $stmt->bindParam('order',$temp_pic,PDO::PARAM_STR);
          $stmt->execute();
					$result_screenshots= $stmt->fetch(PDO::FETCH_ASSOC);
			
					if ($result_screenshots['media_thumbnail']=="") {
						echo '<img src="media/screenshots/comp_default.jpg" width="250" height="188">';
					}
					else {
						echo '<a href="'.$RSDB_intern_link_item.$result_version_newest['comp_id'].'&amp;item2=screens"><img src="media/files/picture/'.urlencode($result_screenshots['media_thumbnail']).'" width="250" height="188" border="0" alt="'.htmlentities($result_screenshots['media_description']).'" /></a>';
					}
			?>
	  </p>
<?php
		}
	}
	else {
		echo "<h3 align=\"center\">".$result_entry_appname['comp_name']." <font size=\"2\">- Overview</font></h3>";
?>
			<p><font size="2">A sum up of <?php echo $result_entry_appname['comp_name']; ?> tested with several ReactOS versions.</font></p>
			<span class="simple"><strong>Application</strong></span>
			<ul class=simple>
			  <li><strong>Name:</strong> <font size="2"><?php echo $result_entry_appname['comp_name']; ?></font></li>
			  <li><strong>Vendor:</strong> <font size="2" face="Arial, Helvetica, sans-serif">
				<?php
						
							echo '<a href="'.$RSDB_intern_link_vendor_sec.$result_entry_vendor['vendor_id'].'">'.$result_entry_vendor['vendor_name'].'</a>';
				
						  ?>
			  </font></li>
			  <li><strong>Description:</strong> <?php echo htmlentities($result_page['grpentr_description']); ?></li>
			</ul>
			<span class="simple"><strong>Compatibility &Oslash;</strong></span>
			<ul class=simple>
			  <li><strong>Award (best):</strong> <font size="2"><img src="media/icons/awards/<?php echo Award::icon($counter_awards_best); ?>.gif" alt="<?php echo Award::name($counter_awards_best); ?>" width="16" height="16" /> <?php echo Award::name($counter_awards_best); ?></font></li>
			  <li><strong>Function &Oslash;:</strong>
				  <?php 
							
							echo Star::drawNormal($counter_stars_function_sum, $counter_stars_user_sum, 5, "tests");
							
							?>
			  </li>
			  <li><strong>Install &Oslash;:</strong> <?php echo Star::drawNormal($counter_stars_install_sum, $counter_stars_user_sum, 5, "tests"); ?></li>
			</ul>
			  <table width="100%">
                <tr bgcolor="#5984C3">
                  <td colspan="4">
                    <table width="100%" border="0" cellpadding="0" cellspacing="0">
                      <tr>
                        <td width="40%"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif" size="2"><strong>&nbsp;
                                <?php
									
						echo $result_entry_appname['comp_name'];
						
					 ?>
                        </strong></font></td>
                        <td width="20%"><div align="center" class="Stil4">Medal</div></td>
                        <td width="20%"><div align="center" class="Stil4">Function</div></td>
                        <td width="20%"><div align="center" class="Stil4">Install</div></td>
                      </tr>
                    </table>
                </tr>
                <?php  
			// Table line
			$farbe1="#E2E2E2";
			$farbe2="#EEEEEE";
			$zaehler="0";
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_appversion = :version AND comp_visible = '1' ORDER BY comp_osversion DESC");
      $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
      $stmt->bindParam('version',$result_entry_appname['comp_appversion'],PDO::PARAM_STR);
      $stmt->execute();
			while($result_sortby_b = $stmt->fetch(PDO::FETCH_ASSOC)) { 
?>
                <tr bgcolor="<?php
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
								 ?>">
                  <td width="40%" bgcolor="<?php echo $farbe; ?>">&nbsp;<?php
					
		  
					echo '<b><a href="';
					echo $RSDB_intern_link_item.$result_sortby_b['comp_id'].'">';
					
					echo "ReactOS ".show_osversion($result_sortby_b['comp_osversion']);

					echo '</a></b>';

				 ?></td>
                  <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1">&nbsp;<img src="media/icons/awards/<?php echo Award::icon($result_sortby_b['comp_award']); ?>.gif" alt="<?php echo Award::name($result_sortby_b['comp_award']); ?>" width="16" height="16" /> <?php echo Award::name($result_sortby_b['comp_award']); ?></font></td>
                  <?php
			
				$counter_stars_install = 0;
				$counter_stars_function = 0;
				$counter_stars_user = 0;
				
				$stmt_sub=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
        $stmt_sub->bindParam('comp_id',$result_sortby_b['comp_id'],PDO::PARAM_STR);
        $stmt_sub->execute();
				while($result_count_stars = $stmt_sub->fetch(PDO::FETCH_ASSOC)) {
					$counter_stars_install += $result_count_stars['test_result_install'];
					$counter_stars_function += $result_count_stars['test_result_function'];
					$counter_stars_user++;
				}
							
			?>
                  <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1"><?php echo Star::drawNormal($counter_stars_function, $counter_stars_user, 5, "tests"); ?></font></td>
                  <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1"><?php echo Star::drawNormal($counter_stars_install, $counter_stars_user, 5, "tests"); ?></font></td>
                </tr>
                <?php
			}
?>
              </table>	
<?php 
	}
?>
</td>
		  </tr>
		</table>
		<br />
		<ul>
		  <li><a href="<?php echo $RSDB_intern_link_submit_appver; ?>"><strong><font size="3">Submit new &quot;<?php echo $result_page['grpentr_name']; ?>&quot; version</font></strong></a><br />
		  Use this function to add an application and ReactOS version entry. Then you will be able to submit a compatibility test report.</li>
	    </ul>
		<p>&nbsp;</p>

	<script type="text/javascript">
	<!--
	
		function setCursor(mode) {
		  var docBody = document.getElementsByTagName("body")[0];
		  docBody.style.cursor = mode;
		}	
	
	
		var xmlhttp=false;
		/* IE 5+ only: */
		/*@cc_on @*/
			/*@if (@_jscript_version >= 5)
				try {
					xmlhttp = new ActiveXObject("Msxml2.XMLHTTP");
				} catch (e) {
				try {
					xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
				} catch (E) {
					xmlhttp = false;
				}
			}
		@end @*/
		
		if (!xmlhttp && typeof XMLHttpRequest != 'undefined') {
			xmlhttp = new XMLHttpRequest();
		}
		
		function ajat_LoadText(serverPage, objID) {
			var obj = document.getElementById(objID);
			xmlhttp.open("GET", serverPage);
			xmlhttp.onreadystatechange = function() {
				setCursor('wait');
				if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
					setCursor('auto');
					obj.innerHTML = xmlhttp.responseText;
				}
			}
			xmlhttp.send(null);
		}
		
	//-->
	</script>

	<script language="JavaScript1.2">
	 	document.getElementById('moreinfo').style.display = "block";
	  
	  
		show_Versions('<?php if ($RSDB_SET_sort != "") { echo "Hide"; } else { echo "Show"; } ?> all Versions <?php if ($RSDB_SET_sort != "") { echo "&lt;&lt;"; } else { echo "&gt;&gt;"; } ?>');
		document.getElementById('versions').style.display = '<?php if ($RSDB_SET_sort != "") { echo "block"; } else { echo "none"; } ?>';
		var TOCstate1 = '<?php if ($RSDB_SET_sort != "") { echo "block"; } else { echo "none"; } ?>';
	

	
	
		function showhideVersions()
		{
			TOCstate1 = (TOCstate1 == 'none') ? 'block' : 'none';
			document.getElementById('versions').style.display = TOCstate1;
			if(TOCstate1 == 'none') {
				show_Versions('Show all Versions &gt;&gt;');
			}
			else {
				show_Versions('Hide all Versions &lt;&lt;');
			}
		}
	</script>
	
<?php
		} // end if {$result_page['grpentr_type'] == "default"}
	}
	include("inc/comp/comp_group_maintainer.php");
?>
		
