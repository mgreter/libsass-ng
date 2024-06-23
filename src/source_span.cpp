/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* Helper class to hold and calculate spans from two Offset positions        */
/*****************************************************************************/
#include "source_span.hpp"

#include "sources.hpp"
#include "ast_nodes.hpp"
#include "file.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Regular value constructor
  // Also known as `pstate`
  SourceSpan::SourceSpan(
    SourceDataObj source,
    const Offset& position,
    const Offset& span) :
    SourceState(source, position),
    span(span)
  {}

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Create SourceSpan for internal things
  SourceSpan SourceSpan::internal32(const char* label)
  {
    return SourceSpan(SASS_MEMORY_NEW(
      SourceString, "sass://internal", label),
      Offset{}, Offset{});
  }

  // Create span between `lhs.start` and `rhs.end` (must be same source)
  SourceSpan SourceSpan::delta(const SourceSpan& lhs, const SourceSpan& rhs)
  {
    return SourceSpan(
      lhs.getSource(), lhs.position,
      Offset::distance(lhs.position,
        rhs.position + rhs.span));
  }

  // Create span between two ast-node source-spans
  SourceSpan SourceSpan::delta(AstNode* lhs, AstNode* rhs)
  {
    return SourceSpan::delta(
      lhs->pstate(), rhs->pstate());
  }

  // Create new span from start to given length
  // Assumes that there are no linefeeds within
  SourceSpan SourceSpan::first(uint32_t length) const
  {
    Offset offset; offset.column = length;
    return SourceSpan(source, position, offset);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Either return path relative to cwd if path is
  // inside cwd, otherwise return absolute path.
  sass::string SourceSpan::getDebugPath(const sass::string& cwd) const
  {
    const char* path = getAbsPath();
    // Convert (potential) absolute path to relative path
    sass::string rel_path(File::abs2rel(path, cwd, cwd));
    return StringUtils::startsWith(rel_path, "../", 3) ? path : rel_path;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool SourceSpan::operator==(const SourceSpan& rhs) const
  {
    return source.ptr() == rhs.source.ptr()
      && position == rhs.position
      && span == rhs.span;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
