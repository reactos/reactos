<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="{lang}" lang="{lang}" dir="{dir}">
<head>
	<meta http-equiv="Content-Type" content="{~ mimetype}; charset={~ charset}" />
	{headlinks}
	{headscripts}
	<title>{pagetitle}</title>
	<style type="text/css" media="screen, projection">/*<![CDATA[*/ @import "{~ stylepath}/{~ stylename}/main.css?5"; /*]]>*/</style>
	<link rel="stylesheet" type="text/css" {if notprintable {media="print"}} href="{~ stylepath}/common/commonPrint.css" />
	<!--[if lt IE 5.5000]><style type="text/css">@import "{~ stylepath}/{~ stylename}/IE50Fixes.css";</style><![endif]-->
	<!--[if IE 5.5000]><style type="text/css">@import "{~ stylepath}/{~ stylename}/IE55Fixes.css";</style><![endif]-->
	<!--[if IE 6]><style type="text/css">@import "{~ stylepath}/{~ stylename}/IE60Fixes.css";</style><![endif]-->
	<!--[if IE 7]><style type="text/css">@import "{~ stylepath}/{~ stylename}/IE70Fixes.css?1";</style><![endif]-->
	<!--[if lt IE 7]><script type="{jsmimetype}" src="{~ stylepath}/common/IEFixes.js"></script>
	<meta http-equiv="imagetoolbar" content="no" /><![endif]-->
	<script type="{jsmimetype}">var skin = '{~ skinname}';var stylepath = '{~ stylepath}';</script>
	<script type="{jsmimetype}" src="{~ stylepath}/common/wikibits.js"><!-- wikibits js --></script>
	{if jsvarurl {<script type="{jsmimetype}" src="{jsvarurl}"><!-- site js --></script>}}
	{if pagecss {<style type="text/css">{pagecss}</style>}}
	{usercss}
	{sitecss}
	{gencss}
	{if userjs {<script type="{jsmimetype}" src="{userjs}"></script>}}
	{if userjsprev {<script type="{jsmimetype}">{userjsprev}</script>}}
	{trackbackhtml}
