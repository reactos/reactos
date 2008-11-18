{serendipity_hookPlugin hook="entries_header"}
<div class='serendipity_date'>{$CONST.TOPICS_OF} {$dateRange.0|@formatTime:"%B, %Y"}</div>

<div class="serendipity_entry">
    {foreach from=$entries item="entries"}
        <ul>
        {foreach from=$entries.entries item="entry"}
            <li><a href="{$entry.link}">{$entry.title}</a>
                <br />{$CONST.POSTED_BY} {$entry.username} {$CONST.ON} {$entry.timestamp|@formatTime:DATE_FORMAT_ENTRY}</li>
        {/foreach}
        </ul>
    {/foreach}
</div>
<div class='serendipity_entryFooter' style="text-align: center">
{serendipity_hookPlugin hook="entries_footer"}</div>
