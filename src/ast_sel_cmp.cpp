/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* This file contains all ast operator functions in one compile unit.        */
/*****************************************************************************/
#include "ast_selectors.hpp"
#include "ast_statements.hpp"
#include "callstack.hpp"

namespace Sass {

  bool SelectorList::operator== (const SelectorList& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (get(i)->hash() != rhs.get(i)->hash()) return false;
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool ComplexSelector::operator== (const ComplexSelector& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool CompoundSelector::operator== (const CompoundSelector& rhs) const
  {
    if (&rhs == this) return true;
    size_t len = size();
    size_t rlen = rhs.size();
    if (len != rlen) return false;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    for (size_t i = 0; i < len; i += 1) {
      if (get(i)->hash() != rhs.get(i)->hash()) return false;
      if (!(*get(i) == *rhs.get(i))) return false;
    }
    return true;
  }

  bool IDSelector::operator== (const IDSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // ID has no namespace
    return name() == rhs.name();
  }

  bool TypeSelector::operator== (const TypeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Match equality hard
    return ns_ == rhs.ns_ &&
      hasNs_ == rhs.hasNs_ &&
      name_ == rhs.name_;
  }

  bool ClassSelector::operator== (const ClassSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Class has no namespace
    return name() == rhs.name();
  }

  bool PlaceholderSelector::operator== (const PlaceholderSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    // Placeholder has no namespace
    return name() == rhs.name();
  }

  bool AttributeSelector::operator== (const AttributeSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    return nsMatch(rhs)
      && op() == rhs.op()
      && name() == rhs.name()
      && value() == rhs.value()
      && modifier() == rhs.modifier();
  }

  bool PseudoSelector::operator== (const PseudoSelector& rhs) const
  {
    if (&rhs == this) return true;
    #ifndef SASS_FORCE_CMP_HASH
    if (hashed() != 0 && rhs.hashed() != 0)
    #endif
      if (hash() != rhs.hash()) return false;
    return nsMatch(rhs)
      && name() == rhs.name()
      && argument() == rhs.argument()
      && isPseudoElement() == rhs.isPseudoElement()
      && ObjEquality()(selector(), rhs.selector());
  }

  // CSS Parent selectors have no distinction feature
  bool CssParentSelector::operator== (const CssParentSelector& rhs) const
  {
    return true;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
