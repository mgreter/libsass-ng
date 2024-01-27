/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/

#include "cssize.hpp"
#include "inspect.hpp"
#include "exceptions.hpp"
#include "dart_helpers.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  // Needs to be in sync with SassOp enum
  uint8_t SassOpPresedence[15] = {
    1, 2, 3, 3, 4, 4, 4, 4,
    5, 5, 6, 6, 6, 9, 255
  };

  // Needs to be in sync with SassOp enum
  const char* SassOpName[16] = {
    "or", "and", "eq", "neq", "gt", "gte", "lt", "lte",
    "plus", "minus", "times", "div", "mod", "seq", "ieseq", "invalid"
  };

  // Needs to be in sync with SassOp enum
  const char* SassOpOperator[16] = {
    "||", "&&", "==", "!=", ">", ">=", "<", "<=",
    "+", "-", "*", "/", "%", "=", "=", "invalid"
  };

  // Precedence is used to decide order
  // in ExpressionParser::addOperator.
  uint8_t sass_op_to_precedence(enum SassOperator op)
  {
    return SassOpPresedence[op];
  }

  // Get readable name for error messages
  const char* sass_op_to_name(enum SassOperator op)
  {
    return SassOpName[op];
  }

  // Get readable name for operator (e.g. `==`)
  const char* sass_op_separator(enum SassOperator op)
  {
    return SassOpOperator[op];
  }

  // Get readable name for list operator (e.g. `,`, `/` or ` `)
  const char* sass_list_separator(enum SassSeparator op)
  {
    switch (op) {
    case SASS_COMMA: return ", ";
    case SASS_SPACE: return " ";
    case SASS_DIV: return " / ";
    default: return "";
    }
  }


  //////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////

}
