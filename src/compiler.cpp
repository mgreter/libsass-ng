/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#include "compiler.hpp"

#include "json.hpp"
#include "eval.hpp"
#include "output.hpp"
#include "sources.hpp"
#include "stylesheet.hpp"
#include "capi_import.hpp"
#include "capi_importer.hpp"
#include "ast_imports.hpp"
#include "parser_scss.hpp"
#include "parser_sass.hpp"
#include "parser_css.hpp"
#include "exceptions.hpp"
#include "remove_placeholders.hpp"

// Functions to register
#include "fn_maps.hpp"
#include "fn_meta.hpp"
#include "fn_math.hpp"
#include "fn_lists.hpp"
#include "fn_texts.hpp"
#include "fn_colors.hpp"
#include "fn_selectors.hpp"

#include "b64/encode.hpp"
#include "plugins.hpp"
#include "file.hpp"

#include "debugger.hpp"

#include <cstring>
#include <csignal>
#ifdef _MSC_VER
#include <excpt.h>
#endif

namespace Sass {

  Compiler::~Compiler()
  {
    // Release rendered source-map and footer
    sass_free_c_string(srcmap);
    sass_free_c_string(footer);
    sources19.clear();
    sheets21.clear();
    for (auto& mod : modules64) {
      delete mod.second;
    }
    #ifdef DEBUG_MSVC_CRT_MEM
    _CrtMemState state{};
    _CrtMemState delta{};
    _CrtMemCheckpoint(&state);
    if (_CrtMemDifference(&delta, &memState, &state))
      _CrtMemDumpStatistics(&delta);
    #endif
  }

  Compiler::Compiler() :
    varRoot(*this),
    state(SASS_COMPILER_CREATED),
    output_path("stream://stdout"),
    entry_point(),
    footer(nullptr),
    srcmap(nullptr),
    error()
  {
    #ifdef DEBUG_MSVC_CRT_MEM
    _CrtMemCheckpoint(&memState);
    #endif
  }

  // Get path of compilation entry point
  // Returns the resolved/absolute path
  sass::string Compiler::getInputPath() const
  {
    // Simply return absolute path of entry point
    if (entry_point && entry_point->getAbsPath()) {
      return entry_point->getAbsPath();
    }
    // Fall back to stdin
    return "stream://stdin";
  }
  // EO getInputPath

  // Get the output path for this compilation
  // Can be explicit or deducted from input path
  sass::string Compiler::getOutputPath() const
  {
    // Specific output path was provided
    if (!output_path.empty()) {
      return output_path;
    }
    // Otherwise deduct it from input path
    sass::string path(getInputPath());
    // Check if input is coming from stdin
    if (path == "stream://stdin") {
      return "stream://stdout";
    }
    // Otherwise remove the file extension
    size_t dotpos = path.find_last_of('.');
    if (dotpos != sass::string::npos) {
      path.erase(dotpos);
    }
    // Use css extension
    path += ".css";
    return path;
  }
  // EO getOutputPath

  // Check if we should write output file
  bool Compiler::hasOutputFile() const
  {
    return !output_path.empty() &&
      output_path != "stream://stdout";
  }
  // EO hasOutputFile

  // Check if we should write srcmap file
  bool Compiler::hasSrcMapFile() const
  {
    return mapopt.mode == SASS_SRCMAP_CREATE ||
      mapopt.mode == SASS_SRCMAP_EMBED_LINK;
  }
  // EO hasSrcMapFile

  void Compiler::parse()
  {
    // Only allow phase right after parsing
    if (state == SASS_COMPILER_CREATED) {
      // Check for valid entry point
      if (entry_point == nullptr) {
        throw Exception::ParserException(*this,
          "No entry-point to compile given");
      }
      // Do initial loading
      entry_point->loadIfNeeded(*this, PWD);
      // Now parse the entry point stylesheet
      sheet = parseRoot(entry_point);
      // Update the compiler state
      state = SASS_COMPILER_PARSED;
    }
  }

  // parse root block from includes (Move to compiler)
  CssRootObj Compiler::compileRoot(bool plainCss)
  {
    StylesheetObj root = sheet;
    if (root == nullptr) return {};

    #ifdef DEBUG_SHARED_PTR
    // Enable reference tracking
    RefCounted::taint = true;
    #endif

    // abort if there is no data
    if (included_sources.size() == 0) return {};
    // abort on invalid root
    if (root.isNull()) return {};

    Eval eval(*this, *this, plainCss);

    // Evaluation may run on another thread than parsing
    varRoot.setStackLimit();

    // debug_ast(root);
    CssRootObj compiled;
    {
      ImportStackFrame iframe(eval.compiler, root->import);
      eval.visitStylesheet(root);
      compiled = eval._combineCss(root);
    }

    // clean up by removing empty placeholders
    // ToDo: maybe we can do this somewhere else?
    RemovePlaceholders remove_placeholders;
    remove_placeholders.visitCssRoot(compiled); // 3%

    #ifdef DEBUG_SHARED_PTR
    // Enable reference tracking
    RefCounted::taint = false;
    #endif

    // return processed tree
    return compiled;
  }
  // EO compileRoot

