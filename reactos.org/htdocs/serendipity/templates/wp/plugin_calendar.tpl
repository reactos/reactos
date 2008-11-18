<table id="wp-calendar">
  <caption>{$plugin_calendar_head.month_date|formatTime:"%B '%y":false}</caption>
  <thead>
    <tr>
      <th abbr="Sunday" scope="col" title="Sunday">S</th>
      <th abbr="Monday" scope="col" title="Monday">M</th>
      <th abbr="Tuesday" scope="col" title="Tuesday">T</th>
      <th abbr="Wednesday" scope="col" title="Wednesday">W</th>
      <th abbr="Thursday" scope="col" title="Thursday">T</th>
      <th abbr="Friday" scope="col" title="Friday">F</th>
      <th abbr="Saturday" scope="col" title="Saturday">S</th>
    </tr>
  </thead>

  <tfoot>
    <tr>
      <td colspan="3" id="prev">{if $plugin_calendar_head.minScroll le $plugin_calendar_head.month_date}<a href="{$plugin_calendar_head.uri_previous}" title="View posts for previous month">&laquo;</a>{/if}</td>
      <td class="pad">&nbsp;</td>
      <td colspan="3" id="next">{if $plugin_calendar_head.maxScroll ge $plugin_calendar_head.month_date}<a href="{$plugin_calendar_head.uri_next}" title="View posts for previous month">&raquo;</a>{/if}</td>
    </tr>
  </tfoot>

  <tbody>
    {foreach from=$plugin_calendar_weeks item="week"}
      <tr>
        {foreach from=$week.days item="day"}
            <td class="serendipity_calendarDay {$day.classes}"{if isset($day.properties.Title)} title="{$day.properties.Title}"{/if}>{if isset($day.properties.Active) and $day.properties.Active}<a href="{$day.properties.Link}">{/if}{$day.name|@default:"&#160;"}{if isset($day.properties.Active) and $day.properties.Active}</a>{/if}</td>
        {/foreach}
      </tr>
    {/foreach}
  </tbody>
</table>
