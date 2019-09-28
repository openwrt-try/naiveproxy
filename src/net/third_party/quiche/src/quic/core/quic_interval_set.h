// Copyright (c) 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_INTERVAL_SET_H_
#define QUICHE_QUIC_CORE_QUIC_INTERVAL_SET_H_

// QuicIntervalSet<T> is a data structure used to represent a sorted set of
// non-empty, non-adjacent, and mutually disjoint intervals. Mutations to an
// interval set preserve these properties, altering the set as needed. For
// example, adding [2, 3) to a set containing only [1, 2) would result in the
// set containing the single interval [1, 3).
//
// Supported operations include testing whether an Interval is contained in the
// QuicIntervalSet, comparing two QuicIntervalSets, and performing
// QuicIntervalSet union, intersection, and difference.
//
// QuicIntervalSet maintains the minimum number of entries needed to represent
// the set of underlying intervals. When the QuicIntervalSet is modified (e.g.
// due to an Add operation), other interval entries may be coalesced, removed,
// or otherwise modified in order to maintain this invariant. The intervals are
// maintained in sorted order, by ascending min() value.
//
// The reader is cautioned to beware of the terminology used here: this library
// uses the terms "min" and "max" rather than "begin" and "end" as is
// conventional for the STL. The terminology [min, max) refers to the half-open
// interval which (if the interval is not empty) contains min but does not
// contain max. An interval is considered empty if min >= max.
//
// T is required to be default- and copy-constructible, to have an assignment
// operator, a difference operator (operator-()), and the full complement of
// comparison operators (<, <=, ==, !=, >=, >). These requirements are inherited
// from value_type.
//
// QuicIntervalSet has constant-time move operations.
//
//
// Examples:
//   QuicIntervalSet<int> intervals;
//   intervals.Add(Interval<int>(10, 20));
//   intervals.Add(Interval<int>(30, 40));
//   // intervals contains [10,20) and [30,40).
//   intervals.Add(Interval<int>(15, 35));
//   // intervals has been coalesced. It now contains the single range [10,40).
//   EXPECT_EQ(1, intervals.Size());
//   EXPECT_TRUE(intervals.Contains(Interval<int>(10, 40)));
//
//   intervals.Difference(Interval<int>(10, 20));
//   // intervals should now contain the single range [20, 40).
//   EXPECT_EQ(1, intervals.Size());
//   EXPECT_TRUE(intervals.Contains(Interval<int>(20, 40)));

#include <stddef.h>
#include <algorithm>
#include <initializer_list>
#include <set>
#include <utility>
#include <vector>

#include <string>

#include "net/third_party/quiche/src/quic/core/quic_interval.h"
#include "net/third_party/quiche/src/quic/platform/api/quic_logging.h"

namespace quic {

template <typename T>
class QuicIntervalSet {
 public:
  typedef QuicInterval<T> value_type;

 private:
  struct IntervalLess {
    bool operator()(const value_type& a, const value_type& b) const;
  };
  typedef std::set<value_type, IntervalLess> Set;

 public:
  typedef typename Set::const_iterator const_iterator;
  typedef typename Set::const_reverse_iterator const_reverse_iterator;

  // Instantiates an empty QuicIntervalSet.
  QuicIntervalSet() {}

  // Instantiates an QuicIntervalSet containing exactly one initial half-open
  // interval [min, max), unless the given interval is empty, in which case the
  // QuicIntervalSet will be empty.
  explicit QuicIntervalSet(const value_type& interval) { Add(interval); }

  // Instantiates an QuicIntervalSet containing the half-open interval [min,
  // max).
  QuicIntervalSet(const T& min, const T& max) { Add(min, max); }

  QuicIntervalSet(std::initializer_list<value_type> il) { assign(il); }

  // Clears this QuicIntervalSet.
  void Clear() { intervals_.clear(); }

  // Returns the number of disjoint intervals contained in this QuicIntervalSet.
  size_t Size() const { return intervals_.size(); }

  // Returns the smallest interval that contains all intervals in this
  // QuicIntervalSet, or the empty interval if the set is empty.
  value_type SpanningInterval() const;

  // Adds "interval" to this QuicIntervalSet. Adding the empty interval has no
  // effect.
  void Add(const value_type& interval);

