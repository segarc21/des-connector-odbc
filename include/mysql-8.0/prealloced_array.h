/* Copyright (c) 2013, 2021, Oracle and/or its affiliates.

	Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>
	(see the next block comment below).
	
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License, version 2.0,
   as published by the Free Software Foundation.

   This program is also distributed with certain software (including
   but not limited to OpenSSL) that is licensed under separate terms,
   as designated in a particular file or component or in included license
   documentation.  The authors of MySQL hereby grant you an additional
   permission to link the program and your derivative works with the
   separately licensed software that they have included with MySQL.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

// ---------------------------------------------------------
// Modified in 2025 by Sergio Miguel Garcia Jimenez <segarc21@ucm.es>,
// hereinafter the DESODBC developer, in the context of the GPLv2 derivate
// work DESODBC, an ODBC Driver of the open-source DBMS Datalog Educational
// System (DES) (see https://des.sourceforge.io/)
// ---------------------------------------------------------

#ifndef PREALLOCED_ARRAY_INCLUDED
#define PREALLOCED_ARRAY_INCLUDED

/**
  @file include/prealloced_array.h
*/

#include <assert.h>
#include <stddef.h>
#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

#include "my_compiler.h"

#include "my_inttypes.h"
#include "my_sys.h"
#include "mysql/psi/psi_memory.h"
#include "mysql/service_mysql_alloc.h"

/*
DESODBC: renaming myodbc to desodbc
*/
namespace desodbc
{

/**
  A typesafe replacement for DYNAMIC_ARRAY. We do our own memory management,
  and pre-allocate space for a number of elements. The purpose is to
  pre-allocate enough elements to cover normal use cases, thus saving
  malloc()/free() overhead.
  If we run out of space, we use malloc to allocate more space.

  The interface is chosen to be similar to std::vector.
  We keep the std::vector property that storage is contiguous.

  We have fairly low overhead over the inline storage; typically 8 bytes
  (e.g. Prealloced_array<TABLE *, 4> needs 40 bytes on 64-bit platforms).

  @remark
  Unlike DYNAMIC_ARRAY, elements are properly copied
  (rather than memcpy()d) if the underlying array needs to be expanded.

  @remark
  Depending on Has_trivial_destructor, we destroy objects which are
  removed from the array (including when the array object itself is destroyed).

  @tparam Element_type The type of the elements of the container.
          Elements must be copyable or movable.
  @tparam Prealloc Number of elements to pre-allocate.
 */
template <typename Element_type, size_t Prealloc>
class Prealloced_array {
  /**
    Is Element_type trivially destructible? If it is, we don't destroy
    elements when they are removed from the array or when the array is
    destroyed.
  */
  static constexpr bool Has_trivial_destructor =
      std::is_trivially_destructible<Element_type>::value;

  bool using_inline_buffer() const { return m_inline_size >= 0; }

  /**
    Gets the buffer in use.
  */
  Element_type *buffer() {
    return using_inline_buffer() ? m_buff : m_ext.m_array_ptr;
  }
  const Element_type *buffer() const {
    return using_inline_buffer() ? m_buff : m_ext.m_array_ptr;
  }

  void set_size(size_t n) {
    if (using_inline_buffer()) {
      m_inline_size = n;
    } else {
      m_ext.m_alloced_size = n;
    }
  }
  void adjust_size(int delta) {
    if (using_inline_buffer()) {
      m_inline_size += delta;
    } else {
      m_ext.m_alloced_size += delta;
    }
  }

 public:
  /// Initial capacity of the array.
  static const size_t initial_capacity = Prealloc;

  /// Standard typedefs.
  typedef Element_type value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef Element_type *iterator;
  typedef const Element_type *const_iterator;

  explicit Prealloced_array(PSI_memory_key psi_key) : m_psi_key(psi_key) {
    static_assert(Prealloc != 0, "We do not want a zero-size array.");
  }

