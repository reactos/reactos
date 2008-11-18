{foreach from=$trackbacks item=trackback}
  <li id="trackback-{$trackback.id}">
    <p><strong>{$trackback.title}</strong><br />{$trackback.body|@strip_tags|@escape:all}</p>
    <p><cite>Trackback by <a href='{$trackback.url|@strip_tags}' rel='external'>{$trackback.author|@default:$CONST.ANONYMOUS}</a> &#8212; {$trackback.timestamp|@formatTime:$CONST.DATE_FORMAT_SHORT} @ <a href="#trackback-{$trackback.id}">{$trackback.timestamp|@formatTime:'%g:%M %a'}</a></cite> </p>
  </li>
{/foreach}