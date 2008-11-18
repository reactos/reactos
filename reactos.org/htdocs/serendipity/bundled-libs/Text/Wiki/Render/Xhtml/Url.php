<?php


class Text_Wiki_Render_Xhtml_Url extends Text_Wiki_Render {
    
    
    var $conf = array(
        'target' => '_blank',
        'images' => true,
        'img_ext' => array('jpg', 'jpeg', 'gif', 'png'),
        'css_inline' => null,
        'css_footnote' => null,
        'css_descr' => null,
        'css_img' => null
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
        // create local variables from the options array (text,
        // href, type)
        extract($options);
        
        // find the rightmost dot and determine the filename
        // extension.
        $pos = strrpos($href, '.');
        $ext = strtolower(substr($href, $pos + 1));
        $href = htmlspecialchars($href);
        
        // does the filename extension indicate an image file?
        if ($this->getConf('images') &&
            in_array($ext, $this->getConf('img_ext', array()))) {
            
            // create alt text for the image
            if (! isset($text) || $text == '') {
                $text = basename($href);
                $text = htmlspecialchars($text);
            }
            
            // generate an image tag
            $css = $this->formatConf(' class="%s"', 'css_img');
            $output = "<img$css src=\"$href\" alt=\"$text\" />";
            
        } else {
            
            // allow for alternative targets on non-anchor HREFs
            if ($href{0} == '#') {
                $target = '';
            } else {
                $target = $this->getConf('target');
            }
            
            // generate a regular link (not an image)
            $text = htmlspecialchars($text);
            $css = $this->formatConf(' class="%s"', "css_$type");
            $output = "<a$css href=\"$href\"";
            
            if ($target) {
                // use a "popup" window.  this is XHTML compliant, suggested by
                // Aaron Kalin.  uses the $target as the new window name.
                $target = htmlspecialchars($target);
                $output .= " onclick=\"window.open(this.href, '$target');";
                $output .= " return false;\"";
            }
            
            // finish up output
            $output .= ">$text</a>";
            
            // make numbered references look like footnotes when no
            // CSS class specified, make them superscript by default
            if ($type == 'footnote' && ! $css) {
                $output = '<sup>' . $output . '</sup>';
            }
        }
        
        return $output;
    }
}
?>