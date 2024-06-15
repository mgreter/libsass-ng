/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Bread and Butter helpers for (optimized) string operations.
// Think of all the java StringUtil classes (guava, apache, etc)
// With optimized versions to compare against static/known strings.
// Note: ignore case is only ASCII aware (no unicode rules apply)
/*****************************************************************************/
#include "string_utils.hpp"

#include "character.hpp"

#include <cstdio>
#include <cstring>

namespace Sass {

  namespace StringUtils {

    using namespace Character;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Compare two characters (ASCII) case insensitive
    // Note: No unicode case sensitivity rules are implemented
    static bool _equalsIgnoreCase(const char a, const char b) {
      return Character::characterEqualsIgnoreCase(a, b);
    }

    // Optimized version where we know one side is already lowercase
    // You must make sure that the first argument is already lowercase!
    // Note: No unicode case sensitivity rules are implemented
    static bool _equalsIgnoreCaseConst(const char a, const char b) {
      return a == b || a == Character::toLowerCase(b);
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `str` starts with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool startsWith(const sass::string& str, const char* prefix, size_t len) {
      return len <= str.size() && std::equal(prefix, prefix + len, str.begin());
    }

    // Check if `str` starts with `prefix` (with unknown length at compile time)
    bool startsWith(const sass::string& str, const sass::string& prefix) {
      return prefix.size() <= str.size() && std::equal(prefix.begin(), prefix.end(), str.begin());
    }

    // Check if `str` ends with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool endsWith(const sass::string& str, const char* suffix, size_t len) {
      return len <= str.size() && std::equal(suffix, suffix + len, str.end() - len);
    }

    // Check if `str` ends with `prefix` (with unknown length at compile time)
    bool endsWith(const sass::string& str, const sass::string& suffix) {
      return suffix.size() <= str.size() && std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `str` starts with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool startsWithIgnoreCase(const sass::string& str, const char* prefix, size_t len) {
      return len <= str.size() && std::equal(prefix, prefix + len, str.begin(), _equalsIgnoreCaseConst);
    }

    // Check if `str` starts with `prefix` (with unknown length at compile time)
    bool startsWithIgnoreCase(const sass::string& str, const sass::string& prefix) {
      return prefix.size() <= str.size() && std::equal(prefix.begin(), prefix.end(), str.begin(), _equalsIgnoreCase);
    }

    // Check if `str` ends with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool endsWithIgnoreCase(const sass::string& str, const char* suffix, size_t len) {
      return len <= str.size() && std::equal(suffix, suffix + len, str.end() - len, _equalsIgnoreCaseConst);
    }

