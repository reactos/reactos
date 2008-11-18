{foreach from=$trackbacks item=trackback name="trackbacks"}
    <li class="{if $smarty.foreach.comments.iteration is odd}graybox{/if}">
        <cite><a href="{$trackback.url|@strip_tags}" {'blank'|@xhtml_target}>{$trackback.title}</a></cite>
        <p>{$trackback.body|@strip_tags|@escape:all}</p>
        <small class="commentmetadata">
            <b>Weblog:</b> {$trackback.author|@default:$CONST.ANONYMOUS}<br />
            <b>{$CONST.TRACKED}:</b> {$trackback.timestamp|@formatTime:'%b %d, %H:%M'}
        {if $entry.is_entry_owner}
            (<a href="{$serendipityBaseURL}comment.php?serendipity[delete]={$trackback.id}&amp;serendipity[entry]={$trackback.entry_id}&amp;serendipity[type]=trackbacks">{$CONST.DELETE}</a>)
        {/if}
        </small>
    </li>
{foreachelse}
    <p class="nocomments">{$CONST.NO_TRACKBACKS}</p>
{/foreach}
