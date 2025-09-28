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

    virtual ~Cssize() {}

  public:

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    Cssize(
      OutputOptions& opt) :
      Inspect(opt)
    {}

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    void visitFunction(Function*) override;
    void visitNumber(Number*) override;
    void visitList(List*) override;
    void visitMap(Map*) override;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
