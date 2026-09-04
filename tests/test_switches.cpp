#include <cassert>
#include "Switch.h"
int main() { Switch junction("S1", "A", {"B", "D"}); assert(junction.setPosition("B", false)); assert(!junction.setPosition("D", true)); assert(!junction.setPosition("X", false)); }
