/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// A few additional helpers around the utf8/unicode libarary
// LibSass stores all strings in unicode via utf8 encoding, as it is
// the most efficient format. Since it is safe to assume that most 
// of sass code is in ASCII range. Even java moved back from utf16
// to utf8 internally, since only a 32bit per char implementation
// would be able able to hold all potential unicode code-points.
// Such an implementation would have constant index access, but
// would use a 4 times fold of memory to store strings. With utf8,
// we get a memory efficient storage, with the downside of non-
// constant index access, with impacts mostly for `substring`
// and `replace`, as we need to seek through the encoded byte
// stream to find the appropriate byte offset addresses.
/*****************************************************************************/

#include "unicode.hpp"

namespace Sass {
  namespace Unicode {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // naming conventions:
    // bytes: raw byte offset (0 based)
    // position: code point offset (0 based)

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Return number of code points in utf8 string
    size_t codePointCount(const sass::string& utf8) {
      return utf8::distance(utf8.begin(), utf8.end());
    }
    // EO codePointCount

    // Return number of code points in utf8 string up to bytes offset.
    size_t codePointCount(const sass::string& utf8, size_t bytes) {
      return utf8::distance(utf8.begin(), utf8.begin() + bytes);
    }
    // EO codePointCount

    // Return the byte offset at a code point position
    size_t byteOffsetAtPosition(const sass::string& utf8, size_t position) {
      sass::string::const_iterator it = utf8.begin();
      utf8::advance(it, position, utf8.end());
      return std::distance(utf8.begin(), it);
    }
    // EO byteOffsetAtPosition

    // Returns utf8 aware substring.
    // Parameters are in code points.
    sass::string substr(
      sass::string& utf8,
      size_t start,
      size_t len)
    {
      // Get initial byte position
      auto first = utf8.begin();
      // Advance to utf8 position
      utf8::advance(first,
        start, utf8.end());
      // Begin to find end position
      auto last = first;
      // Pass npos to indicate til end
      if (len == sass::string::npos) {
        last = utf8.end();
      }
      // Or advance given length in utf8
      else {
        utf8::advance(last,
          len, utf8.end());
      }
      // Return first to last
      return sass::string(
        first, last);
    }
    // EO substr

    // Utf8 aware string replacement.
    // Parameters are in code points.
    // Inserted text must be valid utf8.
    sass::string replace(
      sass::string& text,
      size_t start, size_t len,
      const sass::string& insert)
    {
      // Get initial byte position
      auto first = text.begin();
      // Advance to utf8 position
      utf8::advance(first,
        start, text.end());
      // Begin to find end position
      auto last = first;
      // Pass npos to indicate til end
      if (len == sass::string::npos) {
        last = text.end();
      }
      // Or advance given length in utf8
      else {
        utf8::advance(last,
          len, text.end());
      }
      // Now replace via byte positions
      // UB if `insert` is invalid utf8
      return text.replace(
        first, last,
        insert);
    }
    // EO replace

    /////////////////////////////////////////////////////////////////////////
    // Conversion helpers for windows file-system access
    /////////////////////////////////////////////////////////////////////////

    #ifdef _WIN32

    // utf16 functions
    using std::wstring;

    // convert from utf16/wide string to utf8 string
    sass::string utf16to8(const sass::wstring& utf16)
    {
      sass::string utf8;
      // preallocate expected memory
      utf8.reserve(sizeof(utf16)/2);
      utf8::utf16to8(utf16.begin(), utf16.end(),
                     back_inserter(utf8));
      return utf8;
    }
    // EO utf16to8

    // convert from utf8 string to utf16/wide string
    sass::wstring utf8to16(const sass::string& utf8)
    {
      sass::wstring utf16;
      // preallocate expected memory
      utf16.reserve(codePointCount(utf8)*2);
      utf8::utf8to16(utf8.begin(), utf8.end(),
                     back_inserter(utf16));
      return utf16;
    }
    // EO utf8to16

    #endif

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  }
}
