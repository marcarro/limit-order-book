#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace trading::memory {

template <typename T>
class FixedPool {
public:
    explicit FixedPool(std::size_t capacity) : storage_(capacity), free_indices_(capacity) {
        if (capacity == 0) {
            throw std::invalid_argument("fixed pool capacity must be positive");
        }
        for (std::size_t index = 0; index < capacity; ++index) {
            free_indices_[index] = static_cast<std::uint32_t>(capacity - index - 1);
        }
    }

    T* acquire() {
        if (free_indices_.empty()) {
            return nullptr;
        }
        const std::uint32_t index = free_indices_.back();
        free_indices_.pop_back();
        return &storage_[index];
    }

    void release(T* value) {
        assert(value >= storage_.data() && value < storage_.data() + storage_.size());
        free_indices_.push_back(static_cast<std::uint32_t>(value - storage_.data()));
    }

    std::size_t available() const { return free_indices_.size(); }
    bool full() const { return free_indices_.empty(); }

private:
    std::vector<T> storage_;
    std::vector<std::uint32_t> free_indices_;
};

} // namespace trading::memory
