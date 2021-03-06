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

#include "einu-engine/core/einu_engine.h"

#include "gtest/gtest.h"

namespace einu {

struct C1 : Xnent {};
struct C2 : Xnent {};

using TestEnginePolicy = EnginePolicy<NeedList<XnentList<C1, C2>, XnentList<>>>;

TEST(EinuEngine, CreateEngineWillRegisterComponents) {
  auto engine = EinuEngine<TestEnginePolicy>();
  EXPECT_EQ(GetXnentTypeID<C1>(), 0);
  EXPECT_EQ(GetXnentTypeID<C2>(), 1);
}

}  // namespace einu
