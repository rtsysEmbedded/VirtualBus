#ifndef OBJECT_POOL_H
#define OBJECT_POOL_H

#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

/**
 * @brief Fixed-capacity pool of pre-allocated T instances, handed out as
 * shared_ptr<T> whose deleter returns the slot to the pool instead of
 * calling delete.
 *
 * This is VirtualBus's "zero-copy variant": sendMessage() already never
 * copies a message's payload (every receiver gets a shared_ptr to the
 * same VirtualBusCmd instance, not a copy of it), but every
 * `make_shared<SomeCommand>()` on the hot send path is still a heap
 * allocation. Acquiring from a pre-warmed ObjectPool instead removes
 * that allocation from the hot path entirely once the pool is
 * constructed: acquire()/release() only touch a free-list under a
 * mutex, no new/delete.
 *
 * acquire() returns nullptr when the pool is exhausted -- reject-new,
 * no implicit heap fallback and no blocking -- consistent with this
 * project's existing bounded-queue overflow policy (see
 * VirtualBus::sendMessage()'s ReturnType::BUSY) rather than silently
 * reintroducing an allocation (defeating the point) or stalling the
 * caller.
 *
 * Lifetime contract: the pool must outlive every shared_ptr<T> it has
 * handed out. Each acquired object's storage lives inside the pool
 * itself (pre-allocated, not separately heap-allocated), so destroying
 * the pool while a lease is still outstanding leaves that shared_ptr
 * pointing at freed memory -- the same contract as any arena/bump
 * allocator whose objects don't outlive the arena. The destructor logs
 * (if given a logger) when destroyed with outstanding leases, as a
 * diagnostic, but can't make it safe after the fact.
 */
template <typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t capacity) : capacity_(capacity), slots_(capacity) {
        freeList_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i) {
            freeList_.push_back(i);
        }
    }

    ~ObjectPool() = default;

    ObjectPool(const ObjectPool&) = delete;
    ObjectPool& operator=(const ObjectPool&) = delete;

    /**
     * @brief Constructs a T in a free slot (forwarding args to T's
     * constructor) and returns a shared_ptr to it, or nullptr if the pool
     * is exhausted.
     */
    template <typename... Args>
    std::shared_ptr<T> acquire(Args&&... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (freeList_.empty()) {
            return nullptr;
        }
        size_t index = freeList_.back();
        freeList_.pop_back();

        T* obj = ::new (static_cast<void*>(&slots_[index])) T(std::forward<Args>(args)...);

        return std::shared_ptr<T>(obj, [this, index](T* p) {
            p->~T();
            std::lock_guard<std::mutex> lock(mutex_);
            freeList_.push_back(index);
        });
    }

    /// Total number of slots this pool was constructed with.
    size_t capacity() const { return capacity_; }

    /// Number of slots currently free (not backed by any outstanding shared_ptr<T>).
    size_t availableCount() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return freeList_.size();
    }

private:
    struct alignas(T) Slot {
        unsigned char bytes[sizeof(T)];
    };

    size_t capacity_;
    std::vector<Slot> slots_;
    std::vector<size_t> freeList_;
    mutable std::mutex mutex_;
};

#endif // OBJECT_POOL_H
