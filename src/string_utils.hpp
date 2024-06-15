/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_STRING_UTILS_HPP
#define SASS_STRING_UTILS_HPP

// sass.hpp must go before all system headers
// to get the  __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

namespace Sass {
  namespace StringUtils {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `str` starts with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool startsWith(const sass::string& str, const char* prefix, size_t len);

    // Check if `str` starts with `prefix` (with unknown length at compile time)
    bool startsWith(const sass::string& str, const sass::string& prefix);

    // Check if `str` ends with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool endsWith(const sass::string& str, const char* suffix, size_t len);

    // Check if `str` ends with `prefix` (with unknown length at compile time)
    bool endsWith(const sass::string& str, const sass::string& suffix);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `str` starts with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool startsWithIgnoreCase(const sass::string& str, const char* prefix, size_t len);

    // Check if `str` starts with `prefix` (with unknown length at compile time)
    bool startsWithIgnoreCase(const sass::string& str, const sass::string& prefix);

    // Check if `str` ends with `prefix` (which is of given `len`)
    // Use this when `prefix` to check is static and known at compile time
    bool endsWithIgnoreCase(const sass::string& str, const char* suffix, size_t len);

    // Check if `str` ends with `prefix` (with unknown length at compile time)
    bool endsWithIgnoreCase(const sass::string& str, const sass::string& suffix);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if `a` equals to `b` (which is of given `len`)
    // Use this when `b` to check is static and known at compile time
    // Note: No unicode case sensitivity rules are implemented
    bool equalsIgnoreCase(const sass::string& a, const char* b, size_t len);

    // Check if `a` equals to `b` (with unknown length at compile time)
    // Note: No unicode case sensitivity rules are implemented
    bool equalsIgnoreCase(const sass::string& a, const sass::string& b);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Make the passed string whitespace trimmed.
    void makeTrimmed(sass::string& str);

    // Trim the left side of passed string.
    void makeLeftTrimmed(sass::string& str);

    // Trim the right side of passed string.
    void makeRightTrimmed(sass::string& str);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Make the passed string lowercase
    // Note: No unicode case sensitivity rules are implemented
    void makeLowerCase(sass::string& str);

    // Make the passed string uppercase
    // Note: No unicode case sensitivity rules are implemented
    void makeUpperCase(sass::string& str);

    // Return new string converted to lowercase
    // Note: No unicode case sensitivity rules are implemented
    sass::string toLowerCase(const sass::string& str);

    // Return new string converted to uppercase
    // Note: No unicode case sensitivity rules are implemented
    sass::string toUpperCase(const sass::string& str);


    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Check if string contains white-space only
    // Returns true if string to check is empty
    bool isWhitespaceOnly(const sass::string& str);

    // Replace all occurrences of `search` in string `str` with `replacement`.
    void replaceAll(sass::string& str, const sass::string& search, const sass::string& replacement);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Return list of strings split by `delimiter`.
    // Optionally `trim` all results (default behavior).
    sass::vector<sass::string> split(sass::string str, char delimiter, bool trim = true);

    // Return joined string from all passed strings, delimited by separator.
    sass::string join(const sass::vector<sass::string>& strings, const char* separator);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Returns [name] without a vendor prefix.
    // If [name] has no vendor prefix, it's returned as-is.
    sass::string unvendor(const sass::string& name);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  }
}

#endif