  /**
    Initializes (parts of) the array with default values.
    Using 'Prealloc' for initial_size makes this similar to a raw C array.
  */
  Prealloced_array(PSI_memory_key psi_key, size_t initial_size)
      : m_psi_key(psi_key) {
    static_assert(Prealloc != 0, "We do not want a zero-size array.");

    if (initial_size > Prealloc) {
      // We avoid using reserve() since it requires Element_type to be copyable.
      void *mem =
          my_malloc(m_psi_key, initial_size * element_size(), MYF(MY_WME));
      if (!mem) return;
      m_inline_size = -1;
      m_ext.m_alloced_size = initial_size;
      m_ext.m_array_ptr = static_cast<Element_type *>(mem);
      m_ext.m_alloced_capacity = initial_size;
    } else {
      m_inline_size = initial_size;
    }
    for (size_t ix = 0; ix < initial_size; ++ix) {
      Element_type *p = &buffer()[ix];
      ::new (p) Element_type();
    }
  }

  /**
    An object instance "owns" its array, so we do deep copy here.
   */
  Prealloced_array(const Prealloced_array &that) : m_psi_key(that.m_psi_key) {
    if (this->reserve(that.capacity())) return;
    for (const Element_type *p = that.begin(); p != that.end(); ++p)
      this->push_back(*p);
  }

  Prealloced_array(Prealloced_array &&that) : m_psi_key(that.m_psi_key) {
    *this = std::move(that);
  }

  /**
    Range constructor.

    Constructs a container with as many elements as the range [first,last),
    with each element constructed from its corresponding element in that range,
    in the same order.
  */
  Prealloced_array(PSI_memory_key psi_key, const_iterator first,
                   const_iterator last)
      : m_psi_key(psi_key) {
    if (this->reserve(last - first)) return;
    for (; first != last; ++first) push_back(*first);
  }

  Prealloced_array(std::initializer_list<Element_type> elems)
      : Prealloced_array(PSI_NOT_INSTRUMENTED, elems.begin(), elems.end()) {}

  /**
    Copies all the elements from 'that' into this container.
    Any objects in this container are destroyed first.
   */
  Prealloced_array &operator=(const Prealloced_array &that) {
    this->clear();
    if (this->reserve(that.capacity())) return *this;
    for (const Element_type *p = that.begin(); p != that.end(); ++p)
      this->push_back(*p);
    return *this;
  }

  Prealloced_array &operator=(Prealloced_array &&that) {
    this->clear();
    if (!that.using_inline_buffer()) {
      if (!using_inline_buffer()) my_free(m_ext.m_array_ptr);
      // The array is on the heap, so we can just grab it.
      m_ext.m_array_ptr = that.m_ext.m_array_ptr;
      m_inline_size = -1;
      m_ext.m_alloced_size = that.m_ext.m_alloced_size;
      m_ext.m_alloced_capacity = that.m_ext.m_alloced_capacity;
      that.m_inline_size = 0;
    } else {
      // Move over each element.
      if (this->reserve(that.capacity())) return *this;
      for (Element_type *p = that.begin(); p != that.end(); ++p)
        this->push_back(std::move(*p));
      that.clear();
    }
    return *this;
  }

  /**
    Runs DTOR on all elements if needed.
    Deallocates array if we exceeded the Preallocated amount.
   */
  ~Prealloced_array() {
    if (!Has_trivial_destructor) {
      clear();
    }
    if (!using_inline_buffer()) my_free(m_ext.m_array_ptr);
  }

  size_t capacity() const {
    return using_inline_buffer() ? Prealloc : m_ext.m_alloced_capacity;
  }
  size_t element_size() const { return sizeof(Element_type); }
  bool empty() const { return size() == 0; }
  size_t size() const {
    return using_inline_buffer() ? m_inline_size : m_ext.m_alloced_size;
  }

  Element_type &at(size_t n) {
    assert(n < size());
    return buffer()[n];
  }

  const Element_type &at(size_t n) const {
    assert(n < size());
    return buffer()[n];
  }

  Element_type &operator[](size_t n) { return at(n); }
  const Element_type &operator[](size_t n) const { return at(n); }

