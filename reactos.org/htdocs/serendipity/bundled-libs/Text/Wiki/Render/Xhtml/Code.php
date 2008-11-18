<?php

class Text_Wiki_Render_Xhtml_Code extends Text_Wiki_Render {
    
    var $conf = array(
        'css'      => null, // class for <pre>
        'css_code' => null, // class for generic <code>
        'css_php'  => null, // class for PHP <code>
        'css_html' => null // class for HTML <code>
    );
    
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
    
    function token($options)
    {
        $text = $options['text'];
        $attr = $options['attr'];
        $type = strtolower($attr['type']);
        
        $css      = $this->formatConf(' class="%s"', 'css');
        $css_code = $this->formatConf(' class="%s"', 'css_code');
        $css_php  = $this->formatConf(' class="%s"', 'css_php');
        $css_html = $this->formatConf(' class="%s"', 'css_html');
        
        if ($type == 'php') {
            
            // PHP code example:
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
            
            // replace <br /> tags with simple newlines.
            // replace non-breaking space with simple spaces.
            // translate HTML <font> and color to XHTML <span> and style.
            // courtesy of research by A. Kalin :-).
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
            
            // replace all <code> tags with classed tags
            if ($css_php) {
                $text = str_replace('<code>', "<code$css_php>", $text);
            }
            
            // done
            $text = "<pre$css>$text</pre>";
        
        } elseif ($type == 'html' || $type == 'xhtml') {
        
            // HTML code example:
            // add <html> opening and closing tags,
            // convert tabs to four spaces,
            // convert entities.
            $text = str_replace("\t", "    ", $text);
            $text = "<html>\n$text\n</html>";
            $text = htmlentities($text);
            $text = "<pre$css><code$css_html>$text</code></pre>";
            
        } else {
            // generic code example:
            // convert tabs to four spaces,
            // convert entities.
            $text = str_replace("\t", "    ", $text);
            $text = htmlentities($text);
            $text = "<pre$css><code$css_code>$text</code></pre>";
        }
        
        return "\n$text\n\n";
    }
}
?>