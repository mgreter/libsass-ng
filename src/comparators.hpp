/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_SETTINGS_COMPARATORS_HPP
#define SASS_SETTINGS_COMPARATORS_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

namespace Sass {

  /////////////////////////////////////////////////////////////////////////#
  /////////////////////////////////////////////////////////////////////////#

  template <typename T> class Equatable
  {
  public:
    virtual bool operator==(const T& rhs) const = 0;
    bool operator!=(const T& rhs) const { return !(*this == rhs); }
  };

  template <typename T> class Comparable
  {
  public:
    virtual bool operator<(const T& rhs) const = 0;
    bool operator>(const T& rhs) const { return rhs < *this; }
    bool operator<=(const T& rhs) const { return !(rhs < *this); }
    bool operator>=(const T& rhs) const { return !(*this < rhs); }
  };

  /////////////////////////////////////////////////////////////////////////#
  /////////////////////////////////////////////////////////////////////////#

  class HashCodeProvider {

    // Implement hash functionality
    virtual size_t hash() const = 0;

  };

  // Interface for base class
  // Provides cached hashing
  // Requires `hash()` method
  class Hashable : public HashCodeProvider
  {

  public:

    // Hash is only calculated once and afterwards the value
    // must not be mutated, which is the case with how sass
    // works, although we must be a bit careful not to alter
    // any value that has already been added to a set or map.
    // Must create a copy if you need to alter such an object.
    // Selectors are mostly used as keys in @extend rules.
    mutable size_t hash_ = 0;

    // Returns zero if not yet hashed
    // Useful to speed up comparisons
    size_t hashed() const { return hash_; }

  };

  /////////////////////////////////////////////////////////////////////////#
  // Implement compare and hashing operations for raw pointers
  /////////////////////////////////////////////////////////////////////////#

  static inline size_t splitmix64(size_t x) {
    // http://xorshift.di.unimi.it/splitmix64.c
    x += 0x9e3779b97f4a7c15;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
    x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
    return x ^ (x >> 31);
  }

  static inline size_t hasher(void* ptr) {
    return splitmix64(std::hash<void*>{}(ptr) + getHashSeed());
  }

  template <class T>
  inline size_t PtrHashFn(const T* ptr) {
    return hasher((void*)ptr);
  }

  struct PtrHash {
    template <class T>
    inline size_t operator() (const T* ptr) const {
      return hasher(ptr);
    }
  };

  template <class T>
  inline bool PtrEqualityFn(const T* lhs, const T* rhs) {
    return lhs == rhs; // compare raw pointers
  }

  struct PtrEquality {
    template <class T>
    bool operator() (const T* lhs, const T* rhs) const {
      return PtrEqualityFn<T>(lhs, rhs);
    }
  };

  /////////////////////////////////////////////////////////////////////////#
  // Implement compare and hashing operations for AST Nodes
  /////////////////////////////////////////////////////////////////////////#

  template <class T>
  // Hash the raw pointer instead of object
  inline size_t ObjPtrHashFn(const T& obj) {
    return hasher(obj.ptr());
  }

  struct ObjPtrHash {
    template <class T>
    // Hash the raw pointer instead of object
    size_t operator() (const T& obj) const {
      return ObjPtrHashFn(obj);
    }
  };

  template <class T>
  // Hash the object and its content
  inline size_t ObjHashFn(const T& obj) {
    return obj ? obj->hash() : 0; // or type id?
  }

  struct ObjHash {
    template <class T>
    // Hash the object and its content
    inline size_t operator() (const T& obj) const {
      return ObjHashFn(obj);
    }
  };

  template <class T>
  // Hash the object behind pointer
  inline size_t PtrObjHashFn(const T* obj) {
    return obj ? obj->hash() : 0;
  }

  struct PtrObjHash {
    template <class T>
    // Hash the object behind pointer
    inline size_t operator() (const T* obj) const {
      return PtrObjHashFn(obj);
    }
  };

  template <class T>
  // Compare raw pointers to the object
  inline bool ObjPtrEqualityFn(const T& lhs, const T& rhs) {
    return PtrEqualityFn(lhs.ptr(), rhs.ptr());
  }

  struct ObjPtrEquality {
    template <class T>
    // Compare raw pointers to the object
    inline bool operator() (const T& lhs, const T& rhs) const {
      return ObjPtrEqualityFn<T>(lhs, rhs);
    }
  };

  template <class T>
  // Compare the objects behind the pointers
  inline bool PtrObjEqualityFn(const T* lhs, const T* rhs) {
    if (lhs == nullptr) return rhs == nullptr;
    else if (rhs == nullptr) return false;
    else return lhs == rhs || *lhs == *rhs;
  }

  struct PtrObjEquality {
    template <class T>
    // Compare the objects behind the pointers
    inline bool operator() (const T* lhs, const T* rhs) const {
      return PtrObjEqualityFn<T>(lhs, rhs);
    }
  };

