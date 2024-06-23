/*****************************************************************************/
/* Part of LibSass, released under the MIT license (See LICENSE.txt).        */
/*****************************************************************************/
#ifndef SASS_FLAT_MAP_HPP
#define SASS_FLAT_MAP_HPP

// sass.hpp must go before all system headers
// to get the __EXTENSIONS__ fix on Solaris.
#include "capi_sass.hpp"

#include <cmath>

namespace Sass {

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////
  // A flat map is an optimized map when you know to expect minimal
  // amount of items in the map. In this case we rather should use a
  // vector, which is what this implementation does. It implements the
  // interface of a regular map (partially) by utilizing a vector.
  // My own tests indicate that hash maps start to win when 10 or more
  // items are present. We can assume that e.g. functions typically don't
  // have that many named arguments, probably most often not more than
  // three or four. The interface matches std::unordered_map mostly.
  // Having a compatible interface means we can exchange the
  // implementations easily for performance benchmarks.
  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

  template <typename Key, typename Value,
    std::size_t N = 0,
    class KeyEqual = std::equal_to<Key>,
    class Allocator = std::allocator<std::pair<Key, Value>>
  >
  class FlatMap
  {
  private:

    // Define base types
    using PAIR = std::pair<Key, Value>;
    using TYPE = std::vector<PAIR, Allocator>;

    // Main key/value pair vector
    TYPE items;

  public:

    // Some convenient iterator type aliases
    // using key_type = typename Key;
    // using value_type = typename Value;
    // using key_equal = typename KeyEqual;
    // using allocator_type = typename Allocator;
    using iterator = typename TYPE::iterator;
    using const_iterator = typename TYPE::const_iterator;
    using reverse_iterator = typename TYPE::reverse_iterator;
    using const_reverse_iterator = typename TYPE::const_reverse_iterator;

    // allocator_type get_allocator() const { return Allocator; }

    FlatMap() {
      items.reserve(N);
    }

    // Returns number of key/value pairs
    size_t size() const
    {
      return items.size();
    }
    // EO size

    // Returns if map is empty
    bool empty() const
    {
      return items.empty();
    }
    // EO empty

    // Erases all items
    void clear()
    {
      items.clear();
    }
    // EO clear

    // Returns the number of elements matching specific key 
    size_t count(const Key& key) const
    {
      const_iterator cur = items.begin();
      const_iterator end = items.end();
      while (cur != end) {
        // Compare the normalized keys
        if (cur->first == key) {
          return 1;
        }
        cur++;
      }
      return 0;
    }
    // EO count

    // Removes item with specific key from the map
    void erase(const Key& key)
    {
      iterator cur = items.begin();
      iterator end = items.end();
      while (cur != end) {
        // Compare the normalized keys
        if (cur->first == key) {
          // Error on gcc 4.8
          items.erase(cur);
          return;
        }
        cur++;
      }
    }
    // EO erase(key)

    // Removes iterator's item from the map
    void erase(const iterator& it)
    {
      items.erase(it);
    }
    // EO erase(it)

    // Reserves space for at least the specified number of elements.
    void reserve(size_t size)
    {
      items.reserve(size);
    }
    // EO reserve

    // Finds element with specific key
    iterator find(const Key& key)
    {
      iterator cur = items.begin();
      iterator end = items.end();
      while (cur != end) {
        if (cur->first == key) {
          return cur;
        }
        cur++;
      }
      return end;
    }
    // EO find

    // Finds element with specific key 
    const_iterator find(const Key& key) const
    {
      const_iterator cur = items.begin();
      const_iterator end = items.end();
      while (cur != end) {
        // Compare the normalized keys
        if (cur->first == key) {
          return cur;
        }
        cur++;
      }
      return end;
    }
    // EO const find

    // Access or insert specified element
    Value& operator[](const Key& key)
    {
      iterator cur = items.begin();
      iterator end = items.end();
      while (cur != end) {
        // Compare the normalized keys
        if (cur->first == key) {
          return cur->second;
        }
        cur++;
      }
      // Append empty object
      items.emplace_back(
        std::make_pair(
          key, Value{}));
      // Returns newly added value
      return items.back().second;
    }
    // EO array[]

    // Insert passed key/value pair
    // ToDo: should return pair<it,bool>
    bool insert(const PAIR& kv)
    {
      if (count(kv.first) == 0) {
        // Append the pair
        items.emplace_back(kv);
        // Returns success
        return true;
      }
      // Nothing inserted
      return false;
    }
    // EO insert

    // Insert passed key/value pair
    // ToDo: should return pair<it,bool>
    bool insert(const PAIR&& kv)
    {
      if (count(kv.first) == 0) {
        // Append the pair
        items.emplace_back(std::move(kv));
        // Returns success
        return true;
      }
      // Nothing inserted
      return false;
    }
    // EO insert

    // Insert passed key/value pair
    // ToDo: should return pair<it,bool>
    bool insert(const Key& k, const Value& v)
    {
      if (count(k) == 0) {
        // Append the pair
        items.emplace_back({ k,v });
        // Returns success
        return true;
      }
      // Nothing inserted
      return false;
    }
    // EO insert

    // Access element at specific key
    // Throws if key is not known in map
    const Value& at(const Key& key) const
    {
      const_iterator cur = items.begin();
      const_iterator end = items.end();
      while (cur != end) {
        if (cur->first == key) {
          return cur->second;
        }
        cur++;
      }
      // Throw an error if the item does not exist
      throw std::runtime_error(
        "Key does not exist");
    }
    // EO at

    // Equality comparison operator
    bool operator==(FlatMap<Key, Value, N, KeyEqual, Allocator> rhs) const {
      return items == rhs.items;
    }

    // Returns an iterator for the begin or end position
    const_iterator begin() const { return items.begin(); }
    const_iterator end() const { return items.end(); }
    // iterator begin() { return items.begin(); }
    // iterator end() { return items.end(); }

  };
  // EO Class FlatMap

  /////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////

};

#endif
