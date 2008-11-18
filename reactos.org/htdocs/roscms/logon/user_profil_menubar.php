<table style="border:0" width="100%" cellpadding="0" cellspacing="0">
  <tr valign="top">
  <td style="width:147px" id="leftNav"> 
  <div class="navTitle">Navigation</div>
    <ol>
      <li><a href="http://www.reactos.org/en/index.html">Home</a></li>
      <li><a href="http://www.reactos.org/en/about.html">Info</a></li>
      <li><a href="http://www.reactos.org/en/community.html">Community</a></li>
      <li><a href="http://www.reactos.org/en/dev.html">Development</a></li>
      <li><a href="http://www.reactos.org/roscms/?page=user">myReactOS</a></li>
    </ol>
  <p></p>

<?php
	global $rdf_user_id;
	global $rpm_lang;
	
	if ($rdf_user_id > 1) {
?>
	<div class="navTitle"><?php echo $roscms_langres['Account']; ?></div>   
	<ol> 
		<li title="<?php echo $roscms_intern_login_check_username; ?>">
			&nbsp;Nick:&nbsp;<?php echo substr($roscms_intern_login_check_username, 0, 9); ?> 
		</li>
		<li><a href="<?php echo $roscms_SET_path_ex; ?>my/">My Profile</a></li>
		<li><a href="<?php echo $roscms_SET_path_ex; ?>search/">User Search</a></li>
		<li><a href="<?php echo $roscms_intern_path_server; ?>peoplemap/" target="_blank">User Map</a></li>
		
		<?php if($roscms_security_level >= 1) { ?>
		<li><a href="<?php echo $roscms_SET_path; ?>?page=data" target="_blank">RosCMS Interface</a></li>
		<?php } ?>
		
		<li><a href="<?php echo $roscms_SET_path_ex; ?>logout/"><?php echo $roscms_langres['Logout']; ?></a></li>
	</ol>
	<p></p>
<?php
	}
	else {
?>
	<div class="navTitle"><?php echo $roscms_langres['Account']; ?></div>   
	<ol> 
		<li><a href="<?php echo $roscms_SET_path_ex; ?>login/">Login</a></li>
		<li><a href="<?php echo $roscms_SET_path_ex; ?>register/">Register</a></li>
	</ol>
	<p></p>
<?php
	}
?>
<div class="navTitle">Quick Links</div>   
<ol>
	<li><a href="http://www.reactos.org/forum/">Forum</a></li>
	<li><a href="http://www.reactos.org/wiki/">Wiki</a></li>
	<li><a href="http://www.reactos.org/en/about_userfaq.html">FAQ</a></li>
	<li><a href="http://www.reactos.org/en/about_press.html">Press</a></li>
	<li><a href="http://www.reactos.org/bugzilla/">Bugzilla</a></li>
	<li><a href="http://www.reactos.org/en/community_mailinglists.html">Mailing Lists</a></li>
	<li><a href="http://www.reactos.org/getbuilds/">Trunk Builds</a></li>
</ol>
<p></p>

<div class="navTitle">Language</div>   
      <ol>
        <li> 
          <div align="center"> 
            <select id="select" size="1" name="select" class="selectbox" style="width:140px" onchange="window.location.href = '<?php echo $roscms_SET_path_ex.$rdf_uri_str; ?>?lang=' + this.options[this.selectedIndex].value">
			<optgroup label="current language"> 
			<?php 
				echo "SELECT * 
													FROM languages 
													WHERE lang_id = '". mysql_real_escape_string($rpm_lang) ."' ;";
				$query_roscms_lang = mysql_query("SELECT * 
													FROM languages 
													WHERE lang_id = '". mysql_real_escape_string($rpm_lang) ."' ;");
				$result_roscms_lang = mysql_fetch_array($query_roscms_lang);
				
				echo '<option value="#">'.$result_roscms_lang[1].'</option>';
			?>
              </optgroup>
			  <optgroup label="all languages">
			<?php 
				$query_roscms_lang2 = mysql_query("SELECT * 
													FROM languages 
													ORDER BY lang_level DESC;");
				while ($result_roscms_lang2 = mysql_fetch_array($query_roscms_lang2)) {
				
					if ($result_roscms_lang2["lang_name"] != $result_roscms_lang2["lang_name_org"]) {
						echo '<option value="'.$result_roscms_lang2["lang_id"].'">'.$result_roscms_lang2["lang_name_org"].' ('.$result_roscms_lang2["lang_name"].')</option>';
					}
					else {
						echo '<option value="'.$result_roscms_lang2["lang_id"].'">'.$result_roscms_lang2["lang_name"].'</option>';
					}
				}
			?>
              </optgroup>
            </select>
          </div>
        </li>
      </ol>
<p></p>


      </td>

    <td id="content"><div class="contentSmall">	