  // Adds the interval [min, max) to this QuicIntervalSet. Adding the empty
  // interval has no effect.
  void Add(const T& min, const T& max) { Add(value_type(min, max)); }

  // Same semantics as Add(const value_type&), but optimized for the case where
  // rbegin()->min() <= |interval|.min() <= rbegin()->max().
  void AddOptimizedForAppend(const value_type& interval) {
    if (Empty()) {
      Add(interval);
      return;
    }

    const_reverse_iterator last_interval = intervals_.rbegin();

    // If interval.min() is outside of [last_interval->min, last_interval->max],
    // we can not simply extend last_interval->max.
    if (interval.min() < last_interval->min() ||
        interval.min() > last_interval->max()) {
      Add(interval);
      return;
    }

    if (interval.max() <= last_interval->max()) {
      // interval is fully contained by last_interval.
      return;
    }

    // Extend last_interval.max to interval.max, in place.
    //
    // Set does not allow in-place updates due to the potential of violating its
    // ordering requirements. But we know setting the max of the last interval
    // is safe w.r.t set ordering and other invariants of QuicIntervalSet, so we
    // force an in-place update for performance.
    const_cast<value_type*>(&(*last_interval))->SetMax(interval.max());
  }

  // Same semantics as Add(const T&, const T&), but optimized for the case where
  // rbegin()->max() == |min|.
  void AddOptimizedForAppend(const T& min, const T& max) {
    AddOptimizedForAppend(value_type(min, max));
  }

  // TODO(wub): Similar to AddOptimizedForAppend, we can also have a
  // AddOptimizedForPrepend if there is a use case.

  // Returns true if this QuicIntervalSet is empty.
  bool Empty() const { return intervals_.empty(); }

  // Returns true if any interval in this QuicIntervalSet contains the indicated
  // value.
  bool Contains(const T& value) const;

  // Returns true if there is some interval in this QuicIntervalSet that wholly
  // contains the given interval. An interval O "wholly contains" a non-empty
  // interval I if O.Contains(p) is true for every p in I. This is the same
  // definition used by value_type::Contains(). This method returns false on
  // the empty interval, due to a (perhaps unintuitive) convention inherited
  // from value_type.
  // Example:
  //   Assume an QuicIntervalSet containing the entries { [10,20), [30,40) }.
  //   Contains(Interval(15, 16)) returns true, because [10,20) contains
  //   [15,16). However, Contains(Interval(15, 35)) returns false.
  bool Contains(const value_type& interval) const;

  // Returns true if for each interval in "other", there is some (possibly
  // different) interval in this QuicIntervalSet which wholly contains it. See
  // Contains(const value_type& interval) for the meaning of "wholly contains".
  // Perhaps unintuitively, this method returns false if "other" is the empty
  // set. The algorithmic complexity of this method is O(other.Size() *
  // log(this->Size())). The method could be rewritten to run in O(other.Size()
  // + this->Size()), and this alternative could be implemented as a free
  // function using the public API.
  bool Contains(const QuicIntervalSet<T>& other) const;

  // Returns true if there is some interval in this QuicIntervalSet that wholly
  // contains the interval [min, max). See Contains(const value_type&).
  bool Contains(const T& min, const T& max) const {
    return Contains(value_type(min, max));
  }

  // Returns true if for some interval in "other", there is some interval in
  // this QuicIntervalSet that intersects with it. See value_type::Intersects()
  // for the definition of interval intersection.
  bool Intersects(const QuicIntervalSet& other) const;

  // Returns an iterator to the value_type in the QuicIntervalSet that contains
  // the given value. In other words, returns an iterator to the unique interval
  // [min, max) in the QuicIntervalSet that has the property min <= value < max.
  // If there is no such interval, this method returns end().
  const_iterator Find(const T& value) const;

  // Returns an iterator to the value_type in the QuicIntervalSet that wholly
  // contains the given interval. In other words, returns an iterator to the
  // unique interval outer in the QuicIntervalSet that has the property that
  // outer.Contains(interval). If there is no such interval, or if interval is
  // empty, returns end().
  const_iterator Find(const value_type& interval) const;

  // Returns an iterator to the value_type in the QuicIntervalSet that wholly
  // contains [min, max). In other words, returns an iterator to the unique
  // interval outer in the QuicIntervalSet that has the property that
  // outer.Contains(Interval<T>(min, max)). If there is no such interval, or if
  // interval is empty, returns end().
  const_iterator Find(const T& min, const T& max) const {
    return Find(value_type(min, max));
  }

