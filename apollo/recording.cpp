#include "recording.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

#include "ai.h"
#include "game.h"
#include "game_object.h"
#include "math_helpers.h"
#include "physics.h"
#include "player.h"
#include "profiler.h"
#include "world.h"

using namespace std;

struct Snapshot
{
	Time time;
	uint64_t seed;
	Vector2 minWorld;
	Vector2 maxWorld;

	PlayerInput playerInput;

	GameObject player;
	vector<GameObject> aliens;
	vector<GameObject> bullets;

	vector<RigidBody> rigidBodies;
	vector<CollisionObject> collisionObjects;

	vector<AIModelAlienRandom> randomAIs;
	vector<AIModelAlienShy> shyAIs;
	vector<AIModelAlienChase> chaseAIs;
	vector<AIModelAlienMothership> mothershipAIs;
	vector<AIModelAlienOffspring> offspringAIs;
	vector<AIModelAlienWallHugger> wallHuggerAIs;
};

vector<Snapshot> snapshots;


int CreateSnapshot(Time frameTime, uint64_t frameSeed, const PlayerInput& playerInput)
{
	PROFILER_TIMER_FUNCTION();

	snapshots.emplace_back();
	auto& snapshot = snapshots.back();

	snapshot.time = frameTime;
	snapshot.seed = frameSeed;
	snapshot.minWorld = minWorld;
	snapshot.maxWorld = maxWorld;

	snapshot.playerInput = playerInput;

	snapshot.player = player;
	snapshot.aliens = aliens;
	snapshot.bullets = bullets;

	snapshot.rigidBodies = rigidBodies;
	snapshot.collisionObjects = collisionObjects;

	snapshot.randomAIs = randomAIs;
	snapshot.shyAIs = shyAIs;
	snapshot.chaseAIs = chaseAIs;
	snapshot.mothershipAIs = mothershipAIs;
	snapshot.offspringAIs = offspringAIs;
	snapshot.wallHuggerAIs = wallHuggerAIs;

	return static_cast<int>(snapshots.size() - 1);
}

void ReplaySnapshot(int snapshotIndex, Time& frameTime, uint64_t& frameSeed, PlayerInput& playerInput)
{
	PROFILER_TIMER_FUNCTION();
	assert((snapshotIndex >= 0) && (snapshotIndex < snapshots.size()));
	const auto& snapshot = snapshots[snapshotIndex];

	frameTime = snapshot.time;
	frameSeed = snapshot.seed;
	minWorld = snapshot.minWorld;
	maxWorld = snapshot.maxWorld;

	playerInput = snapshot.playerInput;

	player = snapshot.player;
	aliens = snapshot.aliens;
	bullets = snapshot.bullets;

	rigidBodies = snapshot.rigidBodies;
	collisionObjects = snapshot.collisionObjects;

	randomAIs = snapshot.randomAIs;
	shyAIs = snapshot.shyAIs;
	chaseAIs = snapshot.chaseAIs;
	mothershipAIs = snapshot.mothershipAIs;
	offspringAIs = snapshot.offspringAIs;
	wallHuggerAIs = snapshot.wallHuggerAIs;
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
	assert(snapshot.minWorld == minWorld);
	assert(snapshot.maxWorld == maxWorld);

	assert(snapshot.player.objectId == player.objectId);
	assert(snapshot.aliens.size() == aliens.size());
	assert(snapshot.bullets.size() == bullets.size());

	assert(snapshot.rigidBodies.size() == rigidBodies.size());
	assert(snapshot.collisionObjects.size() == collisionObjects.size());

	assert(snapshot.randomAIs.size() == randomAIs.size());
	assert(snapshot.shyAIs.size() == shyAIs.size());
	assert(snapshot.chaseAIs.size() == chaseAIs.size());
	assert(snapshot.mothershipAIs.size() == mothershipAIs.size());
	assert(snapshot.offspringAIs.size() == offspringAIs.size());
	assert(snapshot.wallHuggerAIs.size() == wallHuggerAIs.size());
}

