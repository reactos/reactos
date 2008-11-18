<?php

class Text_Wiki_Render_Latex_Deflist extends Text_Wiki_Render {

    var $conf = array(
                      'css_dl' => null,
                      'css_dt' => null,
                      'css_dd' => null
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
        $type = $options['type'];
        switch ($type)
            {
            case 'list_start':
                return "\\begin{description}\n";

            case 'list_end':
                return "\\end{description}\n\n";

            case 'term_start':
                return '\item[';

            case 'term_end':
                return '] ';

            case 'narr_start':
                return '{';

            case 'narr_end':
                return "}\n";

            default:
                return '';

            }
    }
}
?>