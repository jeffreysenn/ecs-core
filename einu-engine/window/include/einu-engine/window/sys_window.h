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

#include "einu-engine/window/cmp_window.h"
#include "einu-engine/window/gl_proc.h"

namespace einu {
namespace window {
namespace sys {

void Init();

void Terminate();

void Create(cmp::Window& win);

void Destroy(cmp::Window& win);

void MakeContextCurrent(cmp::Window& win);

void PoolEvents(cmp::Window& win);

void SwapBuffer(cmp::Window& win);

}  // namespace sys
}  // namespace window
}  // namespace einu
