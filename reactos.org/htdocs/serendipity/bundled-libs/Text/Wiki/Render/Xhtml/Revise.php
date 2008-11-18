<?php

class Text_Wiki_Render_Xhtml_Revise extends Text_Wiki_Render {
    
    var $conf = array(
        'css_ins' => null,
        'css_del' => null
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
        if ($options['type'] == 'del_start') {
            $css = $this->formatConf(' class="%s"', 'css_del');
            return "<del$css>";
        }
        
        if ($options['type'] == 'del_end') {
            return "</del>";
        }
        
        if ($options['type'] == 'ins_start') {
            $css = $this->formatConf(' class="%s"', 'css_ins');
            return "<ins$css>";
        }
        
        if ($options['type'] == 'ins_end') {
            return "</ins>";
        }
    }
}
?>