  void Compiler::compile()
  {
    // Only act right after parsing
    if (state == SASS_COMPILER_PARSED) {
      // Compile the parsed ast-tree
      compiled22 = compileRoot(false);
      // Update the compiler state
      state = SASS_COMPILER_COMPILED;
    }
  }

  OutputBuffer Compiler::renderCss()
  {
    #ifdef DEBUG_SHARED_PTR
    // Enable reference tracking
    RefCounted::taint = true;
    #endif

    // Create the emitter object
    Output emitter(*this);
    emitter.reserve(262144); // 256K
    emitter.in_declaration = false;
    // Start the render process
    if (compiled22 != nullptr) {
      emitter.visitCssRoot(compiled22);
    }
    // Finish emitter stream
    emitter.finalize();
    // Update the compiler state
    state = SASS_COMPILER_RENDERED;

    #ifdef DEBUG_SHARED_PTR
    // Enable reference tracking
    RefCounted::taint = false;
    #endif

    // Move buffer from stream
    return emitter.getBuffer();
  }
  // EO renderCss

  // Case 1) output to stdout, source map must be fully inline
  // Case 2) output to path, source map output is deducted from it
  char* Compiler::renderSrcMapLink(const SourceMap& source_map)
  {
    // Source map json must already be there
    if (srcmap == nullptr) return nullptr;
    // Check if we output to stdout (any link would be useless)
    if (output_path.empty() || output_path == "stream://stdout") {
      if (mapopt.path.empty() || mapopt.path == "stream://stdout") {
        // Instead always embed the source-map on stdout
        return renderEmbeddedSrcMap(source_map);
      }
    }
    // Create resulting footer and return a copy
    return sass_copy_string("\n/*# sourceMappingURL=" +
      File::abs2rel(mapopt.path, mapopt.origin, PWD) + " */");

  }
  // EO renderSrcMapLink

  // Memory returned by this function must be freed by caller via `sass_free_c_string`
  char* Compiler::renderEmbeddedSrcMap(const SourceMap& source_map) const
  {
    // Source map json must already be there
    if (srcmap == nullptr) return nullptr;
    // Encode json to base64
    sass::istream is(srcmap);
    sass::ostream buffer;
    buffer << "\n/*# sourceMappingURL=";
    buffer << "data:application/json;base64,";
    base64::encoder E;
    E.encode(is, buffer);
    buffer << " */";
    return sass_copy_string(buffer.str());
  }
  // EO renderEmbeddedSrcMap

  // Update `warnings` from the log, plus a final note if similar
  // warnings were suppressed. Called after every compiler phase, so
  // the note always reflects the total count (it does not alter the
  // suppression state, which spans all phases).
  void Compiler::updateWarnings()
  {
    Logger& logger(*this);
    warnings = logger.logstrm.str();
    if (suppressed > 0) {
      sass::sstream message;
      message << getTerm(Terminal::yellow);
      if (suppressed == 1) {
        message << "Additionally, one similar warning was suppressed!\n";
      }
      else {
        message << "Additionally, " << suppressed
          << " similar warnings were suppressed!\n";
      }
      message << getTerm(Terminal::reset);
      warnings += message.str();
    }
  }
  // EO updateWarnings

  // Memory returned by this function must be freed by caller via `sass_free_c_string`
  char* Compiler::renderSrcMapJson(const SourceMap& source_map)
  {
    // Create the emitter object
    // Sass::OutputBuffer buffer;

    /**********************************************/
    // Create main object to render json
    /**********************************************/
    JsonNode* json_srcmap = json_mkobject();

    /**********************************************/
    // Create the source-map version information
    /**********************************************/
    json_append_member(json_srcmap, "version", json_mknumber(3));

    /**********************************************/
    // Create file reference to whom our mappings apply
    /**********************************************/
    sass::string origin(mapopt.origin);
    origin = File::abs2rel(origin, PWD, PWD);
    JsonNode* json_file_name = json_mkstring(origin.c_str());
    json_append_member(json_srcmap, "file", json_file_name);

    /**********************************************/
    // pass-through source_map_root option
    /**********************************************/
    if (!mapopt.root.empty()) {
      json_append_member(json_srcmap, "sourceRoot",
        json_mkstring(mapopt.root.c_str()));
    }

    /**********************************************/
    // Create the included sources array
    /**********************************************/
    JsonNode* json_sources = json_mkarray();
    for (size_t i = 0; i < included_sources.size(); ++i) {
      const SourceData* source(included_sources[i]);
      sass::string path(source->getAbsPath());
      path = File::rel2abs(path, ".", PWD);
      // Optionally convert to file urls
      if (mapopt.file_urls) {
        if (path[0] == '/') {
          // ends up with three slashes
          path = "file://" + path;
        }
        else {
          // needs additional slash
          path = "file:///" + path;
        }
        // Append item to json array
        json_append_element(json_sources,
          json_mkstring(path.c_str()));
      }
      else {
        path = File::abs2rel(path, ".", PWD);
        // Append item to json array
        json_append_element(json_sources,
          json_mkstring(path.c_str()));
      }
    }
    json_append_member(json_srcmap, "sources", json_sources);

    // add a relative link to the source map output file
    // srcmap_links88.emplace_back(abs2rel(abs_path, file88, CWD));

    /**********************************************/
    // Check if we have any includes to render
    /**********************************************/
    if (mapopt.embed_contents) {
      JsonNode* json_contents = json_mkarray();
      for (size_t i = 0; i < included_sources.size(); ++i) {
        const SourceData* source = included_sources[i];
        JsonNode* json_content = json_mkstring(source->content());
        json_append_element(json_contents, json_content);
      }
      json_append_member(json_srcmap, "sourcesContent", json_contents);
    }

    /**********************************************/
    // So far we have no implementation for names
    /**********************************************/
    json_append_member(json_srcmap, "names", json_mkarray());

    /**********************************************/
    // Create source remapping lookup table
    // For source-maps we need to output sources in
    // consecutive manner, but we might have used various
    // different stylesheets from the prolonged context
    /**********************************************/
    std::unordered_map<size_t, size_t> idxremap;
    for (auto& source : included_sources) {
      idxremap.insert(std::make_pair(
        source->getSrcIdx(),
        idxremap.size()));
    }

    /**********************************************/
    // Finally render the actual source mappings
    // Remap context srcidx to consecutive ordering
    /**********************************************/
    sass::string mappings(source_map.render(idxremap));
    JsonNode* json_mappings = json_mkstring(mappings.c_str());
    json_append_member(json_srcmap, "mappings", json_mappings);

    /**********************************************/
    // Render the json and return result
    // Memory must be freed by consumer!
    /**********************************************/
    char* data = json_stringify(json_srcmap, "\t");
    json_delete(json_srcmap);
    return data;

  }
  // EO renderSrcMapJson

