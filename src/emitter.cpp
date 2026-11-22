#include "emitter.hpp"

#include "charcode.hpp"
#include "character.hpp"
#include "string_utils.hpp"
#include "shim/optional.hpp"

namespace Sass {
    // Default constructor

  // Import some namespaces
  using namespace Charcode;
  using namespace Character;

  Emitter::Emitter(const OutputOptions& outopt)
  : wbuf(outopt.mapopt.mode != SASS_SRCMAP_NONE),
    outopt(outopt),
    indentation(0),
    scheduled_space(0),
    scheduled_linefeed(0),
    parentheses_opened(false),
    force_next_mapping(false),
    scheduled_delimiter(false),
    scheduled_mapping(nullptr),
    in_custom_property(false),
    in_declaration(true),
    separators(),
    in_space_array(false),
    in_comma_array(false)
  { }

  // return buffer as string
  sass::string Emitter::get_buffer(bool trim)
  {
    finalize(false); // flush stuff
    sass::string text(std::move(wbuf.buffer));
    // This potentially makes mappings invalid!?
    if (trim) StringUtils::makeTrimmed(text); // fails 4 without?
    return text; // Should use RVO
  }

  SassOutputStyle Emitter::output_style(void) const
  {
    return outopt.output_style;
  }

  // PROXY METHODS FOR SOURCE MAPS
  
  void Emitter::add_source_index(size_t idx)
  {
    if (wbuf.srcmap) wbuf.srcmap->addSourceIndex(idx); }

  // void Emitter::set_filename(const sass::string& str)
  // { if (wbuf.srcmap) wbuf.srcmap->setFile(str); }

  void Emitter::schedule_mapping(const AstNode* node)
  {
    scheduled_mapping = node;
  }

  void Emitter::add_open_mapping(const AstNode* node, bool optional)
  {
    flush_schedules();
    if (wbuf.srcmap) {
      wbuf.srcmap->addOpenMapping(node,
        optional && !force_next_mapping);
      force_next_mapping = false;
      if (scheduled_mapping) {
        wbuf.srcmap->addOpenMapping(scheduled_mapping, false);
        scheduled_mapping = nullptr;
      }
    }
  }

  void Emitter::add_close_mapping(const AstNode* node, bool optional)
  {
    flush_schedules();
    if (wbuf.srcmap) {
      wbuf.srcmap->addCloseMapping(node,
        optional && !force_next_mapping);
      force_next_mapping = false;
      if (scheduled_mapping) {
        wbuf.srcmap->addCloseMapping(scheduled_mapping, false);
        scheduled_mapping = nullptr;
      }
    }
  }

  void Emitter::move_next_mapping(int start, int end)
  {
    flush_schedules();
    if (wbuf.srcmap) {
      wbuf.srcmap->moveNextMapping(start, end);
    }
  }

  // MAIN BUFFER MANIPULATION

  // add outstanding delimiter
  void Emitter::finalize(bool final)
  {
    scheduled_space = false;
    if (output_style() == SASS_STYLE_COMPRESSED)
      if (final) scheduled_delimiter = false;
    if (scheduled_linefeed)
      scheduled_linefeed = 1;
    flush_schedules();
  }

  // flush scheduled space/linefeed
  void Emitter::flush_schedules(void)
  {
    // check the schedule
    if (scheduled_linefeed) {
      sass::string linefeeds = "";

      for (size_t i = 0; i < scheduled_linefeed; i++)
        linefeeds += outopt.linefeed;
      scheduled_space = false;
      scheduled_linefeed = 0;
      if (scheduled_delimiter) {
        scheduled_delimiter = false;
        write_char(';');
      }
      write_string(linefeeds);

    }
    else if (scheduled_space) {
      sass::string spaces(scheduled_space, ' ');
      scheduled_space = false;
      if (scheduled_delimiter) {
        scheduled_delimiter = false;
        write_char(';');
      }
      write_string(spaces);
    }
    else if (scheduled_delimiter) {
      scheduled_delimiter = false;
      write_char(';');
    }
  }

  // prepend some text or token to the buffer
  void Emitter::prepend_output(const OutputBuffer& output)
  {
    if (wbuf.srcmap) wbuf.srcmap->prepend(output);
    wbuf.buffer = output.buffer + wbuf.buffer;
  }

  // prepend some text or token to the buffer
  void Emitter::prepend_string(const sass::string& text)
  {
    // do not adjust mappings for utf8 bom
    // seems they are not counted in any UA
    if (text.compare("\xEF\xBB\xBF") != 0) {
      if (wbuf.srcmap) wbuf.srcmap->prepend(Offset(text));
    }
    wbuf.buffer = text + wbuf.buffer;
  }

  char Emitter::last_char()
  {
    return wbuf.buffer.back();
  }

