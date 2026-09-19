// Translation unit B for the TaskID ODR regression test. See tu_a.cpp and
// test_taskid_odr_main.cpp for the full explanation -- this file exists
// solely to be a *second* translation unit that includes Task.h, so that
// linking it together with tu_a.cpp reproduces the conditions the
// original ODR bug needed to manifest.
#include "Task.h"
#include "VirtualBus.h"

namespace {
class DummyTaskB : public Task {
public:
    using Task::Task;

protected:
    void run() override {}
};
} // namespace

int makeTaskAndGetId_TU_B(VirtualBus& bus) {
    DummyTaskB task("DummyB", bus, nullptr);
    return task.getID();
}