  /////////////////////////////////////////////////////////////////////////
  // @param imp_path The relative or custom path for be imported
  // @param pstate SourceSpan where import occurred (parent context)
  // @param rule The backing ImportRule that is added to the document
  // @param importers Array of custom importers/headers to go through
  // @param singleton Whether to use all importers or only first successful
  /////////////////////////////////////////////////////////////////////////
  // Release C-API import objects when leaving scope (also on exceptions,
  // e.g. parse errors in imported content or detected import loops).
  struct ImportListReleaser {
    struct SassImportList* list;
    ~ImportListReleaser() { sass_delete_import_list(list); }
  };
  struct ImportReleaser {
    struct SassImport* import;
    ~ImportReleaser() { sass_delete_import(import); }
  };

  bool Compiler::callCustomLoader(const sass::string& imp_path, const SourceSpan& pstate,
    ImportRule* rule, const sass::vector<struct SassImporter*>& importers, bool singleton)
  {
    // unique counter
    size_t count = 0;
    // need one correct import
    bool has_import = false;

    const char* ctx_path = pstate.getAbsPath();

    // Process custom importers and headers.
    // They must be presorted by priorities.
    for (struct SassImporter* importer : importers) {

      // Get the external importer function
      SassImporterLambda fn = importer->lambda;

      // Call the external function, then check what it returned
      struct SassImportList* imports = fn(imp_path.c_str(), importer, this->wrap());
      // External provider want to handle this
      if (imports != nullptr) {
        // Released when leaving this block (incl. exceptions)
        ImportListReleaser importsGuard{ imports };

        // Get the list of possible imports
        // A list with zero items does nothing
        while (sass_import_list_size(imports) > 0) {
          // Increment counter
          ++count;
          // Consume the first item from the list
          struct SassImport* entry = sass_import_list_shift(imports);
          // Released at the end of this iteration (incl. exceptions)
          ImportReleaser entryGuard{ entry };
          Import& import = Import::unwrap(entry);
          // Create a unique path to use as key
          sass::string uniq_path = imp_path;
          // Append counter to the path
          // Note: only for headers!
          if (!singleton && count) {
            sass::sstream path_strm;
            path_strm << uniq_path << ":" << count;
            uniq_path = path_strm.str();
          }
          // create the importer struct
          ImportRequest importer(uniq_path, ctx_path, false);
          // Check if importer returned and error state.
          // Should only happen if importer knows that
          // this import must only be handled by itself.
          if (sass_import_get_error_message(entry)) {
            BackTraces& traces = *this;
            traces.push_back(BackTrace(pstate));
            Exception::CustomImportError err(traces,
              sass_import_get_error_message(entry));
            throw err;
          }
          // Source must be set, even if the actual data is not loaded yet
          else if (SourceDataObj source = import.source) {
            const char* rel_path = import.getImpPath();
            const char* abs_path = import.getAbsPath();
            if (import.isLoaded()) {
              // Resolved abs_path should be set by custom importer
              // Use the created uniq_path as fall-back (enforce?)
              // Store fall-backs on the source, as the import stack,
              // registration and error reporting all expect paths.
              if (abs_path == nullptr || rel_path == nullptr) {
                if (auto src = dynamic_cast<SourceWithPath*>(source.ptr())) {
                  src->setMissingPaths(imp_path, uniq_path);
                  rel_path = import.getImpPath();
                  abs_path = import.getAbsPath();
                }
              }
              sass::string path_key(abs_path ? abs_path : uniq_path);
              // Import is ready to be served
              if (import.syntax == SASS_IMPORT_AUTO)
                import.syntax = SASS_IMPORT_SCSS;
              ImportStackFrame iframe(*this, &import);
              Stylesheet* sheet = registerImport(&import);
              // Add a dynamic import to the import rule
              IncludeImport* inc = SASS_MEMORY_NEW(IncludeImport,
                pstate, ctx_path, path_key, &import);
              // Same as `Eval::resolveIncludeImport` (module and root)
              inc->module32(sheet);
              inc->root47(sheet);
              rule->append(inc);
            }
            // Only a path was returned
            // Try to load it like normal
            else if (abs_path || rel_path) {
              // Create a copy for possible error reporting
              sass::string path(abs_path ? abs_path : rel_path);
              // Pass it on as if it was a regular import
              ImportRequest request(path, ctx_path, false);
              // Search for valid imports (e.g. partials) on the file-system
              // Returns multiple valid results for ambiguous import path
              sass::vector<ResolvedImport> resolved(findIncludes(request, true));
              // Error if no file to import was found
              if (resolved.empty()) {
                BackTraces& traces = *this;
                traces.push_back(BackTrace(pstate));
                Exception::CustomImportNotFound err(traces, path);
                throw err;
              }
              // Error if multiple files to import were found
              else if (resolved.size() > 1) {
                BackTraces& traces = *this;
                traces.push_back(BackTrace(pstate));
                Exception::CustomImportAmbigous err(traces, path);
                throw err;
              }
              // We made sure exactly one entry was found, load its content
              if (ImportObj loaded = loadImport(resolved[0])) {
                ImportStackFrame iframe(*this, loaded);
                Stylesheet* sheet = registerImport(loaded);
                const sass::string& url(resolved[0].abs_path);
                IncludeImport* inc = SASS_MEMORY_NEW(IncludeImport,
                  pstate, ctx_path, url, &import);
                // Same as `Eval::resolveIncludeImport` (module and root)
                inc->module32(sheet);
                inc->root47(sheet);
                rule->append(inc);
              }
              // Import failed to load
              else {
                BackTraces& traces = *this;
                traces.push_back(BackTrace(pstate));
                Exception::CustomImportLoadError err(traces, path);
                throw err;
              }

            }
          }
        }

        // Set success flag
        has_import = true;
        // Break out of loop?
        if (singleton) break;
      }
    }
    // Return result
    return has_import;
  }
  // EO callCustomLoader