  // append a single char to the buffer
  void Emitter::append_char(uint8_t chr)
  {
    // write space/lf
    flush_schedules();
    parentheses_opened = false;
      // add to buffer
    wbuf.buffer.push_back((unsigned char) chr);
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(chr));
  }

  // append a single char to the buffer
  void Emitter::write_char(uint8_t chr)
  {
    parentheses_opened = false;
    // add to buffer
    wbuf.buffer.push_back((unsigned char)chr);
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(chr));
  }

  /*
  static sass::string cleanComment(const sass::string& text)
  {
    size_t minIndent = 10e30,
      curIndent = 0, i = 0;
    while (i < text.size()) {
      if (Character::isNewline(text[i++])) {
        while (Character::isNewline(text[i++])) {}
        curIndent = i; // store current position
        while (Character::isSpaceOrTab(text[i++])) {}
        if (!Character::isNewline(text[i - 1])) {
          // std::cerr << "line has indent " << i - curIndent << "\n";
          minIndent = std::min(minIndent, i - curIndent);
        }
      }
    }

    bool firstLine = true;
    i = 0; sass::string cleaned;

    while (i < text.size()) {
      if (Character::isNewline(text[i++])) {
        firstLine = false;
        cleaned += text[i - 1];
        while (Character::isNewline(text[i++])) {
          cleaned += text[i - 1];
        }
        curIndent = 0;
        while (Character::isSpaceOrTab(text[i++])) {
          curIndent++;
          if (curIndent > minIndent) {
            cleaned += ' ';
          }
        }
        if (!Character::isNewline(text[i - 1])) {
          cleaned += text[i - 1];
        }
        else {
          cleaned += text[i - 1];
        }
      }
      else if (firstLine) {
        cleaned += text[i - 1];
      }
      else {
        curIndent = 0;
        if (Character::isSpaceOrTab(text[i - 1])) {
          while (Character::isSpaceOrTab(text[i++])) {
            curIndent++;
            if (curIndent > minIndent) {
              cleaned += ' ';
            }
            // cleaned += '=';
          }
        }
        cleaned += text[i - 1];
      }
    }

    std::cerr << " min indent is " << minIndent << "\n";
    // ignore first line
    // remove indentation from comment as much as possible
    std::cerr << "=============\n";
    std::cerr << text << "\n";
    std::cerr << "=============\n";
    std::cerr << cleaned << "\n";
    std::cerr << "=============\n";
    return text;
  }*/

  // Append a (multi-line) comment, re-indented like dart-sass does
  // (`_minimumIndentation` and `_writeWithIndent` in serialize.dart):
  // the first line is written as-is. From all following lines we remove
  // the smallest indentation of any non-blank line (but never more than
  // the column the comment started at in the source) and prefix them
  // with the current output indentation. Blank lines are kept as such.
  void Emitter::indent_comment(const sass::string& text, size_t column)
  {
    const size_t size = text.size();
    // Find the end of the first line
    size_t i = text.find('\n');
    if (i == sass::string::npos) {
      append_string(text);
      return;
    }
    // Get minimum indentation of all non-blank following lines
    size_t minIndent = sass::string::npos;
    for (size_t pos = i + 1; pos < size;) {
      size_t start = pos;
      while (pos < size && Character::isSpaceOrTab(text[pos])) ++pos;
      if (pos == size) break;
      if (text[pos] == '\n') { ++pos; continue; }
      minIndent = std::min(minIndent, pos - start);
      pos = text.find('\n', pos);
      if (pos == sass::string::npos) break;
      ++pos;
    }
    // Only whitespace after the first line (can't happen for
    // a comment, which ends with `*/`): write it unchanged.
    if (minIndent == sass::string::npos) {
      append_string(text);
      return;
    }
    minIndent = std::min(minIndent, column);
    // Write the first line as-is
    append_string(text.substr(0, i));
    ++i; // skip over the line-feed
    while (true) {
      // Skip whitespace, counting blank lines
      size_t lineStart = i, newlines = 1;
      while (true) {
        if (i == size) {
          // Only whitespace left, but keep the fact
          append_string(" ");
          return;
        }
        uint8_t chr = text[i++];
        if (Character::isSpaceOrTab(chr)) continue;
        if (chr == '\n') { lineStart = i; ++newlines; continue; }
        break;
      }
      append_string(outopt.linefeed, newlines);
      append_indentation();
      size_t lineEnd = text.find('\n', i);
      if (lineEnd == sass::string::npos) {
        append_string(text.substr(lineStart + minIndent));
        return;
      }
      append_string(text.substr(lineStart + minIndent,
        lineEnd - lineStart - minIndent));
      i = lineEnd + 1;
    }
  }

  // append some text or token to the buffer
  void Emitter::append_string(const sass::string& text)
  {
    // write space/lf
    flush_schedules();
    if (text.empty() == false)
      parentheses_opened = false;
    // add to buffer
    wbuf.buffer.append(text);
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(text));
  }

  // append some text or token to the buffer
  void Emitter::write_string(const sass::string& text)
  {
    if (text.empty() == false)
      parentheses_opened = false;
    // add to buffer
    wbuf.buffer.append(text);
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(text));
  }

  // append some text or token to the buffer
  void Emitter::append_string(const sass::string& text, size_t repeat)
  {
    // write space/lf
    flush_schedules();
    if (text.empty() == false)
      parentheses_opened = false;
    // add to buffer
    // wbuf.buffer.append(text, repeat);
    for (size_t i = 0; i < repeat; i += 1) {
      wbuf.buffer.append(text);
    }
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(text) * (uint32_t)repeat);
  }

  // append some text or token to the buffer
  void Emitter::append_string(const char* text, size_t repeat)
  {
    // write space/lf
    flush_schedules();
    // add to buffer
    // wbuf.buffer.append(text, repeat);
    for (size_t i = 0; i < repeat; i += 1) {
      wbuf.buffer.append(text);
    }
    // account for data in source-maps
    if (wbuf.srcmap) wbuf.srcmap->append(Offset(text) * (uint32_t)repeat);
  }

  // append some text or token to the buffer
  // this adds source-mappings for node start and end
  void Emitter::append_token(const sass::string& text, const AstNode* node)
  {
    add_open_mapping(node, true);
    write_string(text);
    add_close_mapping(node, true);
  }

  // HELPER METHODS

  void Emitter::append_indentation()
  {
    if (output_style() == SASS_STYLE_COMPRESSED) return;
    if (output_style() == SASS_STYLE_COMPACT) return;
    if (in_declaration && in_comma_array) return;
    if (scheduled_linefeed && indentation)
      scheduled_linefeed = 1;
    append_string(outopt.indent, indentation); // 1.5% (realloc)
  }

  void Emitter::append_delimiter()
  {
    scheduled_delimiter = true;
    if (output_style() == SASS_STYLE_COMPACT) {
      if (indentation == 0) {
        append_mandatory_linefeed();
      } else {
        append_mandatory_space();
      }
    } else if (output_style() != SASS_STYLE_COMPRESSED) {
      append_optional_linefeed();
    }
  }

  void Emitter::append_comma_separator()
  {
    scheduled_space = false;
    append_char(',');
    append_optional_space();
  }

  void Emitter::append_colon_separator()
  {
    scheduled_space = false;
    append_char(':');
    if (!in_custom_property) {
      append_optional_space();
    }
  }

  void Emitter::append_mandatory_space()
  {
    if (buffer().empty()) {
      scheduled_space = true;
    }
    else if (!isWhitespace(buffer().back())) {
      scheduled_space = true;
    }
  }

  void Emitter::append_optional_space()
  {
    if ((output_style() != SASS_STYLE_COMPRESSED) && wbuf.buffer.size()) {
      // char back = buffer().back();
      // unsigned char foo = (unsigned char)back;
      // std::cerr << "BACK IS " << back << "\n";
      if (scheduled_delimiter || !isWhitespace(buffer().back())) {
        //if (buffer().back() != $lparen) // breaks escape sequences
        {
          if (!parentheses_opened)
            append_mandatory_space();
        }
      }
    }
  }

  void Emitter::append_special_linefeed()
  {
    if (output_style() == SASS_STYLE_COMPACT) {
      append_mandatory_linefeed();
      for (size_t p = 0; p < indentation; p++)
        append_string(outopt.indent);
    }
  }

  void Emitter::append_optional_linefeed()
  {
    if (in_declaration && in_comma_array) return;
    if (output_style() == SASS_STYLE_COMPACT) {
      append_mandatory_space();
    } else {
      append_mandatory_linefeed();
    }
  }

  void Emitter::append_mandatory_linefeed()
  {
    if (output_style() != SASS_STYLE_COMPRESSED) {
      scheduled_linefeed = 1;
      scheduled_space = false;
    }
  }

  void Emitter::append_scope_opener(AstNode* node)
  {
    scheduled_linefeed = 0;
    append_optional_space();
    flush_schedules();
    if (node) add_open_mapping(node, true);
    write_char('{');
    append_optional_linefeed();
    ++ indentation;
  }
  void Emitter::append_scope_closer(AstNode* node)
  {
    -- indentation;
    scheduled_linefeed = 0;
    if (last_char() == '{') {
      scheduled_space = false;
      scheduled_linefeed = 0;
    }
    else {
      if (output_style() == SASS_STYLE_COMPRESSED)
        scheduled_delimiter = false;
      if (output_style() == SASS_STYLE_EXPANDED) {
        append_optional_linefeed();
        append_indentation();
      }
      else {
        append_optional_space();
      }
    }

    append_char('}');
    if (node) {
      move_next_mapping(-1, -1);
      add_close_mapping(node, true);
    }
    append_optional_linefeed();
    if (indentation != 0) return;
    if (output_style() != SASS_STYLE_COMPRESSED)
      scheduled_linefeed = 2;
  }

}