  template <class T>
  // Compare the objects and its contents
  inline bool ObjEqualityFn(const T& lhs, const T& rhs) {
    return PtrObjEqualityFn(lhs.ptr(), rhs.ptr());
  }

  struct ObjEquality {
    template <class T>
    // Compare the objects and its contents
    inline bool operator() (const T& lhs, const T& rhs) const {
      return ObjEqualityFn<T>(lhs, rhs);
    }
  };

  /////////////////////////////////////////////////////////////////////////#
  // Implement ordering operations for AST Nodes
  /////////////////////////////////////////////////////////////////////////#

  template <class T>
  // Hash the raw pointer instead of object
  inline bool ObjPtrLessThanFn(const T& a, const T& b) {
    return a.ptr() < b.ptr();
  }

  struct ObjPtrLessThan {
    template <class T>
    // Hash the raw pointer instead of object
    bool operator() (const T& a, const T& b) const {
      return ObjPtrLessThanFn(a, b);
    }
  };

  template <class T>
  // Compare the objects behind pointers
  inline bool PtrObjLessThanFn(const T* lhs, const T* rhs) {
    if (lhs == rhs) return false;
    if (lhs == nullptr) return true;
    else if (rhs == nullptr) return false;
    else return *lhs < *rhs;
  }

  struct PtrObjLessThan {
    template <class T>
    // Compare the objects behind pointers
    inline bool operator() (const T* lhs, const T* rhs) const {
      return PtrObjLessThanFn<T>(lhs, rhs);
    }
  };

  template <class T>
  // Compare the objects and its content
  inline bool ObjLessThanFn(const T& lhs, const T& rhs) {
    return PtrObjLessThanFn(lhs.ptr(), rhs.ptr());
  };

  struct ObjLessThan {
    template <class T>
    // Compare the objects and its content
    inline bool operator() (const T& lhs, const T& rhs) const {
      return ObjLessThanFn<T>(lhs, rhs);
    }
  };

  /////////////////////////////////////////////////////////////////////////#
  // Implement hasing and compare for env keys
  // Actually anything with norm and hash function
  /////////////////////////////////////////////////////////////////////////#

  struct EnvKeyHash {
    template <class T>
    inline size_t operator()(const T& str) const
    {
      return str.hash();
    }
  };

  struct EnvKeyEquality {
    template <class T>
    inline bool operator() (const T& lhs, const T& rhs) const {
      return lhs.norm() == rhs.norm();
    }
  };

  struct EnvKeyLessThan {
    template <class T>
    inline bool operator() (const T& lhs, const T& rhs) const {
      return lhs.norm() < rhs.norm();
    }
  };

  /////////////////////////////////////////////////////////////////////////
  // Implement hasing and compare for regular strings (case sensitive)
  /////////////////////////////////////////////////////////////////////////

  struct StringHash {
    template <class T>
    inline size_t operator()(const T& str) const
    {
      return hash_string(str);
    }
  };

  struct StringEquality {
    template <class T>
    inline bool operator() (const T& lhs, const T& rhs) const {
      return lhs == rhs;
    }
  };

  struct StringLessThan {
    template <class T>
    inline bool operator() (const T& lhs, const T& rhs) const {
      return lhs < rhs;
    }
  };

  /////////////////////////////////////////////////////////////////////////
  // Implement hasing and compare for regular strings (case insensitive)
  /////////////////////////////////////////////////////////////////////////

  struct StringHashInsensitive {
    template <class T>
    inline size_t operator()(const T& str) const
    {
      return hash_string_insensitive(str);
    }
  };

  struct StringEqualityInsensitive {
    template <class T> // No predicate on std::equal in C++11
    inline bool operator() (const T& lhs, const T& rhs) const {
      if (lhs.size() != rhs.size()) return false;
      for (size_t i = 0; i < lhs.size(); i += 1) {
        // Optimize to uint32_t to compare 4 bytes at once?
        if ((lhs[i] | 0x20) != (rhs[i] | 0x20)) return false;
      }
      return true;
    }
  };

  struct StringLessThanInsensitive {
    template <class T> // No predicate on std::less in C++11
    inline bool operator() (const T& lhs, const T& rhs) const {
      if (lhs.size() != rhs.size()) return false;
      for (size_t i = 0; i < lhs.size(); i += 1) {
        // Optimize to uint32_t to compare 4 bytes at once?
        if ((lhs[i] | 0x20) < (rhs[i] | 0x20)) return true;
        if ((lhs[i] | 0x20) > (rhs[i] | 0x20)) return false;
      }
      return false; // equals
    }
  };

  // struct StringLessThanInsensitive {
  //   template <class T>
  //   inline bool operator() (const T& lhs, const T& rhs) const {
  //     return std::less(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(),
  //       [](char lhs, char rhs) { return (lhs | 0x20) < (rhs | 0x20); });
  //   }
  // };

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

}

#endif