  // Returns an iterator pointing to the first value_type which contains or
  // goes after the given value.
  //
  // Example:
  //   [10, 20)  [30, 40)
  //   ^                    LowerBound(10)
  //   ^                    LowerBound(15)
  //             ^          LowerBound(20)
  //             ^          LowerBound(25)
  const_iterator LowerBound(const T& value) const;

  // Returns an iterator pointing to the first value_type which goes after
  // the given value.
  //
  // Example:
  //   [10, 20)  [30, 40)
  //             ^          UpperBound(10)
  //             ^          UpperBound(15)
  //             ^          UpperBound(20)
  //             ^          UpperBound(25)
  const_iterator UpperBound(const T& value) const;

  // Returns true if every value within the passed interval is not Contained
  // within the QuicIntervalSet.
  // Note that empty intervals are always considered disjoint from the
  // QuicIntervalSet (even though the QuicIntervalSet doesn't `Contain` them).
  bool IsDisjoint(const value_type& interval) const;

  // Merges all the values contained in "other" into this QuicIntervalSet.
  void Union(const QuicIntervalSet& other);

  // Modifies this QuicIntervalSet so that it contains only those values that
  // are currently present both in *this and in the QuicIntervalSet "other".
  void Intersection(const QuicIntervalSet& other);

  // Mutates this QuicIntervalSet so that it contains only those values that are
  // currently in *this but not in "interval".
  void Difference(const value_type& interval);

  // Mutates this QuicIntervalSet so that it contains only those values that are
  // currently in *this but not in the interval [min, max).
  void Difference(const T& min, const T& max);

  // Mutates this QuicIntervalSet so that it contains only those values that are
  // currently in *this but not in the QuicIntervalSet "other".
  void Difference(const QuicIntervalSet& other);

  // Mutates this QuicIntervalSet so that it contains only those values that are
  // in [min, max) but not currently in *this.
  void Complement(const T& min, const T& max);

  // QuicIntervalSet's begin() iterator. The invariants of QuicIntervalSet
  // guarantee that for each entry e in the set, e.min() < e.max() (because the
  // entries are non-empty) and for each entry f that appears later in the set,
  // e.max() < f.min() (because the entries are ordered, pairwise-disjoint, and
  // non-adjacent). Modifications to this QuicIntervalSet invalidate these
  // iterators.
  const_iterator begin() const { return intervals_.begin(); }

  // QuicIntervalSet's end() iterator.
  const_iterator end() const { return intervals_.end(); }

  // QuicIntervalSet's rbegin() and rend() iterators. Iterator invalidation
  // semantics are the same as those for begin() / end().
  const_reverse_iterator rbegin() const { return intervals_.rbegin(); }

  const_reverse_iterator rend() const { return intervals_.rend(); }

  template <typename Iter>
  void assign(Iter first, Iter last) {
    Clear();
    for (; first != last; ++first)
      Add(*first);
  }

  void assign(std::initializer_list<value_type> il) {
    assign(il.begin(), il.end());
  }

  // Returns a human-readable representation of this set. This will typically be
  // (though is not guaranteed to be) of the form
  //   "[a1, b1) [a2, b2) ... [an, bn)"
  // where the intervals are in the same order as given by traversal from
  // begin() to end(). This representation is intended for human consumption;
  // computer programs should not rely on the output being in exactly this form.
  std::string ToString() const;

  QuicIntervalSet& operator=(std::initializer_list<value_type> il) {
    assign(il.begin(), il.end());
    return *this;
  }

  // Swap this QuicIntervalSet with *other. This is a constant-time operation.
  void Swap(QuicIntervalSet<T>* other) { intervals_.swap(other->intervals_); }

  friend bool operator==(const QuicIntervalSet& a, const QuicIntervalSet& b) {
    return a.Size() == b.Size() &&
           std::equal(a.begin(), a.end(), b.begin(), NonemptyIntervalEq());
  }

  friend bool operator!=(const QuicIntervalSet& a, const QuicIntervalSet& b) {
    return !(a == b);
  }

 private:
  // Simple member-wise equality, since all intervals are non-empty.
  struct NonemptyIntervalEq {
    bool operator()(const value_type& a, const value_type& b) const {
      return a.min() == b.min() && a.max() == b.max();
    }
  };