  void Compiler::applyCustomHeaders(StatementVector& statements, const SourceSpan& pstate)
  {
    // create a custom import to resolve headers
    ImportRuleObj rule = SASS_MEMORY_NEW(ImportRule, pstate);
    // dispatch headers which will add custom functions
    // custom headers are added to the import instance
    if (callCustomHeaders("sass://header", pstate, rule)) {
      statements.push_back(rule.ptr());
    }
  }


  /////////////////////////////////////////////////////////////////////////
  // Interface for built in functions
  /////////////////////////////////////////////////////////////////////////

  // Register built-in mixin and associate mixin callback
  uint32_t Compiler::createBuiltInMixin(const EnvKey& name,
    const sass::string& signature, SassFnSig cb, bool acceptsContent)
  {
    EnvRoot root(*this);
    SourceDataObj source = SASS_MEMORY_NEW(SourceString,
      "sass://signature", "(" + signature + ")");
    CallableSignature* args = CallableSignature::parse(*this, source);
    BuiltInCallable* callable = SASS_MEMORY_NEW(BuiltInCallable, name, args, cb);
    callable->acceptsContent(acceptsContent);
    auto& mixins(varRoot.intMixin);
    uint32_t offset((uint32_t)mixins.size());
    varRoot.intMixin.push_back(callable);
    varRoot.privateMixOffset = offset + 1;
    return offset;
  }

  // Register built-in variable with the associated value
  uint32_t Compiler::createBuiltInVariable(const EnvKey& name, Value* value)
  {
    EnvRoot root(*this);
    auto& variables(varRoot.intVariables);
    uint32_t offset((uint32_t)variables.size());
    varRoot.intVariables.push_back(value);
    varRoot.privateVarOffset = offset + 1;
    return offset;
  }
  // EO createBuiltInVariable

  // Register built-in function with only one parameter list.
  uint32_t Compiler::createBuiltInFunction(const EnvKey& name,
    const sass::string& signature, SassFnSig cb)
  {
    EnvRoot root(*this);
    SourceDataObj source = SASS_MEMORY_NEW(SourceString,
      "sass://signature", "(" + signature + ")");
    auto args = CallableSignature::parse(*this, source);
    BuiltInCallable* callable = SASS_MEMORY_NEW(BuiltInCallable, name, args, cb);
    auto& functions(varRoot.intFunction);
    uint32_t offset((uint32_t)functions.size());
    varRoot.intFunction.push_back(callable);
    varRoot.privateFnOffset = offset + 1;
    return offset;
  }
  // EO createBuiltInFunction

  // Register internal function with only one parameter list.
  // Same as `createBuiltInFunction`, but marks it internal
  uint32_t Compiler::createInternalFunction(const EnvKey& name,
    const sass::string& signature, SassFnSig cb)
  {
    EnvRoot root(*this);
    SourceDataObj source = SASS_MEMORY_NEW(SourceString,
      "sass://signature", "(" + signature + ")");
    auto args = CallableSignature::parse(*this, source);
    BuiltInCallable* callable = SASS_MEMORY_NEW(BuiltInCallable, name, args, cb, true);
    auto& functions(varRoot.intFunction);
    uint32_t offset((uint32_t)functions.size());
    varRoot.intFunction.push_back(callable);
    varRoot.privateFnOffset = offset + 1;
    return offset;
  }
  // EO createInternalFunction


