/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Subclass of inspect, specializing in outputting valid css
// Mostly adding error cases for non css representable values
/*****************************************************************************/
#include "cssize.hpp"

#include "charcode.hpp"
#include "character.hpp"
#include "ast_values.hpp"
#include "exceptions.hpp"

namespace Sass {

  // Import some namespaces
  using namespace Charcode;
  using namespace Character;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  void Cssize::visitFunction(Function* f)
  {
    // Functions can't be represented in css
    throw Exception::InvalidCssValue({}, *f);
  }

  void Cssize::visitMixin(Mixin* value)
  {
    // Mixins can't be represented in css
    throw Exception::InvalidCssValue({}, *value);
  }

  void Cssize::visitModuleRef(ModuleRef* value)
  {
    // Modules can't be represented in css
    throw Exception::InvalidCssValue({}, *value);
  }

  void Cssize::visitMap(Map* value)
  {
    // maps can't be handled when writing to css
    throw Exception::InvalidCssValue({}, *value);
  }

  void Cssize::visitList(List* list)
  {
    if (list->empty() && !list->hasBrackets()) {
      throw Exception::InvalidCssValue({}, *list);
    }
    Inspect::visitList(list);
  }

  void Cssize::visitNumber(Number* n)
  {

    // Check if it can be represented as a division
    if (n->lhsAsSlash() && n->rhsAsSlash()) {
      n->lhsAsSlash()->accept(this);
      append_string("/");
      n->rhsAsSlash()->accept(this);
      return;
    }

    // reduce units
    n->reduce();

    // Like dart-sass (since 1.96), numbers with units that can't be
    // represented in CSS are written as calculations (see Inspect)
    Inspect::visitNumber(n);

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

