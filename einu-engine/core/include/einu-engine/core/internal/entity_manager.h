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
#include <map>

#include "einu-engine/core/i_entity_manager.h"
#include "einu-engine/core/util/object_pool.h"

namespace einu {
namespace internal {

// TODO(Xiaoyue Chen): use hash table + sorted array for entity storage/caching
template <std::size_t max_comp, std::size_t max_single>
class EntityManager final : public IEntityManager {
 public:
  ~EntityManager() { ResetImpl(); }

 private:
  using ComponentMask = StaticXnentMask<max_comp>;
  using XnentTable = std::array<Xnent*, max_comp>;

  struct EntityData {
    ComponentMask* mask;
    XnentTable* comp_table;
  };

  using EntityTable = std::map<EID, EntityData>;
  using EntityDataPool = util::DynamicPool<ComponentMask, XnentTable>;
  using SinglenentTable = std::array<Xnent, max_single>;

  void SetEIDPoolImpl(IEIDPool& eid_pool) noexcept override {
    assert(!eid_pool_ && "eid pool is already set");
    eid_pool_ = &eid_pool;
  }

  void SetComponentPoolImpl(IXnentPool& comp_pool) noexcept override {
    assert(!comp_pool_ && "component pool is already set");
    comp_pool_ = &comp_pool;
  }

  void SetSinglenentPoolImpl(IXnentPool& single_pool) noexcept override {
    assert(!singlenent_pool_ && "singlenent pool is already set");
    singlenent_pool_ = &single_pool;
  }

  void SetPolicyImpl(Policy policy) noexcept override {
    ett_data_pool_.SetGrowth(policy.growth_func);
    ett_data_pool_.GrowExtra(policy.init_size);
  }

  EID CreateEntityImpl() override {
    auto eid = eid_pool_->Acquire();
    auto tup = ett_data_pool_.Acquire();
    ett_table_[eid] =
        EntityData{&std::get<ComponentMask&>(tup), &std::get<XnentTable&>(tup)};
    return eid;
  }

  void ReleaseEntity(typename EntityTable::iterator it) {
    auto eid = it->first;
    auto [mask, table] = it->second;
    for (auto i = std::size_t{0}; i != mask->size(); ++i) {
      if (mask->test(i)) {
        comp_pool_->Release(XnentTypeID{i}, *(*table)[i]);
      }
    }
    mask->reset();
    ett_data_pool_.Release(std::forward_as_tuple(*mask, *table));
    eid_pool_->Release(eid);
  }

  void DestroyEntityImpl(EID eid) override {
    auto it = ett_table_.find(eid);
    assert(it != ett_table_.end() && "entity does not exist");
    ReleaseEntity(it);
    ett_table_.erase(it);
  }

  bool ContainsEntityImpl(EID eid) const override {
    return ett_table_.find(eid) != ett_table_.end();
  }

  Xnent& AddComponentImpl(EID eid, XnentTypeID tid) override {
    auto&& [mask, table] = ett_table_.at(eid);
    auto& comp = comp_pool_->Acquire(tid);
    mask->set(tid);
    (*table)[tid] = &comp;
    return comp;
  }

  void RemoveComponentImpl(EID eid, XnentTypeID tid) override {
    auto&& [mask, table] = ett_table_.at(eid);
    auto& comp = *(*table)[tid];
    comp_pool_->Release(tid, comp);
    mask->reset(tid);
  }

  Xnent& GetComponentImpl(EID eid, XnentTypeID tid) override {
    return const_cast<Xnent&>(
        static_cast<const EntityManager&>(*this).GetComponentImpl(eid, tid));
  }

  const Xnent& GetComponentImpl(EID eid, XnentTypeID tid) const override {
    auto&& [mask, table] = ett_table_.at(eid);
    assert(mask->test(tid) && "entity does not have the component");
    return *(*table)[tid];
  }

  Xnent& AddSinglenentImpl(XnentTypeID tid) override {
    assert(!singlenent_table_[tid] && "singlenent already exists");
    auto& singlenent = singlenent_pool_->Acquire(tid);
    singlenent_table_[tid] = &singlenent;
    return singlenent;
  }

  Xnent& GetSinglenentImpl(XnentTypeID tid) noexcept override {
    return *singlenent_table_[tid];
  }

  const Xnent& GetSinglenentImpl(XnentTypeID tid) const noexcept override {
    return *singlenent_table_[tid];
  }

  void RemoveSinglenentImpl(XnentTypeID tid) override {
    auto& singlenent = *singlenent_table_[tid];
    singlenent_pool_->Release(tid, singlenent);
    singlenent_table_[tid] = nullptr;
  }

  void GetEntitiesWithComponentsImpl(
      EntityBuffer& buffer, const internal::DynamicXnentMask& mask,
      const internal::XnentTypeIDArray& xtid_arr) override {
    auto smask = ToStatic<max_comp>(mask);
    for (auto it = ett_table_.begin(); it != ett_table_.end(); ++it) {
      auto [emask, table] = it->second;
      if ((*emask & smask) == smask) {
        auto eid = it->first;
        buffer.eids.push_back(eid);
        std::transform(xtid_arr.begin(), xtid_arr.end(),
                       std::back_inserter(buffer.comps),
                       [&table](auto xtid) { return (*table)[xtid]; });
      }
    }
  }

  void ResetImpl() noexcept override {
    for (auto it = ett_table_.begin(); it != ett_table_.end(); ++it) {
      ReleaseEntity(it);
    }

    ett_table_.clear();

    for (auto i = std::size_t{0}; i != singlenent_table_.size(); ++i) {
      if (singlenent_table_[i]) {
        RemoveSinglenentImpl(XnentTypeID{i});
      }
    }

    eid_pool_ = nullptr;
    comp_pool_ = nullptr;
    singlenent_pool_ = nullptr;
    ett_data_pool_.Clear();
    singlenent_table_.fill(nullptr);
  }

  IEIDPool* eid_pool_ = nullptr;
  IXnentPool* comp_pool_ = nullptr;
  IXnentPool* singlenent_pool_ = nullptr;
  EntityDataPool ett_data_pool_{};
  EntityTable ett_table_{};
  XnentTable singlenent_table_{};
};

}  // namespace internal
}  // namespace einu
