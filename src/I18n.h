#pragma once
#include <string>
#include <vector>

namespace i18n {

// The 12 interface languages in menu order: zh-TW, en, de, fr, it, es, pt,
// ja, ko, zh-CN, th, vi.
const std::vector<std::string>& languages();
bool isSupported(const std::string& code);
// Interface language for an OS locale name ("zh_TW.UTF-8", "zh-Hant-HK",
// "ja-JP", "de_AT" …): matched by language, Chinese by script / region.
// "" when the language is not offered.
std::string fromLocale(const std::string& locale);
// Saved choice → OS locale → English.
std::string pickLanguage(const std::string& saved, const std::string& osLocale);
// Translated string for `key` (see tools/gen_i18n.py); English when the
// language has no text for it, the key itself when it is unknown.
const char* tr(const std::string& lang, const char* key);

}  // namespace i18n
