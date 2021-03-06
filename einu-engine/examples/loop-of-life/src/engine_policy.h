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

#include "einu-engine/ai/cmp_destination.h"
#include "einu-engine/common/cmp_movement.h"
#include "einu-engine/common/cmp_transform.h"
#include "einu-engine/common/sgl_time.h"
#include "einu-engine/core/einu_engine.h"
#include "einu-engine/core/xnent_list.h"
#include "einu-engine/graphics/cmp_sprite.h"
#include "einu-engine/graphics/sgl_resource_table.h"
#include "einu-engine/graphics/sgl_sprite_batch.h"
#include "einu-engine/window/cmp_window.h"
#include "src/cmp_agent.h"
#include "src/cmp_health.h"
#include "src/sgl_world_state.h"

namespace lol {

using ComponentList = einu::XnentList<
    einu::cmp::Transform, einu::cmp::Movement, einu::window::cmp::Window,
    einu::graphics::cmp::Sprite, einu::ai::cmp::Destination, cmp::Agent,
    cmp::Health, cmp::HealthLoss, cmp::Eat, cmp::Evade, cmp::Panick,
    cmp::Hunger, cmp::Hunt, cmp::Memory, cmp::Reproduce, cmp::GainHealth,
    cmp::Sense, cmp::Wander, cmp::GrassTag, cmp::HerderTag>;

using SinglenentList =
    einu::XnentList<einu::sgl::Time, einu::graphics::sgl::GLResourceTable,
                    einu::graphics::sgl::SpriteBatch, sgl::WorldState>;

using NeedList = einu::NeedList<ComponentList, SinglenentList>;

using EnginePolicy = einu::EnginePolicy<NeedList>;

using Engine = einu::EinuEngine<EnginePolicy>;

}  // namespace lol
