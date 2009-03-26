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

 echo $RSDB_intern_code_view_name; ?>
<script language="JavaScript" type="text/JavaScript">
<!--
	function MM_preloadImages() { //v3.0
	  var d=document; if(d.images){ if(!d.MM_p) d.MM_p=new Array();
		var i,j=d.MM_p.length,a=MM_preloadImages.arguments; for(i=0; i<a.length; i++)
		if (a[i].indexOf("#")!=0){ d.MM_p[j]=new Image; d.MM_p[j++].src=a[i];}}
	}
	
	function MM_swapImgRestore() { //v3.0
	  var i,x,a=document.MM_sr; for(i=0;a&&i<a.length&&(x=a[i])&&x.oSrc;i++) x.src=x.oSrc;
	}
	
	function MM_findObj(n, d) { //v4.01
	  var p,i,x;  if(!d) d=document; if((p=n.indexOf("?"))>0&&parent.frames.length) {
		d=parent.frames[n.substring(p+1)].document; n=n.substring(0,p);}
	  if(!(x=d[n])&&d.all) x=d.all[n]; for (i=0;!x&&i<d.forms.length;i++) x=d.forms[i][n];
	  for(i=0;!x&&d.layers&&i<d.layers.length;i++) x=MM_findObj(n,d.layers[i].document);
	  if(!x && d.getElementById) x=d.getElementById(n); return x;
	}
	
	function MM_swapImage() { //v3.0
	  var i,j=0,x,a=MM_swapImage.arguments; document.MM_sr=new Array; for(i=0;i<(a.length-2);i+=3)
	   if ((x=MM_findObj(a[i]))!=null){document.MM_sr[j++]=x; if(!x.oSrc) x.oSrc=x.src; x.src=a[i+2];}
	}
//-->
</script>
<style type="text/css">
<!--
.Stil1 {
	font-size: 135%;
	font-weight: bold;
}
-->
</style><body onLoad="MM_preloadImages('media/icons/buttons/button_comp_search_m.jpg','media/icons/buttons/button_comp_category_m.jpg','media/icons/buttons/button_comp_name_m.jpg','media/icons/buttons/button_comp_vendor_m.jpg','media/icons/buttons/button_comp_rank_m.jpg','media/icons/buttons/button_comp_submit_m.jpg')">
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p>ReactOS Software and Hardware Compatibility Database</p>

<h1>Compatibility Database - Overview </h1>
<h2>Compatibility Database - Overview</h2>
<table width="100%" border="0" cellpadding="0" cellspacing="0">
  <tr>
    <td width="100%" valign="top">
<p><font size="2">The ReactOS Compatibility Database (CompDB) has stored a lot of information about application and driver compatibility 
  with ReactOS.</font></p>