  // Register built-in functions that can take different
  // functional arguments (best suited will be chosen).
  uint32_t Compiler::createBuiltInOverloadFns(const EnvKey& name,
    const sass::vector<std::pair<const sass::string, SassFnSig>>& overloads)
  {
    SassFnPairs pairs;
    for (const auto& overload : overloads) {
      EnvRoot root(*this);
      SourceDataObj source = SASS_MEMORY_NEW(SourceString,
        "sass://signature", "(" + overload.first + ")");
      auto args = CallableSignature::parse(*this, source);
      pairs.emplace_back(std::make_pair(args, overload.second));
    }
    BuiltInCallables* callable = SASS_MEMORY_NEW(BuiltInCallables, name, pairs);
    auto& functions(varRoot.intFunction);
    uint32_t offset((uint32_t)functions.size());
    varRoot.intFunction.push_back(callable);
    varRoot.privateFnOffset = offset + 1;
    return offset;
  }
  // EO registerBuiltInOverloadFns

  // Register built-in function with only one parameter list.
  uint32_t Compiler::registerBuiltInFunction(const EnvKey& name,
    const sass::string& signature, SassFnSig cb)
  {
    auto fn = createBuiltInFunction(name, signature, cb);
    return varRoot.idxs->fnIdxs[name] = fn; // Expose
  }
  // EO registerBuiltInFunction

    // Register built-in function with only one parameter list.
  uint32_t Compiler::registerInternalFunction(const EnvKey& name,
    const sass::string& signature, SassFnSig cb)
  {
    auto fn = createInternalFunction(name, signature, cb);
    return varRoot.idxs->fnIdxs[name] = fn; // Expose
  }
  // EO registerBuiltInFunction

  void Compiler::exposeFunction(const EnvKey& name, uint32_t idx)
  {
    varRoot.idxs->fnIdxs[name] = idx;
  }

  // Register built-in functions that can take different
  // functional arguments (best suited will be chosen).
  uint32_t Compiler::registerBuiltInOverloadFns(const EnvKey& name,
    const sass::vector<std::pair<const sass::string, SassFnSig>>& overloads)
  {
    auto fn = createBuiltInOverloadFns(name, overloads);
    return varRoot.idxs->fnIdxs[name] = fn; // Expose
  }
  // EO registerBuiltInOverloadFns

  /////////////////////////////////////////////////////////////////////////
  // Interface for external custom functions
  /////////////////////////////////////////////////////////////////////////

  // Register an external custom sass function on the global scope.
  // Main entry point for custom functions passed through the C-API.
  void Compiler::registerCustomFunction(struct SassFunction* function)
  {
    EnvRoot root(*this);
    SourceDataObj source = SASS_MEMORY_NEW(SourceString,
      "sass://signature", sass::string(function->signature));
    ScssParser parser(*this, source.ptr());
    ExternalCallable* callable =
      parser.parseExternalCallable();
    callable->lambda(function->lambda);
    callable->cookie(function->cookie);
    auto& functions(varRoot.intFunction);
    uint32_t offset((uint32_t)functions.size());
    varRoot.intFunction.push_back(callable);
    varRoot.idxs->fnIdxs[callable->name()] = offset;
    varRoot.privateFnOffset = offset + 1;
  }
  // EO registerCustomFunction

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Invoke parser according to import format
  StylesheetObj Compiler::parseSource(ImportObj import)
  {
    Stylesheet* root = nullptr;
    if (import->syntax == SASS_IMPORT_CSS)
    {
      CssParser parser(*this, import->source);
      root = parser.parseRoot();
    }
    else if (import->syntax == SASS_IMPORT_SASS)
    {
      SassParser parser(*this, import->source);
      root = parser.parseRoot();
    }
    else {
      ScssParser parser(*this, import->source);
      root = parser.parseRoot();
    }
    if (root) root->import = import;
    return root;
  }
  // EO parseSource

  sass::string Compiler::sheetCacheKey(const sass::string& absPath, bool isImport) const
  {
    if (!isImport) return absPath;
    // A character that can't be part of a path
    sass::string key(absPath + "\x01import");
    // Find the scope the import is nested in (skip imports)
    const EnvRefs* parent = envstack.empty() ? nullptr : envstack.back();
    while (parent && parent->isImport) parent = parent->pscope;
    // Root imports are shared, but a nested import resolves its variables
    // against its parent scope (and caches those references in the AST).
    // Therefore each nesting scope needs its own parsed stylesheet.
    if (parent && !parent->isInternal53) {
      sass::sstream strm;
      strm << "\x01" << static_cast<const void*>(parent);
      key += strm.str();
    }
    return key;
  }

