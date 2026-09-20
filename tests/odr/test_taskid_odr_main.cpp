// Regression test: TaskID must not violate the One Definition Rule.
//
// Before the fix, Task.h defined TaskID::lastID_ directly in the header:
//
//     static std::atomic<int> lastID_;
//     ...
//     std::atomic<int> TaskID::lastID_{0};   // non-inline definition, in a header
//
// That compiles and links fine as long as Task.h is #included into only
// one .cpp file. tu_a.cpp and tu_b.cpp both #include "Task.h" and each
// instantiate a Task subclass; linking all three files into one binary
// reproduces exactly the situation a real build hits (SendTask.h and
// ReciveTask.h both include Task.h, and any project with more than one
// translation unit touching it at all). If the ODR bug ever comes back,
// this whole test target fails to *link* -- it never gets the chance to
// run and report a normal assertion failure.
#include "test_framework.h"
#include "VirtualBus.h"

int makeTaskAndGetId_TU_A(VirtualBus& bus);
int makeTaskAndGetId_TU_B(VirtualBus& bus);

VB_TEST(TaskID_UniqueAcrossTranslationUnits) {
    VirtualBus bus;
    const int idFromA = makeTaskAndGetId_TU_A(bus);
    const int idFromB = makeTaskAndGetId_TU_B(bus);

    // Linking at all proves the ODR bug is gone. This checks the two TUs
    // are actually sharing a single counter, rather than, say, each
    // silently keeping its own (which a weaker fix could still allow).
    VB_CHECK(idFromA != idFromB);
}

int main() {
    return vbtest::runAll();
}
