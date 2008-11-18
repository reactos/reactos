{foreach from=$comments item=comment name="comments"}
    <a id="c{$comment.id}"></a>
    <li class="{if $smarty.foreach.comments.iteration is odd}graybox{/if}" style="margin-left: {$comment.depth*20}px">
        <cite>{if $comment.url}
                <a href="{$comment.url}" target="_blank">{$comment.author|@default:$CONST.ANONYMOUS}</a>
            {else}
                {$comment.author|@default:$CONST.ANONYMOUS}
            {/if}</cite> {$CONST.SAYS}:<br />
        <small class="commentmetadata">
            <a href="#c{$comment.id}" title="{$CONST.LINK_TO_COMMENT|sprintf:$comment.trace}">#{$comment.trace}</a>
            {$comment.timestamp|@formatTime:$CONST.DATE_FORMAT_SHORT}
            {if $entry.is_entry_owner}
                (<a href="{$comment.link_delete}" onclick="return confirm('{$CONST.COMMENT_DELETE_CONFIRM|@sprintf:$comment.id:$comment.author}');">{$CONST.DELETE}</a>)
            {/if}
            {roscms_can_add_comment}
            {if $entry.allow_comments && $can_add_comment eq 'true'}
                (<a href="#serendipity_CommentForm" onclick="document.getElementById('serendipity_replyTo').value='{$comment.id}';">{$CONST.REPLY}</a>)
            {/if}
        </small>
        <p>{$comment.body}</p>
    </li>
{foreachelse}
    <p class="nocomments">{$CONST.NO_COMMENTS}</p>
{/foreach}