  // Parse the import (updates syntax flag if AUTO was set)
  // Results will be stored at `sheets[source->getAbsPath()]`
  Stylesheet* Compiler::registerImport(ImportObj import, bool isImport)
  {

    SassImportSyntax& format(import->syntax);
    const SourceDataObj& source(import->source);
    const sass::string& abs_path(source->getAbsPath());
    sass::string key(sheetCacheKey(abs_path, isImport));

    // ToDo: We don't take format into account
    auto cached = sheets21.find(key);
    if (cached != sheets21.end()) {
      return cached->second;
    }

    // Register the source only once (it may be parsed twice,
    // as an import and as a module, see sheetCacheKey)
    size_t srcIdx = source->getSrcIdx();
    if (srcIdx >= included_sources.size() ||
      included_sources[srcIdx] != source)
    {
      // Assign unique index to the source
      source->setSrcIdx(included_sources.size());
      // Add source to global include array
      included_sources.emplace_back(source);
    }

    // Auto detect input file format
    if (format == SASS_IMPORT_AUTO) {
      using namespace StringUtils;
      if (endsWithIgnoreCase(abs_path, ".css", 4)) {
        format = SASS_IMPORT_CSS;
      }
      else if (endsWithIgnoreCase(abs_path, ".sass", 5)) {
        format = SASS_IMPORT_SASS;
      }
      else if (endsWithIgnoreCase(abs_path, ".scss", 5)) {
        format = SASS_IMPORT_SCSS;
      }
      else if (abs_path != "stream://stdin") {
        throw Exception::UnknownImport(callStack);
      }
    }

    // Invoke correct parser according to format
    // Also links the stylesheet back to the import
    StylesheetObj stylesheet = parseSource(import);

    // The parsers return a stylesheet or throw. Check anyway, since all
    // callers use the result and a null entry must never get cached.
    if (stylesheet.isNull()) {
      // `sass::string` may use our custom allocator (SASS_CUSTOM_ALLOCATOR),
      // so convert it; `std::runtime_error` only accepts a `std::string`.
      throw std::runtime_error(std::string("Parser returned no stylesheet for ") + abs_path.c_str());
    }

    // for (auto& qwe : stylesheet->modimps) {
    //   Module* mod = qwe.second.second;
    //   mod->
    // }

    sheet54 = stylesheet;

    // Put the parsed stylesheet into the map
    sheets21.insert({ key, stylesheet });

    stylesheet->extender52 = SASS_MEMORY_NEW(ExtensionStore, ExtensionStore::NORMAL, *this);

    stylesheet->determineTransitivelyContainsExtensions();

    // Return pointer, it is already managed
    // Don't call detach, as it could leak then
    return stylesheet.ptr();

  }
  // EO registerImport

