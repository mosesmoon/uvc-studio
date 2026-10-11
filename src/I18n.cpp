#include "I18n.h"

#include <cstring>

#include "I18nData.inc"

namespace i18n {

const std::vector<std::string>& languages() {
  // tools/gen_i18n.py writes the languages in menu order.
  static const std::vector<std::string> codes(std::begin(gen::kLangOrder), std::end(gen::kLangOrder));
  return codes;
}

bool isSupported(const std::string& code) {
  for (const std::string& c : languages())
    if (c == code) return true;
  return false;
}

// Splits "zh_Hant_TW.UTF-8@x" / "zh-Hant-TW" into lower-case subtags.
static std::vector<std::string> subtags(const std::string& locale) {
  std::vector<std::string> parts;
  std::string cur;
  for (char ch : locale) {
    if (ch == '.' || ch == '@') break;  // encoding / modifier
    if (ch == '-' || ch == '_') {
      parts.push_back(cur);
      cur.clear();
      continue;
    }
    cur += (ch >= 'A' && ch <= 'Z') ? (char)(ch - 'A' + 'a') : ch;
  }
  parts.push_back(cur);
  return parts;
}

std::string fromLocale(const std::string& locale) {
  std::vector<std::string> tags = subtags(locale);
  const std::string& language = tags.front();
  if (language == "zh") {
    // Traditional for the Hant script or Taiwan / Hong Kong / Macau,
    // Simplified otherwise.
    for (size_t i = 1; i < tags.size(); ++i) {
      const std::string& t = tags[i];
      if (t == "hant" || t == "tw" || t == "hk" || t == "mo") return "zh-TW";
      if (t == "hans") return "zh-CN";
    }
    return "zh-CN";
  }
  // Other languages by their language subtag alone: Portuguese from any
  // region (pt-PT, pt-BR …) uses the European Portuguese table.
  for (const char* code : {"en", "de", "fr", "it", "es", "pt", "ja", "ko", "th", "vi"})
    if (language == code) return code;
  return "";
}

std::string pickLanguage(const std::string& saved, const std::string& osLocale) {
  if (isSupported(saved)) return saved;
  std::string fromOs = fromLocale(osLocale);
  return fromOs.empty() ? "en" : fromOs;
}

static int languageIndex(const std::string& lang) {
  for (int i = 0; i < gen::kLangCount; ++i)
    if (lang == gen::kLangs[i].code) return i;
  return gen::kEnglish;
}

const char* tr(const std::string& lang, const char* key) {
  for (int k = 0; k < gen::kKeyCount; ++k) {
    if (std::strcmp(gen::kKeys[k], key) != 0) continue;
    const char* text = gen::kLangs[languageIndex(lang)].values[k];
    return text && *text ? text : gen::kLangs[gen::kEnglish].values[k];
  }
  return key;
}

}  // namespace i18n
