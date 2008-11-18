<?php # $Id: serendipity_event_spamblock.php,v 1.54 2005/04/11 08:52:42 garvinhicking Exp $

        @define('PLUGIN_EVENT_SPAMBLOCK_TITLE', 'Spammvörn');
        @define('PLUGIN_EVENT_SPAMBLOCK_DESC', 'Fjöldamargar leiðir til að hindra spamm í athugasemdir');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_BODY', 'Spammhindrun: Óleyfileg skilaboð.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_IP', 'Spammhindrun: Þú getur ekki sent inn athugasemd svona skömmu eftir fyrri athugasemd.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_RBL', 'Spammhindrun: IP tala tölvunnar sem þú ert að senda frá er skráð sem opinn póstþjónn.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_SURBL', 'Spammhindrun: Athugasemdin þín inniheldur slóð sem er skráð í SURBL lista.');

        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_KILLSWITCH', 'Þetta blogg er í "Neyðar-athugasemdalæsingu", vinsamlegast komdu aftur síðar');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE', 'Banna tvírit athugasemda');
        @define('PLUGIN_EVENT_SPAMBLOCK_BODYCLONE_DESC', 'Banna gestum að senda inn athugasemd sem er með sama innihald og áðursend athugasemd');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH', 'Neyðar-athugasemdalæsing');
        @define('PLUGIN_EVENT_SPAMBLOCK_KILLSWITCH_DESC', 'Banna tímabundið athugasemdir fyrir allar færslur. Nytsamlegt ef bloggið þitt er undir spammárás.');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD', 'Tímabil IP tölu banna');
        @define('PLUGIN_EVENT_SPAMBLOCK_IPFLOOD_DESC', 'Leyfa IP tölu einungis að senda inn athugasemd á n mínútna fresti. Nytsamlegt til að hindra athugasemdaflóð.');
        @define('PLUGIN_EVENT_SPAMBLOCK_RBL', 'Synja athugasemdum frá RBL-skráðum netum');
        @define('PLUGIN_EVENT_SPAMBLOCK_RBL_DESC', 'Að virkja þetta mun láta kerfið neita athugasemdum frá netum sem eru skráð á RBL lista. Athugaðu að þetta getur haft áhrif á notendur proxy-þjóna eða innhringinotendur.');
        @define('PLUGIN_EVENT_SPAMBLOCK_SURBL', 'Neita athugasemdum sem innihalda SUBRL-skráð net');
        @define('PLUGIN_EVENT_SPAMBLOCK_SURBL_DESC', 'Neita athugasemdum sem innihalda SURBL-skráð net');
        @define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST', 'Við hvaða RBL skal haft samband?');
        @define('PLUGIN_EVENT_SPAMBLOCK_RBLLIST_DESC', 'Synja athugasemdum byggt á RVL listum sem fengnir hafa verið. Forðast lista með breytileg (dynamic) net.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS', 'Virkja "Captchas"');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_DESC', 'Neyðir notanda til að slá inn slembistreng sem sést í sértilbúinni mynd. Þetta mun hindra sjálfvirkar innsendingar á bloggið. Athugaðu þó að fólk með skerta sjón gæti átt erfitt með að þesa strenginn.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC', 'Til að hindra sjálfvirkar sendingar auglýsinga á bloggið, sláðu þá vinsamlegast inn strenginn á myndinni að neðan í viðkomandi reit. Athugasemdin þín verður einungis send ef strengurinn passar við myndina. Vinsamlegast gakktu úr skugga um að vafrinn þinn styðji og samþykki kökur, annars getur athugasemdin þín getur ekki verið sannprófuð á réttan hátt.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC2', 'Sláðu inn strenginn sem þú sérð hér í reitinn!');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_USERDESC3', 'Sláðu inn strenginn úr spammvarnarmyndinni að ofan: ');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_CAPTCHAS', 'Þú slóst ekki inn strenginn sem var sýndur rétt. Vinsamlegast horfðu á myndina og sláðu inn gildin sem eru sýnd þar.');
        @define('PLUGIN_EVENT_SPAMBLOCK_ERROR_NOTTF', '"Captchas" óvirk á þjóninum þínum. Þú þarft GDLib og freetype pakkana uppsetta fyrir PHP, og þarft að hafa .TTF skrárnar í möppunni þinni.');

        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL', 'Neyða "captchas" eftir hversu marga daga?');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHAS_TTL_DESC', '"Captchas" geta verið sett inn eftir aldri færslanna. Sláðu inn þann fjölda daga sem þú vilt að líði áður en innsettning "captchas" er nauðsynleg. Ef stillt á 0 munu "captchas" alltaf vera notuð.');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION', 'Neyða yfirlestur athugasemda eftir hversu marga daga?');
        @define('PLUGIN_EVENT_SPAMBLOCK_FORCEMODERATION_DESC', 'Þú getur stillt kerfið þannig að allar athugasemdir þurfi samþykki. Sláðu inn þann aldur færsla í dögum sem þú vilt að þurfi yfirlestur til samþykkis. 0 þýðir að kerfið mun ekki sjálfkrafa biðja um samþykki þitt.');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE', 'Hversu margir hlekkir áður en athugasemd þarf samþykki?');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_MODERATE_DESC', 'Þegar athugasemd nær ákveðnum fjölda hlekkja mun sú athugasemd vera send til yfirlesningar. 0 þýðir að engin hlekkjatalning fer fram.');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT', 'Hversu margir hlekkir áður en athugasemd er hafnað?');
        @define('PLUGIN_EVENT_SPAMBLOCK_LINKS_REJECT_DESC', 'Þegar athugasemd nær ákveðnum fjölda hlekkja mun þeirri athugasemd vera hafnaðd. 0 þýðir að engin hlekkjatalning far fram.');

        @define('PLUGIN_EVENT_SPAMBLOCK_NOTICE_MODERATION', 'Vegna ákveðinna skilyrða var athugasemd þín send til yfirlesningar af eiganda bloggkerfisins áður en hún er birt.');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR', 'Litur bakgrunns "captcha"-sins');
        @define('PLUGIN_EVENT_SPAMBLOCK_CAPTCHA_COLOR_DESC', 'Sláðu inn RGB gildi: 0,255,255');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE', 'Staðsetning atburðaskráar (logfile)');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGFILE_DESC', 'Upplýsingar um athugasemdir sem hafa verið sendar til yfirlesturs eða hafnað geta verið skráðar í atburðaskrá. Hafðu þennan streng tóman ef þú vilt hafa atburðaskráningu óvirka.');

        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_KILLSWITCH', 'Neyðar-athugasemdalæsing');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_BODYCLONE', 'Tvírit af athugasemd');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_IPFLOOD', 'IP-synjun');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_RBL', 'RBL-synjun');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_SURBL', 'SURBL-synjun');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_CAPTCHAS', 'Ógilt "captcha" (Fékk: %s, Bjóst við: %s)');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_FORCEMODERATION', 'Sjálfvirk yfirlesningarbón eftir X daga');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_REJECT', 'Of margir hlekkir');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_LINKS_MODERATE', 'Of margir hlekkir');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL', 'Fela netföng notenda sem skrá athugasemdir');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_DESC', 'Mun ekki sýna netföng notenda sem skrá athugasemdir');
        @define('PLUGIN_EVENT_SPAMBLOCK_HIDE_EMAIL_NOTICE', 'Netföng munu ekki vera sýnd, og einungis notuð fyrir tilkynningar sendar í pósti');

        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE', 'Veldu tegund atburðaskráninga');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DESC', 'Atburðaskráning synjaðra athugasemda getur verið gerð í gagnagrunn eða venjulega textaskrá');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_FILE', 'Skrá (sjá "atburðaskrá" valmöguleika að neðan)');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_DB', 'Gagnagrunnur');
        @define('PLUGIN_EVENT_SPAMBLOCK_LOGTYPE_NONE', 'Engin atburðaskráning');

        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS', 'Hvernig skal meðhöndla athugasemdir skráðar gegnum API kerfi?');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_COMMENTS_DESC', 'Þetta hefur áhrif á tegund samþykkis sem þörf er á vegna athugasemda sendra í gegnum API tengingar (Tilvísanir, WFW:commentAPI athugasemdir). Ef stillt á "yfirlestur" munu allar þessar athugasemdir þurfa samþykki fyrir birtingu. Ef stillt á "synja" munu slíkar athugasemdir vera algjörlega bannaðar. Ef stillt á "ekkert" munu athugasemdirnar vera meðhöndlaðar eins og venjulegar athugasemdir.');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_MODERATE', 'yfirlestur');
        @define('PLUGIN_EVENT_SPAMBLOCK_API_REJECT', 'synja');
        @define('PLUGIN_EVENT_SPAMBLOCK_REASON_API', 'Engar API athugasemdir (eins og tilvísanir) leyfðar');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE', 'Virkja orðasíu');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_ACTIVATE_DESC', 'Leitar að ákveðnum strengjum í athugasemdum og merkir þær sem spamm.');

        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS', 'Orðasía fyrir hlekki');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_URLS_DESC', 'Regular Expressions leyfð. Strengir aðskildir með semíkommu (;).');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS', 'Orðasía fyrir höfundanöfn');
        @define('PLUGIN_EVENT_SPAMBLOCK_FILTER_AUTHORS_DESC', 'Regular Expressions leyfð. Strengir aðskildir með semíkommu (;).');
