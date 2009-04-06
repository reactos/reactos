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
  $stmt->bindParam('comp_id',@$_GET['item'],PDO::PARAM_STR);
  $stmt->execute();
	
	$result_page = $stmt->fetch(PDO::FETCH_ASSOC);
	
if ($result_page['comp_id']) {

	echo "<h2>".$result_page['comp_name'] ." [". "ReactOS ".@show_osversion($result_page['comp_osversion']) ."]</h2>"; 
	
	include("inc/comp/comp_item_menubar.php");

	echo test_bar();
?>
<p align="center">The following reports are owned by whoever posted them. We are not responsible for them in any way. </p>
<p align="center"><a href="<?php echo $RSDB_intern_link_submit_comp_test; ?>add"><strong>Submit  Compatibility Test Report</strong></a></p>

<?php

	// Voting - update DB
	if (isset($_GET['vote']) && $_GET['vote'] != '' && isset($_GET['vote2']) && $_GET['vote2'] != '') {
		Star::addVote($_GET['vote'], $_GET['vote2'], "rsdb_item_comp_testresults", "test");
	}

	if ($RSDB_SET_order == "new") {
		$RSDB_TEMP_order = "DESC";
	}
	else {
		$RSDB_TEMP_order = "ASC";
	}


  $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp_testresults WHERE test_visible = '1' AND test_comp_id = :comp_id ORDER BY test_date ".$RSDB_TEMP_order);
  $stmt->bindParam('comp_id',@$_GET['item'],PDO::PARAM_STR);
  $stmt->execute();

	while($result_testreports = $stmt->fetch(PDO::FETCH_ASSOC)) {
		$number = $result_testreports['test_useful_vote_value'];
		$user = $result_testreports['test_useful_vote_user'];
		$result_stars = @($number / $user);
		$result_round_stars = round($result_stars, 0);
		
		if ( $result_round_stars >= $RSDB_SET_threshold || ($result_round_stars < $RSDB_SET_threshold && $result_testreports['test_useful_vote_user'] < 7)) {
			//echo "result_round_stars:".$result_round_stars." | RSDB_SET_threshold:".$RSDB_SET_threshold." | test_useful_vote_user:".$result_testreports['test_useful_vote_user'];
?>
    <table width="100%"  border="0" cellpadding="5" cellspacing="1">
      <tr>
        <td bgcolor="#E2E2E2"><table width="100%"  border="0" cellpadding="0" cellspacing="0" bordercolor="#5984C3">
            <tr>
              <td width="80" rowspan="2" align="center" valign="middle"><div align="left"><strong><?php 
			  
				if (usrfunc_IsAdmin($result_testreports['test_user_id'])) {
					echo '<font size="3" face="Arial, Helvetica, sans-serif" color="#5984C3">&nbsp;Admin';
				}
				else if (usrfunc_IsModerator($result_testreports['test_user_id'])) {
					echo '<font size="2" face="Arial, Helvetica, sans-serif" color="#5984C3">&nbsp;Maintainer';
				}
				else {
					echo '<font size="1" face="Arial, Helvetica, sans-serif" color="#5984C3">&nbsp;User';
				}

			  
			   ?></font></strong></div>                </td>
              <td valign="top"><p><strong><font size="4"><?php echo usrfunc_GetUsername($result_testreports['test_user_id']); ?></font></strong></p></td>
              <td width="300"><div align="left"><font size="1">
              <?php
			  
			  	$RSDB_TEMP_voting_history = strchr($result_testreports['test_useful_vote_user_history'],("|".$RSDB_intern_user_id."="));
				if ($RSDB_TEMP_voting_history == false) {
					echo "Rate this test: ";
					if ($result_testreports['test_useful_vote_user'] > $RSDB_setting_stars_threshold) {
						echo Star::drawVoteable($result_testreports['test_useful_vote_value'], $result_testreports['test_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_testreports['test_id']."&amp;vote2="));Star::drawVoteable($result_testreports['test_useful_vote_value'], $result_testreports['test_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_testreports['test_id']."&amp;vote2="));
					}
					else {
						echo Star::drawVoteable(0, 0, 5, "", ($RSDB_intern_link_item_item2_vote.$result_testreports['test_id']."&amp;vote2="));Star::drawVoteable($result_testreports['test_useful_vote_value'], $result_testreports['test_useful_vote_user'], 5, "", ($RSDB_intern_link_item_item2_vote.$result_testreports['test_id']."&amp;vote2="));
					}
				}
				else {
					echo "Rating: ";
					echo Star::drawNormal($result_testreports['test_useful_vote_value'], $result_testreports['test_useful_vote_user'], 5, "");
				}
			    ?>
              </font></div></td>
            </tr>
            <tr>
              <td valign="top"><font size="2">on <?php echo $result_testreports['test_date']; ?></font></td>
              <td><div align="left"><font size="1">
                <?php 
		
			if ($result_testreports['test_useful_vote_value']) {
				if ($result_testreports['test_useful_vote_user'] == "1") {
					echo "Only a few users has already rated, please rate!";
				}
				elseif ($result_testreports['test_useful_vote_user'] > "1" && $result_testreports['test_useful_vote_user'] <= "6") {
					echo $result_testreports['test_useful_vote_user']." users have already rated, please rate!";
				}
				else {
					echo round(($result_testreports['test_useful_vote_value'] / 5), 0)." of ".$result_testreports['test_useful_vote_user'];
					echo " registered user found this test useful.";
				}
			}
			else {
				echo "No one rated this test, please rate!";
			}
				
		?>
		      </font></div></td>
            </tr>
        </table></td>
      </tr>
      <tr>
        <td colspan="2" bgcolor="#EEEEEE">
          <?php 
		  	if ($result_testreports['test_com_version']) {
		  ?>
		  <p><font size="2"><strong>Application additional information</strong><br />
			  <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_com_version'], ENT_QUOTES))); ?></font></p>
          <?php 
		  	}
		  ?>
		  <p><font size="2"><strong>What works</strong><br />
		  <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_whatworks'], ENT_QUOTES))); ?></font></p>
		  <?php 
		  	if ($result_testreports['test_whatdoesntwork']) {
		  ?>
			  <p><font size="2"><strong>What doesn't</strong><br />
			  <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_whatdoesntwork'], ENT_QUOTES))); ?></font></p>
          <?php 
		  	}
		  	if ($result_testreports['test_whatnottested']) {
		  ?>
			  <p><font size="2"><strong>What has been tested and what not</strong><br />
		  <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_whatnottested'], ENT_QUOTES))); ?></font></p>
          <?php 
		  	}
		  	if ($result_testreports['test_user_comment']) {
		  ?>
			  <p><font size="2"><strong>Tester Comment</strong><br />
			  <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_user_comment'], ENT_QUOTES))); ?></font></p></td>
          <?php 
		  	}
		  ?>
      </tr>
      <tr>
        <td colspan="2" bgcolor="#E2E2E2"><table width="100%"  border="0" cellpadding="0" cellspacing="0" bordercolor="#5984C3">
          <tr>
            <td rowspan="2" valign="top"><p><font size="2"><strong>Conclusion</strong><br />
            <?php echo wordwrap(nl2br(htmlentities($result_testreports['test_conclusion'], ENT_QUOTES))); ?></font></p>			</td>
            <td width="70"><div align="right">Function: </div></td>
            <td width="110">&nbsp;<font size="1"><?php echo Star::drawNormal($result_testreports['test_result_function'], 1, 5, ""); ?></font></td>
          </tr>
          <tr>
            <td><div align="right">Install: </div></td>
            <td>&nbsp;<font size="1"><?php echo Star::drawNormal($result_testreports['test_result_install'], 1, 5, ""); ?></font></td>
          </tr>
        </table></td>
      </tr>
    </table>
<?php
	  		echo "<br />";
		}
		else {
			echo "<p><i>This test report is beneath your threshold!</i></p>";
		}
	}
}
?>
