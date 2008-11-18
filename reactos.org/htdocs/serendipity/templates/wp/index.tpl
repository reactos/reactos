{if $is_embedded != true}
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">

<head>
	<title>{$head_title|@default:$blogTitle} {if $head_subtitle} - {$head_subtitle}{/if}</title>
	
	<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1" />
	<meta name="generator" content="Serendipity v.{$head_version}" />

	<style type="text/css" media="screen">
		@import url( {serendipity_getFile file="wp-layout.css"} );
	</style>
	{serendipity_hookPlugin hook="frontend_header"}
</head>

<body>
{/if}

{if $is_raw_mode != true}
<div id="rap">
<h1 id="header"><a href="{$serendipityBaseURL}" title="{$head_title|@default:$blogTitle}: {$head_subtitle|@default:$blogDescription}">{$head_title|@default:$blogTitle}</a></h1>

<div id="content">

{$CONTENT}

</div>
{/if}

{$raw_data}

{if $leftSidebarElements or $rightSidebarElements}
  <div id="menu">
    <ul>
      {if $leftSidebarElements}{serendipity_printSidebar side="left"}{/if}
      {if $rightSidebarElements}{serendipity_printSidebar side="right"}{/if}
    </ul>
  </div>
{/if}

</div>

<p class="credit"><cite>Powered by <a href="http://www.s9y.org/" title="Powered by Serendipity PHP Weblog"><strong>Serendipity</strong></a></cite></p>
{serendipity_hookPlugin hook="frontend_footer"}
{if $is_embedded != true}
</body>
</html>
{/if}
