<?php # $Id: serendipity_plugin_recententries.php 346 2005-08-01 17:35:25Z garvinhicking $

// Contributed by Christian Machmeier <cm@redsplash.de>

// Probe for a language include with constants. Still include defines later on, if some constants were missing
$probelang = dirname(__FILE__) . '/' . $serendipity['charset'] . 'lang_' . $serendipity['lang'] . '.inc.php';
if (file_exists($probelang)) {
    include $probelang;
}

@define('PLUGIN_RECENTENTRIES_TITLE', 'Recent Entries');
@define('PLUGIN_RECENTENTRIES_BLAHBLAH', 'Shows the titles and dates of the most recent entries');
@define('PLUGIN_RECENTENTRIES_NUMBER', 'Number of entries');
@define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', 'How many entries should be displayed? (Default: 10)');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM', 'Skip front page entries');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', 'Only recent entries that are not on the front page will be shown. (Default: latest ' . $serendipity['fetchLimit'] . ' will be skipped)');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', 'Show all');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', 'Skip front page items');

class serendipity_plugin_recententries extends serendipity_plugin {
    var $title = PLUGIN_RECENTENTRIES_TITLE;

    function introspect(&$propbag) {
        $this->title = $this->get_config('title', $this->title);

        $propbag->add('name',          PLUGIN_RECENTENTRIES_TITLE);
        $propbag->add('description',   PLUGIN_RECENTENTRIES_BLAHBLAH);
        $propbag->add('stackable',     true);
        $propbag->add('author',        'Christian Machmeier');
        $propbag->add('version',       '1.3');
        $propbag->add('requirements',  array(
            'serendipity' => '0.8',
            'smarty'      => '2.6.7',
            'php'         => '4.1.0'
        ));
        $propbag->add('configuration', array('title', 'number', 'number_from', 'dateformat', 'category'));
        $propbag->add('groups', array('FRONTEND_VIEWS'));
    }

    function introspect_config_item($name, &$propbag) {
        switch($name) {
            case 'title':
                $propbag->add('type',        'string');
                $propbag->add('name',        TITLE);
                $propbag->add('description', TITLE_FOR_NUGGET);
                $propbag->add('default',     PLUGIN_RECENTENTRIES_TITLE);
                break;

            case 'number':
                $propbag->add('type', 'string');
                $propbag->add('name', PLUGIN_RECENTENTRIES_NUMBER);
                $propbag->add('description', PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH);
                $propbag->add('default', 10);
                break;

                case 'number_from':
                    $propbag->add('type', 'radio');
                    $propbag->add('name', PLUGIN_RECENTENTRIES_NUMBER_FROM);
                    $propbag->add('description', PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC);
                    $propbag->add('radio',  array(
                        'value' => array('all', 'skip'),
                        'desc'  => array(PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL, PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT)
                        ));
                    $propbag->add('default', 'all');
                break;

            case 'dateformat':
                $propbag->add('type', 'string');
                $propbag->add('name', GENERAL_PLUGIN_DATEFORMAT);
                $propbag->add('description', sprintf(GENERAL_PLUGIN_DATEFORMAT_BLAHBLAH, '%A, %B %e %Y'));
                $propbag->add('default', '%A, %B %e %Y');
                break;

            case 'category':
                $cats    = serendipity_fetchCategories($serendipity['authorid']);
                if (!is_array($cats)) {
                    return false;
                }

                $catlist = serendipity_generateCategoryList($cats, array(0), 4);
                $tmp_select_cats = explode('@@@', $catlist);

                if (!is_array($tmp_select_cats)) {
                    return false;
                }

                $select_cats = array();
                $select_cats['none'] = ALL_CATEGORIES;
                foreach($tmp_select_cats as $cidx => $tmp_select_cat) {
                    $select_cat = explode('|||', $tmp_select_cat);
                    if (!empty($select_cat[0]) && !empty($select_cat[1])) {
                        $select_cats[$select_cat[0]] = $select_cat[1];
                    }
                }

                $propbag->add('type',          'select');
                $propbag->add('select_values', $select_cats);
                $propbag->add('name',          CATEGORY);
                $propbag->add('description',   CATEGORIES_TO_FETCH);
                $propbag->add('default',       'none');
                break;

            default:
                return false;
        }
        return true;
    }

    function generate_content(&$title) {
        global $serendipity;

        $number         = $this->get_config('number');
        $dateformat     = $this->get_config('dateformat');
        $category       = $this->get_config('category', 'none');
        $title          = $this->get_config('title', $this->title);
        $number_from_sw = $this->get_config('number_from');

        $sql_join   = '';
        $sql_where  = '';
        if ($category != 'none' && is_numeric($category)) {
            $sql_join = 'LEFT OUTER JOIN ' . $serendipity['dbPrefix'] . 'entrycat AS ec ON id = ec.entryid
                         LEFT OUTER JOIN ' . $serendipity['dbPrefix'] . 'category AS c  ON ec.categoryid = c.categoryid';
            $sql_where = ' AND (c.category_left BETWEEN ' . implode(' AND ', serendipity_fetchCategoryRange($category)) . ')';
        }

        if (!$number || !is_numeric($number) || $number < 1) {
            $number = 10;
        }

        $sql_number = serendipity_db_limit_sql($number);

        switch($number_from_sw) {
            case 'skip':
                $sql_number = serendipity_db_limit_sql(serendipity_db_limit($serendipity['fetchLimit'], $number));
                break;
        }

        if (!$dateformat || strlen($dateformat) < 1) {
            $dateformat = '%A, %B %e %Y';
        }

        $entries_query = "SELECT id,
                                                title,
                                                timestamp
                                           FROM {$serendipity['dbPrefix']}entries
                                                $sql_join
                                          WHERE isdraft = 'false' AND timestamp <= " . time() . "
                                                $sql_where
                                       ORDER BY timestamp DESC
                                                $sql_number";
        $entries = serendipity_db_query($entries_query);

        if (isset($entries) && is_array($entries)) {
            foreach ($entries as $k => $entry) {
                $entryLink = serendipity_archiveURL(
                               $entry['id'],
                               $entry['title'],
                               'serendipityHTTPPath',
                               true,
                               array('timestamp' => $entry['timestamp'])
                            );

                echo '<a href="' . $entryLink . '" title="' . htmlspecialchars($entry['title']) . '">' . $entry['title'] . '</a><br />'
                     . '<div class="serendipitySideBarDate">'
                     . htmlspecialchars(serendipity_strftime($dateformat, $entry['timestamp']))
                     . '</div><br />';
            }
        }
    }
}

/* vim: set sts=4 ts=4 expandtab : */
?>
