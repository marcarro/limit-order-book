#pragma once
#include <array>
#include <cassert>
#include <vector>
#include <cstddef>

namespace trading {
namespace memory {

// Generic memory block interface
template <typename T>
class MemoryBlock {
public:
    virtual T* allocate() = 0;
    virtual void deallocate(T* obj) = 0;
    virtual bool owns(T* obj) const = 0;
    virtual bool is_full() const = 0;
    virtual size_t capacity() const = 0;
    virtual size_t used() const = 0;
    virtual ~MemoryBlock() = default;
};

// Fixed-size memory block
template <typename T, size_t BlockSize>
class FixedMemoryBlock : public MemoryBlock<T> {
private:
	alignas(T) std::byte storage_[sizeof(T) * BlockSize];
    std::array<bool, BlockSize> used_;
    size_t next_free_ = 0;
    size_t used_count_ = 0;

public:
    FixedMemoryBlock() {
        std::fill(used_.begin(), used_.end(), false);
    }

	~FixedMemoryBlock() {
        // Properly destruct any remaining objects
        for (size_t i = 0; i < BlockSize; ++i) {
            if (used_[i]) {
                T* obj = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
                obj->~T();
            }
        }
    }

    T* allocate() override {
        if (next_free_ >= BlockSize) return nullptr;
        
        while (next_free_ < BlockSize && used_[next_free_]) {
            next_free_++;
        }
        
        if (next_free_ >= BlockSize) return nullptr;
        
        used_[next_free_] = true;
        used_count_++;
		return reinterpret_cast<T*>(&storage_[next_free_++ * sizeof(T)]);
    }

    void deallocate(T* obj) override {
        if (!owns(obj)) return;

		// Calculate index based on pointer arithmetic
        std::byte* ptr = reinterpret_cast<std::byte*>(obj);
        size_t index = (ptr - storage_) / sizeof(T);
        
        assert(index < BlockSize);
        
        if (!used_[index]) return;

		obj->~T();

		used_[index] = false;
		used_count_--;
		if (index < next_free_) {
			next_free_ = index;
		}
        
    }

    bool owns(T* obj) const override {
		std::byte* ptr = reinterpret_cast<std::byte*>(obj);
        return ptr >= storage_ && ptr < (storage_ + sizeof(T) * BlockSize);
    }

    bool is_full() const override {
        return used_count_ >= BlockSize;
    }

    size_t capacity() const override {
        return BlockSize;
    }

    size_t used() const override {
        return used_count_;
    }
};

// Expandable memory pool with multiple blocks
template <typename T, size_t BlockSize = 1024>
class MemoryPool {
private:
    std::vector<std::unique_ptr<MemoryBlock<T>>> blocks_;

public:
    MemoryPool() {
        // start with one block
        blocks_.push_back(
            std::make_unique<FixedMemoryBlock<T, BlockSize>>()
        );
    }

    T* allocate() {
        // try to allocate from existing blocks
        for (auto& block : blocks_) {
            T* obj = block->allocate();
            if (obj != nullptr) return obj;
        }

        // all blocks full, create a new one
        blocks_.push_back(
            std::make_unique<FixedMemoryBlock<T, BlockSize>>()
        );

        return blocks_.back()->allocate();
    }

    void deallocate(T* obj) {
        for (auto& block : blocks_) {
            if (block->owns(obj)) {
                block->deallocate(obj);
                return;
            }
        }

        assert(false && "tried to deallocate object not owned by this pool");
    }

    size_t total_capacity() const {
        size_t total = 0;
        for (const auto& block : blocks_) {
            total += block->capacity();
        }
        return total;
    }

    size_t total_used() const {
        size_t total = 0;
        for (const auto& block : blocks_) {
            total += block->used();
        }
        return total;
    }
};

}

}
