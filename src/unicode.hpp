/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_UNICODE_HPP
#define SASS_UNICODE_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "utf8/checked.h"
#include "utf8/unchecked.h"

namespace Sass {
  namespace Unicode {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // naming conventions:
    // bytes: raw byte offset (0 based)
    // position: code point offset (0 based)

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    // Advances [it] by up to [count] code points, but never past [end].
    // Unlike `utf8::advance` it doesn't throw if the text is too short
    // (positions beyond the end map to the end) or invalid utf8.
    template <typename octet_iterator>
    inline void advance(octet_iterator& it, size_t count, const octet_iterator& end)
    {
      while (count > 0 && it != end) {
        // Skip the lead byte and all its continuation bytes
        ++it;
        while (it != end && (static_cast<uint8_t>(*it) & 0xC0) == 0x80) ++it;
        --count;
      }
    }

    // Return number of code points in utf8 string up to bytes offset.
    size_t codePointCount(const sass::string& utf8, size_t bytes);

    // Return number of code points in utf8 string
    size_t codePointCount(const sass::string& utf8);

    // function that will return the byte offset of a code point in a
    size_t byteOffsetAtPosition(const sass::string& utf8, size_t position);

    // Returns utf8 aware substring.
    // Parameters are in code points.
    sass::string substr(sass::string& utf8, size_t start, size_t len);

    // Utf8 aware string replacement.
    // Parameters are in code points.
    // Inserted text must be valid utf8.
    sass::string replace(sass::string& utf8, size_t start, size_t len, const sass::string& insert);

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    #ifdef _WIN32
    // functions to handle unicode paths on windows
    sass::string utf16to8(const sass::wstring& utf16);
    sass::wstring utf8to16(const sass::string& utf8);
    #endif

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  }
}

#endif
