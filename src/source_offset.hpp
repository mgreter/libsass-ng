/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Base class to help holding and calculating line/column positions
/*****************************************************************************/
#ifndef SASS_OFFSET_HPP
#define SASS_OFFSET_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_def_macros.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Basic class for text file positions
  // The logic how to count characters and
  // to add/subtract are implemented here.
  class Offset
  {
  public:

    // All properties are public (zero based)
    // Getters return human-readable form (+1)
    uint32_t line = 0;
    uint32_t column = 0;

    // Default constructor
    Offset();

    // Create an Offset from the given string
    // Will use `plus` internally on all chars
    Offset(uint8_t character);

    // Create an Offset from the given string
    // Will use `plus` internally on all chars
    Offset(const sass::string& text);

    // Create an Offset from the given char star
    // Will use `plus` internally on all chars
    Offset(const char* beg, const char* end = 0);

    // Create offset with given [line] and [column]
    // Needs static constructor to avoid ambiguity
    static Offset init(size_t line, size_t column);

    // Return the `distance` between [start[ and [end]
    // Gives the solution to the equation `end = start + x`
    static Offset distance(const Offset& start, const Offset& end);

    // Append [character] to increment offset
    void plus(uint8_t character);

    // Append [text] to increment offset
    void plus(const sass::string& text);

    // Assign and increment operator
    void operator+= (const Offset& pos);

    // Plus operator (returns new Offset)
    Offset operator+ (const Offset& off) const;

    // Multiply operator (returns new Offset)
    Offset operator* (uint32_t mul) const;

    // Implement equal and derive unequal
    bool operator==(const Offset& rhs) const;

    // Return line as human readable
    // Starting from one instead of zero
    inline uint32_t getLine() const
    {
      return line + 1;
    }

    // Return column as human readable
    // Starting from one instead of zero
    inline uint32_t getColumn() const
    {
      return column + 1;
    }

    // Return line as raw value
    inline uint32_t getRawLine() const
    {
      return line;
    }

    // Return column as raw value
    inline uint32_t getRawColumn() const
    {
      return column;
    }

    // Delete other operators to make implementation more clear
    // Helps us spot cases where we use undefined implementations
    // bool operator!=(const Offset& rhs) const = delete;
    // bool operator>=(const Offset& rhs) const = delete;
    // bool operator<=(const Offset& rhs) const = delete;
    // bool operator>(const Offset& rhs) const = delete;
    // bool operator<(const Offset& rhs) const = delete;

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
