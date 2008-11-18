<!-- CONTENT START -->

{if $searchresult_tooShort}
	<div class="serendipity_search serendipity_search_tooshort">{$content_message}</div>
{elseif $searchresult_error}
	<div class="serendipity_search serendipity_search_error">{$content_message}</div>
{elseif $searchresult_noEntries}
	<div class="serendipity_search serendipity_search_noentries">{$content_message}</div>
{elseif $searchresult_results}
	<div class="serendipity_search serendipity_search_results">{$content_message}</div>
{else}
	<div class="serendipity_content_message">{$content_message}</div>
{/if}

{$ENTRIES}
{$ARCHIVES}

<!-- CONTENT END -->