    // Check if `str` ends with `prefix` (with unknown length at compile time)
    bool endsWithIgnoreCase(const sass::string& str, const sass::string& suffix) {
      return suffix.size() <= str.size() && std::equal(suffix.rbegin(), suffix.rend(), str.rbegin(), _equalsIgnoreCase);
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `a` equals to `b` (which is of given `len`)
    // Use this when `b` to check is static and known at compile time
    // Note: No unicode case sensitivity rules are implemented
    bool equalsIgnoreCase(const sass::string& a, const char* b, size_t len) {
      return len == a.size() && std::equal(b, b + len, a.begin(), _equalsIgnoreCaseConst);
    }

    // Check if `a` equals to `b` (with unknown length at compile time)
    // Note: No unicode case sensitivity rules are implemented
    bool equalsIgnoreCase(const sass::string& a, const sass::string& b) {
      return a.size() == b.size() && std::equal(b.begin(), b.end(), a.begin(), _equalsIgnoreCaseConst);
    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Make the passed string whitespace trimmed
    void makeTrimmed(sass::string& str)
    {
      makeLeftTrimmed(str);
      makeRightTrimmed(str);
    }
    // EO makeTrimmed

    // Trim the left side of passed string
    void makeLeftTrimmed(sass::string& str)
    {
      if (str.begin() != str.end()) {
        auto pos = std::find_if_not(
          str.begin(), str.end(),
          Character::isWhitespace);
        str.erase(str.begin(), pos);
      }
    }
    // EO makeLeftTrimmed

    // Trim the right side of passed string
    void makeRightTrimmed(sass::string& str)
    {
      if (str.begin() != str.end()) {
        auto pos = std::find_if_not(
          str.rbegin(), str.rend(),
          Character::isWhitespace);
        str.erase(str.rend() - pos);
      }
    }
    // EO makeRightTrimmed

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Make the passed string lowercase
    // Note: No unicode case sensitivity rules are implemented
    void makeLowerCase(sass::string& str)
    {
      for (char& character : str) {
        if (character >= $A && character <= $Z)
          character |= asciiCaseBit;
      }
    }
    // EO makeLowerCase

    // Make the passed string uppercase
    // Note: No unicode case sensitivity rules are implemented
    void makeUpperCase(sass::string& str)
    {
      for (char& character : str) {
        if (character >= $a && character <= $z)
          character &= ~asciiCaseBit;
      }
    }
    // EO makeUpperCase

    // Return new string converted to lowercase
    // Note: No unicode case sensitivity rules are implemented
    sass::string toLowerCase(const sass::string& str) {
      sass::string rv(str);
      for (char& character : rv) {
        if (character >= $A && character <= $Z)
          character |= asciiCaseBit;
      }
      return rv;
    }
    // EO toLowerCase

    // Return new string converted to uppercase
    // Note: No unicode case sensitivity rules are implemented
    sass::string toUpperCase(const sass::string& str) {
      sass::string rv(str);
      for (char& character : rv) {
        if (character >= $a && character <= $z)
          character &= ~asciiCaseBit;
      }
      return rv;
    }
    // EO toUpperCase

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if string contains white-space only
    // Returns true if string to check is empty
    bool isWhitespaceOnly(const sass::string& str)
    {
      for (const char& character : str) {
        if (!isWhitespace(character))
          return false;
      }
      return true;
    }
    // EO isWhitespaceOnly

    // Replace all occurrences of `search` in string `str` with `replacement`.
    void replaceAll(sass::string& str,
      const sass::string& search,
      const sass::string& replacement)
    {
      size_t pos = str.find(search);
      while (pos != std::string::npos)
      {
        str.replace(pos, search.size(), replacement);
        pos = str.find(search, pos + replacement.size());
      }
    }
    // EO replaceAll

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Return list of strings split by `delimiter`.
    // Optionally `trim` all results (default behavior).
    sass::vector<sass::string> split(sass::string str, char delimiter, bool trim)
    {
      sass::vector<sass::string> rv;
      if (trim) StringUtils::makeTrimmed(str);
      if (str.empty()) return rv;
      size_t start = 0, end = str.find_first_of(delimiter);
      // search until we are at the end
      while (end != sass::string::npos) {
        // add substring from current position to delimiter
        sass::string item(str.substr(start, end - start));
        if (trim) StringUtils::makeTrimmed(item);
        if (!trim || !item.empty()) rv.emplace_back(item);
        start = end + 1; // skip delimiter
        end = str.find_first_of(delimiter, start);
      }
      // add last substring from current position to end
      sass::string item(str.substr(start, end - start));
      if (trim) StringUtils::makeTrimmed(item);
      if (!trim || !item.empty()) rv.emplace_back(item);
      // return back
      return rv;
    }
    // EO split

    // Return joined string from all passed strings, delimited by separator.
    sass::string join(const sass::vector<sass::string>& strings, const char* separator)
    {
      switch (strings.size())
      {
      case 0:
        return "";
      case 1:
        return strings[0];
      default:
        size_t size = strings[0].size();
        size_t sep_len = ::strlen(separator);
        for (size_t i = 1; i < strings.size(); i++) {
          size += sep_len + strings[i].size();
        }
        sass::string os;
        os.reserve(size);
        os += strings[0];
        for (size_t i = 1; i < strings.size(); i++) {
          os += separator;
          os += strings[i];
        }
        return os;
      }
    }
    // EO join

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Returns [name] without a vendor prefix.
    // If [name] has no vendor prefix, it's returned as-is.
    sass::string unvendor(const sass::string& name)
    {
      if (name.size() < 2) return name;
      if (name[0] != '-') return name;
      if (name[1] == '-') return name;
      for (size_t i = 2; i < name.size(); i++) {
        if (name[i] == '-') return name.substr(i + 1);
      }
      return name;
    }
    // EO unvendor

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  }
}
