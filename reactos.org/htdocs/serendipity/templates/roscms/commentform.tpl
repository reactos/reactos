<div class="serendipityCommentForm">
    <a id="serendipity_CommentForm"></a>
    <form id="serendipity_comment" action="{$commentform_action}#feedback" method="post">
    <div><input type="hidden" name="serendipity[entry_id]" value="{$commentform_id}" /></div>
    <table border="0" width="100%" cellpadding="3">
        <tr>
            <td class="serendipity_commentsLabel"><label for="serendipity_replyTo">{$CONST.IN_REPLY_TO}</label></td>
            <td class="serendipity_commentsValue">{$commentform_replyTo}</td>
        </tr>

        <tr>
            <td class="serendipity_commentsLabel"><label for="serendipity_commentform_comment">{$CONST.COMMENT}</label></td>
            <td class="serendipity_commentsValue">
                <textarea rows="10" cols="40" id="serendipity_commentform_comment" name="serendipity[comment]">{$commentform_data}</textarea><br />
                {serendipity_hookPlugin hook="frontend_comment" data=$commentform_entry}
            </td>
        </tr>

{if $is_commentform_showToolbar}
        <tr>
            <td>&#160;</td>
            <td class="serendipity_commentsLabel">
    {if $is_allowSubscriptions}
                <br />
                <input id="checkbox_subscribe" type="checkbox" name="serendipity[subscribe]" {$commentform_subscribe} /><label for="checkbox_subscribe">{$CONST.SUBSCRIBE_TO_THIS_ENTRY}</label>
    {/if}
            </td>
       </tr>
{/if}

{if $is_moderate_comments}
       <tr>
            <td class="serendipity_commentsValue serendipity_msg_important" colspan="2">{$CONST.COMMENTS_WILL_BE_MODERATED}</td>
       </tr>
{/if}

       <tr>
            <td>&#160;</td>
            <td><input type="submit" name="serendipity[submit]" value="{$CONST.SUBMIT_COMMENT}" /> <input type="submit" name="serendipity[preview]" value="{$CONST.PREVIEW}" /></td>
        </tr>
    </table>
    </form>
</div>
