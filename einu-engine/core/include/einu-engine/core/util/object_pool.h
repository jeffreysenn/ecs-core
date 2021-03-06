// Copyright (C) 2020  Xiaoyue Chen
//
// This file is part of EINU Engine.
// See <https://github.com/xiaoyuechen/einu.git>.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "einu-engine/core/util/bit.h"

namespace einu {
namespace util {

namespace internal {

template <typename... Ts>
class FixedPoolImpl {
 public:
  using size_type = std::size_t;
  using value_type = std::tuple<Ts...>;
  using reference = std::tuple<Ts&...>;
  using const_reference = std::tuple<const Ts&...>;

  explicit FixedPoolImpl(size_type count)
      : object_arr_tuple_{ObjectArray<Ts>(count)...}, bit_arr_(count, true) {}

  FixedPoolImpl(size_type count, const value_type& value)
      : object_arr_tuple_{ObjectArray<Ts>(count, std::get<Ts>(value))...},
        bit_arr_(count, true) {}

  FixedPoolImpl(const FixedPoolImpl&) = delete;
  FixedPoolImpl& operator=(const FixedPoolImpl&) = delete;
  FixedPoolImpl(FixedPoolImpl&&) = default;
  FixedPoolImpl& operator=(FixedPoolImpl&&) = default;

  size_type Size() const noexcept { return bit_arr_.size(); }
  std::optional<size_type> FreePos() const noexcept {
    return bit_arr_.countl_zero();
  }

  bool Has(const_reference obj) const noexcept {
    auto idx = &std::get<0>(obj) - std::get<0>(object_arr_tuple_).data();
    return 0 <= idx && (unsigned)idx < Size();
  }

  [[nodiscard]] reference Acquire() noexcept {
    auto free_pos = FreePos();
    assert(free_pos.has_value() && "no object available");
    return Acquire(*free_pos);
  }

  [[nodiscard]] reference Acquire(size_type pos_hint) noexcept {
    assert(bit_arr_[pos_hint] && "object at pos is not available");
    bit_arr_[pos_hint] = false;
    return std::forward_as_tuple(
        std::get<ObjectArray<Ts>>(object_arr_tuple_)[pos_hint]...);
  }

  void Release(const_reference obj) noexcept {
    auto idx = &std::get<0>(obj) - std::get<0>(object_arr_tuple_).data();
    assert(!bit_arr_[idx] && "object is already released");
    bit_arr_[idx] = true;
  }

 private:
  template <typename T>
  using ObjectArray = std::vector<T>;

  std::tuple<ObjectArray<Ts>...> object_arr_tuple_;
  util::BitVector bit_arr_;
};

}  // namespace internal

template <typename... Ts>
class FixedPool : public internal::FixedPoolImpl<Ts...> {
  using Pool = internal::FixedPoolImpl<Ts...>;

 public:
  using Pool::Pool;
};

template <typename T>
class FixedPool<T> {
 public:
  using size_type = std::size_t;
  using value_type = T;
  using reference = value_type&;
  using const_reference = const value_type&;

  explicit FixedPool(size_type count) : pool_{count} {}

  FixedPool(size_type count, const value_type& value)
      : pool_{count, std::forward_as_tuple(value)} {}

  size_type Size() const noexcept { return pool_.Size(); }
  std::optional<size_type> FreePos() const noexcept { return pool_.FreePos(); }
  bool Has(const_reference obj) const noexcept { return pool_.Has(obj); }
  [[nodiscard]] reference Acquire() noexcept {
    return std::get<0>(pool_.Acquire());
  }
  [[nodiscard]] reference Acquire(size_type pos_hint) noexcept {
    return std::get<0>(pool_.Acquire(pos_hint));
  }
  void Release(const_reference obj) noexcept {
    pool_.Release(std::forward_as_tuple(obj));
  }

 private:
  internal::FixedPoolImpl<T> pool_;
};

template <typename FixedPool>
bool AllAcquired(const FixedPool& pool) noexcept {
  return !pool.FreePos().has_value();
}

constexpr std::size_t DefaultGrowth(std::size_t pool_size) noexcept {
  return pool_size == 0 ? 1 : pool_size;
}

template <typename... Ts>
class DynamicPool {
  using OnePool = FixedPool<Ts...>;

 public:
  using size_type = typename OnePool::size_type;
  using value_type = typename OnePool::value_type;
  using reference = typename OnePool::reference;
  using const_reference = typename OnePool::const_reference;
  using GrowthFunc = std::function<size_type(size_type)>;

  DynamicPool(size_type count = 0, std::unique_ptr<value_type> value = nullptr,
              GrowthFunc growth = DefaultGrowth) {
    SetValue(std::move(value));
    SetGrowth(growth);
    GrowExtra(count);
  }

  void SetValue(std::unique_ptr<value_type> value) noexcept {
    value_ = std::move(value);
  }

  void SetGrowth(GrowthFunc growth) noexcept { growth_ = growth; }

  value_type* GetValue() const noexcept { return value_.get(); }

  void GrowExtra(size_type delta_size) {
    if (delta_size == 0) return;
    if (value_) {
      pools_.emplace_back(delta_size, *value_);
    } else {
      pools_.emplace_back(delta_size);
    }
    pools_bit_array_.resize(pools_bit_array_.size() + delta_size, true);
  }

  [[nodiscard]] reference Acquire() {
    if (PoolsAllAcquired()) {
      GrowExtra(growth_(Size()));
    }

    auto free_pos = pools_bit_array_.countl_zero();
    assert(free_pos.has_value() &&
           "no pool is free (maybe growth function is wrong)");
    auto& pool = pools_[*free_pos];
    auto&& obj = pool.Acquire();
    pools_bit_array_[*free_pos] = pool.FreePos().has_value();
    return obj;
  }

  void Release(const_reference obj) noexcept {
    auto pool_it =
        std::find_if(pools_.begin(), pools_.end(),
                     [&obj](const auto& pool) { return pool.Has(obj); });
    assert(pool_it != pools_.end() && "object does not belong to this pool");
    pool_it->Release(obj);
    pools_bit_array_.set(pool_it - pools_.begin());
  }

  size_type Size() const noexcept {
    return std::accumulate(
        pools_.begin(), pools_.end(), size_type{0},
        [](auto acc, const auto& pool) { return acc + pool.Size(); });
  }

  void Clear() noexcept {
    value_.reset();
    growth_ = DefaultGrowth;
    pools_.clear();
    pools_bit_array_.clear();
  }

 private:
  using PoolList = std::vector<OnePool>;

  bool PoolsAllAcquired() const noexcept {
    for (const auto& pool : pools_) {
      if (!AllAcquired(pool)) return false;
    }
    return true;
  }

  std::unique_ptr<value_type> value_ = nullptr;
  GrowthFunc growth_;
  PoolList pools_;
  BitVector pools_bit_array_;
};

}  // namespace util
}  // namespace einu