  // Removes overlapping ranges and coalesces adjacent intervals as needed.
  void Compact(const typename Set::iterator& begin,
               const typename Set::iterator& end);

  // Returns true if this set is valid (i.e. all intervals in it are non-empty,
  // non-adjacent, and mutually disjoint). Currently this is used as an
  // integrity check by the Intersection() and Difference() methods, but is only
  // invoked for debug builds (via DCHECK).
  bool Valid() const;

  // Finds the first interval that potentially intersects 'other'.
  const_iterator FindIntersectionCandidate(const QuicIntervalSet& other) const;

  // Finds the first interval that potentially intersects 'interval'.
  const_iterator FindIntersectionCandidate(const value_type& interval) const;

  // Helper for Intersection() and Difference(): Finds the next pair of
  // intervals from 'x' and 'y' that intersect. 'mine' is an iterator
  // over x->intervals_. 'theirs' is an iterator over y.intervals_. 'mine'
  // and 'theirs' are advanced until an intersecting pair is found.
  // Non-intersecting intervals (aka "holes") from x->intervals_ can be
  // optionally erased by "on_hole".
  template <typename X, typename Func>
  static bool FindNextIntersectingPairImpl(X* x,
                                           const QuicIntervalSet& y,
                                           const_iterator* mine,
                                           const_iterator* theirs,
                                           Func on_hole);

  // The variant of the above method that doesn't mutate this QuicIntervalSet.
  bool FindNextIntersectingPair(const QuicIntervalSet& other,
                                const_iterator* mine,
                                const_iterator* theirs) const {
    return FindNextIntersectingPairImpl(
        this, other, mine, theirs,
        [](const QuicIntervalSet*, const_iterator, const_iterator) {});
  }

  // The variant of the above method that mutates this QuicIntervalSet by
  // erasing holes.
  bool FindNextIntersectingPairAndEraseHoles(const QuicIntervalSet& other,
                                             const_iterator* mine,
                                             const_iterator* theirs) {
    return FindNextIntersectingPairImpl(
        this, other, mine, theirs,
        [](QuicIntervalSet* x, const_iterator from, const_iterator to) {
          x->intervals_.erase(from, to);
        });
  }

