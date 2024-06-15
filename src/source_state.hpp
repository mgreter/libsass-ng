/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_SOURCE_STATE_HPP
#define SASS_SOURCE_STATE_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_fwd_decl.hpp"
#include "source_offset.hpp"
#include "source.hpp"

namespace Sass
{
	
	// Stores a reference (shared ptr) to the source code
	// and one offset position (line and column information).
	class SourceState
	{
  protected:

    // The source code reference
    SourceDataObj source;

  public:

		// The position within the source
		Offset position;

    // Regular value constructor
    SourceState(
      SourceData* source = {},
      Offset position = Offset());

		// Return the attach source id
		size_t getSrcIdx() const;

    // Return the requested import path
    const char* getImpPath() const;

    // Return the resolved absolute path
    const char* getAbsPath() const;

    // Return the resolved filename
    const char* getFileName() const;

    // Return the attached source
		SourceData* getSource() const;

    // Return the attached source
    const char* getContent() const;

		// Return line as human readable
		// Starting from one instead of zero
		uint32_t getLine() const
		{
			return position.line + 1;
		}

		// Return column as human readable
		// Starting from one instead of zero
		uint32_t getColumn() const
		{
			return position.column + 1;
		}

    // Return line as raw value
    uint32_t getRawLine() const
    {
      return position.line;
    }

    // Return column as raw value
    uint32_t getRawColumn() const
    {
      return position.column;
    }

  };

}

#endif
