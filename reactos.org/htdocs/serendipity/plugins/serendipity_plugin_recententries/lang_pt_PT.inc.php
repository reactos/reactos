<?php # $Id: lang_ja.inc.php,v 1.4 2005/05/17 11:37:42 garvinhicking Exp $

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P Matos <jmatos@math.ist.utl.pt>                                  #
#                                                                        #
##########################################################################

@define('PLUGIN_RECENTENTRIES_TITLE', 'Entradas recentes');
@define('PLUGIN_RECENTENTRIES_BLAHBLAH', 'Mostra os títulos e datas das entradas mais recentes');
@define('PLUGIN_RECENTENTRIES_NUMBER', 'Número de entradas');
@define('PLUGIN_RECENTENTRIES_NUMBER_BLAHBLAH', 'Quantas entradas devem ser mostradas? Por omissão: 10');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM', 'Ignorar as entradas na primeira página');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_DESC', 'Só entradas recentes que não estão na primeira página serão mostradas. (Por omissão, as ' . $serendipity['fetchLimit'] . ' últimas entradas serão ignoradas)');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_ALL', 'Não. mostrar todas as entradas');
@define('PLUGIN_RECENTENTRIES_NUMBER_FROM_RADIO_RECENT', 'Sim');

/* vim: set sts=4 ts=4 expandtab : */
?>