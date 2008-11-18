<?php
/*
  PROJECT:    People Map of the ReactOS Website
  LICENSE:    GPL v2 or any later version
  FILE:       web/reactos.org/htdocs/peoplemap/index.php
  PURPOSE:    Main web page
  COPYRIGHT:  Copyright 2007-2008 Colin Finck <mail@colinfinck.de>
*/

	require_once("config.inc.php");
	require_once("languages.inc.php");
	
	// Get the domain for the cookie (with a leading dot if possible). Borrowed from "utils.php" of RosCMS.
	function cookie_domain()
	{
		if( isset($_SERVER['SERVER_NAME']) )
		{
			if( preg_match('/(\.[^.]+\.[^.]+$)/', $_SERVER['SERVER_NAME'], $matches) )
				return $matches[1];
			else
				return "";
		}
		else
			return "";
	}
	
	// Get the language and include it
	if( isset($_GET["lang"]) )
		$lang = $_GET["lang"];
	else if( isset($_COOKIE["roscms_usrset_lang"]) )
		$lang = $_COOKIE["roscms_usrset_lang"];
	
	// Check if the language is valid
	$lang_valid = false;
	
	foreach( $peoplemap_languages as $lang_key => $lang_name )
	{
		if( $lang == $lang_key )
		{
			$lang_valid = true;
			break;
		}
	}
	
	if( !$lang_valid )
		$lang = "en";
	
	setcookie( "roscms_usrset_lang", $lang, time() + 5 * 30 * 24 * 3600, "/", cookie_domain() );
	require_once("lang/$lang.inc.php");
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
	<meta http-equiv="content-type" content="text/html; charset=utf-8">
	<title><?php echo $peoplemap_langres["title"]; ?></title>
	<link rel="stylesheet" type="text/css" href="peoplemap.css">
	<!--[if IE 6]><link rel="stylesheet" type="text/css" href="ie6-fixes.css"><![endif]-->
	<script type="text/javascript" src="http://maps.google.com/maps?file=api&amp;v=2&amp;key=<?php echo $GOOGLE_MAPS_KEY; ?>"></script>
	<script type="text/javascript">
	<?php require_once("peoplemap.js.php"); ?>
	</script>
</head>
<body onload="Load()" onunload="Unload()">

<?php
	readfile("http://www.reactos.org/$lang/subsys_extern_menu_top.html");
	readfile("http://www.reactos.org/$lang/subsys_extern_menu_left.html");
?>

<div class="navTitle"><?php echo $peoplemap_langres["language"]; ?></div>
	<ol>
		<li style="text-align: center;">
			<select size="1" onchange="window.location.href = '<?php echo "http://" . $_SERVER["SERVER_NAME"] . $_SERVER["PHP_SELF"]; ?>?lang=' + this.options[this.selectedIndex].value;">
				<?php
					foreach($peoplemap_languages as $lang_key => $lang_name)
						printf('<option value="%s"%s>%s</option>', $lang_key, ($lang_key == $lang ? ' selected' : ''), $lang_name);
				?>
			</select>
		</li>
	</ol>
</td>
<td id="content">

<h1><?php echo $peoplemap_langres["header"]; ?></h1>
<h2><?php echo $peoplemap_langres["title"]; ?></h2>

<p><?php echo $peoplemap_langres["intro"]; ?></p>

