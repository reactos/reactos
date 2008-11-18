<ul>
{foreach from=$plugindata item=item}
        <li>
            {if $item.title != ""}<h2>{$item.title}</h2>{/if}
            {$item.content}
    </li>
{/foreach}
</ul>
