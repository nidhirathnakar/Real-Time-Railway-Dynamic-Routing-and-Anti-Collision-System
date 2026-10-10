#include <cassert>
#include <atomic>
#include <thread>
#include <vector>
#include "TelemetryQueue.h"

int main() {
	TelemetryQueue queue(1);
	TrainTelemetry event;
	event.id = 1;
	assert(queue.push(event));
	assert(queue.pop()->id == 1);
	queue.close();
	assert(!queue.push(event));

	TelemetryQueue concurrentQueue(4);
	constexpr int producerCount = 2;
	constexpr int eventsPerProducer = 50;
	std::atomic<int> consumed{0};
	std::jthread consumer([&](std::stop_token) {
		while (concurrentQueue.pop()) ++consumed;
	});
	std::vector<std::jthread> producers;
	for (int producerId = 1; producerId <= producerCount; ++producerId) {
		producers.emplace_back([&, producerId](std::stop_token) {
			for (int index = 0; index < eventsPerProducer; ++index) {
				TrainTelemetry sample;
				sample.id = producerId;
				assert(concurrentQueue.push(sample));
			}
		});
	}
	for (auto& producer : producers) producer.join();
	concurrentQueue.close();
	consumer.join();
	assert(consumed.load() == producerCount * eventsPerProducer);
}
