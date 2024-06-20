/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Base class (reference counted) to hold loaded content (e.g. from files)
/*****************************************************************************/
#ifndef SASS_SOURCE_HPP
#define SASS_SOURCE_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include "ast_def_macros.hpp"

namespace Sass {

  class SourceSpan;

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // SourceData is the base class to hold loaded content.
  // Need to keep all the content around for error reporting
  // ToDo: could use string views everywhere instead of copies
  // E.g. every string is basically: source with start to end
  class SourceData : public RefCounted
  {
  protected:

    // Allow access for friend class to our privates
    // Used to shim source when a part is interpolated
    friend class SourceItpl;

    // Returns the number of lines. On the first call
    // it will calculate the linefeed lookup table.
    virtual size_t countLines() = 0;

  public:

    // Constructor
    SourceData();

    // The source id is uniquely assigned
    virtual size_t getSrcIdx() const = 0;

    // Set source id (must be unqiue)
    virtual void setSrcIdx(size_t idx) = 0;

    // Return path as it was given for import
    virtual const char* getImpPath() const = 0;

    // Return path as it was resolved by importer
    virtual const char* getAbsPath() const = 0;

    // Return only the filename part
    // ToDo: utilize base_name function
    const char* getFileName() const
    {
      const char* path = getImpPath();
      const char* lastSlash = path;
      while (path && *path != 0) {
        if (*path == '/' || *path == '\\') {
          lastSlash = path + 1;
        }
        path += 1;
      }
      return lastSlash;
    }

    // Returns the requested line. Will take interpolations into
    // account to show more accurate debug messages. Calling this
    // can be rather expensive, so only use it for debugging.
    virtual sass::string getLine(size_t line) = 0;

    // Get raw iterator for raw source
    virtual const char* content() const = 0;
    virtual const char* srcmaps() const = 0;

    // Get raw iterator for raw source
    const char* contentStart() const { return content(); };
    const char* srcmapsStart() const { return srcmaps(); };
    const char* contentEnd() const { return content() + contentSize(); };
    const char* srcmapsEnd() const { return srcmaps() + srcmapsSize(); };

    // Return raw size in bytes
    virtual size_t contentSize() const = 0;
    virtual size_t srcmapsSize() const = 0;

    // Returns adjusted source span regarding interpolation.
    virtual SourceSpan adjustSourceSpan(SourceSpan& pstate) const;

    // Declare wrapper to C-API structure
    CAPI_WRAPPER(SourceData, SassSource);
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
