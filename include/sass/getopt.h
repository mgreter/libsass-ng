/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_C_GETOPT_H
#define SASS_C_GETOPT_H

#include <sass/base.h>
#include <sass/compiler.h>

#ifdef __cplusplus
extern "C" {
#endif

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Just forward declare
  struct SassGetOptEnum;

  // Struct must be known in order to access it in the callback
  // We don't expect this to change much once it is proven good
  // We also don't want to support all cases under the sun!
  union SassOptionValue {
    int integer;
    bool boolean;
    const char* string;
    enum SassOutputStyle style;
    enum SassImportSyntax syntax;
    enum SassSrcMapMode mode;
  };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Errors (invalid options or arguments, but also unexpected failures,
  // e.g. out of memory) are reported on the compiler; check them with
  // `sass_compiler_get_status` and `sass_compiler_get_error`. Once an
  // error is set, further parsing is skipped.
  //
  // The strings passed to `sass_getopt_register_option` and
  // `sass_getopt_register_argument` (names, descriptions and enums)
  // are not copied and must stay valid until the parser is deleted
  // (string literals are fine). Register all options and arguments
  // before calling `sass_getopt_parse`.

  // Create a new option parser to help with parsing config from users
  // Optimized to act like GNU GetOpt Long to consume main `argv` items
  // But can also be used to parse any other list of config strings
  // Returns NULL on failure (the error is set on the compiler).
  ADDAPI struct SassGetOpt* ADDCALL sass_make_getopt(struct SassCompiler* compiler);

  // Delete and finalize the option parser. Make sure to call
  // this before you want to start the actual compilation phase.
  // Finalizing checks for missing or extra arguments and reports
  // them on the compiler. The parser is always freed. Accepts NULL.
  ADDAPI void ADDCALL sass_delete_getopt(struct SassGetOpt* getopt);

  // Utility function to tell LibSass to register its default options
  // It is recommended to always call this function right after creation
  ADDAPI void ADDCALL sass_getopt_populate_options(struct SassGetOpt* getopt);

  // Utility function to tell LibSass to register its default arguments
  ADDAPI void ADDCALL sass_getopt_populate_arguments(struct SassGetOpt* getopt);

  // Parse one config string at a time, as you would normally do with main `argv`.
  ADDAPI void ADDCALL sass_getopt_parse(struct SassGetOpt* getopt, const char* arg);

  // Return string with the full help message describing all commands
  // This is formatted in a similar fashion as GNU tools using getopt
  // Must be freed via `sass_free_c_string`; NULL on failure.
  ADDAPI char* ADDCALL sass_getopt_get_help(struct SassGetOpt* getopt);
  
  // Register additional option that can be parsed
  ADDAPI void ADDCALL sass_getopt_register_option(struct SassGetOpt* getopt,
    // Short and long parameter names
    const char short_name,
    const char* long_name,
    // Description used in help/usage message
    const char* description,
    // Whether to act like a boolean
    const bool boolean,
    // Name of required argument
    const char* argument,
    // Make argument optional. An optional argument is given either via
    // `--name=value`, or as the next item if that is not an option (for
    // enums only if it is a valid enum, otherwise it's a regular argument).
    const bool optional,
    // Value passed to the callback if the optional argument is omitted
    // (an enum name for enum options). If NULL, a zeroed value is passed
    // (enum id 0 or a NULL string). Ignored for required arguments.
    const char* default_value,
    // Arguments must be one of this enum
    const struct SassGetOptEnum* enums,
    // Callback function, where we pass back the given option value
    void (*cb) (struct SassGetOpt* getopt, union SassOptionValue value));

  // Register additional argument that can be parsed
  ADDAPI void ADDCALL sass_getopt_register_argument(struct SassGetOpt* getopt,
    // Whether this argument is optional
    bool optional,
    // Name used in messages
    const char* name,
    // Callback function, where we pass back the given argument value
    void (*cb) (struct SassGetOpt* getopt, const char* value));

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} // __cplusplus defined.
#endif

#endif
