/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_SOURCE_SPAN_HPP
#define SASS_SOURCE_SPAN_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "source_state.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  class SourceSpan : public SourceState
  {

  public:

    // Offset size
    Offset span;

    // Empty constructor
    SourceSpan() {}

    // Regular value constructor
    SourceSpan(SourceDataObj source,
      const Offset& position = Offset(),
      const Offset& span = Offset());

    // Create SourceSpan for internal things
    static SourceSpan internal(const char* path);

    // Create span between `lhs.start` and `rhs.end` (must be same source)
    static SourceSpan delta(const SourceSpan& lhs, const SourceSpan& rhs);

    // Create span between two ast-node source-spans
    static SourceSpan delta(AstNode* lhs, AstNode* rhs);

    // Create new span from start to given length
    // Assumes that there are no linefeeds within
    SourceSpan first(uint32_t length) const;

    // Either return path relative to cwd if path is
    // inside cwd, otherwise return absolute path.
    sass::string getDebugPath() const;

    bool operator==(const SourceSpan& rhs) const;

  public: // down casts

    CAPI_WRAPPER(SourceSpan, SassSrcSpan);

  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  struct StringToken {
    sass::string str;
    SourceSpan pstate;
    operator sass::string&() { return str; }
    operator const sass::string& () { return str; }
    operator SourceSpan&() { return pstate; }
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

} // namespace Sass

#endif