  // The representation for the intervals. The intervals in this set are
  // non-empty, pairwise-disjoint, non-adjacent and ordered in ascending order
  // by min().
  Set intervals_;
};

template <typename T>
auto operator<<(std::ostream& out, const QuicIntervalSet<T>& seq)
    -> decltype(out << *seq.begin()) {
  out << "{";
  for (const auto& interval : seq) {
    out << " " << interval;
  }
  out << " }";

  return out;
}

template <typename T>
void swap(QuicIntervalSet<T>& x, QuicIntervalSet<T>& y);

//==============================================================================
// Implementation details: Clients can stop reading here.

template <typename T>
typename QuicIntervalSet<T>::value_type QuicIntervalSet<T>::SpanningInterval()
    const {
  value_type result;
  if (!intervals_.empty()) {
    result.SetMin(intervals_.begin()->min());
    result.SetMax(intervals_.rbegin()->max());
  }
  return result;
}

template <typename T>
void QuicIntervalSet<T>::Add(const value_type& interval) {
  if (interval.Empty())
    return;
  std::pair<typename Set::iterator, bool> ins = intervals_.insert(interval);
  if (!ins.second) {
    // This interval already exists.
    return;
  }
  // Determine the minimal range that will have to be compacted.  We know that
  // the QuicIntervalSet was valid before the addition of the interval, so only
  // need to start with the interval itself (although Compact takes an open
  // range so begin needs to be the interval to the left).  We don't know how
  // many ranges this interval may cover, so we need to find the appropriate
  // interval to end with on the right.
  typename Set::iterator begin = ins.first;
  if (begin != intervals_.begin())
    --begin;
  const value_type target_end(interval.max(), interval.max());
  const typename Set::iterator end = intervals_.upper_bound(target_end);
  Compact(begin, end);
}

template <typename T>
bool QuicIntervalSet<T>::Contains(const T& value) const {
  value_type tmp(value, value);
  // Find the first interval with min() > value, then move back one step
  const_iterator it = intervals_.upper_bound(tmp);
  if (it == intervals_.begin())
    return false;
  --it;
  return it->Contains(value);
}

template <typename T>
bool QuicIntervalSet<T>::Contains(const value_type& interval) const {
  // Find the first interval with min() > value, then move back one step.
  const_iterator it = intervals_.upper_bound(interval);
  if (it == intervals_.begin())
    return false;
  --it;
  return it->Contains(interval);
}

template <typename T>
bool QuicIntervalSet<T>::Contains(const QuicIntervalSet<T>& other) const {
  if (!SpanningInterval().Contains(other.SpanningInterval())) {
    return false;
  }

  for (const_iterator i = other.begin(); i != other.end(); ++i) {
    // If we don't contain the interval, can return false now.
    if (!Contains(*i)) {
      return false;
    }
  }
  return true;
}

// This method finds the interval that Contains() "value", if such an interval
// exists in the QuicIntervalSet. The way this is done is to locate the
// "candidate interval", the only interval that could *possibly* contain value,
// and test it using Contains(). The candidate interval is the interval with the
// largest min() having min() <= value.
//
// Determining the candidate interval takes a couple of steps. First, since the
// underlying std::set stores intervals, not values, we need to create a "probe
// interval" suitable for use as a search key. The probe interval used is
// [value, value). Now we can restate the problem as finding the largest
// interval in the QuicIntervalSet that is <= the probe interval.
//
// This restatement only works if the set's comparator behaves in a certain way.
// In particular it needs to order first by ascending min(), and then by
// descending max(). The comparator used by this library is defined in exactly
// this way. To see why descending max() is required, consider the following
// example. Assume an QuicIntervalSet containing these intervals:
//
//   [0, 5)  [10, 20)  [50, 60)
//
// Consider searching for the value 15. The probe interval [15, 15) is created,
// and [10, 20) is identified as the largest interval in the set <= the probe
// interval. This is the correct interval needed for the Contains() test, which
// will then return true.
//
// Now consider searching for the value 30. The probe interval [30, 30) is
// created, and again [10, 20] is identified as the largest interval <= the
// probe interval. This is again the correct interval needed for the Contains()
// test, which in this case returns false.
//
// Finally, consider searching for the value 10. The probe interval [10, 10) is
// created. Here the ordering relationship between [10, 10) and [10, 20) becomes
// vitally important. If [10, 10) were to come before [10, 20), then [0, 5)
// would be the largest interval <= the probe, leading to the wrong choice of
// interval for the Contains() test. Therefore [10, 10) needs to come after
// [10, 20). The simplest way to make this work in the general case is to order
// by ascending min() but descending max(). In this ordering, the empty interval
// is larger than any non-empty interval with the same min(). The comparator
// used by this library is careful to induce this ordering.
//
// Another detail involves the choice of which std::set method to use to try to
// find the candidate interval. The most appropriate entry point is
// set::upper_bound(), which finds the smallest interval which is > the probe
// interval. The semantics of upper_bound() are slightly different from what we
// want (namely, to find the largest interval which is <= the probe interval)
// but they are close enough; the interval found by upper_bound() will always be
// one step past the interval we are looking for (if it exists) or at begin()
// (if it does not). Getting to the proper interval is a simple matter of
// decrementing the iterator.
template <typename T>
typename QuicIntervalSet<T>::const_iterator QuicIntervalSet<T>::Find(
    const T& value) const {
  value_type tmp(value, value);
  const_iterator it = intervals_.upper_bound(tmp);
  if (it == intervals_.begin())
    return intervals_.end();
  --it;
  if (it->Contains(value))
    return it;
  else
    return intervals_.end();
}

// This method finds the interval that Contains() the interval "probe", if such
// an interval exists in the QuicIntervalSet. The way this is done is to locate
// the "candidate interval", the only interval that could *possibly* contain
// "probe", and test it using Contains(). The candidate interval is the largest
// interval that is <= the probe interval.
//
// The search for the candidate interval only works if the comparator used
// behaves in a certain way. In particular it needs to order first by ascending
// min(), and then by descending max(). The comparator used by this library is
// defined in exactly this way. To see why descending max() is required,
// consider the following example. Assume an QuicIntervalSet containing these
// intervals:
//
//   [0, 5)  [10, 20)  [50, 60)
//
// Consider searching for the probe [15, 17). [10, 20) is the largest interval
// in the set which is <= the probe interval. This is the correct interval
// needed for the Contains() test, which will then return true, because [10, 20)
// contains [15, 17).
//
// Now consider searching for the probe [30, 32). Again [10, 20] is the largest
// interval <= the probe interval. This is again the correct interval needed for
// the Contains() test, which in this case returns false, because [10, 20) does
// not contain [30, 32).
//
// Finally, consider searching for the probe [10, 12). Here the ordering
// relationship between [10, 12) and [10, 20) becomes vitally important. If
// [10, 12) were to come before [10, 20), then [0, 5) would be the largest
// interval <= the probe, leading to the wrong choice of interval for the
// Contains() test. Therefore [10, 12) needs to come after [10, 20). The
// simplest way to make this work in the general case is to order by ascending
// min() but descending max(). In this ordering, given two intervals with the
// same min(), the wider one goes before the narrower one. The comparator used
// by this library is careful to induce this ordering.
//
// Another detail involves the choice of which std::set method to use to try to
// find the candidate interval. The most appropriate entry point is
// set::upper_bound(), which finds the smallest interval which is > the probe
// interval. The semantics of upper_bound() are slightly different from what we
// want (namely, to find the largest interval which is <= the probe interval)
// but they are close enough; the interval found by upper_bound() will always be
// one step past the interval we are looking for (if it exists) or at begin()
// (if it does not). Getting to the proper interval is a simple matter of
// decrementing the iterator.
template <typename T>
typename QuicIntervalSet<T>::const_iterator QuicIntervalSet<T>::Find(
    const value_type& probe) const {
  const_iterator it = intervals_.upper_bound(probe);
  if (it == intervals_.begin())
    return intervals_.end();
  --it;
  if (it->Contains(probe))
    return it;
  else
    return intervals_.end();
}

template <typename T>
typename QuicIntervalSet<T>::const_iterator QuicIntervalSet<T>::LowerBound(
    const T& value) const {
  const_iterator it = intervals_.lower_bound(value_type(value, value));
  if (it == intervals_.begin()) {
    return it;
  }

  // The previous intervals_.lower_bound() checking is essentially based on
  // interval.min(), so we need to check whether the `value` is contained in
  // the previous interval.
  --it;
  if (it->Contains(value)) {
    return it;
  } else {
    return ++it;
  }
}

template <typename T>
typename QuicIntervalSet<T>::const_iterator QuicIntervalSet<T>::UpperBound(
    const T& value) const {
  return intervals_.upper_bound(value_type(value, value));
}

template <typename T>
bool QuicIntervalSet<T>::IsDisjoint(const value_type& interval) const {
  if (interval.Empty())
    return true;
  value_type tmp(interval.min(), interval.min());
  // Find the first interval with min() > interval.min()
  const_iterator it = intervals_.upper_bound(tmp);
  if (it != intervals_.end() && interval.max() > it->min())
    return false;
  if (it == intervals_.begin())
    return true;
  --it;
  return it->max() <= interval.min();
}

template <typename T>
void QuicIntervalSet<T>::Union(const QuicIntervalSet& other) {
  intervals_.insert(other.begin(), other.end());
  Compact(intervals_.begin(), intervals_.end());
}

template <typename T>
typename QuicIntervalSet<T>::const_iterator
QuicIntervalSet<T>::FindIntersectionCandidate(
    const QuicIntervalSet& other) const {
  return FindIntersectionCandidate(*other.intervals_.begin());
}

template <typename T>
typename QuicIntervalSet<T>::const_iterator
QuicIntervalSet<T>::FindIntersectionCandidate(
    const value_type& interval) const {
  // Use upper_bound to efficiently find the first interval in intervals_
  // where min() is greater than interval.min().  If the result
  // isn't the beginning of intervals_ then move backwards one interval since
  // the interval before it is the first candidate where max() may be
  // greater than interval.min().
  // In other words, no interval before that can possibly intersect with any
  // of other.intervals_.
  const_iterator mine = intervals_.upper_bound(interval);
  if (mine != intervals_.begin()) {
    --mine;
  }
  return mine;
}

template <typename T>
template <typename X, typename Func>
bool QuicIntervalSet<T>::FindNextIntersectingPairImpl(X* x,
                                                      const QuicIntervalSet& y,
                                                      const_iterator* mine,
                                                      const_iterator* theirs,
                                                      Func on_hole) {
  CHECK(x != nullptr);
  if ((*mine == x->intervals_.end()) || (*theirs == y.intervals_.end())) {
    return false;
  }
  while (!(**mine).Intersects(**theirs)) {
    const_iterator erase_first = *mine;
    // Skip over intervals in 'mine' that don't reach 'theirs'.
    while (*mine != x->intervals_.end() && (**mine).max() <= (**theirs).min()) {
      ++(*mine);
    }
    on_hole(x, erase_first, *mine);
    // We're done if the end of intervals_ is reached.
    if (*mine == x->intervals_.end()) {
      return false;
    }
    // Skip over intervals 'theirs' that don't reach 'mine'.
    while (*theirs != y.intervals_.end() &&
           (**theirs).max() <= (**mine).min()) {
      ++(*theirs);
    }
    // If the end of other.intervals_ is reached, we're done.
    if (*theirs == y.intervals_.end()) {
      on_hole(x, *mine, x->intervals_.end());
      return false;
    }
  }
  return true;
}

template <typename T>
void QuicIntervalSet<T>::Intersection(const QuicIntervalSet& other) {
  if (!SpanningInterval().Intersects(other.SpanningInterval())) {
    intervals_.clear();
    return;
  }

  const_iterator mine = FindIntersectionCandidate(other);
  // Remove any intervals that cannot possibly intersect with other.intervals_.
  intervals_.erase(intervals_.begin(), mine);
  const_iterator theirs = other.FindIntersectionCandidate(*this);

  while (FindNextIntersectingPairAndEraseHoles(other, &mine, &theirs)) {
    // OK, *mine and *theirs intersect.  Now, we find the largest
    // span of intervals in other (starting at theirs) - say [a..b]
    // - that intersect *mine, and we replace *mine with (*mine
    // intersect x) for all x in [a..b] Note that subsequent
    // intervals in this can't intersect any intervals in [a..b) --
    // they may only intersect b or subsequent intervals in other.
    value_type i(*mine);
    intervals_.erase(mine);
    mine = intervals_.end();
    value_type intersection;
    while (theirs != other.intervals_.end() &&
           i.Intersects(*theirs, &intersection)) {
      std::pair<typename Set::iterator, bool> ins =
          intervals_.insert(intersection);
      DCHECK(ins.second);
      mine = ins.first;
      ++theirs;
    }
    DCHECK(mine != intervals_.end());
    --theirs;
    ++mine;
  }
  DCHECK(Valid());
}

template <typename T>
bool QuicIntervalSet<T>::Intersects(const QuicIntervalSet& other) const {
  if (!SpanningInterval().Intersects(other.SpanningInterval())) {
    return false;
  }

  const_iterator mine = FindIntersectionCandidate(other);
  if (mine == intervals_.end()) {
    return false;
  }
  const_iterator theirs = other.FindIntersectionCandidate(*mine);

  return FindNextIntersectingPair(other, &mine, &theirs);
}

template <typename T>
void QuicIntervalSet<T>::Difference(const value_type& interval) {
  if (!SpanningInterval().Intersects(interval)) {
    return;
  }
  Difference(QuicIntervalSet<T>(interval));
}

template <typename T>
void QuicIntervalSet<T>::Difference(const T& min, const T& max) {
  Difference(value_type(min, max));
}

template <typename T>
void QuicIntervalSet<T>::Difference(const QuicIntervalSet& other) {
  if (!SpanningInterval().Intersects(other.SpanningInterval())) {
    return;
  }

  const_iterator mine = FindIntersectionCandidate(other);
  // If no interval in mine reaches the first interval of theirs then we're
  // done.
  if (mine == intervals_.end()) {
    return;
  }
  const_iterator theirs = other.FindIntersectionCandidate(*this);

  while (FindNextIntersectingPair(other, &mine, &theirs)) {
    // At this point *mine and *theirs overlap.  Remove mine from
    // intervals_ and replace it with the possibly two intervals that are
    // the difference between mine and theirs.
    value_type i(*mine);
    intervals_.erase(mine++);
    value_type lo;
    value_type hi;
    i.Difference(*theirs, &lo, &hi);

    if (!lo.Empty()) {
      // We have a low end.  This can't intersect anything else.
      std::pair<typename Set::iterator, bool> ins = intervals_.insert(lo);
      DCHECK(ins.second);
    }

    if (!hi.Empty()) {
      std::pair<typename Set::iterator, bool> ins = intervals_.insert(hi);
      DCHECK(ins.second);
      mine = ins.first;
    }
  }
  DCHECK(Valid());
}

template <typename T>
void QuicIntervalSet<T>::Complement(const T& min, const T& max) {
  QuicIntervalSet<T> span(min, max);
  span.Difference(*this);
  intervals_.swap(span.intervals_);
}

template <typename T>
std::string QuicIntervalSet<T>::ToString() const {
  std::ostringstream os;
  os << *this;
  return os.str();
}

// This method compacts the QuicIntervalSet, merging pairs of overlapping
// intervals into a single interval. In the steady state, the QuicIntervalSet
// does not contain any such pairs. However, the way the Union() and Add()
// methods work is to temporarily put the QuicIntervalSet into such a state and
// then to call Compact() to "fix it up" so that it is no longer in that state.
//
// Compact() needs the interval set to allow two intervals [a,b) and [a,c)
// (having the same min() but different max()) to briefly coexist in the set at
// the same time, and be adjacent to each other, so that they can be efficiently
// located and merged into a single interval. This state would be impossible
// with a comparator which only looked at min(), as such a comparator would
// consider such pairs equal. Fortunately, the comparator used by
// QuicIntervalSet does exactly what is needed, ordering first by ascending
// min(), then by descending max().
template <typename T>
void QuicIntervalSet<T>::Compact(const typename Set::iterator& begin,
                                 const typename Set::iterator& end) {
  if (begin == end)
    return;
  typename Set::iterator next = begin;
  typename Set::iterator prev = begin;
  typename Set::iterator it = begin;
  ++it;
  ++next;
  while (it != end) {
    ++next;
    if (prev->max() >= it->min()) {
      // Overlapping / coalesced range; merge the two intervals.
      T min = prev->min();
      T max = std::max(prev->max(), it->max());
      value_type i(min, max);
      intervals_.erase(prev);
      intervals_.erase(it);
      std::pair<typename Set::iterator, bool> ins = intervals_.insert(i);
      DCHECK(ins.second);
      prev = ins.first;
    } else {
      prev = it;
    }
    it = next;
  }
}

template <typename T>
bool QuicIntervalSet<T>::Valid() const {
  const_iterator prev = end();
  for (const_iterator it = begin(); it != end(); ++it) {
    // invalid or empty interval.
    if (it->min() >= it->max())
      return false;
    // Not sorted, not disjoint, or adjacent.
    if (prev != end() && prev->max() >= it->min())
      return false;
    prev = it;
  }
  return true;
}

template <typename T>
void swap(QuicIntervalSet<T>& x, QuicIntervalSet<T>& y) {
  x.Swap(&y);
}

// This comparator orders intervals first by ascending min() and then by
// descending max(). Readers who are satisified with that explanation can stop
// reading here. The remainder of this comment is for the benefit of future
// maintainers of this library.
//
// The reason for this ordering is that this comparator has to serve two
// masters. First, it has to maintain the intervals in its internal set in the
// order that clients expect to see them. Clients see these intervals via the
// iterators provided by begin()/end() or as a result of invoking Get(). For
// this reason, the comparator orders intervals by ascending min().
//
// If client iteration were the only consideration, then ordering by ascending
// min() would be good enough. This is because the intervals in the
// QuicIntervalSet are non-empty, non-adjacent, and mutually disjoint; such
// intervals happen to always have disjoint min() values, so such a comparator
// would never even have to look at max() in order to work correctly for this
// class.
//
// However, in addition to ordering by ascending min(), this comparator also has
// a second responsibility: satisfying the special needs of this library's
// peculiar internal implementation. These needs require the comparator to order
// first by ascending min() and then by descending max(). The best way to
// understand why this is so is to check out the comments associated with the
// Find() and Compact() methods.
template <typename T>
bool QuicIntervalSet<T>::IntervalLess::operator()(const value_type& a,
                                                  const value_type& b) const {
  return a.min() < b.min() || (a.min() == b.min() && a.max() > b.max());
}

}  // namespace quic

#endif  // QUICHE_QUIC_CORE_QUIC_INTERVAL_SET_H_
