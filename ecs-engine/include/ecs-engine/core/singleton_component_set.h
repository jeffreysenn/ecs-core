#pragma once

#include <memory>

#include "ecs-engine/core/component_context.h"
#include "ecs-engine/core/i_singleton_component.h"
#include "ecs-engine/utility/algorithm.h"

namespace ecs {

class SingletonComponentSet {
 public:
  template <typename T, typename... Args>
  T& Make(Args&&... args) {
    auto idx = rtti::GetClassIndex<T>();
    if (idx >= vec_.size()) {
      vec_.resize(idx + 1);
    }
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    auto& ref = *ptr;
    vec_[idx] = std::move(ptr);
    return ref;
  }

  template <typename T>
  const T& Get() const noexcept {
    auto idx = rtti::GetClassIndex<T>();
    return static_cast<const T&>(*vec_[idx]);
  }

  template <typename T>
  T& Get() noexcept {
    return const_cast<T&>(
        static_cast<const SingletonComponentSet&>(*this).Get<T>());
  }

  template <typename... Ts>
  void MakeRestDefault(SingletonComponentList<Ts...>) {
    auto f = [&](auto i) {
      if (i >= vec_.size()) {
        vec_.resize(i + 1);
      }
      if (!vec_[i]) {
        vec_[i] = std::make_unique<
            tmp::TypeAt<SingletonComponentList<Ts...>, i>::value>();
      }
    };
    algo::StaticFor<sizeof...<Ts>>(f);
  }

 private:
  using Ptr = std::unique_ptr<ISingletonComponent>;
  std::vector<Ptr> vec_;
};

}  // namespace ecs