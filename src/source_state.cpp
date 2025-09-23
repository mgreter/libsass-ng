/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "source_state.hpp"

#include "source.hpp"

namespace Sass
{

  /////////////////////////////////////////////////////////////////////////
  // Part of SourceSpan (to store start and end position for spans)
  /////////////////////////////////////////////////////////////////////////

  // Regular value constructor
  SourceState::SourceState(
    SourceData* source,
    Offset position) :
    source(source),
    position(position)
  {
    if (source == nullptr) {
      // std::cerr << "NO SOURCE\n";
    }
    // assert(source != nullptr);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Return via attach source id
  size_t SourceState::getSrcIdx() const
  {
    return source->getSrcIdx();
  }

  // Return via requested import path
  const char* SourceState::getImpPath() const
  {
    return source->getImpPath();
  }

  // Return via resolved absolute path
  const char* SourceState::getAbsPath() const
  {
    return source->getAbsPath();
  }

  // Return via resolved absolute path
  const char* SourceState::getFileName() const
  {
    return source->getFileName();
  }

  // Return via attached source
  const char* SourceState::getContent() const
  {
    return source->content();
  }

  // Return the attached source
  SourceData* SourceState::getSource() const
  {
    return source.ptr();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