  Element_type &back() { return at(size() - 1); }
  const Element_type &back() const { return at(size() - 1); }

  Element_type &front() { return at(0); }
  const Element_type &front() const { return at(0); }

  /**
    begin : Returns a pointer to the first element in the array.
    end   : Returns a pointer to the past-the-end element in the array.
   */
  iterator begin() { return buffer(); }
  iterator end() { return buffer() + size(); }
  const_iterator begin() const { return buffer(); }
  const_iterator end() const { return buffer() + size(); }
  /// Returns a constant pointer to the first element in the array.
  const_iterator cbegin() const { return begin(); }
  /// Returns a constant pointer to the past-the-end element in the array.
  const_iterator cend() const { return end(); }

  /**
    Assigns a value to an arbitrary element, even where n >= size().
    The array is extended with default values if necessary.
    @retval true if out-of-memory, false otherwise.
  */
  bool assign_at(size_t n, const value_type &val) {
    if (n < size()) {
      at(n) = val;
      return false;
    }
    if (reserve(n + 1)) return true;
    resize(n);
    return push_back(val);
  }

  /**
    Reserves space for array elements.
    Copies (or moves, if possible) over existing elements, in case we
    are re-expanding the array.

    @param  n number of elements.
    @retval true if out-of-memory, false otherwise.
  */
  bool reserve(size_t n) {
    if (n <= capacity()) return false;

    void *mem = my_malloc(m_psi_key, n * element_size(), MYF(MY_WME));
    if (!mem) return true;
    Element_type *new_array = static_cast<Element_type *>(mem);

    // Move all the existing elements into the new array.
    size_t old_size = size();
    for (size_t ix = 0; ix < old_size; ++ix) {
      Element_type *new_p = &new_array[ix];
      Element_type &old_p = buffer()[ix];
      ::new (new_p) Element_type(std::move(old_p));  // Move into new location.
      if (!Has_trivial_destructor)
        old_p.~Element_type();  // Destroy the old element.
    }

    if (!using_inline_buffer()) my_free(m_ext.m_array_ptr);

    // Forget the old array;
    m_ext.m_alloced_size = old_size;
    m_inline_size = -1;
    m_ext.m_array_ptr = new_array;
    m_ext.m_alloced_capacity = n;
    return false;
  }

  /**
    Copies an element into the back of the array.
    Complexity: Constant (amortized time, reallocation may happen).
    @return true if out-of-memory, false otherwise
  */
  bool push_back(const Element_type &element) { return emplace_back(element); }

  /**
    Copies (or moves, if possible) an element into the back of the array.
    Complexity: Constant (amortized time, reallocation may happen).
    @return true if out-of-memory, false otherwise
  */
  bool push_back(Element_type &&element) {
    return emplace_back(std::move(element));
  }

  /**
    Constructs an element at the back of the array.
    Complexity: Constant (amortized time, reallocation may happen).
    @return true if out-of-memory, false otherwise
  */
  template <typename... Args>
  bool emplace_back(Args &&... args) {
    const size_t expansion_factor = 2;
    if (size() == capacity() && reserve(capacity() * expansion_factor))
      return true;
    Element_type *p = &buffer()[size()];
    adjust_size(1);
    ::new (p) Element_type(std::forward<Args>(args)...);
    return false;
  }

  /**
    Removes the last element in the array, effectively reducing the
    container size by one. This destroys the removed element.
   */
  void pop_back() {
    assert(!empty());
    if (!Has_trivial_destructor) back().~Element_type();
    adjust_size(-1);
  }

  /**
    The array is extended by inserting a new element before the element at the
    specified position.

    This is generally an inefficient operation, since we need to copy
    elements to make a new "hole" in the array.

    We use std::rotate to move objects, hence Element_type must be
    move-assignable and move-constructible.

    @return an iterator pointing to the inserted value
  */
  iterator insert(const_iterator position, const value_type &val) {
    return emplace(position, val);
  }

