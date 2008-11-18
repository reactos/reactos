<?php
	$profilesearch = "";
	if (array_key_exists("search", $_GET)) $profilesearch=htmlspecialchars($_GET["search"]);


	$sql_user_search_count = "SELECT count(*)  
								FROM users 
								WHERE user_name LIKE '".mysql_real_escape_string($profilesearch)."%'
								OR user_fullname LIKE '".mysql_real_escape_string($profilesearch)."%';";
	$query_user_search_count = mysql_query($sql_user_search_count);
	$result_user_search_count = mysql_fetch_array($query_user_search_count);

	if ($result_user_search_count[0] == 1) {
		$sql_user_search_found = "SELECT user_name  
									FROM users 
									WHERE user_name LIKE '".mysql_real_escape_string($profilesearch)."%'
									OR user_fullname LIKE '".mysql_real_escape_string($profilesearch)."%'
									LIMIT 1;";
		$query_user_search_found = mysql_query($sql_user_search_found);
		$result_user_search_found = mysql_fetch_array($query_user_search_found);
		$rdf_uri_2 = $result_user_search_found["user_name"];
	}

	//echo $result_user_search_count[0]."<br />";

	if ($result_user_search_count[0] != 1 && ($rdf_uri_2 == "" || $profilesearch != "")) {
?>
	<h1><a href="<?php echo $roscms_SET_path_ex; ?>my/">myReactOS</a> &gt; Profile Search</h1>
	<div class="u-h1">Profile Search</div>
	<form id="form1" name="form1" method="get" action="<?php echo $roscms_SET_path_ex."search/"; ?>">
	  <input name="search" type="text" id="search" value="<?php echo $profilesearch; ?>" />
      <input name="cmdsearch" type="submit" id="cmdsearch" value="Search" />
	</form>
	<p>&nbsp;</p>
<?php

		if ($profilesearch != "") {
			echo "<ul>";
			$sql_user_search = "SELECT user_name, user_fullname 
								FROM users
								WHERE user_name LIKE '".mysql_real_escape_string($profilesearch)."%'
								OR user_fullname LIKE '".mysql_real_escape_string($profilesearch)."%'
								ORDER BY user_name ASC
								LIMIT 100;";
			$query_user_search = mysql_query($sql_user_search);
			while ($result_user_search = mysql_fetch_array($query_user_search)) {
					echo "<li><b><a href=\"".$roscms_SET_path_ex."search/".$result_user_search["user_name"]."\">".$result_user_search["user_name"]."</a></b>";
					if ($result_user_search["user_fullname"]) {
						echo "<br />".$result_user_search["user_fullname"];
					}
					echo "<br />&nbsp;</li>";
			}
			echo "</ul>&nbsp;";
		}
	
	}
	else {
		$rdf_uri_2 = rawurldecode($rdf_uri_2);
		
		$sql_user_profil = "SELECT user_id, user_name, user_register, user_fullname, user_email, user_email_activation, user_website, 
								user_country, user_timezone, user_occupation, user_setting_multisession, 
								user_setting_browseragent, user_setting_ipaddress, user_setting_timeout  
							FROM users 
							WHERE user_name = '".mysql_real_escape_string($rdf_uri_2)."'
							LIMIT 1;";
		$query_user_profil = mysql_query($sql_user_profil);
		$result_user_profil = mysql_fetch_array($query_user_profil);
?>
		<h1><a href="<?php echo $roscms_SET_path_ex; ?>my/">myReactOS</a> &gt; Public Profile</h1>
		<div class="u-h1"><?php 
			if(!$result_user_profil)
			{
				echo 'This Profile does not exist!';
			}
			else
			{
				if ($result_user_profil['user_fullname'] != "") {
					echo $result_user_profil['user_fullname'].' ('.$result_user_profil['user_name'].')';
				}
				else {
					echo $result_user_profil['user_name'];
				}
		?></div>
		<div class="u-h2">A person who joined <?php echo $rdf_name; ?> on <?php echo tz($result_user_profil['user_register']); ?>.</div>
		<br />
	
	<?php
		$sql_user_profil = "SELECT user_id, user_name, user_fullname, user_email, user_language, user_website, 
								user_country, user_timezone, user_occupation  
							FROM users 
							WHERE user_name = '".mysql_real_escape_string($rdf_uri_2)."'
							LIMIT 1;";
		$query_user_profil = mysql_query($sql_user_profil);
		$result_user_profil = mysql_fetch_array($query_user_profil);
	?>
	<div align="center">
		<div style="background: <?php echo $rdf_color_logon_background; ?> none repeat scroll 0%; width: 300px;">
		  <div class="corner1">
			<div class="corner2">
			  <div class="corner3">
				<div class="corner4">
				  <div style="text-align:center; padding-top: 4px; padding-bottom: 4px; padding-left: 4px; padding-right: 4px;">
					<div class="login-title">ReactOS Profile</div>
	
					<div class="login-form">
					  <div class="u-desc">Username</div>
					  <div class="u-title"><?php echo htmlspecialchars($result_user_profil['user_name']); ?></div>
					</div>
	
					<?php
						if ($result_user_profil['user_fullname'] != "") {
					?>
					<div class="login-form">
					   <div class="u-desc">First and Last Name</div>
					  <div class="u-title"><?php echo htmlspecialchars($result_user_profil['user_fullname']); ?></div>
					</div>
					<?php
						}
					?>
					<div class="login-form">
					   <div class="u-desc">Country</div>
					  <div class="u-title"><?php 
						if (user_check_country($result_user_profil['user_country'])) {
							$sql_country = "SELECT coun_id, coun_name  
											FROM user_countries 
											WHERE coun_id = '".mysql_real_escape_string($result_user_profil['user_country'])."'
											LIMIT 1;";
							$query_country = mysql_query($sql_country);
							$result_country = mysql_fetch_array($query_country);
							
							echo $result_country['coun_name'];
						}
						else {
							echo "<span style=\"color: red;\">not set</span>";
						}
						
						?></div>
					</div>
	
					<div class="login-form">
					   <div class="u-desc">Language</div>
					  <div class="u-title"><?php 
						if (user_check_lang($result_user_profil['user_language'])) {
							$sql_language = "SELECT lang_id, lang_name    
												FROM user_language 
												WHERE lang_id = '".mysql_real_escape_string($result_user_profil['user_language'])."'
												LIMIT 1;";
							$query_language = mysql_query($sql_language);
							$result_language = mysql_fetch_array($query_language);
							echo $result_language['lang_name'];
						}
						else if ($result_user_profil['user_language'] != "") {
							echo htmlspecialchars($result_user_profil['user_language']);
						}
						else {
							echo "<span style=\"color: red;\">not set</span>";
						}
					  
					   ?></div>
					</div>
	
					<div class="login-form">
					   <div class="u-desc">Timezone</div>
					  <div class="u-title"><?php 
					  
						if (user_check_timezone($result_user_profil['user_timezone'])) {
							$sql_timezone = "SELECT tz_code, tz_name, tz_value2   
												FROM user_timezone 
												WHERE tz_code = '".mysql_real_escape_string($result_user_profil['user_timezone'])."'
												LIMIT 1;";
							$query_timezone = mysql_query($sql_timezone);
							$result_timezone = mysql_fetch_array($query_timezone);
		
							echo $result_timezone['tz_name'].' ('.$result_timezone['tz_value2'].')';
							echo "<div style=\"font-size: 10px;font-weight:lighter;\">server time: ".date("Y-m-d H:i")."<br />local time: ".tz(date("Y-m-d H:i"))."</div>";
						}
						else {
							echo "<span style=\"color: red;\">not set</span>";
						}
					  
					  ?></div>
					</div>
	
					<?php
						if ($result_user_profil['user_website'] != "") {
					?>
					<div class="login-form">
					   <div class="u-desc">Private Website</div>
					  <div class="u-title"><a href="<?php echo $result_user_profil['user_website']; ?>" target="_blank" rel="nofollow"><?php echo htmlspecialchars($result_user_profil['user_website']); ?></a></div>
					</div>
					<?php
						}
						if ($result_user_profil['user_occupation'] != "") {
					?>
					<div class="login-form">
					   <div class="u-desc">Occupation</div>
					  <div class="u-title"><?php echo htmlspecialchars($result_user_profil['user_occupation']); ?></div>
					</div>
					<?php
						}
					?>
					
					<div class="login-form">
					<label for="useroccupation">Location</label>
					<a href="<?php echo $roscms_intern_path_server; ?>peoplemap/" target="_blank" style="color:#333333 !important; text-decoration:underline; font-weight:bold;">Location on the Map</a> </div>
				  </div>
					<div>&nbsp;</div>
					<div><a href="<?php echo $roscms_SET_path_ex."search/"; ?>" style="color:#333333 !important; text-decoration:underline; font-weight:bold;"><strong>&raquo; Profile Search</strong></a></div>
					<div>&nbsp;</div>
				</div>
			  </div>
			</div>
		  </div>
	  </div>
	</div>
	
    <?php
  	}
	}
?>
