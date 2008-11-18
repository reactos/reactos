<p>
  <p><a href='{$serendipityBaseURL}/rss.php?version=2.0&amp;type=comments&amp;cid={$commentform_id}'><abbr title="Really Simple Syndication">RSS</abbr> feed for comments on this post.</a></p>
  <h2 id="postcomment">Leave a comment</h2>

  <form action="{$commentform_action}#feedback" method="post" id="commentform">
    <p>
      {serendipity_hookPlugin hook="frontend_comment" data=$commentform_entry}
    </p>
    <p>
      <input id="name" type="text" name="serendipity[name]" value="{$commentform_name}" size="28" tabindex="1" />
      <label for="author">{$CONST.NAME}</label>
    </p>

    <p>
      <input type="text" id="email" name="serendipity[email]" value="{$commentform_email}" size="28" tabindex="2" />
      <label for="email">{$CONST.EMAIL}</label>
    </p>

    <p>
      <input id="url" type="text" name="serendipity[url]" value="{$commentform_url}" size="28" tabindex="3" />
      <label for="url">{$CONST.HOMEPAGE}</label>
    </p>

    <p>
      <label for="comment">{$CONST.COMMENT}</label><br />
      <textarea id="comment" rows="4" cols="40" name="serendipity[comment]">{$commentform_data}</textarea>
    </p>

    {if $is_commentform_showToolbar}
      <p>
        <input id="checkbox_remember" type="checkbox" name="serendipity[remember]" {$commentform_remember} />
        <label for="checkbox_remember">{$CONST.REMEMBER_INFO}</label>
      </p>
      {if $is_allowSubscriptions}
        <p>
          <input id="checkbox_subscribe" type="checkbox" name="serendipity[subscribe]" {$commentform_subscribe} />
          <label for="checkbox_subscribe">{$CONST.SUBSCRIBE_TO_THIS_ENTRY}</label>
        </p>
      {/if}
    {/if}

    {if $is_moderate_comments}
      <p>{$CONST.COMMENTS_WILL_BE_MODERATED}</p>
    {/if}

    <p>
      <input type="hidden" name="serendipity[entry_id]" value="{$commentform_id}" />
      <input type="submit" name="serendipity[submit]" value="{$CONST.SUBMIT_COMMENT}" />
      <input type="submit" name="serendipity[preview]" value="{$CONST.PREVIEW}" />
    </p>
  </form>
</p>
