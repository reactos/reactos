<?php

    /*
    ReactOS Paste Service
    Copyright (C) 2006  Klemens Friedl <frik85@reactos.org>

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


	$ros_paste_SET_cnp = "";
	if (array_key_exists("cnp", $_GET)) $ros_paste_SET_cnp=htmlspecialchars($_GET["cnp"]);

	if (@!$ros_paste_SET_pasteid) {
		$ros_paste_SET_pasteid = "";
		if (array_key_exists("pasteid", $_GET)) $ros_paste_SET_pasteid=htmlspecialchars($_GET["pasteid"]);
	}
	
	if ($ros_paste_SET_pasteid) {
		create_header();

		$query_pasteid=mysql_query("SELECT * 
									FROM `paste_service` 
									WHERE `paste_id` = '".mysql_real_escape_string($ros_paste_SET_pasteid)."'  
									LIMIT 1 ;");	
		$result_pasteid = mysql_fetch_array($query_pasteid);
		
		if ($result_pasteid['paste_lines'] == "" || $result_pasteid['paste_lines'] <= 0 || compareDate((date('Y')."-".date('m')."-".date('d')),stripDate($result_pasteid['paste_datetime'])) > 7) {
			echo "<p><b>No related paste exists.</b></p>";
		} else if (compareDate((date('Y')."-".date('m')."-".date('d')),stripDate($result_pasteid['paste_datetime'])) <= $result_pasteid['paste_days']) {
		
			echo '<link href="highlight.css" type="text/css" rel="stylesheet" />';
			echo "<h1><a style=\"color:#FFFFFF ! important ;\" href=\"http://www.reactos.org/\">Home</a> &gt; <a style=\"color:#FFFFFF ! important ;\" href=\"".$ros_paste_SET_path."\">Paste Service</a> &gt; ".$ros_paste_SET_pasteid."</h1>";
			echo "<h2>";
			if ($result_pasteid['paste_desc']) {
				echo $result_pasteid['paste_desc'];
			}
			else {
				echo "Paste #".$result_pasteid['paste_id'];
			}
			echo "</h2>";
		
	?>
	
	<p>Pasted by: <b><?php echo $result_pasteid['paste_nick']; ?></b><br />
	Language: <b><?php echo $result_pasteid['paste_lang']; ?></b><br />
	Description: <b><?php echo $result_pasteid['paste_desc']; ?></b><br />
	Pasted on: <b><?php echo $result_pasteid['paste_date']; ?></b><br />
	Expire after: <b><?php echo $result_pasteid['paste_days']; ?> days </b><br />
	Paste history: <b><?php 
	
		if ($result_pasteid['paste_public'] == "1") {
			echo "yes (public)";
		}
		else {
			echo "no (private)";
		}
	
	?></b></p>
	<p><?php 
	
		if (@$ros_paste_SET_pasteflag == "nln") {
			echo '<a href="'. $ros_paste_SET_path_ex . $result_pasteid['paste_id'] .'/">Add line numbers</a>';
		}
		else {
			echo '<a href="'. $ros_paste_SET_path_ex . $result_pasteid['paste_id'] .'/nln/">Remove line numbers</a>';
		}
	?> | 
	  <a href="<?php echo $ros_paste_SET_path_ex . $result_pasteid['paste_id'] .'/text/'; ?>" target="_blank">Download as Text</a> 
	  <a href="<?php echo $ros_paste_SET_path_ex . $result_pasteid['paste_id'] .'/textw/'; ?>" target="_blank">(word-wrap)</a>  | 
	  <a href="<?php echo $ros_paste_SET_path_ex . 'recent/'; ?>">Other recent pastes</a>  | 
	  <a href="<?php echo $ros_paste_SET_path; ?>">Create new paste</a> </p>
	<?php 
	
	
				$filename = $ros_paste_SET_content."/".$result_pasteid['paste_id'].".txt";
				$handle = @fopen($filename, "r");
				$contents = @fread($handle, @filesize($filename));
				@fclose($handle);
				
				if ($contents == "") {
					die("<b>paste expired</b>");
				}
	
				$ros_paste_SET_textcontent = htmlspecialchars($contents);
				if ($result_pasteid['paste_tabs'] != "0") {
					$PASTE_var_tabs = "";
					for($xxx=0; $xxx<$result_pasteid['paste_tabs']; $xxx++){
						$PASTE_var_tabs .= "&nbsp;";
					}
					$ros_paste_SET_textcontent = str_replace("\t",$PASTE_var_tabs,$ros_paste_SET_textcontent);
				}
				
				
				// Content:
				$ros_paste_SET_textcontent = syntax_highlight($ros_paste_SET_textcontent, $result_pasteid['paste_lang']);
	
	
	
			if (@$ros_paste_SET_pasteflag == "nln") {
				echo '<pre class="code" style="margin: 6px;">';
				echo $ros_paste_SET_textcontent;
				echo '</pre>';
			}
			else {
	?>
				<table cellspacing="5" cellpadding="1" border="0">
				<tbody>
				<tr>
				
					<td align="right" valign="top"><pre class="code"><?php 
					
						for ($PASTE_linecount=1; $PASTE_linecount <= $result_pasteid['paste_lines']; $PASTE_linecount++) {
							echo $PASTE_linecount."\n";
						}
					
						?></pre>
					</td>
				<td width="100%" valign="top"><pre class="code"><?php echo $ros_paste_SET_textcontent; ?></pre></td></tr></tbody></table>
<?php
			}
		}
	}
	else {
		if ($ros_paste_SET_cnp == "1") {
		
			$ros_paste_SET_lang = "";
			$ros_paste_SET_usrname = "";
				$ros_paste_SET_nick = "";
				//$ros_paste_SET_husrname = "";
				//$ros_paste_SET_husrid = "";
			$ros_paste_SET_desc = "";
			$ros_paste_SET_cvt_tabs = "";
			$ros_paste_SET_optday = "";
				$ros_paste_SET_days = "";
			$ros_paste_SET_textcontent = "";
			$ros_paste_SET_public = "";
			
		
			if (array_key_exists("lang", $_POST)) $ros_paste_SET_lang=htmlspecialchars($_POST["lang"]);
			if (array_key_exists("usrname", $_POST)) $ros_paste_SET_usrname=htmlspecialchars($_POST["usrname"]);
			if (array_key_exists("nick", $_POST)) $ros_paste_SET_nick=htmlspecialchars($_POST["nick"]);
			//if (array_key_exists("husrname", $_POST)) $ros_paste_SET_husrname=htmlspecialchars($_POST["husrname"]);
			//if (array_key_exists("husrid", $_POST)) $ros_paste_SET_husrid=htmlspecialchars($_POST["husrid"]);
			if (array_key_exists("desc", $_POST)) $ros_paste_SET_desc=htmlspecialchars($_POST["desc"]);
			if (array_key_exists("cvt_tabs", $_POST)) $ros_paste_SET_cvt_tabs=htmlspecialchars($_POST["cvt_tabs"]);
			if (array_key_exists("optday", $_POST)) $ros_paste_SET_optday=htmlspecialchars($_POST["optday"]);
			if (array_key_exists("days", $_POST)) $ros_paste_SET_days=htmlspecialchars($_POST["days"]);
			if (array_key_exists("textcontent", $_POST)) $ros_paste_SET_textcontent=$_POST["textcontent"];
			if (array_key_exists("optpub", $_POST)) $ros_paste_SET_public=htmlspecialchars($_POST["optpub"]);
		
			// Nick:
			$ros_paste_SET_husrname = $RSDB_USER_name;
			$ros_paste_SET_husrid = $RSDB_intern_user_id;
			
			if ($ros_paste_SET_usrname == "1") {
				$PASTE_var_nick = $ros_paste_SET_nick;
			}
			elseif ($ros_paste_SET_usrname == "2") {
				if ($ros_paste_SET_husrname == "") {
					$PASTE_var_nick = "Anonymous";
				}
				else {
					$PASTE_var_nick = $ros_paste_SET_husrname;
				}
			}
			else {
				$PASTE_var_nick = "Anonymous";
			}
			
			// Days:
			if ($ros_paste_SET_optday == "2") {
				$PASTE_var_days = $ros_paste_SET_days;
			}
			else {
				$PASTE_var_days = "7";
			}
			
			// Paste history - recent pastes:
			if ($ros_paste_SET_public != "1" && $ros_paste_SET_public != "2") {
				$ros_paste_SET_public = "1";
			}
			$PASTE_var_public = $ros_paste_SET_public;
			
			
			/*// Paste ID:		
			$tmp_id_check = true;
			while($tmp_id_check) {
					mt_srand((double)microtime()*1000000);
					mt_srand((double)microtime()*65000*mt_rand(1,10000));
					$radomnumber = mt_rand(1,1000);
					$PASTE_var_pasteid = substr(md5($radomnumber), 0, 7);
					
					$query_pasteid=mysql_query("SELECT COUNT(*) 
									FROM `paste_service` 
									WHERE `paste_id` = '".mysql_real_escape_string($PASTE_var_pasteid)."' ;");	
					$result_pasteid = mysql_fetch_row($query_pasteid);
					if ($result_pasteid[0] <= 0) {
						$tmp_id_check = false;
					}
			}*/
			
			/*if ($ros_paste_SET_cvt_tabs != "0") {
				$PASTE_var_tabs = "";
				for($xxx=0; $xxx<$ros_paste_SET_cvt_tabs; $xxx++){
					$PASTE_var_tabs .= "&nbsp;";
				}
				$ros_paste_SET_textcontent = str_replace("\t",$PASTE_var_tabs,$ros_paste_SET_textcontent);
			}*/
			
			// Tabs:
			$PASTE_var_tabs = $ros_paste_SET_cvt_tabs;
			
			// Desc & Content:
			$PASTE_var_desc = $ros_paste_SET_desc;
			$PASTE_var_content = $ros_paste_SET_textcontent;
			//$PASTE_var_content = syntax_highlight($ros_paste_SET_textcontent, $ros_paste_SET_lang);
			
			$PASTE_var_lines = sizeof(explode("\n", $PASTE_var_content));
			$PASTE_var_lang = $ros_paste_SET_lang;
			
			
			$ip = @$_SERVER["HTTP_X_FORWARDED_FOR"]; 
			$proxy = $_SERVER["REMOTE_ADDR"]; 
			$host = @gethostbyaddr($_SERVER["HTTP_X_FORWARDED_FOR"]);
			
			if ($ip == "") {
				$ip = $proxy;
			}
			
			//echo "<p>$ip<br />$proxy<br />$host</p>";
			
			// SQL Statement:
			$tmp_insert_date = date("Y-m-d H:i:s");
			$paste_sql="INSERT INTO `paste_service` ( `paste_date` , `paste_days` , `paste_usrid` , `paste_nick` , `paste_desc` , `paste_lines` , `paste_tabs`, `paste_public` , `paste_lang` , `paste_datetime` , `paste_ip` , `paste_proxy` , `paste_host` ) 
						VALUES ( CURDATE( ) , '".mysql_real_escape_string($PASTE_var_days)."', '".mysql_real_escape_string($ros_paste_SET_husrid)."', '".mysql_real_escape_string($PASTE_var_nick)."', '".mysql_real_escape_string($PASTE_var_desc)."', '".mysql_real_escape_string($PASTE_var_lines)."', '".mysql_real_escape_string($PASTE_var_tabs)."', '".mysql_real_escape_string($PASTE_var_public)."', '".mysql_real_escape_string($PASTE_var_lang)."', '".$tmp_insert_date."', '".mysql_real_escape_string($ip)."', '".mysql_real_escape_string($proxy)."', '".mysql_real_escape_string($host)."' );";
			$paste_query=mysql_query($paste_sql);
			
			$sql_inserted_entry = "SELECT paste_id 
									FROM paste_service 
									WHERE paste_datetime = '".$tmp_insert_date."'
									LIMIT 1;";
			$query_inserted_entry = mysql_query($sql_inserted_entry);
			$result_inserted_entry = mysql_fetch_array($query_inserted_entry);
			
			//echo $sql_inserted_entry;
			//echo $result_inserted_entry['paste_id'];

			if (!@is_dir($ros_paste_SET_content)) {
				@mkdir($ros_paste_SET_content,"0493");
			}
			
			if ($result_inserted_entry['paste_id'] == "") {
				die("");
			}
			
			$filename = $ros_paste_SET_content."/".$result_inserted_entry['paste_id'].".txt";
			
			// Let's make sure the file exists and is writable first.
			if (!@is_writable($filename)) {
			
				// In our example we're opening $filename in append mode.
				// The file pointer is at the bottom of the file hence
				// that's where $somecontent will go when we fwrite() it.
				if (!$handle = @fopen($filename, 'a')) {
					 die("error: open file problem");
				}
			
				// Write $somecontent to our opened file.
				if (@fwrite($handle, $PASTE_var_content) === FALSE) {
					die("error: file write problem");
				}
			
				//echo "Success, wrote to file ($filename)";
			
				@fclose($handle);
			
			}
			
			$update_entry = mysql_query("UPDATE `paste_service` SET `paste_size` = '".mysql_real_escape_string(@filesize($filename))."' WHERE `paste_service`.`paste_id` = ".$result_inserted_entry['paste_id']." LIMIT 1 ;");

			

			// Redirect to paste result page:
			header("Location: ". $ros_paste_SET_path_ex . $result_inserted_entry['paste_id']. "/");
		}
		else {
			create_header();

	?>
		<h1><a href="http://www.reactos.org/">Home</a> &gt; Paste Service</h1>
		<h2>Copy &amp; Paste - paste service</h2>
		<h3>Enter your code/text below</h3>
		<form action="index.php?page=paste&amp;cnp=1" method="post">
		<table border="0" cellpadding="1" cellspacing="1">
		<tr>
		<td valign="top" bgcolor="#E2E2E2" style="padding-left: 0px; padding-right: 5px;"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Language:</font></b></td>
		<td bgcolor="#E2E2E2"><select name="lang" id="lang">
		<option value="Plain Text">Plain Text</option>
		<option value="Bash">Bash *</option>
		<option value="Batch">Batch *</option>
		<option value="C89">C (C89)</option>
		<option value="C" selected="selected">C (C99)</option>
		<option value="C++">C++</option>
		<option value="C#">C#</option>
		<option value="Java">Java</option>
		<option value="Pascal">Pascal</option>
		<option value="Perl">Perl</option>
		<option value="PHP">PHP</option>
		<option value="PL/I">PL/I</option>
		<option value="Python">Python</option>
		<option value="Ruby">Ruby</option>
		<option value="Scheme">Scheme *</option>
		<option value="SQL">SQL</option>
		<option value="VB">Visual Basic</option>
		<option value="XML">XML *</option>
		</select> 
		&nbsp;&nbsp;&nbsp;<font size="1">(* experimental syntax highlighting)</font></td>
	  </tr>
	  <tr>
		<td valign="top" bgcolor="#EEEEEE" style="padding-left: 0px; padding-right: 5px;"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Nickname:</font></b></td>
		<td bgcolor="#EEEEEE"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
		  <input name="usrname" type="radio" id="usrname" value="2" checked="checked" />
	    <?php echo $RSDB_USER_name; ?><br />
		  <input name="usrname" type="radio" id="usrname" value="1" />
		  <input type="text" size="14" maxlength="9" name="nick" id="nick" value=""/>
		</font></td>
	  </tr>
	  <tr>
		<td valign="top" bgcolor="#E2E2E2" style="padding-left: 0px; padding-right: 5px;"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Description:</font></b></td>
		<td bgcolor="#E2E2E2"><input type="text" size="50" maxlength="50" name="desc" id="desc"/></td>
	  </tr>
	  <tr>
		<td valign="top" bgcolor="#EEEEEE" style="padding-left: 0px; padding-right: 5px;"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Convert tabs:</font></b></td>
		<td bgcolor="#EEEEEE">
		  <select name="cvt_tabs" id="cvt_tabs">
			<option value="0" selected="selected">No</option>
			<option value="2">2</option>
			<option value="3">3</option>
			<option value="4">4</option>
			<option value="5">5</option>
			<option value="6">6</option>
			<option value="8">8</option>
			<option value="10">10</option>
		  </select>
		</td>
	  </tr>
	  <tr>
		<td valign="top" bgcolor="#E2E2E2" style="padding-left: 0px; padding-right: 5px;"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Expire after: </font></b></td>
		<td bgcolor="#E2E2E2"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
		  <input name="optday" type="radio"  value="1" checked="checked" />
		  7 days <br />
		  <input name="optday" type="radio"  value="2" />
			<input name="days" type="text" id="days" size="5" maxlength="2"/>
	    days <font size="1">(max. 7 days) </font></font></td>
	  </tr>
	
	  <tr>
	    <td bgcolor="#EEEEEE"><b><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Paste history: </font></b></td>
	    <td bgcolor="#EEEEEE"><font face="Verdana, Arial, Helvetica, sans-serif">
	      <font size="2">
<input name="optpub" type="radio" value="1" checked="checked" />
Yes <font size="1">(list the paste on &quot;recent pastes&quot; page) </font><br />
<input name="optpub" type="radio"  value="2" />
	    No</font>
	    </font></td>
	  </tr>
	  <tr>
		<td colspan="2" bgcolor="#E2E2E2"><textarea name="textcontent" cols="80" rows="20" wrap="off" id="textcontent"></textarea></td>
		</tr>
	  <tr>
		<td colspan="2" bgcolor="#EEEEEE"><ul>
		  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Do not post copyrighted or otherwise restricted material here. </font></li>
		  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Pastes expire  by default after 7 days (if not otherwise selected). </font></li>
		  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif">By posting code/text/content here, you agree to the terms of <a href="<?php echo $ros_paste_SET_path_ex; ?>conditions/">this statement</a>. </font></li>
		  </ul></td>
		</tr>
	  <tr>
	  <td colspan="2">
	
		<p align="center">
		  <input name="submit" type="submit" value="Paste"/>
		</p>    </td>
	  </tr>
	  </table>
	  </form>
	
<?php
		}
	}
?>
