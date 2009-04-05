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
<a href="<?php echo $RSDB_intern_link_db_sec; ?>home"><?php echo $RSDB_langres['TEXT_compdb_short']; ?></a> &gt; Help &amp; FAQ
<a href="<?php echo $RSDB_intern_index_php; ?>?page=about"><img src="media/pictures/compatibility_small.jpg" vspace="1" border="0" align="right"></a>
</h1> 
<p>ReactOS Software and Hardware Compatibility Database.</p>

<h1><a name="top"></a>Help &amp; FAQ</h1>
<p><strong>Content</strong></p>
<ul>
  <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#faq">FAQ</a>
      <ul>
        <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#faq_comp">com&middot;pat&middot;i&middot;ble, <em>adj. </em></a></li>
        <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#faq_compdb">Compatibility Database FAQ </a></li>
      </ul>
  </li>
  <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#sym">Symbols</a>    
    <ul>
      <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#sym_awards">Awards</a></li>
      <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#sym_medals">Other Medals</a></li>
      <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#sym_stars">Stars</a></li>
    </ul>
  </li>
  <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#help">Help</a>
    <ul>
      <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#browse">Browse through the database</a></li>
      <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit">Submit Application/Driver (3 Steps)</a>
        <ul>
          <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit1">Step 1 - general information</a></li>
          <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit2">Step 2 - version information</a></li>
          <li><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit3">Step 3   - test results &amp; screenshots</a></li>
        </ul>
      </li>
    </ul>
  </li>
</ul>

<p>&nbsp;</p>
<h2><a name="faq"></a>FAQ</h2>
<h3><a name="faq_comp"></a>com&middot;pat&middot;i&middot;ble, <em>adj. </em> </h3>
<ul>
  <li>Capable of existing or performing in harmonious, agreeable, or congenial combination with another or others: compatible family relationships. </li>
  <li>Capable of orderly, efficient integration and operation with other elements in a system with no modification or conversion required. </li>
</ul>




<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h3><a name="faq_compdb"></a>Compatibility Database FAQ </h3>
<p>This FAQ answers questions related to the usage of the Compatibility Database. </p>
<ul>
  <p><strong>Q: What is the Compatibility Database? </strong></p>
  <p><strong>A: </strong> The Compatibility Database is a repository for  application and driver compatibility information with ReactOS. In particular it provides the following information: 
  <ul>
    <li>Whether a given application works at all with ReactOS.
    <li>If it is partially working, then which areas of the application have problems.
    <li>How to install and get that application working best. </li>
    <li>The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</li>
  </ul>
    <p><strong>Q: What is ReactOS? </strong></p>
    <p><strong>A: </strong>More information can be found at the <a href="http://www.reactos.org/?page=about_whatisreactos">What is ReactOS?</a> and at the <a href="http://www.reactos.org/?page=about_userfaq">User FAQ</a> pages.    </p>
    <p><strong>Q: What are the benefits of this Compatibility Database? </strong></p>
    <p><strong>A: </strong> The Compatibility Database benefits both ReactOS developers and users. It lets ReactOS developers know which applications regressed. And it lets ReactOS user know  if their application will work in ReactOS.
    <p><strong>Q: How does a new application get added to the database? </strong></p>
    <p><strong>A: </strong> Every registered user can submit new applications or driver, version information, test reports, etc. </p>
    <p><strong>Q: How can I submit test reports of unstable ReactOS versions?</strong></p>
    <p><strong>A: </strong>The Compatibility  Database is for <i><b>release versions</b></i> of ReactOS, use <a href="http://www.reactos.org/bugzilla/">Bugzilla</a> for development builds. [<a href="http://www.reactos.org/wiki/index.php/File_Bugs">more</a>]</p>
    <p><strong>Q: How can I submit Screenshots? </strong></p>
    <p><strong>A: </strong> Click on the &quot;submit screenshot&quot; (or similar) link on the related page. </p>
    <p><strong>Q: How can I submit How-to's? </strong></p>
    <p><strong>A: </strong>This feature is currently not available. Please use the forum/comment function for the meanwhile. </p>
    <p><strong>Q: What do the different color awards and medals mean? </strong></p>
    <p><strong>A:</strong> They indicate different levels of functionality in ReactOS. <a href="<?php echo $RSDB_intern_link_HRU_help; ?>#sym">Go here</a> for more information. </p>
    <p><strong>Q: Why isn't my favorite Windows application in the database? </strong></p>
    <p><strong>A: </strong>Probably because no one has submitted it to the database yet. We'd love it if you would <a href="<?php echo $RSDB_intern_link_db_sec; ?>submit">submit the application to the database</a>.  </p>
    <p> <strong>Q: If I don't see a Windows application in the database, does that mean that it won't run in ReactOS? </strong></p>
    <p> <strong>A:</strong> Not necessarily. Many applications work perfectly in ReactOS without any modification whatsoever. However, we may simply not be aware of them. So, just because an application isn't in our database doesn't mean that your application won't run. We'd love it if you would <a href="<?php echo $RSDB_intern_link_db_sec; ?>submit">submit the application to the database</a>. </p>
    <p> <strong>Q: If I don't see my favorite application here, what's the easiest way to find out if it will run in ReactOS or not? </strong></p>
  <p> <strong>A:</strong> <a href="http://www.reactos.org/?page=download">Download ReactOS</a> and find out. That is by far the fastest, easiest way to answer this question. Regardless of whether the answer is "yes" or "no", we'd love it if you would <a href="<?php echo $RSDB_intern_link_db_sec; ?>submit">submit the application to the database</a>. </p>
  <p> <strong>Q: Are there any Windows applications that you can't get working in ReactOS? </strong></p>
  <p> <strong>A:</strong> In a later version of ReactOS, you will be able to run almost all Win32 applications and all WinNT 3+ driver.</p>
  <p><strong>Q: Is it possible to run Linux applications in ReactOS? </strong></p>
  <p> <strong>A:</strong> In a later version of ReactOS, you will be able to run almost all posix compatible linux/unix/bsd applications with the ReactOS posix subsystem.</p>