  // Called once to register all built-in functions.
  // This will invoke parsing for parameter lists.
  void Compiler::loadBuiltInFunctions()
  {
    Functions::Meta::registerFunctions(*this);
    Functions::Math::registerFunctions(*this);
    Functions::Maps::registerFunctions(*this);
    Functions::Lists::registerFunctions(*this);
    Functions::Colors::registerFunctions(*this);
    Functions::Texts::registerFunctions(*this);
    Functions::Selectors::registerFunctions(*this);
    // Global built-in functions that are deprecated in favor of a module
    // function, like dart-sass (`withDeprecationWarning`). The global and
    // module function may share the same callable, the `global` flag at
    // call time tells them apart. Functions that may also be called as
    // plain CSS functions (invert, grayscale, saturate, alpha, opacity and
    // abs) warn on their own, only if not used as plain CSS functions.
    static const char* const deprecatedGlobals[][2] = {
      // color
      { "red", "color.red" }, { "green", "color.green" },
      { "blue", "color.blue" }, { "mix", "color.mix" },
      { "hue", "color.hue" }, { "saturation", "color.saturation" },
      { "lightness", "color.lightness" }, { "complement", "color.complement" },
      { "adjust-hue", "color.adjust" }, { "lighten", "color.adjust" },
      { "darken", "color.adjust" }, { "desaturate", "color.adjust" },
      { "opacify", "color.adjust" }, { "fade-in", "color.adjust" },
      { "transparentize", "color.adjust" }, { "fade-out", "color.adjust" },
      { "adjust-color", "color.adjust" }, { "scale-color", "color.scale" },
      { "change-color", "color.change" },
      // math
      { "ceil", "math.ceil" }, { "floor", "math.floor" },
      { "max", "math.max" }, { "min", "math.min" },
      { "percentage", "math.percentage" }, { "random", "math.random" },
      { "round", "math.round" }, { "unit", "math.unit" },
      { "comparable", "math.compatible" }, { "unitless", "math.is-unitless" },
      // list
      { "length", "list.length" }, { "nth", "list.nth" },
      { "set-nth", "list.set-nth" }, { "join", "list.join" },
      { "append", "list.append" }, { "zip", "list.zip" },
      { "index", "list.index" }, { "is-bracketed", "list.is-bracketed" },
      { "list-separator", "list.separator" },
      // map
      { "map-get", "map.get" }, { "map-merge", "map.merge" },
      { "map-remove", "map.remove" }, { "map-keys", "map.keys" },
      { "map-values", "map.values" }, { "map-has-key", "map.has-key" },
      // selector
      { "is-superselector", "selector.is-superselector" },
      { "simple-selectors", "selector.simple-selectors" },
      { "selector-parse", "selector.parse" },
      { "selector-nest", "selector.nest" },
      { "selector-append", "selector.append" },
      { "selector-extend", "selector.extend" },
      { "selector-replace", "selector.replace" },
      { "selector-unify", "selector.unify" },
      // string
      { "unquote", "string.unquote" }, { "quote", "string.quote" },
      { "to-upper-case", "string.to-upper-case" },
      { "to-lower-case", "string.to-lower-case" },
      { "unique-id", "string.unique-id" }, { "str-length", "string.length" },
      { "str-insert", "string.insert" }, { "str-index", "string.index" },
      { "str-slice", "string.slice" },
      // meta
      { "feature-exists", "meta.feature-exists" }, { "inspect", "meta.inspect" },
      { "type-of", "meta.type-of" }, { "keywords", "meta.keywords" },
      { "global-variable-exists", "meta.global-variable-exists" },
      { "variable-exists", "meta.variable-exists" },
      { "function-exists", "meta.function-exists" },
      { "mixin-exists", "meta.mixin-exists" },
      { "content-exists", "meta.content-exists" },
      { "module-variables", "meta.module-variables" },
      { "module-functions", "meta.module-functions" },
      { "module-mixins", "meta.module-mixins" },
      { "get-function", "meta.get-function" },
      { "get-mixin", "meta.get-mixin" }, { "call", "meta.call" },
    };
    for (const auto& entry : deprecatedGlobals) {
      EnvRef fidx = varRoot.findFnIdx(EnvKey(entry[0]), "");
      if (!fidx.isValid()) continue;
      Callable* callable = varRoot.getFunction(fidx);
      if (callable == nullptr) continue;
      if (BuiltInCallable* fn = callable->isaBuiltInCallable()) {
        fn->globalReplacement(entry[1]);
      }
      else if (BuiltInCallables* fns = callable->isaBuiltInCallables()) {
        fns->globalReplacement(entry[1]);
      }
    }
  }
  // EO loadBuiltInFunctions

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  Stylesheet* Compiler::parseRoot(ImportObj import)
  {

    // Insert ourself onto the sources cache
    sources19.insert({ import->getAbsPath(), import });

    // Register all built-in functions 
    loadBuiltInFunctions();

    #ifdef DEBUG_SHARED_PTR
    // Enable reference tracking
    RefCounted::taint = true;
    #endif

    // load and register import
    ImportStackFrame iframe(*this, import);
    Stylesheet* sheet = registerImport(import);

    #ifdef DEBUG_SHARED_PTR
    // Disable reference tracking
    RefCounted::taint = false;
    #endif

    // Return root node
    return sheet;

  }
  // EO parseRoot

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  ImportStackFrame::ImportStackFrame(Compiler& compiler, Import* import) :
    compiler(compiler)
  {

    auto& source(import->source);
    auto& stack(compiler.import_stack);
    const char* path = source->getAbsPath();
    // Check existing import stack for possible recursion before pushing.
    // If the constructor throws, the destructor (which pops) never runs,
    // so pushing first would leave a stale entry on the import stack.
    for (size_t i = stack.size(); i-- > 0;) {
      const char* parent = stack[i]->source->getAbsPath();
      if (parent && path && std::strcmp(parent, path) == 0) {
        // Use the messages of dart-sass, depending on the kind of the
        // current load (the stack trace shows all files of the loop)
        const BackTraces& calls(compiler.callStack);
        const BackTrace* current = calls.empty() ? nullptr : &calls.back();
        if (current && (current->name == Strings::useRule ||
          current->name == Strings::forwardRule)) {
          throw Exception::RuntimeException(compiler,
            "Module loop: this module is already being loaded.");
        }
        if (current && current->fn && current->name == "load-css") {
          throw Exception::RuntimeException(compiler, "Module loop: "
            + File::abs2rel(path, compiler.PWD, compiler.PWD)
            + " is already being loaded.");
        }
        throw Exception::RuntimeException(compiler,
          "This file is already being loaded.");
      }
    }
    stack.push_back(import);
  }

