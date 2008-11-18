<?php # 

##########################################################################
# serendipity - another blogger...                                       #
##########################################################################
#                                                                        #
# (c) 2003 Jannis Hermanns <J@hacked.it>                                 #
# http://www.jannis.to/programming/serendipity.html                      #
#                                                                        #
# Translated by                                                          #
# Joao P Matos <jmatos@math.ist.utl.pt>                                  #
#                                                                        #
#                                                                        #
##########################################################################

@define('PLUGIN_EVENT_SPAMBLOCK_TITLE', 'Protecção anti Spam');
@define('PLUGIN_EVENT_SPAMBLOCK_DESC', 'Oferece uma imensidade de possibilidades para proteger o seu blogue contra Spam nos comentários.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', 'Protecção contra o Spam: mensagem não válida.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', 'Protecção contra o Spam: não pode juntar um comentário suplementar num intervalo tão curto.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_RBL', 'Protecção contra o Spam: o endereço IP do computador que usa para escrever o comentário está listado como um relé aberto.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_SURBL', 'Protecção contra o Spam: o seu comentário contém um endereço listado no SURBL.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', 'Este blogue está em modo "Bloqueio urgente de comentários", pelo que se agradece que tente mais tarde.');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', 'Não autorizar a duplicação de comentários');
@define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', 'Não autorizar os utilizadores a juntar comentários que têm o mesmo conteúdo dum comentário existente.');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', 'Bloqueio de urgência de comentários');
@define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', 'Permite-lhe inactivar temporariamente os comentários de todos os artigos. Prático se o seu blogue está sob um ataque de Spam.');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'Intervalo de bloqueio de endereço IP');
@define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', 'Só autorizar um endereço IP a submeter comentários cada n minutos. Prático para evitar um dilúvio de comentários.');
@define('PLUGIN_EVENT_SPAMBLOCK_RBL', 'Recusar comentários por lista negra');
@define('PLUGIN_EVENT_SPAMBLOCK_RBL_DESC', 'Si active, cette option permet de refuser les commentaires ventant d\'hôtes listés dans les RBLs (listes noires). Notez que cela peut avoir un effet sur les utilisateurs derrière un proxy ou dont le fournisseur internet est sur liste noire.');
@define('PLUGIN_EVENT_SPAMBLOCK_SURBL', 'Recusar comentários usando SURBL');
@define('PLUGIN_EVENT_SPAMBLOCK_SURBL_DESC', 'Recusa comentários contendo ligações para máquinas listadas na base de dados <a href="http://www.surbl.org">SURBL</a>');
@define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST', 'RBLs a contactar');
@define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST_DESC', 'Bloqueia os comentários com base nas listas RBL definidas aqui.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Activar os captchas');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', 'Força os utilizadores a introduzir um texto mostrado por uma imagem gerada automaticamente para evitar que sistemas automatizados possam adicionar comentários. É de notar que isto causa problemas a pessoas com deficiências visuais.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', 'Para evitar o spam por robots automatizados (spambots), agradecemos que introduza os caracteres que vê abaixo no campo de formulário para esse efeito. Certifique-se que o seu navegador gere e aceita cookies, caso contrário o seu comentário não poderá ser registado.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', 'Introduza o texto que está a ver no campo!');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', 'Introduza o texto da imagem anti-spam acima: ');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', 'Não introduziu correctamente o texto da imagem anti-spam. Por favor corrija o seu código verificando de novo a imagem.');
@define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', 'Os captchas não estão disponíveis no seu servidor. É preciso que a GDLib e as bibliotecas freetype estejam compiladas na sua instalação de PHP.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', 'Captchas automáticos depois de X dias');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', 'Os captchas podem ser activados automaticamente depois de um certo número de dias para cada artigo. Para activá-los sempre, introduza um 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', 'Moderação automática depois de X dias');
@define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', 'A moderação dos comentários pode ser activada automaticamente depois de um certo número de dias após a publicação de um artigo. Para não utilizar a moderação automática, introduza um 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', 'Moderação automática depois de X ligações');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', 'A moderação dos comentários pode ser activada automaticamente se o número de ligações contidos num comentário ultrapassa um número estabelecido. Para não utilizar esta função, introduza um 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', 'Recusa automática para além de X ligações');
@define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', 'Um comentário pode ser recusado automaticamente se o número de ligações ultrapassa um certo número. Para não usar esta função, introduza 0.');
@define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', 'Devido a certas condições, o seu comentário está sujeito a moderação pelo autor do blogue antes de ser publicado.');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'Cor de fundo do captcha');
@define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'Introduza valores RGB: 0,255,255');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', 'Ficheiro de log');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', 'A informação sobre os comentários recusados/moderados pode ser registada num ficheiro de log, indique uma localização para esse ficheiro se quiser usar esta função.');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', 'Bloqueio de urgência de comentários');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', 'Duplicação de comentário');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'Bloqueio por IP');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_RBL', 'Bloqueio por RBL');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_SURBL', 'Bloqueio por SURBL');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', 'Captcha inválido (Introduzido: %s, Válido: %s)');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'Moderação automática depois de X dias');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', 'Máximo de ligações');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', 'demasiadas ligações');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', 'Mascarar os endereços de email');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', 'Masque les adresses Emqil des utilisateurs qui ont écrit des commentaires');
@define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', 'Les adresses Email ne sont pas affichées, et sont seulement utilisées pour la communication.');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', 'Escolha um método para os logs');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', 'Os logs de comentários recusados podem ter como suporte um ficheiro de texto, ou uma base de dados.');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', 'Ficheiro (ver a opção \'Ficheiro de log\')');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', 'Base de dados');
@define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', 'Não usar logs');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'Gestão dos comentários por interface');
@define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'Define como o Serendipity gere os comentários feitos pela interface (retroligações, comentários WFW:commentAPI). Se seleccionar "moderação", estes comentários estarão sempre sujeitos a moderação. Com "recusar", não são autorizados. Com "nenhum", estes comentários serão geridos como comentários tradicionais.');
@define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', 'moderação');
@define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', 'recusa');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', 'nenhum');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', 'Filtragem de palavras chave');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', 'Marca todos os comentários contendo as palavras chave definidas como Spam.');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', 'Filtragem por palavras chave para as ligações');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', 'Marca todos os comentários cujas ligações contêm palavras chave consideradas como definidoras de Spam. As expressões regulares são autorizadas, separe as palavras chave por ponto e vírgula (;).');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', 'Filtragem por nome de autor');
@define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', 'Marca todos os comentários cujo nome de autor contém palavras chave consideradas como indicadoras de Spam. As expressões regulares são autorizadas, separe as palavras chave por ponto e vírgula (;).');


