/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
/* Implementations are mostly a direct code-port from dart-sass.             */
/*****************************************************************************/
#include "scanner_string.hpp"

#include "charcode.hpp"
#include "character.hpp"
#include "exceptions.hpp"
#include "utf8/checked.h"
#include <algorithm>

namespace Sass {

  // Import some namespaces
  using namespace Charcode;
  using namespace Character;

  StringScanner::StringScanner(
    Logger& logger,
    SourceDataObj source) :
    source(source),
    startpos(source->contentStart()),
    endpos(source->contentEnd()),
    position(source->contentStart()),
    sourceUrl(source->getAbsPath()),
    srcid(source->getSrcIdx()),
    offset(),
    relevant(),
    logger(logger)
  {
    // consume BOM before checking the full unicode range?
    // this check can be expensive for big source files
    // maybe we do this better on the fly when parsing?
    auto invalid = utf8::find_invalid(startpos, endpos);
    if (invalid != endpos) {
      SourceSpan pstate(source);
      Offset start(startpos, invalid);
      pstate.position.line = start.line;
      pstate.position.column = start.column;
      throw Exception::InvalidUnicode(pstate, {});
    }
  }

  // Check wether current position start new line
  bool StringScanner::isOnNewline() const
  {
    const char* track = position - 1;
    while (track >= startpos) {
      if (isNewline(*track)) {
        return true;
      }
      else if (isWhitespace(*track)) {
        track -= 1;
      }
      else {
        return false;
      }
    }
    return true;
  }

  // Whether the scanner has completely consumed [string].
  bool StringScanner::isDone() const
  {
    return position >= endpos;
  }

  // Called whenever a character is consumed.
  // Used to update scanner line/column position.
  void StringScanner::consumedChar(uint8_t character, uint8_t next)
  {
    switch (character) {
    case $space:
    case $tab:
    case $vt:
    case $ff:
      offset.column += 1;
      break;
    case $cr:
      // Like dart-sass (and `SourceWithPath::countLines`), a CR not
      // followed by LF is a line break; CRLF breaks once at the LF.
      if (next == $lf) {
        offset.column += 1;
        break;
      }
      offset.line += 1;
      offset.column = 0;
      break;
    case $lf:
      offset.line += 1;
      offset.column = 0;
      break;
    default:
      // skip over 10xxxxxx and 01xxxxxx
      // count ASCII and first utf8 bytes
      if (Character::isCharacter(character)) {
        // 64 => first utf8 byte
        // 128 => regular ASCII char
        offset.column += 1;
        // sync relevant position
        relevant = offset;
      }
      break;
    }
  }


  // Consumes a single character and returns its character
  // code. This throws a [FormatException] if the string has
  // been fully consumed. It doesn't affect [lastMatch].
  uint8_t StringScanner::readChar()
  {
    if (isDone()) fail("more input");
    uint8_t ascii = *position;
    consumedChar(ascii, position + 1 < endpos ? position[1] : 0);
    position += 1;
    return ascii;
  }

  // Returns the character code of the character [offset] away
  // from [position]. [offset] defaults to zero, and may be negative
  // to inspect already-consumed characters. This returns `null` if
  // [offset] points outside the string. It doesn't affect [lastMatch].

  // Note: passed offset may lead to out of bound memory access
  uint8_t StringScanner::peekChar(size_t offset) const
  {
    const char* cur = position + offset;
    if (cur < startpos || cur >= endpos) {
      return 0;
    }
    return *cur;
  }

  // Optimized for no offset argument
  uint8_t StringScanner::peekChar() const
  {
    const char* cur = position;
    if (cur < startpos || cur >= endpos) {
      return 0;
    }
    return *cur;
  }

  // Same as above, but stores next char into passed variable.
  // Note: passed offset may lead to out of bound memory access
  bool StringScanner::peekChar(uint8_t& chr, size_t offset) const
  {
    const char* cur = position + offset;
    if (cur < startpos || cur >= endpos) {
      return false;
    }
    chr = *cur;
    return true;
  }

  // Optimized for no offset argument
  bool StringScanner::peekChar(uint8_t& chr) const
  {
    const char* cur = position;
    if (cur < startpos || cur >= endpos) {
      return false;
    }
    chr = *cur;
    return true;
  }

  // If the next character in the string is [character], consumes it.
  // Returns whether or not [character] was consumed.
  bool StringScanner::scanChar(uint8_t character)
  {
    if (isDone()) return false;
    uint8_t ascii = *position;
    if (character != ascii) return false;
    consumedChar(character, position + 1 < endpos ? position[1] : 0);
    position += 1;
    return true;
  }

