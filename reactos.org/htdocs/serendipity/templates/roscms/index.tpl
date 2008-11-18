<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN"
    "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" >

<head>
	<title>ReactOS Homepage - Blogs</title>
	
	<meta http-equiv="Content-Type" content="application/xml"; charset="utf-8"  />
	<meta http-equiv="pics-label" content='(pics-1.1 "http://www.icra.org/ratingsv02.html" l gen true for "http://www.reactos.com/" r (cz 1 lz 1 nz 1 oz 1 vz 1))' />
	<meta http-equiv="Pragma" content="no-cache" />
	<meta name="Publisher" content="ReactOS Web Team" />
	<meta name="Copyright" content="ReactOS Foundation" />
	<meta name="generator" content="RosCMS" />
	<meta name="Keywords" content="reactos, ros, ros32, win32 ros64, roscms, operating system, wiki, forum, download, information, support database, compatibility database, compatibility, package database, reactos package manager, reactos content management system, reactos 0.1, reactos 0.2, reactos 0.3, ros 0.1, ros 0.2, ros 0.3" />
	<meta name="Description" content="ReactOS Homepage - ..." />
	<meta name="Page-topic" content="Operating System, Computer, Software, downloads" />
	<meta name="Audience" content="all" />
	<meta name="Content-language" content="en" />
	<meta name="Page-type" content="Operating System/Information/News" />
	<meta name="Robots" content="index,follow" />
	
	<link rel="SHORTCUT ICON" href="http://www.reactos.org/favicon.ico" />
	<link rel="stylesheet" type="text/css" href="http://www.reactos.org/style.css" />
	<link rel="stylesheet" type="text/css" href="{$head_link_stylesheet}" />
	<link rel="alternate"  type="application/rss+xml" title="{$blogTitle} RSS feed" href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/index.rss2" />
	<link rel="alternate"  type="application/x.atom+xml"  title="{$blogTitle} Atom feed"  href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/atom.xml" 
{if $entry_id}
	<link rel="pingback" href="{$serendipityBaseURL}comment.php?type=pingback&amp;entry_id={$entry_id}" />
{/if}

{serendipity_hookPlugin hook="frontend_header"}

</head>
<body>
<div id="top">
  <div id="topMenu"> 
    <!-- 
       Use <p> to align things for links/lynx, then in the css make it
	   margin: 0; and use text-align: left/right/etc;.
   -->
	<p style="text-align:left;"> 
		<a href="http://www.reactos.org/">Home</a> <span style="color: #ffffff">|</span> 
		<a href="http://www.reactos.org/?page=about">Info</a> <span style="color: #ffffff">|</span> 
		<a href="http://www.reactos.org/?page=community">Community</a> <span style="color: #ffffff">|</span> 
		<a href="http://www.reactos.org/?page=dev">Development</a> <span style="color: #ffffff">|</span> 
		<a href="http://www.reactos.org/roscms/?page=user">myReactOS</a> </p>
	 </div>
	</div>
<table style="border:0" width="100%" cellpadding="0" cellspacing="0">
  <tr valign="top">
  <td style="width:147px" id="leftNav"> 
  <div class="navTitle">Navigation</div>
    <ol>
      <li><a href="http://www.reactos.org/">Home</a></li>
      <li><a href="http://www.reactos.org/?page=about">Info</a></li>
      <li><a href="http://www.reactos.org/?page=community">Community</a></li>
      <li><a href="http://www.reactos.org/?page=dev">Development</a></li>
      <li><a href="http://www.reactos.org/roscms/?page=user">myReactOS</a></li>
    </ol>
  <p></p>

 <div class="navTitle">Search</div>   
 <div class="navBox"><form method="get" action="/serendipity/index.php" style="padding:0;margin:0">
  <div style="text-align:center;">

   <input type="hidden"  name="serendipity[action]" value="search" />
   <input name="serendipity[searchTerm]" value=""  size="12" maxlength="80" class="searchInput" type="text" />
   <input name="btnG" value="Go" type="submit" class="button" />

  </div></form>
 </div>
<p></p>
{if $rightSidebarElements > 0}
    {serendipity_printSidebar side="right"}
    {serendipity_printSidebar side="left"}
{/if}
      </td>

    <td id="content"><div class="contentSmall">	

<h1><a href="/xhtml/en/community.html">ReactOS Community</a> &gt;
<a href="/serendipity/">ReactOS Blogs</a></h1>

<table width='100' border='0' align='right'>
  <tr>
    <td><div align='right'><a href="/serendipity/index.php?/feeds/index.rss2"><img src='http://www.reactos.org/images/rss_200.png' width='80' height='15' border='0' alt='RSS 2.0 News Feed'></a></div></td>

  </tr>
  <tr>
    <td><div align='right'><a href="/serendipity/index.php?/feeds/atom10.xml"><img src='http://www.reactos.org/images/atom_100.png' width='80' height='15' border='0' alt='Atom 1.0 News Feed'></a></div></td>
  </tr>
</table>
{$CONTENT}
</div></td>
 </tr>
</table>

<!--
     links/lynx/etc.. dont handle css (atleast not external
     files by default) so dont overly depend on it.
 -->
<table border='0' align='right'>
 <tr>
  <td valign='center'><address>Powered by</address></td>
  <td><a title="Powered by Serendipity" href="http://www.s9y.org/"><img src="/serendipity/templates/default/img/s9y_banner_small.png" alt="Serendipity PHP Weblog" style="border: 0px" /></a></td>
 <tr>
</table>
<br />
<br />
<br />
<hr style="height: 1px;"/>

<address style="text-align:center;">
  ReactOS is a registered trademark or a trademark of ReactOS Foundation in the United States and other countries.</address>
</body>
</html>									
