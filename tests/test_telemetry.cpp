#include <cassert>
#include "TelemetryQueue.h"
int main() { TelemetryQueue queue(1); TrainTelemetry event; event.id = 1; assert(queue.push(event)); assert(queue.pop()->id == 1); queue.close(); assert(!queue.push(event)); }