  /**
    The array is extended by inserting a new element before the element at the
    specified position. The element is moved into the array, if possible.

    This is generally an inefficient operation, since we need to copy
    elements to make a new "hole" in the array.

    We use std::rotate to move objects, hence Element_type must be
    move-assignable and move-constructible.

    @return an iterator pointing to the inserted value
  */
  iterator insert(const_iterator position, value_type &&val) {
    return emplace(position, std::move(val));
  }

  /**
    The array is extended by inserting a new element before the element at the
    specified position. The element is constructed in-place.

    This is generally an inefficient operation, since we need to copy
    elements to make a new "hole" in the array.

    We use std::rotate to move objects, hence Element_type must be
    move-assignable and move-constructible.

    @return an iterator pointing to the inserted value
  */
  template <typename... Args>
  iterator emplace(const_iterator position, Args &&... args) {
    const difference_type n = position - begin();
    emplace_back(std::forward<Args>(args)...);
    std::rotate(begin() + n, end() - 1, end());
    return begin() + n;
  }

  /**
    Similar to std::set<>::insert()
    Extends the array by inserting a new element, but only if it cannot be found
    in the array already.

    Assumes that the array is sorted with std::less<Element_type>
    Insertion using this function will maintain order.

    @retval A pair, with its member pair::first set an iterator pointing to
            either the newly inserted element, or to the equivalent element
            already in the array. The pair::second element is set to true if
            the new element was inserted, or false if an equivalent element
            already existed.
  */
  std::pair<iterator, bool> insert_unique(const value_type &val) {
    std::pair<iterator, iterator> p = std::equal_range(begin(), end(), val);
    // p.first == p.second means we did not find it.
    if (p.first == p.second) return std::make_pair(insert(p.first, val), true);
    return std::make_pair(p.first, false);
  }

  /**
    Similar to std::set<>::erase()
    Removes a single element from the array by value.
    The removed element is destroyed.
    This effectively reduces the container size by one.

    This is generally an inefficient operation, since we need to copy
    elements to fill the "hole" in the array.

    Assumes that the array is sorted with std::less<Element_type>.

    @retval number of elements removed, 0 or 1.
  */
  size_type erase_unique(const value_type &val) {
    std::pair<iterator, iterator> p = std::equal_range(begin(), end(), val);
    if (p.first == p.second) return 0;  // Not found
    erase(p.first);
    return 1;
  }

  /**
    Similar to std::set<>::count()

    @note   Assumes that array is maintained with insert_unique/erase_unique.

    @retval 1 if element is found, 0 otherwise.
  */
  size_type count_unique(const value_type &val) const {
    return std::binary_search(begin(), end(), val);
  }

  /**
    Removes a single element from the array.
    The removed element is destroyed.
    This effectively reduces the container size by one.

    This is generally an inefficient operation, since we need to move
    or copy elements to fill the "hole" in the array.

    We use std::move to move objects, hence Element_type must be
    move-assignable.
  */
  iterator erase(const_iterator position) {
    assert(position != end());
    return erase(position - begin());
  }

  /**
    Removes a single element from the array.
  */
  iterator erase(size_t ix) {
    assert(ix < size());
    iterator pos = begin() + ix;
    if (pos + 1 != end()) std::move(pos + 1, end(), pos);
    pop_back();
    return pos;
  }

  /**
    Removes tail elements from the array.
    The removed elements are destroyed.
    This effectively reduces the containers size by 'end() - first'.
   */
  void erase_at_end(const_iterator first) {
    const_iterator last = cend();
    const difference_type diff = last - first;
    if (!Has_trivial_destructor) {
      for (; first != last; ++first) first->~Element_type();
    }
    adjust_size(-diff);
  }

