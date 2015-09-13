#pragma once

#include <cstdint>

#include "game.h"

struct PlayerInput;

int CreateSnapshot(Time frameTime, uint64_t frameSeed, const PlayerInput& playerInput);

void ReplaySnapshot(int snapshotIndex, Time& frameTime, uint64_t& frameSeed, PlayerInput& playerInput);

int GetSnapshotCount();

void ValidateSnapshot(int snapshotIndex, Time frameTime, uint64_t frameSeed);


