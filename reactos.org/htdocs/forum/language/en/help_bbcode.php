<?php
/**
*
* help_bbcode [English]
*
* @package language
* @version $Id: help_bbcode.php 8479 2008-03-29 00:22:48Z naderman $
* @copyright (c) 2005 phpBB Group
* @license http://opensource.org/licenses/gpl-license.php GNU Public License
*
*/

/**
*/
if (!defined('IN_PHPBB'))
{
	exit;
}

// DEVELOPERS PLEASE NOTE
//
// All language files should use UTF-8 as their encoding and the files must not contain a BOM.
//
// Placeholders can now contain order information, e.g. instead of
// 'Page %s of %s' you can (and should) write 'Page %1$s of %2$s', this allows
// translators to re-order the output of data while ensuring it remains correct
//
// You do not need this where single placeholders are used, e.g. 'Message %d' is fine
// equally where a string contains only two placeholders which are used to wrap text
// in a url you again do not need to specify an order e.g., 'Click %sHERE%s' is fine

$help = array(
	array(
		0 => '--',
		1 => 'Introduction'
	),
	array(
		0 => 'What is BBCode?',
		1 => 'BBCode is a special implementation of HTML. Whether you can actually use BBCode in your posts on the forum is determined by the administrator. In addition you can disable BBCode on a per post basis via the posting form. BBCode itself is similar in style to HTML, tags are enclosed in square brackets [ and ] rather than &lt; and &gt; and it offers greater control over what and how something is displayed. Depending on the template you are using you may find adding BBCode to your posts is made much easier through a clickable interface above the message area on the posting form. Even with this you may find the following guide useful.'
	),
	array(
		0 => '--',
		1 => 'Text Formatting'
	),
	array(
		0 => 'How to create bold, italic and underlined text',
		1 => 'BBCode includes tags to allow you to quickly change the basic style of your text. This is achieved in the following ways: <ul><li>To make a piece of text bold enclose it in <strong>[b][/b]</strong>, e.g. <br /><br /><strong>[b]</strong>Hello<strong>[/b]</strong><br /><br />will become <strong>Hello</strong></li><li>For underlining use <strong>[u][/u]</strong>, for example:<br /><br /><strong>[u]</strong>Good Morning<strong>[/u]</strong><br /><br />becomes <span style="text-decoration: underline">Good Morning</span></li><li>To italicise text use <strong>[i][/i]</strong>, e.g.<br /><br />This is <strong>[i]</strong>Great!<strong>[/i]</strong><br /><br />would give This is <i>Great!</i></li></ul>'
	),
	array(
		0 => 'How to change the text colour or size',
		1 => 'To alter the colour or size of your text the following tags can be used. Keep in mind that how the output appears will depend on the viewers browser and system: <ul><li>Changing the colour of text is achieved by wrapping it in <strong>[color=][/color]</strong>. You can specify either a recognised colour name (eg. red, blue, yellow, etc.) or the hexadecimal triplet alternative, e.g. #FFFFFF, #000000. For example, to create red text you could use:<br /><br /><strong>[color=red]</strong>Hello!<strong>[/color]</strong><br /><br />or<br /><br /><strong>[color=#FF0000]</strong>Hello!<strong>[/color]</strong><br /><br />Both will output <span style="color:red">Hello!</span></li><li>Changing the text size is achieved in a similar way using <strong>[size=][/size]</strong>. This tag is dependent on the template the user has selected but the recommended format is a numerical value representing the text size in percent, starting at 20 (very small) through to 200 (very large) by default. For example:<br /><br /><strong>[size=30]</strong>SMALL<strong>[/size]</strong><br /><br />will generally be <span style="font-size:30%;">SMALL</span><br /><br />whereas:<br /><br /><strong>[size=200]</strong>HUGE!<strong>[/size]</strong><br /><br />will be <span style="font-size:200%;">HUGE!</span></li></ul>'
	),
	array(
		0 => 'Can I combine formatting tags?',
		1 => 'Yes, of course you can, for example to get someones attention you may write:<br /><br /><strong>[size=200][color=red][b]</strong>LOOK AT ME!<strong>[/b][/color][/size]</strong><br /><br />this would output <span style="color:red;font-size:200%;"><strong>LOOK AT ME!</strong></span><br /><br />We don’t recommend you output lots of text that looks like this though! Remember it is up to you, the poster, to ensure tags are closed correctly. For example the following is incorrect:<br /><br /><strong>[b][u]</strong>This is wrong<strong>[/b][/u]</strong>'
	),
	array(
		0 => '--',
		1 => 'Quoting and outputting fixed-width text'
	),
	array(
		0 => 'Quoting text in replies',
		1 => 'There are two ways you can quote text, with a reference or without.<ul><li>When you utilise the Quote function to reply to a post on the board you should notice that the post text is added to the message window enclosed in a <strong>[quote=&quot;&quot;][/quote]</strong> block. This method allows you to quote with a reference to a person or whatever else you choose to put! For example to quote a piece of text Mr. Blobby wrote you would enter:<br /><br /><strong>[quote=&quot;Mr. Blobby&quot;]</strong>The text Mr. Blobby wrote would go here<strong>[/quote]</strong><br /><br />The resulting output will automatically add &quot;Mr. Blobby wrote:&quot; before the actual text. Remember you <strong>must</strong> include the quotation marks &quot;&quot; around the name you are quoting, they are not optional.</li><li>The second method allows you to blindly quote something. To utilise this enclose the text in <strong>[quote][/quote]</strong> tags. When you view the message it will simply show the text within a quotation block.</li></ul>'
	),
	array(
		0 => 'Outputting code or fixed width data',
		1 => 'If you want to output a piece of code or in fact anything that requires a fixed width, e.g. Courier type font you should enclose the text in <strong>[code][/code]</strong> tags, e.g.<br /><br /><strong>[code]</strong>echo &quot;This is some code&quot;;<strong>[/code]</strong><br /><br />All formatting used within <strong>[code][/code]</strong> tags is retained when you later view it. PHP syntax highlighting can be enabled using <strong>[code=php][/code]</strong> and is recommended when posting PHP code samples as it improves readability.'
	),
	array(
		0 => '--',
		1 => 'Generating lists'
	),
	array(
		0 => 'Creating an Unordered list',
		1 => 'BBCode supports two types of lists, unordered and ordered. They are essentially the same as their HTML equivalents. An unordered list outputs each item in your list sequentially one after the other indenting each with a bullet character. To create an unordered list you use <strong>[list][/list]</strong> and define each item within the list using <strong>[*]</strong>. For example to list your favourite colours you could use:<br /><br /><strong>[list]</strong><br /><strong>[*]</strong>Red<br /><strong>[*]</strong>Blue<br /><strong>[*]</strong>Yellow<br /><strong>[/list]</strong><br /><br />This would generate the following list:<ul><li>Red</li><li>Blue</li><li>Yellow</li></ul>'
	),
	array(
		0 => 'Creating an Ordered list',
		1 => 'The second type of list, an ordered list, gives you control over what is output before each item. To create an ordered list you use <strong>[list=1][/list]</strong> to create a numbered list or alternatively <strong>[list=a][/list]</strong> for an alphabetical list. As with the unordered list, items are specified using <strong>[*]</strong>. For example:<br /><br /><strong>[list=1]</strong><br /><strong>[*]</strong>Go to the shops<br /><strong>[*]</strong>Buy a new computer<br /><strong>[*]</strong>Swear at computer when it crashes<br /><strong>[/list]</strong><br /><br />will generate the following:<ol style="list-style-type: arabic-numbers"><li>Go to the shops</li><li>Buy a new computer</li><li>Swear at computer when it crashes</li></ol>Whereas for an alphabetical list you would use:<br /><br /><strong>[list=a]</strong><br /><strong>[*]</strong>The first possible answer<br /><strong>[*]</strong>The second possible answer<br /><strong>[*]</strong>The third possible answer<br /><strong>[/list]</strong><br /><br />giving<ol style="list-style-type: lower-alpha"><li>The first possible answer</li><li>The second possible answer</li><li>The third possible answer</li></ol>'
	),
	array(
		0 => '--',
		1 => 'Creating Links'
	),
	array(
		0 => 'Linking to another site',
		1 => 'phpBB BBCode supports a number of ways of creating URIs (Uniform Resource Indicators) better known as URLs.<ul><li>The first of these uses the <strong>[url=][/url]</strong> tag, whatever you type after the = sign will cause the contents of that tag to act as a URL. For example to link to phpBB.com you could use:<br /><br /><strong>[url=http://www.phpbb.com/]</strong>Visit phpBB!<strong>[/url]</strong><br /><br />This would generate the following link, <a href="http://www.phpbb.com/">Visit phpBB!</a> Please notice that the link opens in the same window or a new window depending on the users browser preferences.</li><li>If you want the URL itself displayed as the link you can do this by simply using:<br /><br /><strong>[url]</strong>http://www.phpbb.com/<strong>[/url]</strong><br /><br />This would generate the following link, <a href="http://www.phpbb.com/">http://www.phpbb.com/</a></li><li>Additionally, phpBB features something called <i>Magic Links</i>, this will turn any syntactically correct URL into a link without you needing to specify any tags or even the leading http://. For example typing www.phpbb.com into your message will automatically lead to <a href="http://www.phpbb.com/">www.phpbb.com</a> being output when you view the message.</li><li>The same thing applies equally to e-mail addresses, you can either specify an address explicitly for example:<br /><br /><strong>[email]</strong>no.one@domain.adr<strong>[/email]</strong><br /><br />which will output <a href="mailto:no.one@domain.adr">no.one@domain.adr</a> or you can just type no.one@domain.adr into your message and it will be automatically converted when you view.</li></ul>As with all the BBCode tags you can wrap URLs around any of the other tags such as <strong>[img][/img]</strong> (see next entry), <strong>[b][/b]</strong>, etc. As with the formatting tags it is up to you to ensure the correct open and close order is following, for example:<br /><br /><strong>[url=http://www.google.com/][img]</strong>http://www.google.com/intl/en_ALL/images/logo.gif<strong>[/url][/img]</strong><br /><br />is <span style="text-decoration: underline">not</span> correct which may lead to your post being deleted so take care.'
	),
	array(
		0 => '--',
		1 => 'Showing images in posts'
	),
	array(
		0 => 'Adding an image to a post',
		1 => 'phpBB BBCode incorporates a tag for including images in your posts. Two very important things to remember when using this tag are: many users do not appreciate lots of images being shown in posts and secondly the image you display must already be available on the internet (it cannot exist only on your computer for example, unless you run a webserver!). To display an image you must surround the URL pointing to the image with <strong>[img][/img]</strong> tags. For example:<br /><br /><strong>[img]</strong>http://www.google.com/intl/en_ALL/images/logo.gif<strong>[/img]</strong><br /><br />As noted in the URL section above you can wrap an image in a <strong>[url][/url]</strong> tag if you wish, e.g.<br /><br /><strong>[url=http://www.google.com/][img]</strong>http://www.google.com/intl/en_ALL/images/logo.gif<strong>[/img][/url]</strong><br /><br />would generate:<br /><br /><a href="http://www.google.com/"><img src="http://www.google.com/intl/en_ALL/images/logo.gif" alt="" /></a>'
	),
	array(
		0 => 'Adding attachments into a post',
		1 => 'Attachments can now be placed in any part of a post by using the new <strong>[attachment=][/attachment]</strong> BBCode, if the attachments functionality has been enabled by a board administrator and if you are given the appropriate permissions to create attachments. Within the posting screen is a drop-down box (respectively a button) for placing attachments inline.'
	),
	array(
		0 => '--',
		1 => 'Other matters'
	),
	array(
		0 => 'Can I add my own tags?',
		1 => 'If you are an administrator on this board and have the proper permissions, you can add further BBCodes through the Custom BBCodes section.'
	)
);

?>