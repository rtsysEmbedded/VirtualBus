// Translation unit A for the TaskID ODR regression test (see
// test_taskid_odr_main.cpp for the full explanation).
//
// TaskID::lastID_ used to be defined directly in Task.h as a non-inline
// static (`std::atomic<int> TaskID::lastID_{0};` sitting in the header
// itself). That only links as long as Task.h is #included by a single
// .cpp file; a second .cpp that also includes it reproduces "multiple
// definition of TaskID::lastID_" at link time -- exactly the situation a
// real build hits once more than one translation unit touches Task.h
// (which SendTask.h and ReciveTask.h both do). This file and tu_b.cpp
// deliberately both include Task.h and instantiate a Task subclass, so
// simply linking this test binary together is the regression test.
#include "Task.h"
#include "VirtualBus.h"

namespace {
class DummyTaskA : public Task {
public:
    using Task::Task;

protected:
    void run() override {}
};
} // namespace

int makeTaskAndGetId_TU_A(VirtualBus& bus) {
    DummyTaskA task("DummyA", bus, nullptr);
    return task.getID();
}
