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
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_intern_code_view_name; ?></a> &gt; Submit Application/Driver
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p>ReactOS Software and Hardware Compatibility Database.</p>

<h1>Submit Application - Step 1 </h1>
<h2>Submit Application - Step 1 </h2>
  <?php 
  
if ($RSDB_intern_user_id <= 0) {
	please_register(); 
}
else {
		$RSDB_TEMP_subok = "";
		$RSDB_TEMP_vendmode = "";
		$RSDB_TEMP_cboCategory = "";
		$RSDB_TEMP_txtname = "";
		$RSDB_TEMP_txtdesc = "";
		$RSDB_TEMP_searchvendor = "";
		$RSDB_TEMP_vendrock = "";
		$RSDB_TEMP_rockhide = "";
		$RSDB_TEMP_cboVendor = "";
		$RSDB_TEMP_txtvname = "";
		$RSDB_TEMP_txtvurl = "";
		
		if (array_key_exists("subok", $_POST)) $RSDB_TEMP_subok=htmlspecialchars($_POST["subok"]);
		if (array_key_exists("vendmode", $_POST)) $RSDB_TEMP_vendmode=htmlspecialchars($_POST["vendmode"]);
		if (array_key_exists("category", $_POST)) $RSDB_TEMP_cboCategory=htmlspecialchars($_POST["category"]);
		if (array_key_exists("txtname", $_POST)) $RSDB_TEMP_txtname=htmlspecialchars($_POST["txtname"]);
		if (array_key_exists("txtdesc", $_POST)) $RSDB_TEMP_txtdesc=htmlspecialchars($_POST["txtdesc"]);
		if (array_key_exists("searchvendor", $_POST)) $RSDB_TEMP_searchvendor=htmlspecialchars($_POST["searchvendor"]);
		if (array_key_exists("vendrock", $_POST)) $RSDB_TEMP_vendrock=htmlspecialchars($_POST["vendrock"]);
		if (array_key_exists("rockhide", $_POST)) $RSDB_TEMP_rockhide=htmlspecialchars($_POST["rockhide"]);
		if (array_key_exists("vendor", $_POST)) $RSDB_TEMP_cboVendor=htmlspecialchars($_POST["vendor"]);
		if (array_key_exists("txtvname", $_POST)) $RSDB_TEMP_txtvname=htmlspecialchars($_POST["txtvname"]);
		if (array_key_exists("txtvurl", $_POST)) $RSDB_TEMP_txtvurl=htmlspecialchars($_POST["txtvurl"]);


		$RSDB_TEMP_SUBMIT_valid = true;
		
		//echo "<p>".$RSDB_TEMP_txtname."</p>";
		//echo "MODE:".$RSDB_TEMP_vendmode."|".$RSDB_TEMP_rockhide;
		
		if ($RSDB_TEMP_subok == "okay") {
			if ($RSDB_TEMP_cboCategory == 0) {
				msg_bar("Invalid Category!");
				echo "<br />";
				$RSDB_TEMP_SUBMIT_valid = false;
			}

			if (strlen($RSDB_TEMP_txtname) <= 1) {
				msg_bar("The 'Application/Driver Name' textbox is (almost) empty  ...");
				$RSDB_TEMP_SUBMIT_valid = false;
				echo "<br />";
			}
			if (strlen($RSDB_TEMP_txtdesc) <= 3) {
				msg_bar("The 'Description' textbox is (almost) empty  ...");
				$RSDB_TEMP_SUBMIT_valid = false;
				echo "<br />";
			}
			
			if ($RSDB_TEMP_vendmode == "" || $RSDB_TEMP_vendmode == "none") {
				msg_bar("Invalid Vendor!");
				echo "<br />";
				$RSDB_TEMP_SUBMIT_valid = false;
			}
			else {
				if ($RSDB_TEMP_vendmode == "select") {
					if ($RSDB_TEMP_cboVendor == 0 && strlen($RSDB_TEMP_txtvname) <= 1) {
						msg_bar("Invalid Vendor name!");
						$RSDB_TEMP_SUBMIT_valid = false;
						echo "<br />";
					}
				}
				if ($RSDB_TEMP_vendmode == "rock") {
					if ($RSDB_TEMP_rockhide == 0) {
						msg_bar("Invalid Vendor name!");
						echo "<br />";
						$RSDB_TEMP_SUBMIT_valid = false;
					}
				}
				if ($RSDB_TEMP_vendmode == "add") {
					if (strlen($RSDB_TEMP_txtvname) <= 1) {
						msg_bar("The Vendor name is (almost) empty  ...");
						$RSDB_TEMP_SUBMIT_valid = false;
						echo "<br />";
					}
					if (strlen($RSDB_TEMP_txtvurl) <= 3) {
						msg_bar("The Vendor Website textbox is (almost) empty  ...");
						$RSDB_TEMP_SUBMIT_valid = false;
						echo "<br />";
					}
				}
			}
      $stmt=CDBConnection::getInstance()->prepare("SELECT COUNT(*) FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_name = :name ORDER BY grpentr_id DESC LIMIT 1");
      $stmt->bindParam('name',$RSDB_TEMP_txtname,PDO::PARAM_STR);
      $stmt->execute();
			
			$result_check = $stmt->fetch();
			if ($result_check[0] != 0) {
				msg_bar("Double Entry Prevention: please check the data!");
				add_log_entry("low", "comp_group_submit", "submit", "Double Entry Prevention!", "Double Entry Prevention! \n\nAn error occur while checking the data. The data is invalid! Please submit the data again, thx. \n\nIf this message appear more then one times and you are sure everything is valid and okay, then please report this to the ReactOS forum.", $RSDB_intern_user_id);
				$RSDB_TEMP_SUBMIT_valid = false;
				echo "<br />";
			}
		}
			

	if ($RSDB_TEMP_subok != "okay" || $RSDB_TEMP_SUBMIT_valid != true) {

?>
	<noscript>
		<p>Sorry, the submit function is only usable with ECMAScript enabled! The noscript methode may be available soon!</p>
	</noscript>
	
<fieldset><legend>&nbsp;<b><font color="#000000">Submit an Application in 3 Steps</font></b>&nbsp;</legend>
	<table width="100%" border="0" cellpadding="1" cellspacing="5">
      <tr>
        <td width="33%"><h4>Step 1</h4></td>
        <td width="34%"><h4><font color="#999999">Step 2</font></h4></td>
        <td width="33%"><h4><font color="#999999">Step 3</font></h4></td>
      </tr>
      <tr>
        <td valign="top"><p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>general information</b>:</font></p>
            <ul>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">application name</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">short  decription</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">category</font></li>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">vendor </font></li>
            </ul></td>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>version information</b></font></p>
            <ul>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">application version</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">ReactOS</font> <font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">version</font></li>
            </ul></td>
        <td valign="top"><p><font color="#999999" size="2" face="Verdana, Arial, Helvetica, sans-serif">submit <b>test results &amp; screenshots</b></font></p>
            <ul>
              <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><font color="#999999">What works</font></font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">What does not work</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Describe what you have tested and what not</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Application function</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Installation routine</font></li>
              <li><font color="#999999" size="1" face="Verdana, Arial, Helvetica, sans-serif">Conclusion</font></li>
            </ul></td>
      </tr>
    </table>
</fieldset>
<br />
	<table width="100%" border="0">
    <tr>
      <td width="5%">&nbsp;</td>
      <td width="90%"><table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag1" style="display:table ">
          <tr>
            <td> 
              <table width="100%" border="0">
                <tr>
                  <td width="54"><img src="media/icons/info/submitapp.png" width="56" height="56"></td>
                  <td width="20" height="56">&nbsp;</td>
                  <td><h3> Welcome to the Submit Application Wizard - Step 1 </h3></td>
                </tr>
              </table>
              
              <table width="170" border="0" align="right">
                <tr>
                  <td><font size="2" face="Arial, Helvetica, sans-serif">
				  	<h1>Submit Checklist</h1>
                      
                    </font><ul>
                      <li><font size="2" face="Arial, Helvetica, sans-serif"><b>Test</b> the application/driver in ReactOS</font>.</li>
                      <li><font size="2" face="Arial, Helvetica, sans-serif"><b>Write down</b> testing results and take screenshots</font>.</li>
                      <li><font size="2" face="Arial, Helvetica, sans-serif"><b>Visit </b>the<b> vendor</b> <b>website</b> and collect information.</font></li>
                    </ul></td>
                </tr>
              </table>
              <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">This Submit Application<sup>&dagger;</sup> Wizard- Step 1  will guide you through the submission process. The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds<sup>&Dagger;</sup>. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</font></p>              
              <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
				  <label for="searchinput"><strong><br />
				  Application name:</strong></label> 
				  &nbsp;<font size="1"><em>(no vendor name, no versions number)</em></font><br />
				  <input name="searchinput" type="text" id="searchinput" tabindex="0" onblur="loadItemList(this.value,'submit','comp','ajaxload','submitresult')" onkeyup="loadItemList(this.value,'submit','comp','ajaxload','submitresult')" size="30" maxlength="100"/>
				  <img id="ajaxload" src="images/ajax_loading.gif"  style="display: none" />
			  </font></p>
			  <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
			  <div id="submitresult" style="display: none"></div>
			  </font></p>
			  <p>&nbsp;</p>
			  <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><sup>&dagger;</sup></font><strong><font size="1" face="Verdana, Arial, Helvetica, sans-serif"> </font></strong><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><em>&quot;application&quot; means in this context &quot;application or driver&quot;.
		      
			  </em></font><br />
			  <font size="2" face="Verdana, Arial, Helvetica, sans-serif"><sup>&Dagger;</sup></font> <font size="1" face="Verdana, Arial, Helvetica, sans-serif"><i>development builds are marked with: SVN, RC1, RC2, etc. </i></font></p>
			  <hr size="1" noshade color="#CCCCCC" id="sdsd">			  <table width="100%"  border="0">
                <tr>
                  <td width="50%" align="left"><button name="helpwizp" type="button" value="Help" onclick="WizHelp()">&nbsp;Help&nbsp;</button></td>
                  <td width="50%" align="right"><button name="nextwizp2" id="nextwizp2" type="button" value="Next Page" onclick="WizPag2()" disabled >&nbsp;Next&nbsp;&nbsp;&gt;&nbsp;</button></td>
                </tr>
              </table>			  </td>
          </tr>
        </table>
		
		
		<form name="RSDB_comp_submitapp" method="post" action="<?php echo $RSDB_intern_link_db_sec; ?>submit">
		<table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag2" style="display:none ">
          <tr>
            <td><table width="100%" height="106" border="0">
              <tr>
                <td align="left" valign="bottom"><p><font size="4" face="Verdana, Arial, Helvetica, sans-serif"><strong>Submit Application Wizard</strong></font><br />
                    <font size="3" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Input the application data </font></p></td>
                <td width="10%" height="56" valign="top"><div align="right"><img src="media/icons/info/submitapp.png" width="56" height="56"></div></td>
              </tr>
              <tr>
                <td height="21" colspan="2" align="left" valign="top"><hr size="1" noshade color="#CCCCCC" id="sdsd">
                    <p align="right"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">2/3</font></p></td>
              </tr>
              <tr>
                <td height="21" colspan="2" align="left" valign="top">

				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><strong>Application name:</strong>  <font size="1"><em>(e.g. &quot;Firefox&quot;, &quot;OpenOffice.org&quot;, etc; no vendor name and no version number/data)</em></font><br>
                        <strong>
						<input name="txtname" type="text" id="txtname" onkeyup="checkFields()" value="<?php echo htmlentities($RSDB_TEMP_txtname); ?>" size="50" maxlength="100" /> 
						<input name="txtregex" type="hidden" id="txtregex" value="\b[0-9]+\b" />			  
                    </strong></font></p>
				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><strong>Short Description:</strong> <font size="1"><em>(255 characters)</em></font><br>
                        <input name="txtdesc" type="text" id="txtdesc" onkeyup="checkFields()" value="<?php echo htmlentities($RSDB_TEMP_txtdesc); ?>" size="50" maxlength="255" />
			</font></p>
                    <p><font size="2"><strong><font face="Verdana, Arial, Helvetica, sans-serif">Category:</font></strong><font face="Verdana, Arial, Helvetica, sans-serif"><br>
                      <select name="category" id="category" onchange="checkFields()">
						<option value="0" <?php 
							if  ($RSDB_TEMP_cboCategory == "") {
								echo "selected";
							}
						?>>Select a category</option>
                        <?php 
							$RSDB_intern_selected = $RSDB_TEMP_cboCategory;
							include("inc/comp/sub/tree_category_combobox.php");
						?>
                    </select>
				</font></font></p>
			<div id="vendorfield" style="display: block">
				<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
					<label for="searchvendor"><strong>Vendor name:</strong></label> 
					</font><font face="Verdana, Arial, Helvetica, sans-serif"><font size="1"><i>(Vendor, Company, Team or Project)</i></font></font><font size="2" face="Verdana, Arial, Helvetica, sans-serif"></font><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><br />
					<input name="searchvendor" type="text" id="searchvendor" tabindex="0"  onblur="loadItemList(this.value,'submit_vendor','vendor','ajaxvendload','vendorresult')" onkeyup="loadItemList(this.value,'submit_vendor','vendor','ajaxvendload','vendorresult')" size="30" maxlength="100"/> <img id="ajaxvendload" src="images/ajax_loading.gif"  style="display: none" />
					</font></p>
					<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">
					<div id="vendorresult" style="display: none"></div>
				</font></p>
			</div>
			<div id="vendorfield2" style="display: none">
				<p><font face="Verdana, Arial, Helvetica, sans-serif"><font size="2"><strong>Vendor name:</strong><br>
				  <input name="vendrock" type="text" disabled id="vendrock" value="<?php echo htmlentities($RSDB_TEMP_vendrock); ?>" size="30" maxlength="100"> 
		            &nbsp;</font><font size="1">[<a href="javascript://" onclick="ChangeVendor()">change vendor</a>]
		            <input name="rockhide" type="hidden" id="rockhide" value="<?php echo htmlentities($RSDB_TEMP_rockhide); ?>">
		            </font></font></p>
			</div>
			<div id="vendorfield3" style="display: none">
                    <p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><strong>Vendor name:</strong> <em><font size="1">(Vendor, Company, Team or Project)</font></em><br>
					<select name="vendor" id="vendor" onchange="checkFields()">
						<option value="0" <?php 
						if  ($RSDB_TEMP_cboVendor == "") {
						echo "selected";
						}
						?>>Select a vendor</option>
						<?php
							$RSDB_intern_selected = $RSDB_TEMP_cboVendor;
							include("inc/comp/sub/tree_vendor_combobox.php");
						?>
					</select>
					</font><font face="Verdana, Arial, Helvetica, sans-serif"> <font size="2"> &nbsp;</font></font><font face="Verdana, Arial, Helvetica, sans-serif"><font size="1">[<a href="javascript://" onclick="ChangeVendor()">search for vendor</a>]</font></font><font size="2" face="Verdana, Arial, Helvetica, sans-serif">                    </font></p>
			</div>
				<div id="vendorsubmit" style="display: none">
                      <p><font face="Verdana, Arial, Helvetica, sans-serif"><strong><font size="2">Vendor name:</font></strong> <font size="1"><i>(Vendor, Company, Team or Project)</i></font><br>
                            <input name="txtvname" type="text" id="txtvname" size="30" maxlength="100" onkeyup="checkFields()" />
                            <font size="2"> &nbsp;</font><font size="1">[<a href="javascript://" onclick="ChangeVendor()">search for vendor</a>]</font><font face="Verdana, Arial, Helvetica, sans-serif"><font size="1"> </font><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><i> </i></font></font></font></p>
                      <p><font face="Verdana, Arial, Helvetica, sans-serif"> <strong><font size="2">Vendor website:</font><font face="Verdana, Arial, Helvetica, sans-serif"> <font size="1"><i></i></font></font></strong><font size="1" face="Verdana, Arial, Helvetica, sans-serif"><i> (e.g. http://www.company.com)</i></font><br>
                            <input name="txtvurl" type="text" id="txtvurl" value="http://" size="30" maxlength="100" onkeyup="checkFields()" />
                            <font face="Verdana, Arial, Helvetica, sans-serif"><font size="2"> &nbsp;</font></font><font size="1">[<a href="javascript://" onclick="window.open('http://www.google.com/search?q='+document.getElementById('txtvname').value)">search for website</a>] <em>(new window) </em></font></font></p>
				    </div>
					<p>&nbsp;</p>
					<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">By clicking &quot;Submit&quot; below you agree to be bound by the <a href="<?php echo $RSDB_intern_index_php; ?>?page=conditions" target="_blank">submit conditions</a>.</font></p>
					<hr size="1" noshade color="#CCCCCC" id="sdsd">
				<div align="right">
				  <table width="100%"  border="0" cellpadding="0" cellspacing="0">
                    <tr>
                      <td width="50%" align="left"><button name="backwizp1" type="button" value="Back to Page 1" onclick="WizPag1()">&nbsp;&lt;&nbsp;&nbsp;Back&nbsp;</button>						</td>
                      <td width="50%" align="right"><input name="vendmode" id="vendmode" type="hidden" value="">
                      <input name="subok" type="hidden" value="okay">
				  <input type="submit" name="Submit" id="Submit" value="&nbsp;Submit&nbsp;&nbsp;&gt;&nbsp;" disabled></td>
                    </tr>
                  </table>		
				</div></td>
              </tr>
            </table>
		
          </tr>
        </table></form>
      </td>
      <td width="5%">&nbsp;</td>
    </tr>
  </table>
  <script language="JavaScript1.2">
<!--

	var brow = navigator.appName;
	
	function WizPag1() {
		if (brow=="Microsoft Internet Explorer") { // work-around for IE CSS bug
			document.getElementById('pag1').style.display = 'block';
		}
		else {
			document.getElementById('pag1').style.display = 'table';
		}
		document.getElementById('pag2').style.display = 'none';
	}
	
	function WizPag2() {
		document.getElementById('pag1').style.display = 'none';
		if (brow=="Microsoft Internet Explorer") { // work-around for IE CSS bug
			document.getElementById('pag2').style.display = 'block';
		}
		else {
			document.getElementById('pag2').style.display = 'table';
		}
		if (document.getElementById('txtname').value == "") {
			document.getElementById('txtname').value = document.getElementById('searchinput').value;
		}
	}	

	function WizHelp() {
		alert("This Submit Application wizard will guide you through the submission process.\n\nPlease read the 'Help & FAQ' page (see menu bar) before you submit anything.\n\nIf you have problems with this wizard or suggestions/ideas, please visit the ReactOS Forum (htttp://www.reactos.org/forum/ -> Website Sub-Forum) and report problems or let us know your suggestions, ideas to improve the website.\n\nThank you for using the ReactOS Compatibility Database!");
	}
	
	function enableButtonWizPageNext2(opt) {
		if (opt == 1) {
			document.getElementById('nextwizp2').disabled=true;
		}
		else if (opt == 2) {
			document.getElementById('nextwizp2').disabled=false;
		}
	}
	
	function UseThisVendor(vendid,vendname) {
		document.getElementById('vendorfield').style.display = 'none';
		document.getElementById('vendorfield2').style.display = 'block';
		document.getElementById('vendorfield3').style.display = 'none';
		document.getElementById('vendorsubmit').style.display = 'none';
		document.getElementById('vendrock').value = vendname;
		document.getElementById('vendmode').value = "rock";
		document.getElementById('rockhide').value = vendid;
		
		checkFields();
	}

	function ChangeVendor() {
		document.getElementById('vendorfield').style.display = 'block';
		document.getElementById('vendorfield2').style.display = 'none';
		document.getElementById('vendorfield3').style.display = 'none';
		document.getElementById('vendorsubmit').style.display = 'none';
		document.getElementById('vendrock').value = "";
		document.getElementById('vendmode').value = "none";
		checkFields();
	}

	function SelectVendor() {
		document.getElementById('vendorfield').style.display = 'none';
		document.getElementById('vendorfield2').style.display = 'none';
		document.getElementById('vendorfield3').style.display = 'block';
		document.getElementById('vendorsubmit').style.display = 'none';
		document.getElementById('vendrock').value = "";
		document.getElementById('vendmode').value = "select";
		checkFields();
	}

	function AddVendor() {
		document.getElementById('vendorfield').style.display = 'none';
		document.getElementById('vendorfield2').style.display = 'none';
		document.getElementById('vendorfield3').style.display = 'none';
		document.getElementById('vendorsubmit').style.display = 'block';
		document.getElementById('vendrock').value = "";
		document.getElementById('vendmode').value = "add";
		checkFields();
	}

	function checkVenbox() {
		checkFields();
	}

	function checkFields() {
		var re;
		var m;
		var s;
		
		document.getElementById('category').style.background = '#FFFFFF';
		document.getElementById('txtname').style.background = '#FFFFFF';
		document.getElementById('txtdesc').style.background = '#FFFFFF';
		document.getElementById('searchvendor').style.background = '#FFFFFF';
		document.getElementById('vendrock').style.background = '#FFFFFF';
		document.getElementById('vendor').style.background = '#FFFFFF';
		document.getElementById('txtvname').style.background = '#FFFFFF';
		document.getElementById('txtvurl').style.background = '#FFFFFF';
		checking = false;
		
		if (document.getElementById('category').value == "0") {
			document.getElementById('category').style.background = '#FF9900';
			checking = true;
		}
		if (document.getElementById('txtname').value.length < 2) {
			document.getElementById('txtname').style.background = '#FF9900';
			checking = true;
		}
		if (document.getElementById('txtdesc').value.length <= 5) {
			document.getElementById('txtdesc').style.background = '#FF9900';
			checking = true;
		}
		if (document.getElementById('vendorfield').style.display == "block") {
			checking = true;
			if (document.getElementById('searchvendor').value.length < 2) { 
				document.getElementById('searchvendor').style.background = '#FF9900';
			}
		}
		if (document.getElementById('vendrock').value.length < 2 && document.getElementById('vendorfield2').style.display == "block") {
			document.getElementById('vendrock').style.background = '#FF9900';
			checking = true;
		}
		if (document.getElementById('vendor').value == "0" && document.getElementById('vendorfield3').style.display == "block") {
			document.getElementById('vendor').style.background = '#FF9900';
			checking = true;
		}
		if (document.getElementById('txtvname').value.length < 2 && document.getElementById('vendorsubmit').style.display == "block") {
			checking = true;
			document.getElementById('txtvname').style.background = '#FF9900';
		}
		if (document.getElementById('txtvurl').value.length <= 8 && document.getElementById('vendorsubmit').style.display == "block") {
			document.getElementById('txtvurl').style.background = '#FF9900';
			checking = true;
		}
		
		re = new RegExp(document.getElementById('txtregex').value);
		m = re.exec(document.getElementById('txtname').value);
		if (m != null) {
			alert("Please remove the application version number from the application name field! You will be able to input the application version later!");
			document.getElementById('txtname').style.background = '#FF9900';
			checking = true;
		}
		
		
		if (checking == false) {
			document.getElementById('Submit').disabled=false;
		}
		else {
			document.getElementById('Submit').disabled=true;
		}
	}

-->
</script>
<?php
			if ($RSDB_TEMP_SUBMIT_valid != true) {
				?>
				<script language="JavaScript">
				<!--
						document.getElementById('vendorfield').style.display = 'none';
						document.getElementById('vendorfield2').style.display = 'none';
						document.getElementById('vendorfield3').style.display = 'block';
						document.getElementById('vendorsubmit').style.display = 'none';
						document.getElementById('vendrock').value = "";
						checkFields();
						//alert("asds");
				-->
				</script>
				<?php
			}
		}
	else {

		if ($RSDB_TEMP_subok == "okay" && $RSDB_TEMP_SUBMIT_valid == true) { 
			
	
			if (strlen($RSDB_TEMP_txtvname) >= 1 && $RSDB_TEMP_vendmode == "add") {
				$rem_adr = "";
				if (array_key_exists('REMOTE_ADDR', $_SERVER)) $rem_adr=htmlspecialchars($_SERVER['REMOTE_ADDR']);
				
				$stmt=CDBConnection::getInstance()->prepare("INSERT INTO rsdb_item_vendor ( vendor_id , vendor_name , vendor_fullname , vendor_url , vendor_email , vendor_infotext , vendor_problem , vendor_usrid , vendor_usrip , vendor_date ) VALUES ('', :name, :fullname, :url, '', '', '', :user_id, :ip, NOW() )");
        $stmt->bindParam('name',$RSDB_TEMP_txtvname,PDO::PARAM_STR);
        $stmt->bindParam('fullname',$RSDB_TEMP_txtvname,PDO::PARAM_STR);
        $stmt->bindParam('url',$RSDB_TEMP_txtvurl,PDO::PARAM_STR);
        $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
        $stmt->bindParam('ip',$rem_adr,PDO::PARAM_STR);
        $stmt->execute();
				
        $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_item_vendor WHERE vendor_name = :name ORDER BY vendor_id DESC LIMIT 1");
        $stmt->bindParam('name',$RSDB_TEMP_txtvname,PDO::PARAM_STR);
        $stmt->execute();
				$result_vendor_entry = $stmt->fetch(PDO::FETCH_ASSOC)
				
				$RSDB_TEMP_cboVendor = $result_vendor_entry['vendor_id'];

			}
			if ($RSDB_TEMP_vendmode == "rock") {
				$RSDB_TEMP_cboVendor = $RSDB_TEMP_rockhide;
			}

      $stmt=CDBConnection::getInstance()->prepare("INSERT INTO rsdb_groups ( grpentr_id, grpentr_name, grpentr_visible, grpentr_category, grpentr_vendor, grpentr_description, grpentr_usrid, grpentr_date ) VALUES ('', :name, '1', :category, :vendor, :description, :user_id , NOW() )";
      $stmt->bindParam('name',$RSDB_TEMP_txtname,PDO::PARAM_STR);
      $stmt->bindParam('category',$RSDB_TEMP_cboCategory,PDO::PARAM_STR);
      $stmt->bindParam('vendor',$RSDB_TEMP_cboVendor,PDO::PARAM_STR);
      $stmt->bindParam('description',$RSDB_TEMP_txtdesc,PDO::PARAM_STR);
      $stmt->bindParam('user_id',$RSDB_intern_user_id,PDO::PARAM_STR);
      $stmt->execute();
			
      $stmt=CDBConnection::getInstance()->prepare("SELECT * FROM rsdb_groups WHERE grpentr_visible = '1' AND grpentr_name = :name ORDER BY grpentr_id DESC LIMIT 1");
      $stmt->bindParam('name',$RSDB_TEMP_txtname,PDO::PARAM_STR);
      $stmt->execute();
			
			$result_page1 = $stmt->fetch();
			
			// Stats update:
      $stmt=CDBConnection::getInstance()->prepare("UPDATE rsdb_stats SET stat_s_grp = (stat_s_grp + 1) WHERE stat_date = '". date("Y-m-d") ."' LIMIT 1");
      $stmt->execute();
			
?>
<table width="100%" border="0">
    <tr>
      <td width="5%">&nbsp;</td>
      <td width="90%">

<table width="100%" border="1" cellpadding="30" cellspacing="0" bordercolor="#CCCCCC" id="pag3">
  <tr>
    <td><table width="100%" height="106" border="0">
        <tr>
          <td align="left" valign="bottom"><p><font size="4" face="Verdana, Arial, Helvetica, sans-serif"><strong>Submit Application Wizard</strong></font><br />
                  <font size="3" face="Verdana, Arial, Helvetica, sans-serif">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Further information </font></p></td>
          <td width="10%" height="56" valign="top"><div align="right"><img src="media/icons/info/submitapp.png" width="56" height="56"></div></td>
        </tr>
        <tr>
          <td height="21" colspan="2" align="left" valign="top"><hr size="1" noshade color="#CCCCCC" id="sdsd">
              <p align="right"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">3/3</font></p></td>
        </tr>
        <tr>
          <td height="21" colspan="2" align="left" valign="top">	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><?php echo $RSDB_TEMP_txtname; ?> has been saved to the database!</font></p>
	<p><font size="2" face="Verdana, Arial, Helvetica, sans-serif">Thank you for your help! This was part one of three, the next step will be to submit application versions specific data. </font></p>
	<p>&nbsp;</p>
	<ul>
	  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><a href="<?php echo $RSDB_intern_link_group_comp.$result_page1['grpentr_id']; ?>&amp;addbox=submit">Please <strong>add more information</strong> to the &quot;<b><?php echo $result_page1['grpentr_name']; ?></b>&quot; entry!</a></font></li>
	</ul>
            <p>&nbsp;</p>
            <p align="center"><img src="images/progress-end.gif" style="width:1px; height:20px;" /><img src="images/progress-todo.gif" name="p1" id="p1" style="width:20px; height:20px;" /><img src="images/progress-todo.gif" name="p2"id="p2" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p3"id="p3" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p4"id="p4" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p5"id="p5" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p6"id="p6" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p7"id="p7" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p8"id="p8" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p9"id="p9" style="width:20; height:20px;" /><img src="images/progress-todo.gif" name="p10"id="p10" style="width:20; height:20px;" /><img src="images/progress-end.gif"
			   style="width:1px; height:20px;" /></p>
              <p align="center"><font size="2" face="Verdana, Arial, Helvetica, sans-serif">You are being redirected in <b><span id="counter">10</span></b> seconds. </font></p>             
		<p>&nbsp;</p>
	 <hr size="1" noshade color="#CCCCCC" id="sdsd">
               <div align="right">
				<button name="nextwiz" id="nextwiz" type="button" value="NextWizard" onclick="NextBigStep()" >&nbsp;Next&nbsp;&nbsp;&gt;&nbsp;</button>
            </div></td>
        </tr>
      </table>
  </tr>
</table></td>
      <td width="5%">&nbsp;</td>
    </tr>
</table>
<script language="JavaScript">
<!--
var start=new Date();
	start=Date.parse(start)/1000;
	var counts=10;
	var count2="0";
	
	function CountDown(){
		var now=new Date();
		now=Date.parse(now)/1000;
		var x=parseInt(counts-(now-start),10);
		document.getElementById('counter').innerHTML = x;
		switch(x) {
			case 9:
				count2="1";
				break;
			case 8:
				count2="2";
				break;
			case 7:
				count2="3";
				break;
			case 6:
				count2="4";
				break;
			case 5:
				count2="5";
				break;
			case 4:
				count2="6";
				break;
			case 3:
				count2="7";
				break;
			case 2:
				count2="8";
				break;
			case 1:
				count2="9";
				break;
			case 0:
				count2="10";
				break;
		}
		if (count2 != "0") {
			document.getElementById("p"+count2).src = "images/progress-done.gif";
		}
		if(x>0){
			timerID=setTimeout("CountDown()", 100);
		}else{
			location.href="<?php echo $RSDB_intern_link_group_comp_javascript.$result_page1['grpentr_id']; ?>&addbox=submit";
		}
	}

	function NextBigStep(){
		location.href="<?php echo $RSDB_intern_link_group_comp_javascript.$result_page1['grpentr_id']; ?>&addbox=submit";
	}
	
	window.setTimeout('CountDown()',100);
-->
</script>
<?php
		}
		else {
			echo "<p>An error occur while checking the data. The data is invalid! Please submit the data again, thx. If this message appear more then one times and you are sure everything is valid and okay, then please report this to the ReactOS forum (please explain everything detailed so that we can recover the problem and fix it as soon as possible then).</p>";
			echo '<p><a href="javascript:history.go(-1)">Click here to go back!</a></p>';
		}
	}
}
?>
