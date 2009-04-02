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


$stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_comp WHERE comp_visible = '1' AND comp_id = :comp_id ORDER BY comp_name ASC") ;
$stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
$stmt->execute();
$result_page = $stmt->fetch(PDO::FETCH_ASSOC);

if ($result_page['comp_id']) {

	echo "<h2>".htmlentities($result_page['comp_name']) ." [". "ReactOS ".@show_osversion($result_page['comp_osversion']) ."]</h2>"; 



if ($RSDB_intern_user_id <= 0) {
	Message::loginRequired(); 
}
else {
	
	include("inc/comp/comp_item_menubar.php");
	
	echo "<h3>Submit a Report - Step 3</h3>";

	$RSDB_TEMP_SUBMIT_valid = true;
	
	$RSDB_TEMP_submitpost = "";
	$RSDB_TEMP_txtwhatwork = "";
	$RSDB_TEMP_txtwhatnot = "";
	$RSDB_TEMP_txtwhatnottested = "";
	$RSDB_TEMP_txtcomment = "";
	$RSDB_TEMP_optfunc = "";
	$RSDB_TEMP_optinstall = "";
	$RSDB_TEMP_txtconclusion = "";
	$RSDB_TEMP_txtrevision = "";
	
	if (array_key_exists("submitpost", $_POST)) $RSDB_TEMP_submitpost=htmlspecialchars($_POST["submitpost"]);
	if (array_key_exists("txtwhatwork", $_POST)) $RSDB_TEMP_txtwhatwork=htmlspecialchars($_POST["txtwhatwork"]);
	if (array_key_exists("txtwhatnot", $_POST)) $RSDB_TEMP_txtwhatnot=htmlspecialchars($_POST["txtwhatnot"]);
	if (array_key_exists("txtwhatnottested", $_POST)) $RSDB_TEMP_txtwhatnottested=htmlspecialchars($_POST["txtwhatnottested"]);
	if (array_key_exists("txtcomment", $_POST)) $RSDB_TEMP_txtcomment=htmlspecialchars($_POST["txtcomment"]);
	if (array_key_exists("optfunc", $_POST)) $RSDB_TEMP_optfunc=htmlspecialchars($_POST["optfunc"]);
	if (array_key_exists("optinstall", $_POST)) $RSDB_TEMP_optinstall=htmlspecialchars($_POST["optinstall"]);
	if (array_key_exists("txtconclusion", $_POST)) $RSDB_TEMP_txtconclusion=htmlspecialchars($_POST["txtconclusion"]);
	if (array_key_exists("txtrevision", $_POST)) $RSDB_TEMP_txtrevision=htmlspecialchars($_POST["txtrevision"]);

	if ($RSDB_TEMP_submitpost == "yes") {
		if (strlen($RSDB_TEMP_txtwhatwork) <= 3) {
			Message::show("The 'What works' textbox is (almost) empty  ...");
			echo "<br />";
			$RSDB_TEMP_SUBMIT_valid = false;
		}
		/*if (strlen($RSDB_TEMP_txtwhatnot) <= 3) {
			Message::show("The 'What does not work' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}*/
		if ((strlen($RSDB_TEMP_txtwhatnottested) <= 3) || $RSDB_TEMP_txtwhatnottested != "Features tested:\n\n\nNOT tested features:\n") {
			Message::show("The 'What has been tested and what not' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if ($RSDB_TEMP_optfunc < 1 || $RSDB_TEMP_optfunc > 5) {
			Message::show("Application function: please select the star(s) ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if ($RSDB_TEMP_optinstall < 1 || $RSDB_TEMP_optinstall > 5) {
			Message::show("Installation routine: please select the star(s) ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
		if (strlen($RSDB_TEMP_txtconclusion) <= 3) {
			Message::show("The 'Conclusion' textbox is (almost) empty  ...");
			$RSDB_TEMP_SUBMIT_valid = false;
			echo "<br />";
		}
	}
	if ($RSDB_TEMP_SUBMIT_valid == "yes" && $RSDB_TEMP_submitpost == true) {
		$stmt=CDBConnection::getInstance()->prepare("INSERT INTO rsdb_item_comp_testresults ( test_id, test_comp_id, test_visible, test_whatworks, test_whatdoesntwork, test_whatnottested, test_date, test_result_install, test_result_function, test_user_comment, test_conclusion, test_user_id, test_user_submit_timestamp, test_useful_vote_value, test_useful_vote_user, test_useful_vote_user_history, test_com_version) VALUES ('', :comp_id, '1', :whatworks, :whatnot, :whatnottestet, NOW(), :install, :function, :comment, :conclusion, :user_id, NOW( ) , '', '', '', :version );");
    $stmt->bindParam('comp_id',$RSDB_SET_item,PDO::PARAM_STR);
    $stmt->bindParam('whatworks',$RSDB_TEMP_txtwhatwork,PDO::PARAM_STR);
    $stmt->bindParam('whatnot',$RSDB_TEMP_txtwhatnot,PDO::PARAM_STR);
    $stmt->bindParam('whatnottested',$RSDB_TEMP_txtwhatnottested,PDO::PARAM_STR);
    $stmt->bindParam('install',$RSDB_TEMP_optinstall,PDO::PARAM_STR);
    $stmt->bindParam('function',$RSDB_TEMP_optfunc,PDO::PARAM_STR);
    $stmt->bindParam('comment',$RSDB_TEMP_txtcomment,PDO::PARAM_STR);
    $stmt->bindParam('conclusion',$RSDB_TEMP_txtconclusion,PDO::PARAM_STR);
    $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
    $stmt->bindParam('version',$RSDB_TEMP_txtrevision,PDO::PARAM_STR);
    $stmt->execute();
		echo "<p><b>Your Compatibility Test Report has been saved!</b></p>";
		echo "<p>&nbsp;</p>";
		echo '<p><b><a href="'.$RSDB_intern_link_item_item2.'screens&amp;addbox=add">Submit Screenshots</a></b></p>';
		echo "<p>&nbsp;</p>";
		echo "<p><a href=\"". $RSDB_intern_link_item_item2 ."tests\">Show all compatibility test reports</a></p>";
		
		
		// Stats update:
		$CDBConnection::getInstance()->exec("UPDATE rsdb_stats SET stat_s_ictest = (stat_s_ictest + 1) WHERE stat_date = '". date("Y-m-d") ."'";
	}
	else {
?>

<form name="RSDB_comp_testreport" method="post" action="<?php echo $RSDB_intern_link_submit_comp_test; ?>submit">
<fieldset><legend>&nbsp;<b><font color="#000000">Submit an Application in 3 Steps</font></b>&nbsp;</legend>
	<table width="100%" border="0" cellpadding="1" cellspacing="5">
      <tr>
        <td width="33%"><h4><font color="#999999">Step 1</font></h4></td>
        <td width="34%"><h4><font color="#999999">Step 2</font></h4></td>
        <td width="33%"><h4>Step 3</h4></td>
      </tr>
      <tr>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>general information</b>:</font></p>
          <ul>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">application name</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">short  decription</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">category</font></li>
            <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">vendor </font></li>
          </ul></td>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>version information</b></font></p>
            <ul>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">application version</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">ReactOS</font> <font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">version</font></li>
            </ul></td>
        <td valign="top"><p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>test results &amp; screenshots</b></font></p>
            <ul>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">What works</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">What does not work</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Describe what you have tested and what not</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Application function</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Installation routine</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Conclusion</font></li>
            </ul></td>
      </tr>
    </table>
</fieldset>
<font size="1" face="Verdana, Arial, Helvetica, sans-serif"><br />
</font>
<p><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Please report compatibility with ReactOS release versions only,  write full sentenses and avoid abbreviations!</font></p>
<p><font size="1" face="Verdana, Arial, Helvetica, sans-serif">The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</font></p>
<p>&nbsp;</p>
<p><font size="4"><?php echo "<b>".htmlentities($result_page['comp_name']) ."</b>"; ?></font> and <font size="4"><?php echo "<b>". "ReactOS ".show_osversion($result_page['comp_osversion']) ."</b>"; ?></font> release version</p>
<p>&nbsp;</p>
<p><font size="2"><strong>What works:</strong><br />
    <textarea name="txtwhatwork" cols="70" rows="5" id="txtwhatwork"><?php echo $RSDB_TEMP_txtwhatwork; ?></textarea>
</font></p>
<p><font size="2"><strong>What does not work</strong>: (optional) <br />
      <textarea name="txtwhatnot" cols="70" rows="5" id="txtwhatnot"><?php echo $RSDB_TEMP_txtwhatnot; ?></textarea>
</font></p>
<p><font size="2"><strong>Describe what you have tested and what not:</strong><br />
      <textarea name="txtwhatnottested" cols="70" rows="6" id="txtwhatnottested"><?php 
	  
	if ($RSDB_TEMP_txtwhatnottested) {
		echo $RSDB_TEMP_txtwhatnottested; 
	}
	else {
		echo "Features tested:\n\n\nNOT tested features:\n";
	}
	
	  
	  ?></textarea>
</font></p>
<p><font size="2"><strong>Tester Comment:</strong> (optional) <br />
      <textarea name="txtcomment" cols="70" rows="5" id="txtcomment"><?php echo $RSDB_TEMP_txtcomment; ?></textarea>
</font></p>
<p><font size="2"><strong>Application additional information: </strong> (optional)<br>
    <input name="txtrevision" type="text" id="txtrevision" value="<?php echo $RSDB_TEMP_txtrevision; ?>" size="15" maxlength="15">
    <em><font size="1">(language, misc versions string; e.g. &quot;3.4 German&quot;, &quot;Demo&quot;, etc.)</font></em></font></p>
<p>&nbsp;</p>
<p><strong><font size="2">Application function:</font></strong></p>
<ul>
  <li><font size="2">
    <input name="optfunc" type="radio" value="5" <?php if ($RSDB_TEMP_optfunc == 5) echo "checked"; ?>> 
    5 stars = works fantastic</font></li>
  <li><font size="2">
    <input type="radio" name="optfunc" value="4" <?php if ($RSDB_TEMP_optfunc == 4) echo "checked"; ?>>
     4 stars = works good, minor bugs </font></li>
  <li><font size="2">
    <input name="optfunc" type="radio" value="3" <?php if ($RSDB_TEMP_optfunc == 3) echo "checked"; ?>>
    3 stars = works with bugs </font></li>
  <li><font size="2">
    <input type="radio" name="optfunc" value="2" <?php if ($RSDB_TEMP_optfunc == 2) echo "checked"; ?>>
    2 stars = major bugs</font></li>
  <li><font size="2">
    <input name="optfunc" type="radio" value="1" <?php if ($RSDB_TEMP_optfunc == 1) echo "checked"; ?>> 
    1 star = doesn't work, or crash while start phase </font></li>
</ul>
<p><strong><font size="2">Installation routine:</font></strong></p>
<ul>
  <li><font size="2">
    <input type="radio" name="optinstall" value="5" <?php if ($RSDB_TEMP_optinstall == 5) echo "checked"; ?>>
5 stars = works fantastic or no install routine </font></li>
  <li><font size="2">
  <input type="radio" name="optinstall" value="4" <?php if ($RSDB_TEMP_optinstall == 4) echo "checked"; ?>>
4 stars = works good, minor bugs </font></li>
  <li><font size="2">
  <input name="optinstall" type="radio" value="3" <?php if ($RSDB_TEMP_optinstall == 3) echo "checked"; ?>>
3 stars =  works with bugs </font></li>
  <li><font size="2">
  <input type="radio" name="optinstall" value="2" <?php if ($RSDB_TEMP_optinstall == 2) echo "checked"; ?>>
2 stars =  major bugs</font></li>
  <li><font size="2">
  <input name="optinstall" type="radio" value="1" <?php if ($RSDB_TEMP_optinstall == 1) echo "checked"; ?>>
1 star = doesn't work, or crash while start phase </font></li>
  </ul>
<p>&nbsp;</p>
<p><font size="2"><strong>Conclusion:</strong><br />
    <textarea name="txtconclusion" cols="70" rows="5" id="txtconclusion"><?php echo $RSDB_TEMP_txtconclusion; ?></textarea>
</font></p>
<p><font size="2">
  <input name="submitpost" type="hidden" id="submitpost" value="yes">
  Everyone will be able to vote on your compatibility test.</font></p>
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
