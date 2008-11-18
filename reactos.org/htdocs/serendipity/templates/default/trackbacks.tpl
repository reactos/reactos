{foreach from=$trackbacks item=trackback}
    <div class="serendipity_comment">
        <div class="serendipity_commentBody">
            <a href="{$trackback.url|@strip_tags}" {'blank'|@xhtml_target}>{$trackback.title}</a><br />
            {$trackback.body|@strip_tags|@escape:all}
        </div>
        <div class="serendipity_comment_source">
            <b>Weblog:</b> {$trackback.author|@default:$CONST.ANONYMOUS}<br />
            <b>{$CONST.TRACKED}:</b> {$trackback.timestamp|@formatTime:'%b %d, %H:%M'}
        {if $entry.is_entry_owner}
            (<a href="{$serendipityBaseURL}comment.php?serendipity[delete]={$trackback.id}&amp;serendipity[entry]={$trackback.entry_id}&amp;serendipity[type]=trackbacks">{$CONST.DELETE}</a>)
        {/if}
        </div>
    </div>
{foreachelse}
    <div class="serendipity_center">{$CONST.NO_TRACKBACKS}</div>
{/foreach}
