/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_STYLESHEET_HPP
#define SASS_STYLESHEET_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_css.hpp"
#include "modules.hpp"
#include "import.hpp"

namespace Sass {

  // Parsed stylesheet from loaded resource
  // Not yet evaluated or anything, just parsed
  class Stylesheet final : public AstNode,
    public Vectorized<Statement>,
    public Module
  {
  public:

    // Import object through which this module was loaded.
    // It also has the input type (css vs sass) attached
    ImportObj import; // ToDo: maybe just need url?

    Stylesheet(const SourceSpan& pstate, size_t reserve = 0);

    Stylesheet(const SourceSpan& pstate, StatementVector&& vec);

  };

}

#endif
