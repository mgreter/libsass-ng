#include "ast_fwd_decl.hpp"

#include "ast_css.hpp"
#include "ast_nodes.hpp"
#include "ast_values.hpp"
#include "ast_statements.hpp"
#include "ast_supports.hpp"
#include "ast_selectors.hpp"

namespace Sass {
  CssMediaQueryVector::CssMediaQueryVector() {}
  CssMediaQueryVector::CssMediaQueryVector(const sass::vector<CssMediaQueryObj>& queries)
        : sass::vector<CssMediaQueryObj>(queries)
    {
    }

  CssMediaQueryVector::CssMediaQueryVector(sass::vector<CssMediaQueryObj>&& queries)
    : sass::vector<CssMediaQueryObj>(std::move(queries))
  {
  }

    bool CssMediaQueryVector::operator==(const sass::vector<CssMediaQueryObj>& rhs) const
    {
      // Abort early if sizes do not match
      if (size() != rhs.size()) return false;
      // Abort early if hashes exist and don't match
      // if (hash_ && rhs.hash_ && hash_ != rhs.hash_) return false;
      // Otherwise test each node for object equality in order
      return std::equal(begin(), end(), rhs.begin(), ObjEqualityFn<CssMediaQueryObj>);
    }



}
