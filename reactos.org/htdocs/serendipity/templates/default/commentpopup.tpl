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
    <link rel="stylesheet" type="text/css" href="{$serendipityHTTPPath}serendipity.css.php" />
    <link rel="alternate"  type="application/rss+xml" title="{$blogTitle} RSS feed" href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/index.rss2" />
    <link rel="alternate"  type="application/x.atom+xml"  title="{$blogTitle} Atom feed"  href="{$serendipityBaseURL}{$serendipityRewritePrefix}feeds/atom.xml" />
</head>

<body class="s9y_wrap" id="serendipity_comment_page">

{if $is_comment_added}

    {$CONST.COMMENT_ADDED}{$comment_string.0}<a href="{$comment_url}">{$comment_string.1}</a>{$comment_string.2}<a href="#" onclick="self.close()">{$comment_string.3}</a>{$comment_string.4}

{elseif $is_comment_notadded}

    {$CONST.COMMENT_NOT_ADDED}{$comment_string.0}<a href="{$comment_url}">{$comment_string.1}</a>{$comment_string.2}<a href="#" onclick="self.close()">{$comment_string.3}</a>{$comment_string.4}

{elseif $is_comment_empty}

    {$comment_string.0}<a href="#" onclick="history.go(-1)">{$comment_string.1}</a>

{elseif $is_showtrackbacks}

    <div class="serendipity_commentsTitle">{$CONST.TRACKBACKS}</div><br />
    <dl>
        <dt><b>{$CONST.TRACKBACK_SPECIFIC}:</b><br /></dt>
        <dd><a rel="nofollow" href="{$comment_url}">{$comment_url}</a><br /></dd>

        <dt><b>{$CONST.DIRECT_LINK}:</b><br /></dt>
        <dd><a href="{$comment_entryurl}">{$comment_entryurl}</a></dd>
    </dl>

    {serendipity_printTrackbacks entry=$entry_id}

{elseif $is_showcomments}

    <div class="serendipity_commentsTitle">{$CONST.COMMENTS}</div>

    {serendipity_printComments entry=$entry_id}
    {if $is_comment_allowed}
        <div class="serendipity_commentsTitle">{$CONST.ADD_COMMENT}</div>
        {$COMMENTFORM}
    {else}
        <div class="serendipity_center serendipity_msg_important">{$CONST.COMMENTS_CLOSED}</div>
    {/if}

{/if}

</body>
</html>
