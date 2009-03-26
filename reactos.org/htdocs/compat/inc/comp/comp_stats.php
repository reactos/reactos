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
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_intern_code_view_name; ?></a> &gt; Database Statistics
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p>ReactOS Software and Hardware Compatibility Database</p>

<h1>Database Statistics</h1>
<h2>Database Statistics</h2>
<p>&nbsp;</p>
<h3>Records Database </h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Compatibility <strong></strong><strong>category</strong> entries:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT COUNT('cat_id')
												FROM `rsdb_categories`
												WHERE `cat_visible` = '1' 
												AND `cat_comp` = '1' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records[0]."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Compatibility <strong></strong><strong>group</strong> entries:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT COUNT('cat_id')
												FROM `rsdb_groups`
												WHERE `grpentr_visible` = '1' 
												AND `grpentr_comp` = '1' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records[0]."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Compatibility <strong></strong><strong>item </strong>entries:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
      <?php
		$query_date_entry_records=mysql_query("SELECT COUNT('comp_id')
												FROM `rsdb_item_comp`
												WHERE `comp_visible` = '1' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records[0]."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Compatibility <strong>test report</strong> entries:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
    <?php
		$query_date_entry_records=mysql_query("SELECT COUNT('test_id')
												FROM `rsdb_item_comp_testresults`
												WHERE `test_visible` = '1' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records[0]."</b>";

    ?>
</font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Compatibility <strong>forum</strong> entries:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
    <?php
		$query_date_entry_records=mysql_query("SELECT COUNT('fmsg_id')
												FROM `rsdb_item_comp_forum`
												WHERE `fmsg_visible` = '1' ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records[0]."</b>";

    ?>
</font></div></td>
  </tr>
</table>
<p>&nbsp;</p>
<h3>Submissions - Applications</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application / Driver</strong></font></div></td>
    <td width="30%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Vendor</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
  </tr>
 <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

	if (usrfunc_IsAdmin($RSDB_intern_user_id) == true) {
 
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_groups`
												WHERE `grpentr_visible` = '1' 
												AND `grpentr_comp` = '1'
												ORDER BY `grpentr_id` DESC 
												LIMIT 0 , 25 ;");	
	}
	else {
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_groups`
												WHERE `grpentr_visible` = '1' 
												AND `grpentr_comp` = '1'
												ORDER BY `grpentr_id` DESC 
												LIMIT 0 , 5 ;");	
	}
	
	while($result_date_entry_records = mysql_fetch_array($query_date_entry_records)) {
?>
  <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
    <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif"><?php
		echo $result_date_entry_records['grpentr_date'];

    ?></font></div></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php
		echo "<b><a href=\"". $RSDB_intern_link_group_comp.$result_date_entry_records['grpentr_id']."\">".$result_date_entry_records['grpentr_name']."</a></b>";

    ?>
</font></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php
 		$query_date_vendor=mysql_query("SELECT *
												FROM `rsdb_item_vendor`
												WHERE `vendor_id` = '". mysql_escape_string($result_date_entry_records['grpentr_vendor']) ."' 
												LIMIT 1 ;");	
		$result_date_vendor = mysql_fetch_array($query_date_vendor);
		echo "<a href=\"". $RSDB_intern_link_vendor_sec_comp.$result_date_vendor['vendor_id'] ."\">".$result_date_vendor['vendor_name']."</a>";
		?>
</font></td>
    <td title="<?php echo $result_date_entry_records['grpentr_usrid']; ?>"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".usrfunc_GetUsername($result_date_entry_records['grpentr_usrid'])."</b>";

    ?>
    </font></div></td>
  </tr>
<?php 
	}
?>
</table>
<p>&nbsp;</p>
<h3>Submissions - Application Versions Data</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="30%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>ReactOS</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
  </tr>
 <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

	if (usrfunc_IsAdmin($RSDB_intern_user_id) == true) {
 
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_item_comp`
												WHERE `comp_visible` = '1' 
												ORDER BY `comp_id` DESC 
												LIMIT 0 , 25 ;");	
	}
	else {
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_item_comp`
												WHERE `comp_visible` = '1' 
												ORDER BY `comp_id` DESC 
												LIMIT 0 , 5 ;");	
	}
	
	while($result_date_entry_records = mysql_fetch_array($query_date_entry_records)) {
?>
  <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
    <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif"><?php
		echo $result_date_entry_records['comp_date'];

    ?></font></div></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_entry_records['comp_id']."\">".$result_date_entry_records['comp_name']."</a></b>";

    ?>
</font></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
&nbsp;
<?php
		echo "ReactOS ".@show_osversion($result_date_entry_records['comp_osversion']);

    ?>
    </font></td>
    <td title="<?php echo $result_date_entry_records['comp_usrid']; ?>"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".usrfunc_GetUsername($result_date_entry_records['comp_usrid'])."</b>";

    ?>
    </font></div></td>
  </tr>
<?php 
	}
?>
</table>
<p>&nbsp;</p>
<h3>Submissions - Compatibility Test Report </h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="25%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Function</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong> Install </strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
  </tr>
 <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

	if (usrfunc_IsAdmin($RSDB_intern_user_id) == true) {
 
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_item_comp_testresults`
												WHERE `test_visible` = '1' 
												ORDER BY `test_id` DESC 
												LIMIT 0 , 25 ;");	
	}
	else {
 		$query_date_entry_records=mysql_query("SELECT *
												FROM `rsdb_item_comp_testresults`
												WHERE `test_visible` = '1' 
												ORDER BY `test_id` DESC 
												LIMIT 0 , 5 ;");	
	}
	
	while($result_date_entry_records = mysql_fetch_array($query_date_entry_records)) {
?>
  <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
    <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
      <?php
		echo $result_date_entry_records['test_user_submit_timestamp'];

    ?>
    </font></div></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php
 		$query_date_vendor=mysql_query("SELECT *
										FROM `rsdb_item_comp`
										WHERE `comp_id` = '". mysql_escape_string($result_date_entry_records['test_comp_id']) ."' 
										LIMIT 1 ;");	
		$result_date_vendor = mysql_fetch_array($query_date_vendor);
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=tests\">".$result_date_vendor['comp_name']."</a></b>";

    ?>
</font></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
  <?php 
		echo draw_stars_small($result_date_entry_records['test_result_function'], 1, 5, "");
	?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
&nbsp;
  <?php
		echo draw_stars_small($result_date_entry_records['test_result_install'], 1, 5, "");
    ?>
    </font></div></td>
    <td title="<?php echo $result_date_entry_records['test_user_id']; ?>"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".usrfunc_GetUsername($result_date_entry_records['test_user_id'])."</b>";

    ?>
    </font></div></td>
  </tr>
<?php 
	}
?>
</table>
<p>&nbsp;</p>
<h3>Submissions - Forum entries </h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="40%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Topic</strong></font></div></td>
    <td width="25%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
  </tr>
 <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

 
	$query_date_entry_records=mysql_query("SELECT *
											FROM `rsdb_item_comp_forum`
											WHERE `fmsg_visible` = '1' 
											ORDER BY `fmsg_id` DESC 
											LIMIT 0 , 25 ;");	
	while($result_date_entry_records = mysql_fetch_array($query_date_entry_records)) {
?>
  <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
    <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif"><?php
		echo $result_date_entry_records['fmsg_date'];

    ?></font></div></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php

 		$query_date_vendor=mysql_query("SELECT *
										FROM `rsdb_item_comp`
										WHERE `comp_id` = '". mysql_escape_string($result_date_entry_records['fmsg_comp_id']) ."' 
										LIMIT 1 ;");	
		$result_date_vendor = mysql_fetch_array($query_date_vendor);

		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=forum&amp;fstyle=fthreads&amp;msg=". mysql_escape_string($result_date_entry_records['fmsg_id']) ."\">".$result_date_entry_records['fmsg_subject']."</a></b>";

    ?>
</font></td>
    <td><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
      <?php
		echo "<a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=tests\">".$result_date_vendor['comp_name']."</a>";

    ?>
    </font></div></td>
    <td title="<?php echo $result_date_entry_records['fmsg_user_id']; ?>"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".usrfunc_GetUsername($result_date_entry_records['fmsg_user_id'])."</b>";

    ?>
    </font></div></td>
  </tr>
<?php 
	}
?>
</table>
<p>&nbsp;</p>
<h3>Submissions - Screenshots</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="40%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Description</strong></font></div></td>
    <td width="25%"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="20%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>User</strong></font></div></td>
  </tr>
 <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

 
	$query_date_entry_records=mysql_query("SELECT *
											FROM `rsdb_object_media`
											WHERE `media_visible` = '1' 
											ORDER BY `media_id` DESC 
											LIMIT 0 , 25 ;");	
	while($result_date_entry_records = mysql_fetch_array($query_date_entry_records)) {
?>
  <tr bgcolor="<?php
									$cellcolorcounter++;
									if ($cellcolorcounter == "1") {
										echo $cellcolor1;
										$farbe = $cellcolor1;
									}
									elseif ($cellcolorcounter == "2") {
										$cellcolorcounter="0";
										echo $cellcolor2;
										$farbe = $cellcolor2;
									}
								 ?>">
    <td><div align="center"><font size="1" face="Arial, Helvetica, sans-serif">
      <?php
		echo $result_date_entry_records['media_date'];

    ?>
    </font></div></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">
      &nbsp;
      <?php
	  
 		$query_date_vendor=mysql_query("SELECT * 
										FROM `rsdb_item_comp` 
										WHERE `comp_media` = '". mysql_escape_string($result_date_entry_records['media_groupid']) ."' 
										LIMIT 1 ;");	
		$result_date_vendor = mysql_fetch_array($query_date_vendor);
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=screens&amp;entry=". mysql_escape_string($result_date_entry_records['media_id']) ."\">".htmlentities($result_date_entry_records['media_description'])."</a></b>";

    ?>
</font></td>
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
          <?php
		echo "<a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=screens\">".$result_date_vendor['comp_name']."</a>";

    ?>
    </font></td>
    <td title="<?php echo $result_date_entry_records['media_user_id']; ?>"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".usrfunc_GetUsername($result_date_entry_records['media_user_id'])."</b>";

    ?>
    </font></div></td>
  </tr>
<?php 
	}
?>
</table>
<p>&nbsp;</p>

<?php 
	if (usrfunc_IsAdmin($RSDB_intern_user_id) == true) {
?>
<h3>Records Today (<?php

		$query_date_entry_records=mysql_query("SELECT stat_date, stat_pviews, stat_visitors, stat_users, stat_s_cat, stat_s_grp, stat_s_icomp, stat_s_ictest, stat_s_icbb, stat_s_icvotes, stat_s_media, stat_s_votes, stat_vislst, stat_usrlst, stat_brow_IE, stat_brow_MOZ, stat_brow_OPERA, stat_brow_KHTML, stat_brow_text, stat_brow_other, stat_os_winnt, stat_os_ros, stat_os_unix, stat_os_bsd, stat_os_linux, stat_os_mac, stat_os_other, stat_reflst 
												FROM `rsdb_stats` 
												ORDER BY `stat_date` DESC 
												LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);

		 echo $result_date_entry_records['stat_date'];
 
  ?>)</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Page views</strong>:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_pviews']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Unique visitors</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_visitors']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Unique registered users</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_users']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted categories</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_cat']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted groups</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_grp']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted comaptibility items</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icomp']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted compatibility tests</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_ictest']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted comaptibility forum entries</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icbb']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted media files</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_media']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td height="21"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Compatibility votes</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icvotes']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Votes total</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_votes']."</b>";

    ?>
    </font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Internet Explorer</strong> users:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_IE']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Mozilla</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_MOZ']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Opera</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_OPERA']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Konqueror / Safari</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_KHTML']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>text browser</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_text']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>other browser</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
    <?php
		echo "<b>".$result_date_entry_records['stat_brow_other']."</b>";

    ?>
</font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Windows / ReactOS</strong> users:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_winnt']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>ReactOS</strong> (if the browser detect it) users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_ros']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Unix</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_unix']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>BSD</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_bsd']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Linux</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_linux']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Mac</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_mac']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>other operating system</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_other']."</b>";

    ?>
    </font></div></td>
  </tr>
</table>
<p>&nbsp;</p>
<?php
	}
?>
<h3>Records Yesterday (<?php

		$query_date_entry_records=mysql_query("SELECT stat_date, stat_pviews, stat_visitors, stat_users, stat_s_cat, stat_s_grp, stat_s_icomp, stat_s_ictest, stat_s_icbb, stat_s_icvotes, stat_s_media, stat_s_votes, stat_vislst, stat_usrlst, stat_brow_IE, stat_brow_MOZ, stat_brow_OPERA, stat_brow_KHTML, stat_brow_text, stat_brow_other, stat_os_winnt, stat_os_ros, stat_os_unix, stat_os_bsd, stat_os_linux, stat_os_mac, stat_os_other, stat_reflst 
												FROM `rsdb_stats` 
												ORDER BY `stat_date` DESC 
												LIMIT 1, 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);

		 echo $result_date_entry_records['stat_date'];
 
  ?>)</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Page views</strong>:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_pviews']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif"><strong>&nbsp;Unique visitors</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_visitors']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Unique registered users</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_users']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted categories</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_cat']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted groups</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_grp']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted comaptibility items</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icomp']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted compatibility tests</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_ictest']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted comaptibility forum entries</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icbb']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Submitted media files</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_media']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td height="21"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Compatibility votes</strong>:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_icvotes']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Votes total</strong>:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_s_votes']."</b>";

    ?>
    </font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Internet Explorer</strong> users:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_IE']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Mozilla</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_MOZ']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Opera</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_OPERA']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Konqueror / Safari</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_KHTML']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>text browser</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_brow_text']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>other browser</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
    <?php
		echo "<b>".$result_date_entry_records['stat_brow_other']."</b>";

    ?>
</font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Windows / ReactOS</strong> users:</font></td>
    <td width="35%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_winnt']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>ReactOS</strong> (if the browser detect it) users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_ros']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Unix</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_unix']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>BSD</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_bsd']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Linux</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_linux']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>Mac</strong> users:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_mac']."</b>";

    ?>
    </font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;<strong>other operating system</strong> users:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		echo "<b>".$result_date_entry_records['stat_os_other']."</b>";

    ?>
    </font></div></td>
  </tr>
</table>
<p>&nbsp;</p>
<h3>Records per Day</h3>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>page views</strong> ever:</font></td>
    <td width="15%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
      <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_pviews
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_pviews` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_pviews']."</b>";

    ?>
    </font></div></td>
    <td width="20%" bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>unique visitors</strong> ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_visitors
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_visitors` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_visitors']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>unique registered users</strong> ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_users
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_users` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_users']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted categories</strong> ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_cat
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_cat` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_cat']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted groups</strong> ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_grp
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_grp` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_grp']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted comaptibility items</strong> ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_icomp
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_icomp` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_icomp']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted compatibility tests</strong> ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_ictest
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_ictest` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_ictest']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted comaptibility forum entries</strong> ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_icbb
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_icbb` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_icbb']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>submitted media files</strong> ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_media
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_media` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_media']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td height="21"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>compatibility votes</strong> ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_icvotes
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_icvotes` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_icvotes']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong> votes total</strong> ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_s_votes
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_s_votes` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_s_votes']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Internet Explorer</strong> users ever:</font></td>
    <td width="15%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_IE
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_IE` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_IE']."</b>";

    ?>
    </font></div></td>
    <td width="20%" bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Mozilla</strong> users ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_MOZ
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_MOZ` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_MOZ']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Opera</strong> users ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_OPERA
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_OPERA` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_OPERA']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Konqueror / Safari</strong> users ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_KHTML
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_KHTML` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_KHTML']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>text browser</strong> users ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_text
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_text` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_text']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>other browser</strong> users ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_brow_other
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_brow_other` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_brow_other']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
</table>
<br>
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="65%" bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Windows / ReactOS</strong> users ever:</font></td>
    <td width="15%" bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_winnt
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_winnt` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_winnt']."</b>";

    ?>
    </font></div></td>
    <td width="20%" bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>ReactOS</strong> (if the browser detect it) users  ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_ros
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_ros` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_ros']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Unix</strong> users ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_unix
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_unix` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_unix']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>BSD</strong> users ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_bsd
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_bsd` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_bsd']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Linux</strong> users ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_linux
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_linux` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_linux']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#EEEEEE">
    <td><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>Mac</strong> users ever:</font></td>
    <td><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_mac
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_mac` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_mac']."</b>";

    ?>
    </font></div></td>
    <td><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
  <tr bgcolor="#5984C3">
    <td bgcolor="#E2E2E2"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;Most <strong>other operating system</strong> users ever:</font></td>
    <td bgcolor="#E2E2E2"><div align="right"><font size="2" face="Arial, Helvetica, sans-serif">
        <?php
		$query_date_entry_records=mysql_query("SELECT stat_date, stat_os_other
										FROM `rsdb_stats`
										WHERE 1 
										ORDER BY `stat_os_other` DESC 
										LIMIT 1 ;");	
		$result_date_entry_records = mysql_fetch_array($query_date_entry_records);
		echo "<b>".$result_date_entry_records['stat_os_other']."</b>";

    ?>
    </font></div></td>
    <td bgcolor="#E2E2E2"><div align="center"><font size="2" face="Arial, Helvetica, sans-serif"><?php echo $result_date_entry_records['stat_date']; ?></font></div></td>
  </tr>
</table>
<p>&nbsp;</p>
