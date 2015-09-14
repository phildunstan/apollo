#include "recording.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "game.h"
#include "game_object.h"
#include "physics.h"
#include "player.h"
#include "profiler.h"
#include "world.h"

using namespace std;

struct Snapshot
{
	Time time;
	uint64_t seed;

	PlayerInput playerInput;

	GameObject player;
	vector<GameObject> aliens;
	vector<GameObject> bullets;

	vector<RigidBody> rigidBodies;
	vector<CollisionObject> collisionObjects;

	vector<std::unique_ptr<AIModel>> aiModels;
};

vector<Snapshot> snapshots;


int CreateSnapshot(Time frameTime, uint64_t frameSeed, const PlayerInput& playerInput)
{
	PROFILER_TIMER_FUNCTION();

	snapshots.emplace_back();
	auto& snapshot = snapshots.back();

	snapshot.time = frameTime;
	snapshot.seed = frameSeed;

	snapshot.playerInput = playerInput;

	snapshot.player = player;
	snapshot.aliens = aliens;
	snapshot.bullets = bullets;

	snapshot.rigidBodies = rigidBodies;
	snapshot.collisionObjects = collisionObjects;

	snapshot.aiModels.resize(aiModels.size());
	transform(cbegin(aiModels), cend(aiModels), begin(snapshot.aiModels), [] (const unique_ptr<AIModel>& aiModelPtr) { return (aiModelPtr ? aiModelPtr->Clone() : unique_ptr<AIModel>()); });

	return static_cast<int>(snapshots.size() - 1);
}

void ReplaySnapshot(int snapshotIndex, Time& frameTime, uint64_t& frameSeed, PlayerInput& playerInput)
{
	PROFILER_TIMER_FUNCTION();
	assert((snapshotIndex >= 0) && (snapshotIndex < snapshots.size()));
	const auto& snapshot = snapshots[snapshotIndex];

	frameTime = snapshot.time;
	frameSeed = snapshot.seed;

	playerInput = snapshot.playerInput;

	player = snapshot.player;
	aliens = snapshot.aliens;
	bullets = snapshot.bullets;

	rigidBodies = snapshot.rigidBodies;
	collisionObjects = snapshot.collisionObjects;

	aiModels.resize(snapshot.aiModels.size());
	transform(cbegin(snapshot.aiModels), cend(snapshot.aiModels), begin(aiModels), [] (const unique_ptr<AIModel>& aiModelPtr) { return (aiModelPtr ? aiModelPtr->Clone() : unique_ptr<AIModel>()); });
}

int GetSnapshotCount()
{
	return static_cast<int>(snapshots.size());
}

void ValidateSnapshot(int snapshotIndex, Time frameTime, uint64_t frameSeed)
{
	PROFILER_TIMER_FUNCTION();
	assert((snapshotIndex >= 0) && (snapshotIndex < snapshots.size()));
	const auto& snapshot = snapshots[snapshotIndex];

	assert(snapshot.time == frameTime);
	assert(snapshot.seed == frameSeed);

	assert(snapshot.player.objectId == player.objectId);
	assert(snapshot.aliens.size() == aliens.size());
	assert(snapshot.bullets.size() == bullets.size());

	assert(snapshot.rigidBodies.size() == rigidBodies.size());
	assert(snapshot.collisionObjects.size() == collisionObjects.size());

	assert(snapshot.aiModels.size() == aiModels.size());
}