  /**
    Removes a range of elements from the array.
    The removed elements are destroyed.
    This effectively reduces the containers size by 'last - first'.

    This is generally an inefficient operation, since we need to move
    or copy elements to fill the "hole" in the array.

    We use std::move to move objects, hence Element_type must be
    move-assignable.
   */
  iterator erase(const_iterator first, const_iterator last) {
    /*
      std::move() wants non-const input iterators, otherwise it cannot move and
      must always copy the elements. Convert first and last from const_iterator
      to iterator.
    */
    iterator start = begin() + (first - cbegin());
    iterator stop = begin() + (last - cbegin());
    if (first != last) erase_at_end(std::move(stop, end(), start));
    return start;
  }

  /**
    Exchanges the content of the container by the content of rhs, which
    is another vector object of the same type. Sizes may differ.

    We use std::swap to do the operation.
   */
  void swap(Prealloced_array &rhs) {
    // Just swap pointers if both arrays have done malloc.
    if (!using_inline_buffer() && !rhs.using_inline_buffer()) {
      std::swap(m_ext.m_alloced_size, rhs.m_ext.m_alloced_size);
      std::swap(m_ext.m_alloced_capacity, rhs.m_ext.m_alloced_capacity);
      std::swap(m_ext.m_array_ptr, rhs.m_ext.m_array_ptr);
      std::swap(m_psi_key, rhs.m_psi_key);
      return;
    }
    std::swap(*this, rhs);
  }

  /**
    Requests the container to reduce its capacity to fit its size.
   */
  void shrink_to_fit() {
    // Cannot shrink the pre-allocated array.
    if (using_inline_buffer()) return;
    // No point in swapping.
    if (size() == capacity()) return;
    Prealloced_array tmp(m_psi_key, begin(), end());
    if (size() <= Prealloc) {
      /*
        The elements fit in the pre-allocated array. Destruct the
        heap-allocated array in this, and copy the elements into the
        pre-allocated array.
      */
      this->~Prealloced_array();
      new (this) Prealloced_array(tmp.m_psi_key, tmp.begin(), tmp.end());
    } else {
      // Both this and tmp have a heap-allocated array. Swap pointers.
      swap(tmp);
    }
  }

  /**
    Resizes the container so that it contains n elements.

    If n is smaller than the current container size, the content is
    reduced to its first n elements, removing those beyond (and
    destroying them).

    If n is greater than the current container size, the content is
    expanded by inserting at the end as many elements as needed to
    reach a size of n. If val is specified, the new elements are
    initialized as copies of val, otherwise, they are
    value-initialized.

    If n is also greater than the current container capacity, an automatic
    reallocation of the allocated storage space takes place.

    Notice that this function changes the actual content of the
    container by inserting or erasing elements from it.
   */
  void resize(size_t n, const Element_type &val = Element_type()) {
    if (n == size()) return;
    if (n > size()) {
      if (!reserve(n)) {
        while (n != size()) push_back(val);
      }
      return;
    }
    if (!Has_trivial_destructor) {
      while (n != size()) pop_back();
    }
    set_size(n);
  }

  /**
    Removes (and destroys) all elements.
    Does not change capacity.
   */
  void clear() {
    if (!Has_trivial_destructor) {
      for (Element_type *p = begin(); p != end(); ++p)
        p->~Element_type();  // Destroy discarded element.
    }
    set_size(0);
  }

 private:
  PSI_memory_key m_psi_key;

  // If >= 0, we're using the inline storage in m_buff, and contains the real
  // size. If negative, we're using external storage (m_array_ptr),
  // and m_alloced_size contains the real size.
  int m_inline_size = 0;

  // Defined outside the union because we need an initializer to avoid
  // "may be used uninitialized" for -flto builds, and
  // MSVC rejects initializers for individual members of an anonymous union.
  // (otherwise, we'd make it an anonymous struct, to avoid m_ext everywhere).
  struct External {
    Element_type *m_array_ptr;
    size_t m_alloced_size;
    size_t m_alloced_capacity;
  };

  union {
    External m_ext{};
    Element_type m_buff[Prealloc];
  };
};
static_assert(sizeof(Prealloced_array<void *, 4>) <= 40,
              "Check for no unexpected padding");

} /* namespace myodbc */

#endif  // PREALLOCED_ARRAY_INCLUDED
