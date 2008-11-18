{foreach from=$plugindata item=item}
  <li id="{$item.class}">{$item.title}
    {$item.content}
  </li>
{/foreach}
