<!-- ENTRIES START -->
    {serendipity_hookPlugin hook="entries_header" addData="$entry_id"}

    {foreach from=$entries item="dategroup"}
        {foreach from=$dategroup.entries item="entry"}
        <div class="post">
            <h2 id="post-{$entry.id}"><a href="{$entry.link}">{$entry.title}</a></h2>
            {if !$is_single_entry}<small>{$entry.timestamp|@formatTime:DATE_FORMAT_ENTRY}</small>{/if}

            {if $entry.categories}
            <span class="categoryIcon">
            {foreach from=$entry.categories item="category"}
                {if $category.category_icon}
                    <a href="{$category.category_link}"><img class="categoryIcon" title="{$category.category_name|@escape}{$category.category_description|@emptyPrefix}" alt="{$category.category_name|@escape}" src="{$category.category_icon}" /></a>
                {/if}
            {/foreach}
            </span>
            {/if}

            <div class="entrytext">
                <p>
                {$entry.body}
                {if $is_single_entry}
                <a id="extended"></a>{$entry.extended}
                {/if}
                </p>

                {if $entry.has_extended and not $is_single_entry and not $entry.is_extended}
                <br /><a href="{$entry.link}#extended">{$CONST.VIEW_EXTENDED_ENTRY|@sprintf:$entry.title}</a><br /><br />
                {/if}

            <p class="postmetadata{if $is_single_entry} graybox{/if}">
                <small>
                {if $is_single_entry}

				{$CONST.ENTRY_POSTED_BY} <a href="{$entry.link_author}">{$entry.author}</a> {$CONST.ON} {$entry.timestamp|@formatTime:DATE_FORMAT_ENTRY} {$CONST.AT} {$entry.timestamp|@formatTime:"%H:%M"}.
                {if $entry.categories}
                   {$CONST.FILED_UNDER} {foreach from=$entry.categories item="category" name="categories"}<a href="{$category.category_link}">{$category.category_name|@escape}</a>{if not $smarty.foreach.categories.last}, {/if}{/foreach}.
                {/if}
                {if $entry.allow_comments}
                    {$CONST.LEAVE_RESPONSE|sprintf:'<a href="#serendipity_CommentForm">':'</a>':"<a href=\"$entry[link]\">":'</a>'}
                    {if $entry.moderate_comments}
                        {$CONST.SUBJECT_TO_MODERATION}
                    {/if}
                {else}
                    {$CONST.COMMENTS_CLOSED}
                {/if}

                {else}
                {$CONST.POSTED_BY} <a href="{$entry.link_author}">{$entry.author}</a>
                {if $entry.categories}
                   {$CONST.IN} {foreach from=$entry.categories item="category" name="categories"}<a href="{$category.category_link}">{$category.category_name|@escape}</a>{if not $smarty.foreach.categories.last}, {/if}{/foreach}
                {/if}

                {if $dategroup.is_sticky}
                    {$CONST.ON}
                {else}
                    {$CONST.AT}
                {/if} <a href="{$entry.link}">{if $dategroup.is_sticky}{$entry.timestamp|@formatTime:DATE_FORMAT_ENTRY} {/if}{$entry.timestamp|@formatTime:'%H:%M'}</a>

                {if $entry.has_comments}
                    {if $use_popups}
                        | <a href="{$entry.link_popup_comments}" onclick="window.open(this.href, 'comments', 'width=480,height=480,scrollbars=yes'); return false;">{$entry.label_comments} ({$entry.comments})</a>
                    {else}
                        | <a href="{$entry.link}#comments">{$entry.label_comments} ({$entry.comments})</a>
                    {/if}
                {/if}

                {if $entry.has_trackbacks}
                    {if $use_popups}
                        | <a href="{$entry.link_popup_trackbacks}" onclick="window.open(this.href, 'comments', 'width=480,height=480,scrollbars=yes'); return false;">{$entry.label_trackbacks} ({$entry.trackbacks})</a>
                    {else}
                        | <a href="{$entry.link}#trackbacks">{$entry.label_trackbacks} ({$entry.trackbacks})</a>
                    {/if}
                {/if}

                {if $entry.is_entry_owner and not $is_preview}
                        | <a href="{$entry.link_edit}">{$CONST.EDIT_ENTRY}</a>
                {/if}
                {/if}
                {$entry.add_footer}
                </small>
            </p>
            </div>
        </div>
        <!--
        <rdf:RDF xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
                 xmlns:trackback="http://madskills.com/public/xml/rss/module/trackback/"
                 xmlns:dc="http://purl.org/dc/elements/1.1/">
        <rdf:Description
                 rdf:about="{$entry.link_rdf}"
                 trackback:ping="{$entry.link_trackback}"
                 dc:title="{$entry.title}"
                 dc:identifier="{$entry.rdf_ident}" />
        </rdf:RDF>
        -->
        {$entry.plugin_display_dat}

        {if $is_single_entry and not $use_popups and not $is_preview}
            {if $CONST.DATA_UNSUBSCRIBED}
                <br /><div class="serendipity_center serendipity_msg_notice">{$CONST.DATA_UNSUBSCRIBED|@sprintf:$CONST.UNSUBSCRIBE_OK}</div><br />
            {/if}

            {if $CONST.DATA_TRACKBACK_DELETED}
                <br /><div class="serendipity_center serendipity_msg_notice">{$CONST.DATA_TRACKBACK_DELETED|@sprintf:$CONST.TRACKBACK_DELETED}</div><br />
            {/if}

            {if $CONST.DATA_TRACKBACK_APPROVED}
                <br /><div class="serendipity_center serendipity_msg_notice">{$CONST.DATA_TRACKBACK_APPROVED|@sprintf:$CONST.TRACKBACK_APPROVED}</div><br />
            {/if}

            {if $CONST.DATA_COMMENT_DELETED}
                <br /><div class="serendipity_center serendipity_msg_notice">{$CONST.DATA_COMMENT_DELETED|@sprintf:$CONST.COMMENT_DELETED}</div><br />
            {/if}

            {if $CONST.DATA_COMMENT_APPROVED}
                <br /><div class="serendipity_center serendipity_msg_notice">{$CONST.DATA_COMMENT_APPROVED|@sprintf:$CONST.COMMENT_APPROVED}</div><br />
            {/if}
                <a id="trackbacks"></a><h3>{$CONST.TRACKBACKS}</h3>
                    <div class="serendipity_center">
                        <a style="font-weight: normal" href="{$entry.link_trackback}" onclick="alert('{$CONST.TRACKBACK_SPECIFIC_ON_CLICK|@escape:htmlall}'); return false;" title="{$CONST.TRACKBACK_SPECIFIC_ON_CLICK|@escape}">{$CONST.TRACKBACK_SPECIFIC}</a>
                    </div>
                    <ol class="commentlist">
                        {serendipity_printTrackbacks entry=$entry.id}
                    </ol>
        {/if}

        {if $is_single_entry and not $is_preview}
                <a id="comments"></a><h3>{$CONST.COMMENTS}</h3>
                <div class="serendipity_center">{$CONST.DISPLAY_COMMENTS_AS}
                {if $entry.viewmode eq $CONST.VIEWMODE_LINEAR}
                    ({$CONST.COMMENTS_VIEWMODE_LINEAR} | <a href="{$entry.link_viewmode_threaded}#comments">{$CONST.COMMENTS_VIEWMODE_THREADED}</a>)
                {else}
                    (<a href="{$entry.link_viewmode_linear}#comments">{$CONST.COMMENTS_VIEWMODE_LINEAR}</a> | {$CONST.COMMENTS_VIEWMODE_THREADED})
                {/if}
                </div>
                <br />
                <ol class="commentlist">
                    {serendipity_printComments entry=$entry.id mode=$entry.viewmode}
                </ol>

                {if $entry.is_entry_owner}
                    {if $entry.allow_comments}
                    <div class="serendipity_center">(<a href="{$entry.link_deny_comments}">{$CONST.COMMENTS_DISABLE}</a>)</div>
                    {else}
                    <div class="serendipity_center">(<a href="{$entry.link_allow_comments}">{$CONST.COMMENTS_ENABLE}</a>)</div>
                    {/if}
                {/if}
                <a id="feedback"></a>

                {foreach from=$comments_messagestack item="message"}
                <div class="serendipity_center serendipity_msg_important">{$message}</div>
                {/foreach}

                {if $is_comment_added}

                <br />
                <div class="serendipity_center serendipity_msg_notice">{$CONST.COMMENT_ADDED}</div>

                {elseif $is_comment_moderate}

                <br />
                <div class="serendipity_center serendipity_msg_notice">{$CONST.COMMENT_ADDED}<br />{$CONST.THIS_COMMENT_NEEDS_REVIEW}</div>

                {else}

                <br />
                <h3>{$CONST.ADD_COMMENT}</h3>
                {$COMMENTFORM}

                {/if}
        {/if}

        {$entry.backend_preview}
        {/foreach}
    {foreachelse}
    {if not $plugin_clean_page}
        {$CONST.NO_ENTRIES_TO_PRINT}
    {/if}
    {/foreach}

    <div class="navigation" style="text-align: center">
    {if $footer_prev_page}
        <a href="{$footer_prev_page}">&laquo; {$CONST.PREVIOUS_PAGE}</a>&#160;&#160;
    {/if}

    {if $footer_info}
        ({$footer_info})
    {/if}

    {if $footer_next_page}
        <a href="{$footer_next_page}">&raquo; {$CONST.NEXT_PAGE}</a>
    {/if}

    {serendipity_hookPlugin hook="entries_footer"}
    </div>
<!-- ENTRIES END -->
