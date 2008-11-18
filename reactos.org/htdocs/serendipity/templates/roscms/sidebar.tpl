{foreach from=$plugindata item=item}
  {if $item.title != ""}<div class="navTitle">{$item.title}</div>{/if}
    <ol>
      {roscms_sidebar_transform content=$item.content}
    </ol>
  <p></p>
{/foreach}
