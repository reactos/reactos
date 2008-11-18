{if $is_embedded != true}
{if $is_xhtml}
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
           "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
{else}
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
           "http://www.w3.org/TR/html4/loose.dtd">
{/if}

<html>
<head>
    <title>{$head_title|@default:$blogTitle} {if $head_subtitle} - {$head_subtitle}{/if}</title>
    <meta http-equiv="Content-Type" content="text/html; charset={$head_charset}" />
    <meta name="Powered-By" content="Serendipity v.{$head_version}" />
    <link rel="stylesheet" type="text/css" href="{$head_link_stylesheet}" />
    <link rel="alternate"  type="application/rss+xml" title="{$blogTitle} RSS feed" href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/index.rss2" />
    <link rel="alternate"  type="application/x.atom+xml"  title="{$blogTitle} Atom feed"  href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/atom.xml" />
{if $entry_id}
    <link rel="pingback" href="{$serendipityBaseURL}comment.php?type=pingback&amp;entry_id={$entry_id}" />
{/if}

{serendipity_hookPlugin hook="frontend_header"}
</head>

<body>
{else}
{serendipity_hookPlugin hook="frontend_header"}
{/if}
<div id="page">
<div id="header" onclick="location.href='{$serendipityBaseURL}';" style="cursor: pointer;">
    <div id="headerimg">
        <h1>{$head_title|@default:$blogTitle}</h1>
        <div class="description">{$head_subtitle|@default:$blogDescription}</div>
    </div>
</div>
<hr />

<div id="content" class="narrowcolumn">
    {$CONTENT}
</div>

{if $rightSidebarElements > 0}
    <div id="sidebar">
    {serendipity_printSidebar side="right"}
    {serendipity_printSidebar side="left"}
    </div>
{/if}

<hr />
<div id="footer">
    <p>
    {$CONST.PROUDLY_POWERED_BY} <a href="http://www.s9y.org">Serendipity {$serendipityVersion}</a>.<br />
    Design is <a href="http://binarybonsai.com/kubrick/">Kubrick</a>, by Michael Heilemann, ported by <a href="http://blog.dreamcoder.dk">Tom Sommer</a>.
    </p>
</div>

</div>
{serendipity_hookPlugin hook="frontend_footer"}
{if $is_embedded != true}
</body>
</html>
{/if}
