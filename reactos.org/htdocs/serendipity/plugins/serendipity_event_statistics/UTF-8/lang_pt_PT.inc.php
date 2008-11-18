<?php # $Id:$

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# João P. Matos <jmatos@math.ist.utl.pt>                                 #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_STATISTICS_NAME', 'Estatísticas');
@define('PLUGIN_EVENT_STATISTICS_DESC', 'Junta uma ligação a estatísticas interessantes no seu painel entradas, incluindo um contador de visitantes');
@define('PLUGIN_EVENT_STATISTICS_OUT_STATISTICS', 'Estatísticas');
@define('PLUGIN_EVENT_STATISTICS_OUT_FIRST_ENTRY', 'Primeira entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_LAST_ENTRY', 'Última entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_ENTRIES', 'Número total de entradas');
@define('PLUGIN_EVENT_STATISTICS_OUT_ENTRIES', 'entradas');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_PUBLIC', ' ... públicos');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOTAL_DRAFTS', ' ... rascunhos');
@define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES', 'Categorias');
@define('PLUGIN_EVENT_STATISTICS_OUT_CATEGORIES2', 'categorias');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES', 'Distribuição das entradas');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_CATEGORIES2', 'entradas');
@define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES', 'Imagens recebidas');
@define('PLUGIN_EVENT_STATISTICS_OUT_UPLOADED_IMAGES2', 'imagen(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES', 'Distribuição dos tipes de imagem');
@define('PLUGIN_EVENT_STATISTICS_OUT_DISTRIBUTION_IMAGES2', 'ficheiros');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS', 'Comentários recebidos');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS2', 'comentário(s)');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS3', 'Entradas mais comentadas');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPCOMMENTS', 'Comentadores mais frequentes');
@define('PLUGIN_EVENT_STATISTICS_OUT_LINK', 'ligação');
@define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS', 'Subscritores');
@define('PLUGIN_EVENT_STATISTICS_OUT_SUBSCRIBERS2', 'subscritor(es)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS', 'Entradas mais subscritas');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPSUBSCRIBERS2', 'subscritor(es)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS', 'Retroligações recebidas');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS2', 'retroligações');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK', 'Entradas com maior número de retroligações');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACK2', 'retroligações');
@define('PLUGIN_EVENT_STATISTICS_OUT_TOPTRACKBACKS3', 'Pessoas que fizeram mais retroligações');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE', 'comentários por notícia (estimativa)');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE', 'retroligações por notícia (estimation)');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY', 'entradas por dia (estimativa)');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK', 'entradas por semana (estimativa)');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH', 'entradas por mês (estimativa)');
@define('PLUGIN_EVENT_STATISTICS_OUT_COMMENTS_PER_ARTICLE2', 'comentários/entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_TRACKBACKS_PER_ARTICLE2', 'retroligações/entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_DAY2', 'entradas/dia');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_WEEK2', 'entradas/semana');
@define('PLUGIN_EVENT_STATISTICS_OUT_ARTICLES_PER_MONTH2', 'entradas/mês');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS', 'Número total de caracteres');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS2', 'caracteres');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE', 'Caracteres por entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_CHARS_PER_ARTICLE2', 'caracteres/entrada');
@define('PLUGIN_EVENT_STATISTICS_OUT_LONGEST_ARTICLES', 'As %s entradas mais longas');
@define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS', 'Número máximo de elementos');
@define('PLUGIN_EVENT_STATISTICS_MAX_ITEMS_DESC', 'Quantos itens por estatística? Valor por omissão: 20');

//Language constants for the Extended Visitors feature
@define('PLUGIN_EVENT_STATISTICS_EXT_ADD', 'Estatísticas Adicionais de Visitantes');
@define('PLUGIN_EVENT_STATISTICS_EXT_ADD_DESC', 'Juntar Estatísticas Adicionais de Visitantes? (por omissão: não)');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT1', 'Não!');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT2', 'Sim, no fundo da página');
@define('PLUGIN_EVENT_STATISTICS_EXT_OPT3', 'Sim, no topo da página');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISITORS', 'Nº de visitantes');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISTODAY', 'Nº de visitantes hoje');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISTOTAL', 'Nº total de visitantes');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISSINCE', 'As estatísticas adicionais de visitantes têm coligido dados desde');
@define('PLUGIN_EVENT_STATISTICS_EXT_VISLATEST', 'Visitantes mais recentes');
@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS', 'Referenciadores mais importantes');
@define('PLUGIN_EVENT_STATISTICS_EXT_TOPREFS_NONE', 'Nenhum referenciador registado por enquanto.');
@define('PLUGIN_EVENT_STATISTICS_OUT_EXT_STATISTICS', 'Estatísticas Adicionais de Visitantes');
@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS', 'Banir da contagem navegadores');
@define('PLUGIN_EVENT_STATISTICS_BANNED_HOSTS_DESC', 'Inserir navegadores que devem ser excluídos da contagem, separados por "|"');


/* vim: set sts=4 ts=4 expandtab : */
?>