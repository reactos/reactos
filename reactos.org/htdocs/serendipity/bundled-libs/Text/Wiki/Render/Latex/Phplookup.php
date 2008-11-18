<?php

class Text_Wiki_Render_Latex_Phplookup extends Text_Wiki_Render {
    
    var $conf = array('target' => '_blank');
    
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
        return 'Phplookup: NI';
        
        $text = trim($options['text']);
        
        $target = $this->getConf('target', '');
        if ($target) {
            $target = " target=\"$target\"";
        }
        
        return "<a$target href=\"http://php.net/$text\">$text</a>";
    }
}
?>