</head>
<body {if body_ondblclick {ondblclick="{body_ondblclick}"}} {if body_onload {onload="{body_onload}" }} class="{~ nsclass} {~ dir}">
<div id="globalWrapper">
	<div id="column-content">
		<div id="content">
			<a name="top" id="top"></a>
			{if sitenotice {<div id="siteNotice">{sitenotice}</div> }}
			<h1 class="firstHeading">{title}</h1>
			<div id="bodyContent">
				<h3 id="siteSub">{msg {tagline}}</h3>
				<div id="contentSub">{subtitle}</div>
				{if undelete {<div id="contentSub2"><span class="subpages">{undelete}</span></div> }}
				{if newtalk {<div class="usermessage">{newtalk}</div> }}
				{if showjumplinks {
					<div id="jump-to-nav">{msg {jumpto}} <a href="#column-one">{msg {jumptonavigation}}</a>, 
					<a href="#searchInput">{msg {jumptosearch}}</a></div> 
				}}
				<!-- start content -->
				{bodytext}
				{if catlinks {<div id="catlinks">{catlinks}</div> }}
				<!-- end content -->
				<div class="visualClear"></div>
			</div>
		</div>
	</div>
	<div id="column-one">
		<div id="p-cactions" class="portlet">
			<h5>{msg {views}}</h5>
			<ul>
				{if is_special {
					<li id="ca-article" class="selected"><a href="{request_url}">{msg {specialpage}}</a></li>
				} {
					<li id="ca-{nskey}" {selecttab {subject} subject_newclass}><a href="{subject_url}">{msg nskey}</a></li>
					<li id="ca-talk" {selecttab {talk} talk_newclass}><a href="{talk_url}">{msg {talk}}</a></li>
					{if can_edit {
						{if is_talk {
							<li id="ca-edit" {selecttab {edit} {istalk}}><a href="{edit_url}">{msg {edit}}</a></li>
							<li id="ca-addsection" {selecttab {addsection}}><a href="{localurl {action=edit&section=new}}">{msg {addsection}}</a></li>
						} {
							<li id="ca-edit" {selecttab {edit}}><a href="{edit_url}">{msg {edit}}</a></li>
						}}
					} {
						<li id="ca-viewsource" {selecttab {edit}}><a href="{edit_url}">{msg {viewsource}}</a></li>
					}}

					{if article_exists {
						<li id="ca-history" {selecttab {history}}><a href="{localurl {action=history}}">{msg {history_short}}</a></li>
						{if {{ is_allowed {protect} }} {
							{if is_ns_mediawiki {} {
								{if is_protected {
									<li id="ca-protect" {selecttab {protect}}><a href="{localurl {action=unprotect}}">{msg {unprotect}}</a></li>
								} {
									<li id="ca-protect" {selecttab {protect}}><a href="{localurl {action=protect}}">{msg {protect}}</a></li>
								}}
							}}
						}}
						
						{if {{ is_allowed {delete} }} {
								<li id="ca-delete" {selecttab {delete}}><a href="{localurl {action=delete}}">{msg {delete}}</a></li>
						}}
						{if {{ is_allowed {move} }} {
							{if can_move {
									<li id="ca-move" {selecttab {move}}><a href="{move_url}">{msg {move}}</a></li>
							}}
						}}
						{if is_loggedin {
							{if is_watching {
								<li id="ca-unwatch" {selecttab {watch}}><a href="{localurl {action=unwatch}}">{msg {unwatch}}</a></li>
							} {
								<li id="ca-watch" {selecttab {watch}}><a href="{localurl {action=watch}}">{msg {watch}}</a></li>
							}}
						}}
					}}
				}}
				{extratabs {<li id="ca-$id" $class><a href="$href">$text</a></li>}}
			</ul>
		</div>
		<div class="portlet" id="p-personal">
			<h5>{msg {personaltools}}</h5>
			<div class="pBody">
				<ul>						
					{personal_urls { <li id="pt-$key" $classactive ><a href="$href" $class>$text</a></li> }}
				</ul>
			</div>
		</div>
		<div class="portlet" id="p-logo">
			<a style="background-image: url({~ logopath});" href="{mainpage}" title="{msg {mainpage}}"></a>
		</div>
		<script type="{jsmimetype}"> if (window.isMSIE55) fixalpha(); </script>
		{sidebar {
			<div class='portlet' id="p-$bar">
				<h5>$barname</h5>
				<div class='pBody'>
				<ul>
		} {
				</ul>
				</div>
			</div>
		} {<li id="$id" $classactive><a href="$href">$text</a></li>
			}
		}

		<div id="p-search" class="portlet">
			<h5><label for="searchInput">{msg {search}}</label></h5>
			<div id="searchBody" class="pBody">
				<form action="{searchaction}" id="searchform"><div>
					<input id="searchInput" name="search" type="text" {
						}{if {{fallbackmsg {accesskey-search} {} }} {accesskey="{fallbackmsg {accesskey-search} {} }"}}{
						}{if search { value="{search}"}} />
					<input type='submit' name="go" class="searchButton" id="searchGoButton"	value="{msg {go}}" />&nbsp;
					<input type='submit' name="fulltext" class="searchButton" value="{msg {search}}" />
				</div></form>
			</div>
		</div>
		<div class="portlet" id="p-tb">
			<h5>{msg {toolbox}}</h5>
			<div class="pBody">
				<ul>
					{if notspecialpage    {<li id="t-whatlinkshere"><a href="{nav_whatlinkshere}">{msg {whatlinkshere}}</a></li> }}
					{if article_exists {<li id="t-recentchangeslinked"><a href="{nav_recentchangeslinked}">{msg {recentchangeslinked}}</a></li> }}
					{if nav_trackbacklink {<li id="t-trackbacklink"><a href="{nav_trackbacklink}">{msg {trackbacklink}}</a></li>}}
					{if feeds
						{<li id="feedlinks">{feeds {<span id="feed-$key"><a href="$href">$text</a>&nbsp;</span>}}
					</li>}}
					{if is_userpage {
						<li id="t-contributions"><a href="{nav_contributions}">{msg {contributions}}</a></li>
						{if {{is_allowed {block}}} {<li id="t-blockip"><a href="{nav_blockip}">{msg {blockip}}</a></li>}}
						{if is_loggedin     {<li id="t-emailuser"><a href="{nav_emailuser}">{msg {emailuser}}</a></li>}}
					}}
					{if nav_upload        {<li id="t-upload"><a href="{nav_upload}">{msg {upload}}</a></li>}}
					{if nav_specialpages  {<li id="t-specialpages"><a href="{nav_specialpages}">{msg {specialpages}}</a></li>}}
					{if nav_print         {<li id="t-print"><a href="{nav_print}">{msg {printableversion}}</a></li>}}
					{if nav_permalink     {<li id="t-permalink"><a href="{nav_permalink}">{msg {permalink}}</a></li>}}
					{if is_permalink      {<li id="t-ispermalink">{msg {permalink}}</li>}}

					{toolboxend}
				</ul>
			</div>
		</div>
		{language_urls {
			<div id="p-lang" class="portlet">
				<h5>{msg {otherlanguages}}</h5>
				<div class="pBody">
					<ul>
						$body
					</ul>
				</div>
			</div>
		} {
			<li class="$class"><a href="$href">$text</a></li>
		}}	
	</div><!-- end of the left (by default at least) column -->
	<div class="visualClear"></div>
	<div id="footer">
		{if poweredbyico { <div id="f-poweredbyico">{poweredbyico}</div> }}
		{if copyrightico { <div id="f-copyrightico">{copyrightico}</div> }}

		<ul id="f-list">
			{if lastmod                {  <li id="lastmod">{lastmod}</li> }}
			{if viewcount              {  <li id="viewcount">{viewcount}</li> }}
			{if numberofwatchingusers  {  <li id="numberofwatchingusers">{numberofwatchingusers}</li> }}
			{if credits                {  <li id="credits">{credits}</li> }}
			{if is_currentview         {  <li id="copyright">{normalcopyright}</li> }}
			{if usehistorycopyright    {  <li id="copyright">{historycopyright}</li> }}
			{if privacy                {  <li id="privacy">{privacy}</li> }}
			{if about                  {  <li id="about">{about}</li> }}
			{if disclaimer             {  <li id="disclaimer">{disclaimer}</li> }}
			{if tagline                {  <li id="tagline">{tagline}</li> }}
		</ul>
	</div>
	<script type="text/javascript"> if (window.runOnloadHook) runOnloadHook();</script>
</div>
{reporttime}
{if {} { vim: set syn=html ts=2 : }}
</body></html>