@define('PLUGIN_EVENT_SPAMBLOCK_REASON_CHECKMAIL', 'Endereço de email inválido');
@define('PLUGIN_EVENT_SPAMBLOCK_CHECKMAIL', 'Verificar endereços de email?');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS', 'Campos de comentário obrigatórios');
@define('PLUGIN_EVENT_SPAMBLOCK_REQUIRED_FIELDS_DESC', 'Introduza uma lista de campos de preenchimento obrigatório quando um utilizador submete comentários. Separe os diversos campos com uma vírgula ",". As chaves disponíveis são: nome, email, url, replyTo, comentário');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_REQUIRED_FIELD', 'Não especificou o campo %s!');

@define('PLUGIN_EVENT_SPAMBLOCK_CONFIG', 'Configuração de métodos Anti-Spam');
@define('PLUGIN_EVENT_SPAMBLOCK_ADD_AUTHOR', 'Bloquear este autor via plugin Spamblock');
@define('PLUGIN_EVENT_SPAMBLOCK_ADD_URL', 'Bloquear esta URL via plugin Spamblock');
@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_AUTHOR', 'Desbloquear este autor via plugin Spamblock');
@define('PLUGIN_EVENT_SPAMBLOCK_REMOVE_URL', 'Desbloquear esta URL via plugin Spamblock');

@define('PLUGIN_EVENT_SPAMBLOCK_BLOGG_SPAMLIST', 'Activar filtragem de URL via a lista negra de blogg.de');
@define('PLUGIN_EVENT_SPAMBLOCK_REASON_BLOGG_SPAMLIST', 'Filtrado pela lista negra de blogg.deq');

/* vim: set sts=4 ts=4 expandtab : */
?>