  ImportStackFrame::~ImportStackFrame()
  {
    compiler.import_stack.pop_back();
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Normalize a folder path to end with exactly one slash. On Windows a
  // trailing backslash (e.g. `C:\inc\`) otherwise resulted in `C:/inc//`
  // paths for every file found there. Roots like `/` or `C:/` are kept.
  static sass::string asFolderPath(sass::string path)
  {
    #ifdef _WIN32
      std::replace(path.begin(), path.end(), '\\', '/');
    #endif
    while (path.size() > 1 && path.back() == '/') path.pop_back();
    if (path.empty() || path.back() != '/') path += '/';
    return path;
  }

  // Add additional include paths, which can be path separated
  void Compiler::addIncludePaths(const sass::string& paths)
  {
    // Check if we have anything to do
    if (paths.empty()) return;
    // Load plugins from all paths
    sass::vector<sass::string> split =
      StringUtils::split(paths, PATH_SEP, true);
    for (sass::string& path : split) {
      if (path.empty()) continue;
      sass::string folder(asFolderPath(path));
      // Use the same path for the same folder (see findIncludes)
      if (File::is_absolute_path(folder)) {
        folder = asFolderPath(File::get_long_path(folder));
      }
      includePaths.emplace_back(folder);
    }
  }
  // EO addIncludePaths

  // Load plugins from paths, which can be path separated
  void Compiler::loadPlugins(const sass::string& paths)
  {
    Plugins plugins(*this);
    // Check if we have anything to do
    if (paths.empty()) return;
    // Load plugins from all paths
    sass::vector<sass::string> split =
      StringUtils::split(paths, PATH_SEP, true);
    for (sass::string& path : split) {
      if (path.empty()) continue;
      plugins.load_plugins(asFolderPath(path));
    }
    // Sort the merged arrays by callback priorities
    sort(cHeaders.begin(), cHeaders.end(), cmpImporterPrio);
    sort(cImporters.begin(), cImporters.end(), cmpImporterPrio);
  }
  // EO loadPlugins

  /////////////////////////////////////////////////////////////////////////
  // Helpers for `sass_prepare_context`
  // Obsolete when c_ctx and cpp_ctx are merged.
  /////////////////////////////////////////////////////////////////////////

  void Compiler::addCustomHeader(struct SassImporter* header)
  {
    if (header == nullptr) return;
    cHeaders.emplace_back(header);
    // need to sort the array afterwards (no big deal)
    sort(cHeaders.begin(), cHeaders.end(), cmpImporterPrio);
  }

  void Compiler::addCustomImporter(struct SassImporter* importer)
  {
    if (importer == nullptr) return;
    cImporters.emplace_back(importer);
    // need to sort the array afterwards (no big deal)
    sort(cImporters.begin(), cImporters.end(), cmpImporterPrio);
  }

  void Compiler::addCustomFunction(struct SassFunction* function)
  {
    if (function == nullptr) return;
    cFunctions.emplace_back(function);
  }

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Implementation for `sass_compiler_find_file`
  // Paths used by `findFile` and `findInclude`
  sass::vector<sass::string> Compiler::lookupPaths() const
  {
    // create the vector with paths to lookup
    sass::vector<sass::string> incpaths;
    incpaths.reserve(1 + includePaths.size());
    // First look relative to the last import (if any). The import
    // stack is empty if called outside of a compilation phase.
    if (!import_stack.empty()) {
      const Import& import = import_stack.back();
      if (const char* abs_path = import.getAbsPath()) {
        incpaths.emplace_back(File::dir_name(abs_path));
      }
    }
    // Then look in every include path
    incpaths.insert(incpaths.end(), includePaths.begin(), includePaths.end());
    return incpaths;
  }

  // Resolves like an `@import` (partials, extensions,
  // directory index and import-only files)
  sass::string Compiler::findInclude(const sass::string& path)
  {
    return File::find_include(path, PWD, lookupPaths(), true, fileExistsCache);
  }

  sass::string Compiler::findFile(const sass::string& path)
  {
    return File::find_file(path, PWD, lookupPaths(), fileExistsCache);
  }

  // Look for all possible filename variants (e.g. partials)
  // Returns all results (e.g. for ambiguous valid imports)
  // Returns a copy: the cache is an open addressing hash map, so any later
  // insert can move its entries. A returned reference was used after
  // nested imports had been resolved (custom importer returning a path),
  // which read freed memory and crashed.
  sass::vector<ResolvedImport> Compiler::findIncludes(const ImportRequest& import, bool forImport)
  {
    // Try to find cached result first (separate cache per mode)
    auto& cache(resolveCache[forImport ? 1 : 0]);
    auto it = cache.find(import);
    if (it != cache.end()) return it->second;

    // make sure we resolve against an absolute path
    sass::string base_path(File::rel2abs(import.base_path, ".", PWD));

    // first try to resolve the load path relative to the base path
    sass::vector<ResolvedImport> vec(File::resolve_includes(
      base_path, import.imp_path, PWD, forImport, fileExistsCache));

    // then search in every include path (but only if nothing found yet)
    for (size_t i = 0, S = includePaths.size(); vec.size() == 0 && i < S; ++i)
    {
      // Resolve relative include paths against the working directory, like
      // the base path, so the same file always gets the same absolute path
      // (otherwise e.g. `-I inc` loaded `inc/_m.scss` as another module).
      sass::string include_path(File::rel2abs(includePaths[i], ".", PWD));
      sass::vector<ResolvedImport> resolved(File::resolve_includes(
        include_path, import.imp_path, PWD, forImport, fileExistsCache));
      vec.insert(vec.end(), resolved.begin(), resolved.end());
    }
    // Store in the cache and return a copy
    cache[import] = vec;
    return vec;
  }

  // Load import from the file-system and create source object
  Import* Compiler::loadImport(const ResolvedImport& import)
  {
    // Try to find the item in the cache first
    auto cached = sources19.find(import.abs_path);
    if (cached != sources19.end()) return cached->second;
    // Try to read source and (ToDo) optional mappings
    if (ImportObj loaded = File::read_import(import, PWD)) {
      sources19.insert({ import.abs_path, loaded });
      return loaded.ptr();
    }
    // Throw error if read has failed
    throw Exception::IoError(*this,
      "File not found or unreadable",
        File::abs2rel(import.abs_path, PWD, PWD));
  }
  // EO loadImport

  // Update precision and epsilon etc.
  //void Compiler::setPrecision(int precision)
  //{
  //  // Logger::setPrecision(precision);
  //  OutputOptions::setPrecision(precision);
  //}

}

// ..\..\..\input.scss -I ..\..\..\..\sass-bench --no-term-color --no-term-unicode -t expanded