</ul>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h2><a name="sym"></a>Symbols</h2>

<h3><a name="sym_awards"></a>Awards</h3>
  
<table width="400" border="0" cellpadding="3" cellspacing="1">
  <tr> 
    <td width="100"> 
    <div align="center"><img src="media/icons/awards/platinum_32.gif" alt="Platinum" width="32" height="32"></div></td>
    <td width="100"> 
    <div align="center"><img src="media/icons/awards/gold_32.gif" alt="Gold" width="32" height="32"></div></td>
    <td width="100"> 
    <div align="center"><img src="media/icons/awards/silver_32.gif" alt="Silver" width="32" height="32"></div></td>
    <td width="100"> 
    <div align="center"><img src="media/icons/awards/bronze_32.gif" alt="Bronze" width="32" height="32"></div></td>
  </tr>
  <tr valign="top"> 
    <td> <div align="center"><font size="3">Platinum</font></div></td>
    <td> <div align="center"> <font size="3">Gold</font></div></td>
    <td> <div align="center"> <font size="3">Silver</font></div></td>
    <td> <div align="center"> <font size="3">Bronze</font></div></td>
  </tr>
</table>
<ul>
  <li><strong>Platinum Award:</strong> Applications and drivers which run better 
    (e.g. more stable, faster, better graphic, etc.) in ReactOS than in Microsoft 
    Windows(TM).</li>
  <li> <strong>Gold Award:</strong> Applications and drivers which install and 
    run virtually flawless on a out-of-the-box ReactOS installation.</li>
  <li> <strong>Silver Award:</strong> Applications and drivers which we hope we 
    can easily fix so they make it to Gold status.</li>
  <li> <strong>Bronze Award:</strong> Applications and drivers which works with 
    some minor problems/bugs.</li>
</ul>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>

<h3><a name="sym_medals"></a>Other Medals</h3>
<table width="330" border="0" cellpadding="3" cellspacing="1">
  <tr> 
    <td width="110"> 
      <div align="center"><img src="media/icons/awards/honor_32.gif" alt="Honorable Mention" width="32" height="32"></div></td>
    <td width="110"> 
      <div align="center"><img src="media/icons/awards/questionmark_32.gif" alt="Untested" width="32" height="32"></div></td>
    <td width="110"><div align="center"><img src="media/icons/awards/fail_32.gif" alt="Known not to work" width="32" height="32"></div></td>
  </tr>
  <tr valign="top"> 
    <td> 
      <div align="center"><font size="3">Honorable Mention</font></div></td>
    <td><div align="center"><font size="3">Untested</font></div></td>
    <td><div align="center">Known not to work</div></td>
  </tr>
</table>
<ul>
  <li><strong>Honorable Mention:</strong> Applications and drivers 


 which run well enough to be usable. However  it has enough bugs to prevent it from running flawlessly.</li>
  <li><strong>Untested:</strong> Applications and drivers where no compatibility 
    data is stored in the database.</li>
  <li><strong>Known not to work:</strong> Applications and drivers which do not 
    install/run on a out-of-the-box ReactOS installation.</li>
</ul>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>