<div id="StartList" align="center" style="display: none">
<a href="<?php echo $RSDB_intern_link_db_sec; ?>category&amp;cat=0&amp;ajax=true" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('CompCategory','','media/icons/buttons/button_comp_category_m.jpg',1)"><img src="media/icons/buttons/button_comp_category.jpg" alt="Browse by Category" name="CompCategory" width="232" height="50" border="0"></a><br>
<a href="<?php echo $RSDB_intern_link_db_sec; ?>name&amp;letter=all&amp;ajax=true" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('CompName','','media/icons/buttons/button_comp_name_m.jpg',1)"><img src="media/icons/buttons/button_comp_name.jpg" alt="Browse by Name" name="CompName" width="232" height="50" border="0"></a><br>
<a href="<?php echo $RSDB_intern_link_db_sec; ?>vendor&amp;letter=all&amp;ajax=true" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('CompVendor','','media/icons/buttons/button_comp_vendor_m.jpg',1)"><img src="media/icons/buttons/button_comp_vendor.jpg" alt="Browse by Vendor" name="CompVendor" width="232" height="50" border="0"></a><br>
<a href="<?php echo $RSDB_intern_link_db_sec; ?>rank&amp;ajax=true" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('CompRank','','media/icons/buttons/button_comp_rank_m.jpg',1)"><img src="media/icons/buttons/button_comp_rank.jpg" alt="Browse by Rank" name="CompRank" width="232" height="50" border="0"></a><br>
<a href="<?php echo $RSDB_intern_link_db_sec; ?>submit&amp;ajax=true" onMouseOut="MM_swapImgRestore()" onMouseOver="MM_swapImage('SubApp','','media/icons/buttons/button_comp_submit_m.jpg',1)"><img src="media/icons/buttons/button_comp_submit.jpg" alt="Submit Application" name="SubApp" width="232" height="50" border="0"></a><br>
  <table width="231" height="49" border="0" cellpadding="0" cellspacing="0">
      <tr>
        <td width="231" height="49" align="right" valign="middle" nowrap background="media/icons/buttons/button_comp_search.jpg">
	    <table width="100%"  border="0">
          <tr>
            <td width="110">&nbsp;</td>
            <td><input name="searchinput" type="text" id="searchinput" tabindex="0" onBlur="loadItemList(this.value,'table','comp','ajaxload','sresult')" onKeyUp="loadItemList(this.value,'table','comp','ajaxload','sresult')" size="10" maxlength="50" style="background-color: #FFFFFF; color: #000000; font-family: Verdana; font-size: x-small; font-style: normal; border-left : 1px solid #FFFFFF; border-right : 1px solid #FFFFFF; border-top : 1px solid #FFFFFF; border-bottom : 1px solid #FFFFFF;" /><img id="ajaxload" src="images/ajax_loading.gif"  style="display: none">
			<script language="JavaScript" type="text/JavaScript">
				<!--
					var brow = navigator.appName;
					if (brow=="Netscape") {
						document.getElementById('searchinput').size=15;
					}
				-->
			</script>
			<?php
				//include("inc/comp/sub/search_box.php");
			?>
		</td>
          </tr>
        </table>
        </td>
      </tr>
  </table>
</div>
<div align="center">
	<noscript>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>category&amp;cat=0&amp;ajax=false"><img src="media/icons/buttons/button_comp_category.jpg" alt="Browse by Category" name="CompCategory" width="232" height="50" border="0"></a><br>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>name&amp;letter=all&amp;ajax=false"><img src="media/icons/buttons/button_comp_name.jpg" alt="Browse by Name" name="CompName" width="232" height="50" border="0"></a><br>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>vendor&amp;letter=all&amp;ajax=false"><img src="media/icons/buttons/button_comp_vendor.jpg" alt="Browse by Vendor" name="CompVendor" width="232" height="50" border="0"></a><br>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>rank&amp;ajax=false"><img src="media/icons/buttons/button_comp_rank.jpg" alt="Browse by Rank" name="CompRank" width="232" height="50" border="0"></a><br>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>submit&amp;ajax=false"><img src="media/icons/buttons/button_comp_submit.jpg" alt="Submit Application" name="SubApp" width="232" height="50" border="0"></a><br>
		<a href="<?php echo $RSDB_intern_link_db_sec; ?>search&amp;ajax=false"><img src="media/icons/buttons/button_comp_search_noscript.jpg" alt="Search by Query" name="CompSearch" width="232" height="50" border="0"></a><br>
	</noscript>
</div>
</td>
    <td align="right" valign="top"><img src="media/pictures/compatibility.jpg" alt="ReactOS Compatibility Database" height="300" width="400"></td>
  </tr>
