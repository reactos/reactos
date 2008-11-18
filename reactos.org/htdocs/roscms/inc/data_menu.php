<?php
    /*
    RosCMS - ReactOS Content Management System
    Copyright (C) 2005  Klemens Friedl <frik85@reactos.org>

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
?>
<style type="text/css">
<!--

	.roscms_page {
		padding-bottom:		1ex;
		padding-left:		1ex;
		padding-right:		1ex;
		padding-top:		1ex;
	}

	#myReactOS {
		text-align:			right;
	}
	
	.tc1 {
		background:transparent url(images/corner_tl_sharp.gif) no-repeat scroll left top;
		height:4px;
	}
	.tc2 {
		background:transparent url(images/corner_tr_sharp.gif) no-repeat scroll right top;
		height:4px;
		padding:1px 6px;
	}
	.tc3 {
		cursor:pointer;
		font-weight:bold;
		text-align:center;
	}
	
	table#mt th {
		background-color:#E5ECF9;
		cursor:pointer;
		font-weight:bold;
		text-align:center;
	}	
	
	table#mt div.tblbl {
		font-family:arial,sans-serif;
		font-size:small;
		padding:0px 1em 2px;
	}
	
	#mt {
		margin-top:0.5ex;
	}
	
	table {
		border-collapse:separate;
	}
	
	table#mt div.tblbl a {
		color: #000000;
		text-decoration: none;
	}
	 
	table#mt div.tblbl a:hover {
		color: #000000;
		background-color: #E5ECF9;
		text-decoration: none;
	}
	
	table#mt th.int2 {
		background-color: #C9DAF8;
	}	
	
	table#mt th.int2 a:hover {
		color: #000000;
		background-color: #C9DAF8;
	}	
	
	.submenu {
		margin-left: -6px; /* fix FF glitch */
		margin-right: -5.5px; /* fix FF glitch */
		padding-top: 4px;
		padding-right: 3px;
		padding-bottom: 3px;
		padding-left: 4px;
		background-color: #C9DAF8;
		background-image: none;
		background-repeat: repeat;
		background-attachment: scroll;
		background-x-position: 0%;
		background-y-position: 0%;
	}
		
-->
</style>
<?php

	global $roscms_security_level;
	global $roscms_security_memberships;
	global $roscms_intern_script_name;
	global $RosCMS_GET_branch;
	global $rpm_page;
	
	if (array_key_exists("branch", $_GET)) $RosCMS_GET_branch=htmlspecialchars($_GET["branch"]);

	$curmenu = "";

	switch ($rpm_page) {
		default:
		/*case "home";
			$curmenu = "welcome";
			break;*/
		case "data";
			switch ($RosCMS_GET_branch) {
				default:
				case "website";
					$curmenu = "website";
					break;
				case "welcome";
					$curmenu = "welcome";
					break;
				case "reactos";
					$curmenu = "reactos";
					break;
				case "user";
					$curmenu = "user";
					break;
				case "maintain";
					$curmenu = "maintain";
					break;
				case "stats";
					$curmenu = "stats";
					break;
			}
			break;
		/*case "user";
			$curmenu = "user";
			break;	*/
	}

?>
<script type="text/javascript" language="javascript">
<!--

	function pagerefresh() {
		exitmsg = '';
		window.location.href = '<?php echo $roscms_intern_page_link."data&branch=".$RosCMS_GET_branch; ?>';
	}

	function roscms_mainmenu(temp_page) {
		exitmsg = '';
		
		temp_page2 = '';
		
		switch (temp_page) {
			case 'welcome':
				temp_page2 = 'data&branch=welcome';
				break;
			case 'website':
				temp_page2 = 'data&branch=website';
				break;
			case 'rost':
				temp_page2 = 'data&branch=reactos';
				break;
			case 'user':
				temp_page2 = 'data&branch=user';
				break;
			case 'maintain':
				temp_page2 = 'data&branch=maintain';
				break;
			case 'stats':
				temp_page2 = 'data&branch=stats';
				break;
		}
		
		window.location.href = '<?php echo $roscms_intern_page_link; ?>'+temp_page2;
	}