<h3><a name="sym_stars"></a>Stars</h3>
<h4>Application function</h4>
<ul>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"> 5/5 = works fantastic</font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 4/5 = works good, minor bugs </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 3/5 = works with bugs </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 2/5 = major bugs</font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 1/5 = does not work, or crash while start phase </font></li>
</ul>
<h4>(Un-)Installation routine</h4>
<ul>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"> 5/5 = works fantastic or no install routine </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 4/5 = works good, minor bugs </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 3/5 = works with bugs </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 2/5 = major bugs</font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 1/5 = does not work, or crash while start phase </font></li>
</ul>
<h4>Rating / Voting</h4>
<ul>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"> 5/5 = fantastic</font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 4/5 = good </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 3/5 = okay </font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 2/5 = boring / uninteresting</font></li>
  <li><font size="2"><img src="media/icons/stars/star_gold.gif" alt="*" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"><img src="media/icons/stars/star_gray.gif" alt="_" width="15" height="15"> 1/5 = bad / spam</font></li>
</ul>

<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>

<h2><a name="help"></a>Help</h2>
<h3><a name="browse"></a>Browse through the database</h3>
<p>You can browse the Compatibility Database by Category, Applicationname, Vendor , Rank and search by query.<br>
Click on the related button on the frontpage or use the search function, the Compatibility Database starts returning results before you finish typing. </p>
<p><img src="media/pictures/help/startcomp.jpg" width="400" height="228" border="1"></p>
<p>If you want to browse even faster then this hint is for you:</p>
<ul>
  <li>Press [Alt] + [s] keys and start typing an application name (or parts of it). While you typing you see a real-time results in the search box (menu bar, left side).</li>
  <li>Press [Tab] several times to select the entry you want to view.</li>
  <li>Press [Shift] + [Tab] to go the inverted way and a select the entry above the current selected entry </li>
  <li>Press [Enter] to open the selected entry.</li>
</ul>
<p>This methode is useable on all ReactOS Compatibility Database pages. </p>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h3><a name="submit"></a>Submit Application/Driver in 3 Steps</h3>
<ul>
  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><b><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit1">Step 1:</a></b> submit <b>general information</b></font>
    <ul>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">application name</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">short  decription</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">category</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">vendor </font></li>
    </ul>
  </li>
</ul>
<ul>
  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><b><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit2">Step 2:</a></b> submit <b>version information</b></font>
    <ul>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">application version</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">ReactOS</font> <font size="1" face="Verdana, Arial, Helvetica, sans-serif">version</font></li>
    </ul>
  </li>
</ul>
<ul>
  <li><font size="2" face="Verdana, Arial, Helvetica, sans-serif"><b><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#submit3">Step 3:</a></b> submit <b>test results &amp; screenshots</b></font>
    <ul>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">What works</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">What does not work</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Describe what you have tested and what not</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Application function</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Installation routine</font></li>
      <li><font size="1" face="Verdana, Arial, Helvetica, sans-serif">Conclusion</font></li>
    </ul>
  </li>
</ul>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h4><a name="submit1"></a>Step 1</h4>
<p>To submit a <b>new application</b> (e.g. &quot;AbiWord&quot;):</p>
<ol>
  <li>Click on the &quot;Submit Application&quot; link in the menu bar (left side; on every page).</li>
  <li>Input the application/driver name. </li>
  <li> Follow the instructions (see screenshot).</li>
</ol>
<p><img src="media/pictures/help/submitcomp1.jpg" width="600" height="387" border="1"></p>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h4><a name="submit2"></a>Step 2</h4>
<p>To submit an <strong>application versions information </strong> (e.g. &quot;AbiWord 2.2&quot;):</p>
<ol>
  <li>Browse to the application (group entry).</li>
  <li>Click on the &quot;Submit new 'AbiWord' version&quot; link.</li>
  <li>Follow the instructions (see screenshot).</li>
</ol>
<p><img src="media/pictures/help/submitcomp2.jpg" width="400" height="343" border="1"></p>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
<h4><a name="submit3"></a>Step 3 </h4>
<p>To submit an <strong>application test report</strong> (e.g. &quot;AbiWord 2.2 tested in ReactOS 0.3.0&quot;):</p>
<ol>
  <li> Browse to the application versions  entry.</li>
  <li>Click on the &quot;Submit Compatibility Test Report&quot; link.</li>
  <li>Follow the instructions  (see screenshot).</li>
</ol>
<p><img src="media/pictures/help/submitcomp3.jpg" width="500" height="189" border="1"></p>
<p><em><a href="<?php echo $RSDB_intern_link_HRU_help; ?>#top">top</a></em></p>
