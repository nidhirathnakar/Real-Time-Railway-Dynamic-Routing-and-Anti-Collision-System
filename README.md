# Real-Time Railway Anti-Collision Simulator

This is a C++20 software simulation and educational project. It is not certified railway safety-critical software and must not be used to control real railway infrastructure.

All internal physical distances are stored in meters. Route and graph output converts meters to kilometers; train velocity, time, and braking use SI units.

The simulator demonstrates a modular railway control workflow:

```text
RAILWAY SIMULATOR
				|
	+-----+------+
	|            |
Infrastructure Trains
	|            |
RailwayGraph  Train
	|            |
Track Blocks  Movement
	|            |
Reservations  Braking Model
	|            |
Interlocking  Movement Authority
	+-----+------+
				|
	 Route Engine
				|
		 Dijkstra
				|
 Dynamic Routing
				|
Collision Detector
				|
	Safety System
				|
	+-----+------+
	|            |
Telemetry  Multithreading
	+-----+------+
				|
		 Metrics
				|
 Final Simulator
```

- `RailwayGraph` and `RouteEngine`: directed weighted infrastructure and Dijkstra routing with blocked-edge alternatives.
- `TrackBlock`, `BlockManager`, `ReservationManager`, and `InterlockingController`: synchronized block state, atomic reservations, and fail-safe movement authorization.
- `Switch`: protected junction position changes.
- `Train` and `BrakingModel`: deterministic movement, velocity updates, braking, stopping distance, and arrival state.
- `CollisionDetector`: safe, near-miss, and collision classification for trains sharing a block.
- `TelemetryQueue`: bounded producer-consumer communication using `std::mutex` and `std::condition_variable`.
- `SimulationEngine`: RAII-managed `std::jthread` workers, deterministic ticks, rerouting, occupancy, safety, and shutdown.
- `Metrics` and `Logger`: atomic counters, latency measurements, and synchronized operational events.

The included scenario reserves the shortest route for Train 1, blocks that preferred path for Train 2, reroutes Train 2 through the alternate path, applies braking to an approaching train, and reports a safety summary.

## Build and run

```text
cmake -S . -B build
cmake --build build
build/railway_simulator
ctest --test-dir build --output-on-failure
```

On Windows PowerShell, run the executable with `.\build\railway_simulator.exe`.

## About

This project is a C++20 educational simulation of railway interlocking, dynamic routing, train movement, block reservation, telemetry, and anti-collision behavior. It models a railway as a directed weighted graph and divides track edges into physical blocks shared by multiple trains.

It demonstrates object-oriented design, STL containers, Dijkstra routing, deterministic simulation ticks, braking physics, synchronized shared state, producer-consumer communication, and event-driven metrics.

This is software simulation only. It is not certified railway safety-critical software and must not be connected to or used for controlling real railway infrastructure.

## Component responsibilities

- `RailwayGraph`: stores directed nodes and weighted track edges.
- `RouteEngine`: calculates shortest routes using Dijkstra and `std::priority_queue`.
- `TrackBlock`: represents a physical track section and its state.
- `BlockManager`: authoritatively tracks reservations and occupancy with `std::shared_mutex`.
- `ReservationManager`: atomically maps block IDs to train owners.
- `InterlockingController`: serializes movement authorization and rejects conflicting routes.
- `Switch`: protects junction positions and refuses changes in a protected area.
- `Train`: updates position, velocity, state, braking, and route progress.
- `BrakingModel`: calculates deterministic SI-unit stopping distance.
- `CollisionDetector`: classifies safe separation, near misses, and collisions.
- `TelemetryQueue`: bounded producer-consumer communication using a queue and condition variables.
- `SimulationEngine`: owns scenario setup, worker lifecycle, occupancy processing, safety checks, and shutdown.
- `Metrics`: records event-driven safety, routing, reservation, completion, and latency values.
- `Logger`: provides synchronized operational and safety output.

## Default scenario

The sample network uses meters internally:

```text
A -> B = 10000 m
A -> D = 15000 m
B -> C = 10000 m
C -> F = 10000 m
D -> E = 10000 m
E -> F = 10000 m
```

Train 1 receives `A -> B -> C -> F`, a 30000-meter route displayed as 30 km. Train 2 first requests the same route, receives a real interlocking rejection, and is then dynamically rerouted through `A -> D -> E -> F`.

## Safety and physics flow

```text
Route request
	-> availability check
	-> reservation transaction
	-> interlocking authorization
	-> train movement
	-> occupancy update
	-> braking and collision checks
	-> telemetry and event metrics
```

Stopping distance uses:

```text
reaction distance = velocity * reaction time
braking distance  = velocity^2 / (2 * deceleration)
stopping distance = reaction distance + braking distance
```

An emergency-brake metric is recorded only when a train transitions into braking. Repeated ticks in the same braking state are not counted as new activations.

## Concurrency design

- Each train runs in a RAII-managed `std::jthread`.
- A separate `std::jthread` consumes telemetry.
- `std::stop_token` is available to worker functions.
- `std::atomic` protects velocity, brake state, counters, and latency totals.
- `std::mutex` protects train state, queue operations, logging, and interlocking transactions.
- `std::shared_mutex` protects blocks, reservations, and switches.
- `std::condition_variable` prevents busy-waiting in the bounded telemetry queue.

Train workers produce telemetry. The consumer updates `BlockManager`, runs `CollisionDetector`, records safety events, and measures telemetry latency.

## Event-driven metrics

Metrics are generated by actual subsystem results:

- A rejected conflicting authorization records a failed reservation and blocked-route attempt.
- A calculated and assigned alternate route records a route change.
- A denied unsafe movement followed by safe rerouting records a prevented collision.
- A transition into braking records one emergency-brake activation.
- A train reaching its destination records a completion event.

## Source layout

```text
include/    Public class and value-type declarations
src/        Production implementations and main entry point
tests/      Subsystem and integration tests
data/       Reserved for future scenario/configuration files
build/      Generated CMake output
```

## Limitations

This remains an educational console simulation. It does not provide certified signaling hardware integration, formal safety assurance, persistent configuration, or real-world railway operating-rule enforcement. It must not be used to control real railway infrastructure.