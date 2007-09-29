#ifndef TRANSLATIONS_H__
#define TRANSLATIONS_H__

//bool Translations_FromData(const char* langs, const char* data, size_t data_len);
bool Translations_SetCurrentLanguage(const char* lang);
const char* Translatations_GetTranslation(const char* txt);
const WCHAR* Translatations_GetTranslationW(const char* txt);
void Translations_FreeData();

#define _TR(x) Translatations_GetTranslation(x)
#define _TRN(x) x
#define _TRW(x) Translatations_GetTranslationW(x)
#define _TRWN(x) x

#endif
