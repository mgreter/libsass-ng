/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
// Small helper to create VLQ encoding for source maps
/*****************************************************************************/
#ifndef SASS_BASE64VLQ_H
#define SASS_BASE64VLQ_H

#include <string>

namespace Sass {

  class Base64VLQ {

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    const char* CHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const int VLQ_BASE_SHIFT = 5;
    const int VLQ_BASE = 1 << VLQ_BASE_SHIFT;
    const int VLQ_BASE_MASK = VLQ_BASE - 1;
    const int VLQ_CONTINUATION_BIT = VLQ_BASE;

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  public:

    void encode(sass::string& buffer, const int number) const
    {
      int vlq = (number < 0) ? ((-number) << 1) + 1 : (number << 1) + 0;

      do {
        int digit = vlq & VLQ_BASE_MASK;
        vlq >>= VLQ_BASE_SHIFT;
        if (vlq > 0) {
          digit |= VLQ_CONTINUATION_BIT;
        }
        buffer += base64_encode(digit);
      } while (vlq > 0);

    }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  private:

    inline char base64_encode(const int number) const
    {
      int index = number;
      if (index < 0) index = 0;
      if (index > 63) index = 63;
      return CHARACTERS[index];
    }

    // int to_vlq_signed(const int number) const
    // {
    //   return (number < 0) ? ((-number) << 1) + 1 : (number << 1) + 0;
    // }

    /////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

  };

}

#endif
