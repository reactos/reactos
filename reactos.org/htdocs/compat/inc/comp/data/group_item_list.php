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

?>
	<base href="<?php echo $RSDB_intern_path_server.$RSDB_intern_path; ?>" />
    <table width="100%" border="0" >
      <tr>
        <td><div align="left"><span class="Stil2">Sort by:
                  <?php

				if ($RSDB_SET_sort == "item" || $RSDB_SET_sort == "") {
					if ($RSDB_ENV_ajax == "true") { ?>
						<b>Application version</b> | <a href="javascript://" onclick="ajat_LoadText('<?php echo $RSDB_intern_link_export; ?>grpitemlst&amp;sort=ros','group_version_list')">ReactOS version</a><?php 
					}
					else {
						echo "<b>Application version</b> | <a href='".$RSDB_intern_link_group_sort."ros#ver'>ReactOS version</a>";
					}

					$RSDB_intern_sortby_SQL_a_query = "SELECT * 
														FROM `rsdb_item_comp` 
														WHERE `comp_groupid` = " . $RSDB_SET_group . "
														ORDER BY `comp_appversion` DESC ;";
					
					
					$RSDB_intern_sortby_headline_field = "comp_appversion";
					$RSDB_intern_sortby_linkname_field = "comp_osversion";
				}
				if ($RSDB_SET_sort == "ros") {
					if ($RSDB_ENV_ajax == "true") { ?>
						<a href="javascript://" onclick="ajat_LoadText('<?php echo $RSDB_intern_link_export; ?>grpitemlst&amp;sort=item','group_version_list')">Application version</a> | <b>ReactOS version</b><?php 
					}
					else {
						echo "<a href='".$RSDB_intern_link_group_sort."item#ver'>Application version</a> | <b>ReactOS version</b>";
					}
					
					
					
					$RSDB_intern_sortby_headline_field = "comp_osversion";
					$RSDB_intern_sortby_linkname_field = "comp_appversion";
				}
				

		 ?>
        </span></div></td>
      </tr>
      <?php
	// Table
	$RSDB_intern_TEMP_version_saved_a = "";
  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id ORDER BY comp_osversion DESC");
  $stmt->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
  $stmt->bindParam();
	while($result_sortby_a = $stmt->fetch(PDO::FETCH_ASSOC)) {
		if ($result_sortby_a[$RSDB_intern_sortby_headline_field] != $RSDB_intern_TEMP_version_saved_a) {
		
				if ($RSDB_SET_sort == "item" || $RSDB_SET_sort == "") {
              $stmt_comp=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_appversion = :version ORDER BY comp_osversion DESC, comp_status ASC");
              $stmt_comp->bindParam('version',$result_sortby_a['comp_appversion'],PDO::PARAM_STR);
				}
				if ($RSDB_SET_sort == "ros") {
              $stmt_comp=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_groupid = :group_id AND comp_osversion = :version ORDER BY comp_appversion DESC, comp_status ASC");
              $stmt_comp->bindParam('version',$result_sortby_a['comp_osversion'],PDO::PARAM_STR);
														//echo $RSDB_intern_sortby_SQL_b_query;
				}
        $stmt_comp->bindParam('group_id',$RSDB_SET_group,PDO::PARAM_STR);
		
?>
      <tr>
        <td>
          <table width="100%">
            <tr bgcolor="#5984C3">
              <td colspan="5">
                <table width="100%" border="0" cellpadding="0" cellspacing="0">
                  <tr>
                    <td width="30%"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>&nbsp;
                            <?php
									
						if ($RSDB_SET_sort == "item" || $RSDB_SET_sort == "") {
							echo $result_sortby_a['comp_name']." (v. ";
							echo $result_sortby_a[$RSDB_intern_sortby_headline_field].")"; 
						}
						else {
							echo "ReactOS ".show_osversion($result_sortby_a[$RSDB_intern_sortby_headline_field]); 
						}
						
					 ?>
                    </strong></font></td>
                    <td width="20%"><div align="center" class="Stil4">Medal</div></td>
                    <td width="20%"><div align="center" class="Stil4">Function</div></td>
                    <td width="20%"><div align="center" class="Stil4">Install</div></td>
                    <td width="10%"><div align="center"><span class="Stil4">Status</span></div></td>
                  </tr>
                </table>
            </tr>
            <?php  
			// Table line
			$farbe1="#E2E2E2";
			$farbe2="#EEEEEE";
			$zaehler="0";
      $stmt_comp->execute();
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
              <td width="30%" bgcolor="<?php echo $farbe; ?>">&nbsp;
                  <?php
					
					/*if ($result_sortby_b['comp_status'] == "0") {
						echo "<b>";
					}*/
			  
					echo '<a href="';
					echo $RSDB_intern_link_item.$result_sortby_b['comp_id'].'">';
					
					if ($RSDB_SET_sort == "ros") {
						echo $result_sortby_b['comp_name']." (v. ";
						echo $result_sortby_b[$RSDB_intern_sortby_linkname_field].")";
					}
					else {
						echo "ReactOS ".show_osversion($result_sortby_b[$RSDB_intern_sortby_linkname_field]);
					}				

					echo '</a>';

					/*if ($result_sortby_b['comp_status'] == "0") {
						echo "</b>";
					}*/

				 ?></td>

              <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1">&nbsp;<img src="media/icons/awards/<?php echo draw_award_icon($result_sortby_b['comp_award']); ?>.gif" alt="<?php echo draw_award_name($result_sortby_b['comp_award']); ?>" width="16" height="16" /> <?php echo draw_award_name($result_sortby_b['comp_award']); ?></font></td>
              <?php
			
				$counter_stars_install = 0;
				$counter_stars_function = 0;
				$counter_stars_user = 0;
				
				$stmt_test=CDBConnection::getInstance()->prepare("SELECT SUM(test_result_install) AS install_sum, SUM(test_result_function) AS function_sum, COUNT(*) AS user_sum FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_comp_id ASC");
        $stmt_test->bindParam('comp_id',$result_sortby_b['comp_id'],PDO::PARAM_STR);
        $stmt_test->execute();
        $tmp=$stmt_test->fetchOnce(PDO::FETCH_ASSOC);

				$counter_stars_install += $tmp['install_sum'];
				$counter_stars_function += $tmp['function_sum'];
				$counter_stars_user += $tmp['user_sum'];

			?>
              <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1"><?php echo draw_stars($counter_stars_function, $counter_stars_user, 5, "tests"); ?></font></td>
              <td width="20%" bgcolor="<?php echo $farbe; ?>"><font size="1"><?php echo draw_stars($counter_stars_install, $counter_stars_user, 5, "tests"); ?></font></td>
              <td width="10%" bgcolor="<?php echo $farbe; ?>" <?php 
	
				$counter_testentries = 0;
				$counter_forumentries = 0;
				$counter_screenshots = 0;

        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id");
        $stmt_count->bindParam('comp_id',$result_sortby_b['comp_id'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_testentries = $stmt_count->fetchOnce(PDO::FETCH_NUM);
				$counter_testentries += $result_count_testentries[0];
				
				// Forum entries:
        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_item_comp_forum WHERE fmsg_visible = '1' AND fmsg_comp_id = :comp_id");
        $stmt_count->bindParam('comp_id',$result_sortby_b['comp_id'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_forumentries = $stmt_count->fetchOnce(PDO::FETCH_NUM);
				$counter_forumentries += $result_count_forumentries[0];

				// Screenshots:
        $stmt_count=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_object_media WHERE media_visible = '1' AND media_groupid = :group_id");
        $stmt_count->bindParam('group_id',$result_sortby_b['comp_media'],PDO::PARAM_STR);
        $stmt_count->execute();
				$result_count_screenshots = $stmt_count->fetchOnce(PDO::FETCH_ASSOC);;
				$counter_screenshots += $result_count_screenshots[0];

			/*switch ($result_sortby_b['comp_status']) {
				default:
				case 0:
					echo "<b>Release</b>";
					break;
				case 1:
					echo "<b>RC 1</b>";
					break;
				case 2:
					echo "<b>RC 2</b>";
					break;
				case 3:
					echo "<b>RC 3</b>";
					break;
				case 4:
					echo "<b>RC 4</b>";
					break;
				case 5:
					echo "<b>RC 5</b>";
					break;
				case 100:
					echo "<b>Unstable (SVN)</b>";
					break;
			}*/  
			   ?> title="<?php echo "Tests: ".$counter_testentries.", Forum entries: ".$counter_forumentries.", Screenshots: ".$counter_screenshots; ?>"><div align="center"><font size="1"><table width="100%" border="0" cellpadding="1" cellspacing="1">
            <tr>
              <td width="33%"><div align="center"><?php if ($counter_testentries > 0) { ?><a href="<?php echo $RSDB_intern_link_item_EX.$result_sortby_b['comp_id'].$RSDB_URI_slash2; ?>item2=tests"><img src="media/icons/info/test.gif" alt="Compatibility Test Report entries" border="0" width="13" height="13"></a><?php } else { echo "&nbsp;"; } ?></div></td>
              <td width="33%"><div align="center"><?php if ($counter_forumentries > 0) { ?><a href="<?php echo $RSDB_intern_link_item_EX.$result_sortby_b['comp_id'].$RSDB_URI_slash2; ?>item2=forum"><img src="media/icons/info/forum.gif" alt="Forum entries" border="0" width="13" height="13"></a><?php } else { echo "&nbsp;"; } ?></div></td>
              <td width="33%"><div align="center"><?php if ($counter_screenshots > 0) { ?><a href="<?php echo $RSDB_intern_link_item_EX.$result_sortby_b['comp_id'].$RSDB_URI_slash2; ?>item2=screens"><img src="media/icons/info/screenshot.gif" alt="Screenshots" border="0" width="13" height="13"></a><?php } else { echo "&nbsp;"; } ?></div></td>
            </tr>
          </table></font></div></td>            </tr>
            <?php
			}
?>
          </table>
      <tr>
        <td>
          <?php
			$RSDB_intern_TEMP_version_saved_a = $result_sortby_a[$RSDB_intern_sortby_headline_field];
		}
	}
?>
        </td>
      </tr>
    </table>
