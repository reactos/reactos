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


$query_page = mysql_query("SELECT * 
							FROM `rsdb_item_comp` 
							WHERE `comp_visible` = '1'
							AND `comp_id` = " . $RSDB_SET_item . "
							ORDER BY `comp_name` ASC") ;

$result_page = mysql_fetch_array($query_page);		

if ($result_page['comp_id']) {

echo "<h2>".$result_page['comp_name'] ." [ReactOS ".  @show_osversion($result_page['comp_osversion']) ."]</h2>"; 

if ($RSDB_intern_user_id <= 0) {
	please_register(); 
}
else {
	
	include('inc/tree/tree_item_menubar.php');
	
	echo "<h3>Submit Screenshot</h3>";

	$RSDB_TEMP_SUBMIT_valid = "no";
	$RSDB_TEMP_picgrpnr = 0;
	$RSDB_TEMP_picorder = 0;

	$RSDB_TEMP_txtdesc = "";
	$rem_adr = "";
	
	if (array_key_exists("txtdesc", $_POST)) $RSDB_TEMP_txtdesc=htmlspecialchars($_POST["txtdesc"]);
	
	if (array_key_exists('REMOTE_ADDR', $_SERVER)) $rem_adr=htmlspecialchars($_SERVER['REMOTE_ADDR']);


	include('inc/tools/upload_picture.php');



	if ($RSDB_TEMP_SUBMIT_valid == "yes") {
	
		if ($result_page['comp_media'] == 0) {
		
			$query_media_entry=mysql_query("SELECT * 
											FROM `rsdb_object_media` 
											ORDER BY `media_groupid` DESC   
											LIMIT 1;");	
			$result_media_entry = mysql_fetch_array($query_media_entry);
			
			$RSDB_TEMP_picgrpnr = $result_media_entry['media_groupid'];
			$RSDB_TEMP_picgrpnr++;
			
			$update_item_entry = "UPDATE `rsdb_item_comp` SET `comp_media` = '". $RSDB_TEMP_picgrpnr ."' WHERE `comp_id` = '". $result_page['comp_id'] ."' LIMIT 1 ;";
			mysql_query($update_item_entry);
			
			$RSDB_TEMP_picorder = 1;
		
		}
		else {
		
			$query_media_entry=mysql_query("SELECT * 
											FROM `rsdb_object_media` 
											WHERE `media_groupid` = '". $result_page['comp_media'] ."'
											AND `media_visible` = '1'
											ORDER BY `media_order` DESC  
											LIMIT 1;");	
			$result_media_entry = mysql_fetch_array($query_media_entry);
			
			$RSDB_TEMP_picgrpnr = $result_page['comp_media'];
			$RSDB_TEMP_picorder = $result_media_entry['media_order'];
			$RSDB_TEMP_picorder++;
		
		}
				
		$report_submit="INSERT INTO `rsdb_object_media` ( `media_id` , `media_groupid` , `media_visible` , `media_order` , `media_file` , `media_filetype` , `media_thumbnail` , `media_description` , `media_exif` , `media_date` , `media_user_id` , `media_user_ip`  ) 
						VALUES ('', '".mysql_real_escape_string($RSDB_TEMP_picgrpnr)."', '1', '".mysql_real_escape_string($RSDB_TEMP_picorder)."', '".mysql_real_escape_string($Tdbfile)."', 'picture', '".mysql_real_escape_string($Tdbfiletb)."', '".mysql_real_escape_string($RSDB_TEMP_txtdesc)."', '".mysql_real_escape_string($infoExif)."', NOW( ), '".mysql_real_escape_string($RSDB_intern_user_id)."', '".mysql_real_escape_string($rem_adr)."' );";
		$db_report_submit=mysql_query($report_submit);
		echo "<p><b>Your screenshot has been stored!</b></p>";
		echo "<p><b><a href=\"". $RSDB_intern_link_item_item2 ."screens\">View screenshots</a></b></p>";
		echo "<p><a href=\"". $RSDB_intern_link_submit_comp_screenshot ."add\">Submit new screenshot</a></p>";
		
		// Stats update:
		$update_stats_entry = "UPDATE `rsdb_stats` SET
								`stat_s_media` = (stat_s_media + 1) 
								WHERE `stat_date` = '". date("Y-m-d") ."' LIMIT 1 ;";
		mysql_query($update_stats_entry);
		
	}
	else {
?>

<form name="RSDB_comp_screenshot" enctype="multipart/form-data" method="post" action="<?php echo $RSDB_intern_link_submit_comp_screenshot; ?>submit">
<p>Upload a screenshot of <b><?php echo $result_page['comp_name']; ?></b> running in <b>ReactOS <?php echo @show_osversion($result_page['comp_osversion']); ?></b>.</p>
<p>&nbsp;</p>
<p><strong>Screenshot image:</strong><br> 
    <input name="file" type="file" size="30" maxlength="255">
    <em><font size="1">(allowed formats: jpg, png, gif; max. filesize: 250 KB; 4:3)</font></em> </p>
<p>
  <strong>Description:</strong><br> 
  <input name="txtdesc" type="text" id="txtdesc" size="60" maxlength="255">
</p>
<p>&nbsp;</p>
<p><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><em>Please do <strong>not submit screenshots</strong> from <strong>other applications</strong> and/or <strong>operating systems</strong>! </em></font></p>
<p>&nbsp;</p>
<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">By clicking &quot;Submit&quot; below you agree to be bound by the <a href="<?php echo $RSDB_intern_index_php; ?>?page=conditions" target="_blank">submit conditions</a>.</font></p>
<p>
	<input type="submit" name="Submit" value="Submit">
</p>
</form>
<?php
	}
	
}
}
?>