</table>
<div id="sresult" style="display: none"></div>
<h3>Features</h3>
<p>Some of the <strong>features of the Compatibility Database</strong> are:</p>
<ul>
  <li>Browse through the database in several different ways (by <a href="<?php echo $RSDB_intern_link_db_sec; ?>category&amp;cat=0">category</a>, <a href="<?php echo $RSDB_intern_link_db_sec; ?>name&amp;letter=all">name</a>, <a href="<?php echo $RSDB_intern_link_db_sec; ?>vendor&amp;letter=all">vendor</a>, <a href="<?php echo $RSDB_intern_link_db_sec; ?>rank">ranks</a>, <a href="<?php echo $RSDB_intern_link_db_sec; ?>search">search</a>).</li>
  <li>Submit application entries, compatibility test reports, vendor information, screenshots and forum messages. </li>
  <li>Ability to vote on test reports, forum messages, screenshots, etc.</li>
  <li>Ability to customize the layout, several settings (like personal threshold, etc.) and behaviour of the Support Database.</li>
  <li>Ability to sign up to be an application maintainer.<br />
</li>
</ul>
<h3>Recent submissions<font color="#B5E196"></font></h3>
<p>There are <a href="<?php echo $RSDB_intern_link_db_sec; ?>stats"><b> 
  <?php

	$query_count_cat=mysql_query("SELECT COUNT('cat_id')
							FROM `rsdb_groups`
							WHERE `grpentr_visible` = '1' 
							AND `grpentr_comp` = '1' ;");	
	$result_count_cat = mysql_fetch_row($query_count_cat);
	echo $result_count_cat[0];


?> applications and drivers</b></a> currently in the database.</p>

<div style="margin:0; margin-top:10px; width:520px; margin-right:10px; border:1px solid #dfdfdf; padding:0em 1em 1em 1em; background-color:#EAF0F8;">
<br />
<table width="500" border="0" cellpadding="1" cellspacing="1">
  <tr bgcolor="#5984C3">
    <td width="15%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Time</strong></font></div></td>
    <td width="50%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Application</strong></font></div></td>
    <td width="35%" bgcolor="#5984C3"><div align="center"><font color="#FFFFFF" face="Arial, Helvetica, sans-serif"><strong>Function</strong></font></div></td>
  </tr>
  <?php 
		$cellcolor1="#E2E2E2";
		$cellcolor2="#EEEEEE";
		$cellcolorcounter="0";

	$query_date_entry_records=mysql_query("SELECT *
											FROM `rsdb_item_comp_testresults`
											WHERE `test_visible` = '1' 
											ORDER BY `test_id` DESC 
											LIMIT 0 , 5 ;");	
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
    <td><font size="2" face="Arial, Helvetica, sans-serif"> &nbsp;
          <?php
 		$query_date_vendor=mysql_query("SELECT *
										FROM `rsdb_item_comp`
										WHERE `comp_id` = '". mysql_escape_string($result_date_entry_records['test_comp_id']) ."' 
										LIMIT 1 ;");	
		$result_date_vendor = mysql_fetch_array($query_date_vendor);
		echo "<b><a href=\"". $RSDB_intern_link_item_comp.$result_date_vendor['comp_id'] ."&amp;item2=tests\">".$result_date_vendor['comp_name']."</a></b>";

    ?>
    </font></td>
    <td><div align="left"><font size="2" face="Arial, Helvetica, sans-serif">&nbsp;
              <?php 
		echo draw_stars_small($result_date_entry_records['test_result_function'], 1, 5, "");
	?>
    </font></div></td>
  </tr>
  <?php 
	}
?>
</table>
</div>
<?php
	if ($RSDB_intern_user_id <= 0) {
?>
	<p><font size="2">Some of the features of the ReactOS Compatibility Database require that you have a <a href="<?php echo $RSDB_intern_loginsystem_fullpath; ?>?page=register">myReactOS account</a> and are <a href="<?php echo $RSDB_intern_loginsystem_fullpath; ?>?page=login">logged in</a>.</font></p>
<?php
	}
?>
<script language="JavaScript" type="text/JavaScript">
<!--
	document.getElementById('StartList').style.display = "block";
	document.getElementById('searchinput').focus();
	document.getElementById('searchinput').select();
-->
</script>