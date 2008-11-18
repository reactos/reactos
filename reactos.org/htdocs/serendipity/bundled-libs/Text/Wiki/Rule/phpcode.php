<?php
/* vim: set expandtab tabstop=4 shiftwidth=4: */
// +----------------------------------------------------------------------+
// | PHP version 4                                                        |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2003 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.0 of the PHP license,       |
// | that is bundled with this package in the file LICENSE, and is        |
// | available through the world-wide-web at                              |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Paul M. Jones <pmjones@ciaweb.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: phpcode.php,v 1.3 2004/12/02 10:54:32 nohn Exp $


/**
* 
* This class implements a Text_Wiki_Rule to find sections marked as code
* examples.  Blocks are marked as the string <code> on a line by itself,
* followed by the inline code example, and terminated with the string
* </code> on a line by itself.  The code example is run through the
* native PHP highlight_string() function to colorize it, then surrounded
* with <pre>...</pre> tags when rendered as XHTML.
*
* @author Paul M. Jones <pmjones@ciaweb.net>
*
* @package Text_Wiki
*
*/

class Text_Wiki_Rule_phpcode extends Text_Wiki_Rule {
    
    
    /**
    * 
    * The regular expression used to find source text matching this
    * rule.
    * 
    * @access public
    * 
    * @var string
    * 
    */
    
    var $regex = '/^(\<php\>)\n(.+)\n(\<\/php\>)(\s|$)/Umsi';
    
    
    /**
    * 
    * Generates a token entry for the matched text.  Token options are:
    * 
    * 'text' => The full matched text, not including the <code></code> tags.
    * 
    * @access public
    *
    * @param array &$matches The array of matches from parse().
    *
    * @return A delimited token number to be used as a placeholder in
    * the source text.
    *
    */
    
    function process(&$matches)
    {    
        $options = array('text' => $matches[2]);
        return $this->addToken($options) . $matches[4];
    }
    
    
    /**
    * 
    * Renders a token into text matching the requested format.
    * 
    * @access public
    * 
    * @param array $options The "options" portion of the token (second
    * element).
    * 
    * @return string The text rendered from the token options.
    * 
    */
    
    function renderXhtml($options)
    {
		// add the PHP tags
		$text = "<?php\n" . $options['text'] . "\n?>"; // <?php
		
		// convert tabs to four spaces
		$text = str_replace("\t", "    ", $text);
		
		// colorize the code block (also converts HTML entities and adds
		// <code>...</code> tags)
		ob_start();
		highlight_string($text);
		$text = ob_get_contents();
		ob_end_clean();
		
		// replace <br /> tags with simple newlines
		//$text = str_replace("<br />", "\n", $text);
		
		// replace non-breaking space with simple spaces
		//$text = str_replace("&nbsp;", " ", $text);
		
		// replace <br /> tags with simple newlines
		// replace non-breaking space with simple spaces
		// translate old HTML to new XHTML
		// courtesy of research by A. Kalin :-)
		$map = array(
			'<br />'  => "\n",
			'&nbsp;'  => ' ',
			'<font'   => '<span',
			'</font>' => '</span>',
			'color="' => 'style="color:'
		);
		$text = strtr($text, $map);
	   
		// get rid of the last newline inside the code block
		// (becuase higlight_string puts one there)
		if (substr($text, -8) == "\n</code>") {
			$text = substr($text, 0, -8) . "</code>";
		}
		
	   // done
		return "\n<pre>$text</pre>\n";
    }
}
?>
