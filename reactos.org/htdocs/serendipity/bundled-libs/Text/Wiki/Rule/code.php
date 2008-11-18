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
// $Id: code.php,v 1.4 2004/12/02 10:54:32 nohn Exp $


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

class Text_Wiki_Rule_code extends Text_Wiki_Rule {
    
    
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
    
    var $regex = '/^(\<code( .+)?\>)\n(.+)\n(\<\/code\>)(\s|$)/Umsi';
    
    
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
        // are there additional arguments?
        $args = trim($matches[2]);
        
        if ($args == '') {
            $options = array(
                'text' => $matches[3],
                'args' => array('type' => '')
            );
        } else {
            $options = array(
                'text' => $matches[3],
                'args' => $this->getMacroArgs($args)
            );
        }
        
        return $this->addToken($options) . $matches[5];
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
        // trim opening and closing whitespace
        $text = trim($options['text']);
        $args = $options['args'];
        
        if (strtolower($args['type']) == 'php') {
        	
        	// PHP code example
        	
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
            $text = "<pre>$text</pre>";
        
        } elseif (strtolower($args['type']) == 'html') {
        
            // HTML code example:
            // add <html> opening and closing tags,
            // convert tabs to four spaces,
            // convert entities.
            $text = str_replace("\t", "    ", $text);
            $text = "<html>\n$text\n</html>";
            $text = htmlentities($text);
            $text = "<pre><code>$text</code></pre>";
            
        } else {
            // generic code example:
            // convert tabs to four spaces,
            // convert entities.
            $text = str_replace("\t", "    ", $text);
            $text = htmlentities($text);
            $text = "<pre><code>$text</code></pre>";
        }
        
        return "\n$text\n";
    }
}
?>