<table width="100%" cellspacing="0" cellpadding="0">
	<tr>
		<td id="map_td">
			<div class="bubble_bg">
				<div class="rounded_ll">
				<div class="rounded_lr">
				<div class="rounded_ul">
				<div class="rounded_ur">
				
				<div class="bubble">
					<div id="map">
						<!-- The map will be here -->
						
						<noscript><strong><?php echo $peoplemap_langres["activatejs"]; ?></strong></noscript>
					</div>
				</div>
				
				</div>
				</div>
				</div>
				</div>
			</div>
		</td>
		<td id="toolbox_td">
			<div class="bubble_bg">
				<div class="rounded_ll">
				<div class="rounded_lr">
				<div class="rounded_ul">
				<div class="rounded_ur">
			
				<div class="bubble">
					<div id="counttext">
						<?php
							$db = mysql_connect($DB_HOST, $DB_USER, $DB_PASS) or die("Could not connect to the database!");
							
							$query = "SELECT COUNT(*) FROM $DB_ROSCMS.users;";
							$result = mysql_query($query, $db);
							$user_count = mysql_result($result, 0);
							
							$query = "SELECT COUNT(*) FROM $DB_PEOPLEMAP.user_locations;";
							$result = mysql_query($query, $db);
							$location_count = mysql_result($result, 0);
							
							echo $peoplemap_langres["count1"] . "0" . $peoplemap_langres["count2"] . $location_count . $peoplemap_langres["count3"] . $user_count . $peoplemap_langres["count4"];
							
							echo "<script type=\"text/javascript\">";
							echo "LocationCount = $location_count;";
							echo "UserCount = $user_count;";
							echo "</script>";
						?>
					</div><br>
				
					<table>
						<tr>
							<td><?php echo $peoplemap_langres["icons"]; ?>:</td>
							<td><input type="radio" name="icon" value="marker" checked="checked" onclick="SwitchIcon(this.value);"></td>
							<td id="icon_marker"></td>
							<td><input type="radio" name="icon" value="circle" onclick="SwitchIcon(this.value);"></td>
							<td><div id="icon_circle"></div></td>
						</tr>
					</table>
				</div>
				
				</div>
				</div>
				</div>
				</div>
			</div>

			<div class="bubble_bg" id="toolbox0_bubble">
				<div class="rounded_ll">
				<div class="rounded_lr">
				<div class="rounded_ul">
				<div class="rounded_ur">
				
				<div class="bubble">
					<table id="toolbox0_head" onclick="ToggleToolbox(0);">
						<tr>
							<td id="toolbox0_image"></td>
							<td class="toolbox_head"><strong><?php echo $peoplemap_langres["filter"]; ?></strong></td>
						</tr>
					</table>
					
					<div id="toolbox0_controls">
						<?php
							echo $peoplemap_langres["filter_intro"] . "<br><br>";
							
							$query = "SELECT usrgroup_name_id, usrgroup_name FROM $DB_ROSCMS.usergroups WHERE usrgroup_visible = 1;";
							$result = mysql_query($query, $db) or die("Query failed #1!");
							
							echo "<table>";
							
							$iconcode  = '<script type="text/javascript">';
							$iconcode .= 'IconTable = new Object();';
							
							while($row = mysql_fetch_row($result))
							{
								echo "<tr>";
								echo "<td><input type=\"checkbox\" name=\"usergroups\" onclick=\"ToggleUserGroup(this, '" . $row[0] . "');\"></td>";
								echo "<td><div class=\"colorbox\" style=\"background: " . current($MARKERS) . "\"></div></td>";
								echo "<td>" . $row[1] . "</td>";
								echo "<td><img id=\"ajaxloading_" . $row[0] . "\" style=\"visibility: hidden;\" src=\"images/ajax_loading.gif\" alt=\"\"></td>";
								echo "</tr>";
								
								$iconcode .= "IconTable['" . $row[0] . "'] = '" . current($MARKERS) . "';";
								
								next($MARKERS);
							}
							
							echo "</table>";
							
							$iconcode .= '</script>';
							echo $iconcode;
						?>
					</div>
				</div>
			
				</div>
				</div>
				</div>
				</div>
			</div>

			<div class="selectable_bubble_bg" id="toolbox1_bubble" onmouseover="BubbleHover(this, true);" onmouseout="BubbleHover(this, false);">
				<div class="rounded_ll">
				<div class="rounded_lr">
				<div class="rounded_ul">
				<div class="rounded_ur">
				
				<div class="bubble">
					<table id="toolbox1_head" onclick="ToggleToolbox(1);">
						<tr>
							<td id="toolbox1_image"><td>
							<td class="toolbox_head"><strong><?php echo $peoplemap_langres["add"]; ?></strong></td>
						</tr>
					</table>

					<div id="toolbox1_controls">
						<p>
							<select id="add_subject" size="1">
								<option><?php echo $peoplemap_langres["add_username"]; ?></option>
								<option><?php echo $peoplemap_langres["add_fullname"]; ?></option>
							</select>:
							
							<input type="text" id="add_query" size="16" onkeyup="GetUser()">
							<img id="ajaxloading_add" style="visibility: hidden;" src="images/ajax_loading.gif" alt="">
						</p>
						
						<div id="add_user_result">
							<!-- The result of the user name search will be here -->
						</div>
					</div>
				</div>
			
				</div>
				</div>
				</div>
				</div>
			</div>

			<div class="selectable_bubble_bg" id="toolbox2_bubble" onmouseover="BubbleHover(this, true);" onmouseout="BubbleHover(this, false);">
				<div class="rounded_ll">
				<div class="rounded_lr">
				<div class="rounded_ul">
				<div class="rounded_ur">
				
				<div class="bubble">
					<table id="toolbox2_head" onclick="ToggleToolbox(2);">
						<tr>
							<td id="toolbox2_image"><td>
							<td class="toolbox_head"><strong><?php echo $peoplemap_langres["mylocation"]; ?></strong></td>
						</tr>
					</table>
	
					<div id="toolbox2_controls">
						<?php
							$logintext  = $peoplemap_langres["mylocation_login"];
							$logintext .= "<br><br>";
							$logintext .= "<div style=\"text-align: center;\">";
							$logintext .= "<a href=\"/roscms/?page=login&amp;target=" . urlencode($_SERVER["PHP_SELF"]) . "\">" . $peoplemap_langres["mylocation_login_page"] . "</a>";
							$logintext .= "<script type=\"text/javascript\">MyUserId = -1;</script>";
							$logintext .= "</div>";
							
							if($_COOKIE["roscmsusrkey"])
							{
								$query = "SELECT usersession_user_id FROM $DB_ROSCMS.user_sessions WHERE usersession_id = '" . mysql_real_escape_string($_COOKIE["roscmsusrkey"]) . "' LIMIT 1;";
								$result = mysql_query($query, $db) or die("Query failed #2!");
								
								if(mysql_num_rows($result) > 0)
									$userid = mysql_result($result, 0);
								
								mysql_close($db);
								
								if($userid)
								{
									echo $peoplemap_langres["mylocation_intro"];
									
									echo "<ul>";
									
									echo "<li>";
									echo $peoplemap_langres["mylocation_marker_intro"] . "<br>";
									echo "<input type=\"button\" onclick=\"SetLocationMarker();\" value=\"" . $peoplemap_langres["mylocation_marker_button"] . "\">";
									echo "<br><br>";
									echo "</li>";
									
									echo "<li>";
									echo $peoplemap_langres["mylocation_coordinates_intro"] . "<br>";
									echo "<table>";
									echo "<tr><td>" . $peoplemap_langres["latitude"] . ":</td><td><input type=\"text\" id=\"mylocation_latitude\" size=\"10\" onkeyup=\"CheckCoordinate(this);\">&deg;</td></tr>";
									echo "<tr><td>" . $peoplemap_langres["longitude"] . ":</td><td><input type=\"text\" id=\"mylocation_longitude\" size=\"10\" onkeyup=\"CheckCoordinate(this);\">&deg;</td></tr>";
									echo "</table>";
									echo "<input type=\"button\" onclick=\"SetLocationCoordinates();\" value=\"" . $peoplemap_langres["mylocation_coordinates_button"] . "\"> ";
									echo "<img id=\"ajaxloading_setlocation_coordinates\" style=\"visibility: hidden;\" src=\"images/ajax_loading.gif\" alt=\"\">";
									echo "<br><br>";
									echo "</li>";
									
									echo "<li>";
									echo $peoplemap_langres["mylocation_delete_intro"] . "<br>";
									echo "<input type=\"button\" onclick=\"DeleteMyLocation();\" value=\"" . $peoplemap_langres["mylocation_delete_button"] . "\">";
									echo "</li>";
									
									echo "</ul>";
									
									echo "<script type=\"text/javascript\">";
									echo "MyUserId = $userid;";
									echo "</script>";
								}
								else
									echo $logintext;
							}
							else
								echo $logintext;
						?>
					</div>
				</div>
			
				</div>
				</div>
				</div>
				</div>
			</div>
		</td>
	</tr>
</table>

</td>
</tr>
</table>

</body>
</html>
