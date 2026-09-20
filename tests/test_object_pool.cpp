// Regression test for ObjectPool<T>, the "zero-copy variant" building
// block: a fixed-capacity pool of pre-allocated T instances handed out as
// shared_ptr<T> whose custom deleter returns the slot to the pool instead
// of calling delete, so sendMessage()'s hot path can construct a message
// in place of a make_shared<T>() heap allocation.
//
// Uses InverterCommand (a concrete VirtualBusCmd subclass already in the
// tree) as the pooled type, rather than a throwaway test-only struct, so
// this also exercises the pool against a real, non-trivial command class
// (virtual destructor, logger_ member, etc.) instead of only a POD.
#include "test_framework.h"

#include "ObjectPool.h"
#include "InverterCommand.h"

#include <atomic>
#include <thread>
#include <vector>

VB_TEST(ObjectPool_CapacityAndAvailableCountAfterConstruction) {
    ObjectPool<InverterCommand> pool(3);
    VB_CHECK(pool.capacity() == 3);
    VB_CHECK(pool.availableCount() == 3);
}

VB_TEST(ObjectPool_AcquireConstructsAUsableObjectAndConsumesASlot) {
    ObjectPool<InverterCommand> pool(2);

    auto cmd = pool.acquire();
    VB_CHECK(cmd != nullptr);
    if (!cmd) return; // avoid a null deref crashing the whole test binary
    VB_CHECK(pool.availableCount() == 1);

    cmd->setVoltage(400.0);
    cmd->setCurrent(12.5);
    VB_CHECK(cmd->getVoltage() == 400.0);
    VB_CHECK(cmd->getCurrent() == 12.5);
    VB_CHECK(cmd->getType() == CommandType::Inverter);
}

VB_TEST(ObjectPool_ReleasingViaSharedPtrDestructionReturnsSlotToPool) {
    ObjectPool<InverterCommand> pool(1);

    {
        auto cmd = pool.acquire();
        VB_CHECK(cmd != nullptr);
        VB_CHECK(pool.availableCount() == 0);
    }
    // cmd went out of scope: its custom deleter must have run T::~T() and
    // pushed the slot index back onto the free list.
    VB_CHECK(pool.availableCount() == 1);
}

VB_TEST(ObjectPool_ReusedSlotIsFreshlyConstructedNotCarryingOverOldState) {
    ObjectPool<InverterCommand> pool(1);

    auto first = pool.acquire();
    VB_CHECK(first != nullptr);
    if (!first) return;
    first->setVoltage(999.0);
    first.reset();

    auto second = pool.acquire();
    VB_CHECK(second != nullptr);
    if (!second) return; // avoid a null deref crashing the whole test binary
    // If acquire() ever skipped the placement-new (e.g. handed back the
    // slot's leftover bytes instead of constructing a new T), this would
    // observe first's old voltage instead of InverterCommand's default 0.0.
    VB_CHECK(second->getVoltage() == 0.0);
}

VB_TEST(ObjectPool_AcquireReturnsNullptrWhenExhausted_NoImplicitHeapFallback) {
    ObjectPool<InverterCommand> pool(2);

    auto a = pool.acquire();
    auto b = pool.acquire();
    VB_CHECK(a != nullptr);
    VB_CHECK(b != nullptr);
    VB_CHECK(pool.availableCount() == 0);

    auto c = pool.acquire();
    VB_CHECK(c == nullptr);
    // A failed acquire() must not have disturbed the outstanding leases or
    // silently consumed a slot.
    VB_CHECK(pool.availableCount() == 0);
}

VB_TEST(ObjectPool_ExhaustedPoolRecoversOnceALeaseIsReleased) {
    ObjectPool<InverterCommand> pool(1);

    auto a = pool.acquire();
    VB_CHECK(pool.acquire() == nullptr);

    a.reset();
    auto b = pool.acquire();
    VB_CHECK(b != nullptr);
}

VB_TEST(ObjectPool_SharedPtrAliasingKeepsSlotHeldUntilLastCopyReleases) {
    ObjectPool<InverterCommand> pool(1);

    auto a = pool.acquire();
    std::shared_ptr<InverterCommand> alias = a;
    VB_CHECK(alias.use_count() == 2);

    a.reset();
    // A second live shared_ptr to the same object must keep the slot
    // occupied -- only dropping the last reference should free it.
    VB_CHECK(pool.availableCount() == 0);

    alias.reset();
    VB_CHECK(pool.availableCount() == 1);
}

VB_TEST(ObjectPool_ConcurrentAcquireReleaseNeverOverAllocatesOrCorruptsFreeList) {
    // Hammer a small pool from several threads at once: total outstanding
    // leases must never exceed capacity, and after every thread finishes
    // the free list must be back to exactly full. A broken free-list
    // push/pop (e.g. missing the mutex) would show up here as either a
    // torn read handing the same slot index to two threads (caught below
    // via each thread writing/reading back a distinct value) or a final
    // availableCount() != capacity().
    constexpr size_t kCapacity = 4;
    constexpr int kThreads = 8;
    constexpr int kItersPerThread = 2000;

    ObjectPool<InverterCommand> pool(kCapacity);
    std::atomic<int> corruptions{0};

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&pool, &corruptions, t]() {
            for (int i = 0; i < kItersPerThread; ++i) {
                auto cmd = pool.acquire();
                if (!cmd) {
                    continue; // pool momentarily full; expected under contention
                }
                double marker = static_cast<double>(t * 100000 + i);
                cmd->setVoltage(marker);
                std::this_thread::yield();
                if (cmd->getVoltage() != marker) {
                    ++corruptions;
                }
            }
        });
    }
    for (auto& th : threads) {
        th.join();
    }

    VB_CHECK(corruptions.load() == 0);
    VB_CHECK(pool.availableCount() == kCapacity);
}

int main() {
    return vbtest::runAll();
}
