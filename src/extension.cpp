/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "extension.hpp"

#include "callstack.hpp"
#include "ast_helpers.hpp"
#include "exceptions.hpp"
#include "ast_css.hpp"
#include "extender.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  // Static function to create a copy with a new extender
  /////////////////////////////////////////////////////////////////////////
  Extension* Extension::withExtender(ComplexSelectorObj& newExtender) const
  {
    return SASS_MEMORY_NEW(Extension,
      newExtender->pstate(),
      newExtender, target, mediaContext, isOptional);
  }

  void Extension::AddAllTo(ExtSet& list)
  {
    {
      list.insert(this);
      if (IsMerged()) {
        merged->AddAllTo(list);
      }
    }
  }

  void Extension::EraseAllFrom(ExtSet& list)
  {
    {
      list.erase(this);
      if (IsMerged()) {
        merged->EraseAllFrom(list);
      }
    }
  }

  // Creates a one-off extension that's not intended to be modified over time.
  // If [specificity] isn't passed, it defaults to `extender.maxSpecificity`.

  Extension::Extension(
    const SourceSpan& pstate,
    ComplexSelectorObj& extender,
    const SimpleSelectorObj& target,
    CssMediaQueryVector* mediaContext,
    bool isOriginal, bool isOptional) :
    pstate(pstate),
    extender(pstate, extender, 0, isOriginal, mediaContext),
    target(target),
    specificity(extender->maxSpecificity()),
    isOptional(isOptional),
    isOriginal(isOriginal),
    isConsumed(false),
    mediaContext(mediaContext),
    merged(nullptr)
  {
    //std::cerr << "CREATED INIT " << this << "\n";
  }


//  Extension::Extension(Extender extender) :
//    extender(extender),
//    specificity(0),
//    isOptional(true),
//    isOriginal(false),
//    isConsumed(false)
//  {}
//
  // Copy constructor

  Extension::Extension(const Extension & extension) :
    extender(extension.extender),
    target(extension.target),
    specificity(extension.specificity),
    isOptional(extension.isOptional),
    isOriginal(extension.isOriginal),
    isConsumed(extension.isConsumed),
    mediaContext(extension.mediaContext),
    merged(extension.merged)
  {
    //std::cerr << "CREATED COPY " << this << "\n";
  }

  Extension::Extension() :
    extender(SourceSpan::internal32("Ext"), {}, 0, false),
    specificity(0),
    isOptional(false),
    isOriginal(false),
    isConsumed(false)
  {
    // std::cerr << "CREATED NEW " << this << "\n";
  }

  Extension& Extension::operator=(const Extension& other)
  {
    extender = other.extender;
    target = other.target;
    specificity = other.specificity;
    isOptional = other.isOptional;
    isOriginal = other.isOriginal;
    isConsumed = other.isConsumed;
    mediaContext = other.mediaContext;
    merged = other.merged;
    return *this;
  }

  /////////////////////////////////////////////////////////////////////////
  // Asserts that the [mediaContext] for a selector is
  // compatible with the query context for this extender.
  /////////////////////////////////////////////////////////////////////////
  void Extension::assertCompatibleMediaContext(CssMediaQueryVector* mediaQueryContext, BackTraces& traces) const
  {

    if (this->mediaContext.isNull()) return;

    // if (mediaQueryContext && mediaContext.ptr() == mediaQueryContext) return;
    if (ObjEqualityFn<CssMediaQueryVectorObj>(mediaQueryContext, mediaContext)) return;

    throw Exception::ExtendAcrossMedia(traces, this);

  }

  /////////////////////////////////////////////////////////////////////////
  // Asserts that the [mediaContext] for a selector is
  // compatible with the query context for this extender.
  /////////////////////////////////////////////////////////////////////////
  void Extender::assertCompatibleMediaContext(CssMediaQueryVector* mediaQueryContext, BackTraces& traces) const
  {

    if (this->mediaContext.isNull()) return;

    // if (mediaQueryContext && mediaContext.ptr() == mediaQueryContext) return;
    if (ObjEqualityFn<CssMediaQueryVectorObj>(mediaQueryContext, mediaContext)) return;

    throw Exception::ExtendAcrossMedia(traces, this);

  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