-->
</script>
<div id="myReactOS" style="padding-right: 10px;">
	<b><?php echo $roscms_intern_login_check_username; ?></b>
	<?php
	
		if ($roscms_security_level > 1) {
			echo "| SecLev: ". $roscms_security_level." (". str_replace("|", ", ", substr($roscms_security_memberships, 1, strlen($roscms_security_memberships)-2)) .")";
		} 
	?>
	| <span onclick="pagerefresh()" style="color:#006090; cursor:pointer;"><img src="images/reload.gif" alt="reload page" width="16" height="16" /> <span style="text-decoration:underline;">reload</span></span>  
	| <a href="<?php echo $roscms_intern_page_link; ?>logout">Sign out</a></div>
<div class="roscms_page">
	<table id="mt" border="0" cellpadding="0" cellspacing="0" width="100%">
	  <tbody>
		<tr>
		  <th class="int<?php if ($curmenu == "welcome") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('welcome')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">Welcome</div></th>
		  <td>&nbsp;&nbsp;</td>
		  
		  <th class="int<?php if ($curmenu == "website") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('website')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">Website</div></th>
		  <td>&nbsp;&nbsp;</td>

		  <?php if ($roscms_security_level > 1) { ?>
		  <th class="int<?php if ($curmenu == "reactos") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('rost')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">ReactOS</div></th>
		  <td>&nbsp;&nbsp;</td>
		  <?php } ?>
		  
		  <?php if ((roscms_security_grp_member("transmaint") || roscms_security_grp_member("ros_admin") || roscms_security_grp_member("ros_sadmin"))) { ?>
		  <th class="int<?php if ($curmenu == "user") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('user')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">User</div></th>
		  <td>&nbsp;&nbsp;</td>
		  <?php } ?>

		  <?php if ($roscms_security_level == 3) { ?>
		  <th class="int<?php if ($curmenu == "maintain") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('maintain')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">Maintain</div></th>
		  <td>&nbsp;&nbsp;</td>
		  <th class="int<?php if ($curmenu == "stats") { echo "2"; } else { echo "1"; } ?>" onclick="roscms_mainmenu('stats')"> <div class="tc1">
			<div class="tc2">
			  <div class="tc3"></div>
			</div>
		  </div>
			  <div class="tblbl">Statistics</div></th>
		  <td>&nbsp;&nbsp;</td>
		  <?php } ?>
		  
		  <td width="100%"><div align="right" id="ajaxloadinginfo" style="visibility:hidden;"><img src="images/ajax_loading.gif" alt="loading ..." width="13" height="13" /></div></td>
		</tr>
	  </tbody>
	</table>
	
	<div class="tc2" style="background-color:#C9DAF8;">
		<div class="submenu" style="font-family:Verdana, Arial, Helvetica, sans-serif; font-size:10px;">
			<?php 
				if ($curmenu == "welcome") { 
					echo 'RosCMS v3 - Welcome page';
				}
				else if ($curmenu == "website") { 
					echo 'Quick Links: <a href="'.$roscms_intern_page_link.'data&branch=welcome#web_news_langgroup">Translation Group News</a> | <a href="http://www.reactos.org/?page=tutorial_roscms" target="_blank">Text- &amp; Video-Tutorials</a> | <a href="http://www.reactos.org/forum/viewforum.php?f=18" target="_blank">Website Forum</a>';
				}
				else if ($curmenu == "reactos") { 
					echo 'ReactOS Translation Interface';
				}
				else if ($curmenu == "user") { 
					echo 'User Account Management Interface';
				}
				else if ($curmenu == "maintain") { 
					echo 'RosCMS Maintainer Interface';
				}
				else if ($curmenu == "stats") { 
					echo 'RosCMS Website Statistics';
				}
			?>
		</div>
	</div>

