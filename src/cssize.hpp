/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Subclass of inspect, specializing in outputting valid css
/*****************************************************************************/
#ifndef SASS_CSSIZE_HPP
#define SASS_CSSIZE_HPP

#include "inspect.hpp"

namespace Sass {

  class Cssize : public Inspect {
  public:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Cssize(
      OutputOptions& opt) :
      Inspect(opt)
    {}

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    virtual void visitFunction(Function*) override;
    virtual void visitNumber(Number*) override;
    virtual void visitList(List*) override;
    virtual void visitMap(Map*) override;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