  // If the next character in the string is [character], consumes it.
  // If [character] could not be consumed, throws a [FormatException]
  // describing the position of the failure. [name] is used in this
  // error as the expected name of the character being matched; if
  // it's empty, the character itself is used instead.
  void StringScanner::expectChar(uint8_t character, const sass::string& name, bool advance)
  {
    if (!scanChar(character)) {
      if (advance && !isDone()) {
        relevant = offset;
      }
      if (name.empty()) {
        if (character == $quote) {
          fail("\"\\\"\"");
        }
        else {
          sass::string msg("\"");
          msg += character;
          fail(msg + "\"");
        }
      }
      fail(name);
    }
  }

  // If [pattern] matches at the current position of the string, scans forward
  // until the end of the match. Returns whether or not [pattern] matched.
  bool StringScanner::scan(const sass::string& pattern)
  {
    // First check the whole pattern without side effects. This used to
    // update the line/column offset for every matching character, also
    // when the pattern only matched partially (e.g. the first `/` of
    // "//" before `/*`), so later spans were reported a column too far.
    // It also checked `isDone()` for the start position only, so it
    // could read at or past `endpos` near the end of the input.
    size_t len = pattern.size();
    if (size_t(endpos - position) < len) return false;
    if (!std::equal(pattern.begin(), pattern.end(), position)) return false;
    // Pattern matched, now consume it and update the offset
    for (const char* cur = position; cur < position + len; cur += 1) {
      consumedChar(*cur, cur + 1 < endpos ? cur[1] : 0);
    }
    position += len;
    return true;
  }

  // If [pattern] matches at the current position of the string, scans
  // forward until the end of the match. If [pattern] did not match,
  // throws a [FormatException] describing the position of the failure.
  // [name] is used in this error as the expected name of the pattern
  // being matched; if it's `null`, the pattern itself is used instead.
  void StringScanner::expect(const sass::string& pattern, const sass::string& name)
  {
    if (!scan(pattern)) {
      if (name.empty()) {
        fail(pattern);
      }
      fail(name);
    }
  }

  // If the string has not been fully consumed,
  // this throws a [FormatException].
  void StringScanner::expectDone() const
  {
    if (isDone()) return;
    SourceSpan span(rawSpan());
    CallStackFrame frame(logger, span);
    throw Exception::ParserException(
      logger, "expected no more input.");
  }

  // Returns whether or not [pattern] matches at the current position
  // of the string. This doesn't move the scan pointer forward.
  bool StringScanner::matches(const sass::string& pattern) const
  {
    const char* cur = position;
    for (char chr : pattern) {
      if (chr != *cur) {
        return false;
      }
      cur += 1;
    }
    return true;
  }

  // Returns the substring of [string] between [start] and [end].
  // Unlike [String.substring], [end] defaults to [position]
  // rather than the end of the string.
  sass::string StringScanner::substring(const char* start, const char* end) const
  {
    if (end == nullptr) end = position;
    return sass::string(start, end);
  }

  SourceSpan StringScanner::errorSpan() const
  {
    const char* cur = position;
    const char* lastNewline = nullptr;
    while (cur > startpos && Character::isWhitespace((uint8_t)cur[-1])) {
      if (Character::isNewline((uint8_t)cur[-1])) lastNewline = cur - 1;
      cur -= 1;
    }
    // Only if there is any non-whitespace before
    if (lastNewline == nullptr || cur == startpos) return rawSpan();
    Offset local(startpos, lastNewline);
    return spanAt(local);
  }

  // Throws a [FormatException] describing that [name] is
  // expected at the current position in the string.
  void StringScanner::fail(
    const sass::string& name) const
  {
    SourceSpan span(relevantSpan());
    CallStackFrame frame(logger, span);
    sass::string msg("expected " + name + ".");
    throw Exception::ParserException(logger, msg);
  }

  // Throws a [FormatException] with [traces] and [pstate].
  void StringScanner::error(
    const sass::string& message,
    const BackTraces& traces,
    const SourceSpan& pstate) const
  {
    CallStackFrame frame(logger, pstate);
    throw Exception::ParserException(traces, message);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  bool StringScanner::hasLineBreak(
    const char* before) const
  {
    const char* cur = before;
    while (cur < endpos) {
      if (*cur == '\r') return true;
      if (*cur == '\n') return true;
      cur += 1;
    }
    return false;
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}
