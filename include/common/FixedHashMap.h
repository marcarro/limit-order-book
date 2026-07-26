#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace trading::memory {

template <typename Key, typename Value>
class FixedHashMap {
public:
    explicit FixedHashMap(std::size_t max_entries)
        : slots_(slot_count_for(max_entries)), mask_(slots_.size() - 1), max_entries_(max_entries) {}

    Value* find(const Key& key) {
        const std::size_t slot = find_slot(key);
        return slot == slots_.size() ? nullptr : &slots_[slot].value;
    }

    const Value* find(const Key& key) const {
        const std::size_t slot = find_slot(key);
        return slot == slots_.size() ? nullptr : &slots_[slot].value;
    }

    bool insert(const Key& key, const Value& value) {
        if (size_ == max_entries()) {
            return false;
        }

        std::size_t slot = index_for(key);
        while (slots_[slot].occupied) {
            if (slots_[slot].key == key) {
                return false;
            }
            slot = (slot + 1) & mask_;
        }

        slots_[slot] = Slot{key, value, true};
        ++size_;
        return true;
    }

    bool erase(const Key& key) {
        const std::size_t slot = find_slot(key);
        if (slot == slots_.size()) {
            return false;
        }

        erase_slot(slot);
        --size_;
        return true;
    }

    std::size_t size() const { return size_; }
    std::size_t max_entries() const { return max_entries_; }
    bool full() const { return size_ == max_entries(); }

private:
    struct Slot {
        Key key{};
        Value value{};
        bool occupied = false;
    };

    static std::size_t slot_count_for(std::size_t max_entries) {
        if (max_entries == 0) {
            throw std::invalid_argument("fixed hash map capacity must be positive");
        }

        std::size_t slots = 2;
        while (slots < max_entries * 2) {
            slots <<= 1;
        }
        return slots;
    }

    std::size_t index_for(const Key& key) const {
        return std::hash<Key>{}(key) & mask_;
    }

    std::size_t find_slot(const Key& key) const {
        std::size_t slot = index_for(key);
        while (slots_[slot].occupied) {
            if (slots_[slot].key == key) {
                return slot;
            }
            slot = (slot + 1) & mask_;
        }
        return slots_.size();
    }

    void erase_slot(std::size_t hole) {
        std::size_t cursor = (hole + 1) & mask_;
        while (slots_[cursor].occupied) {
            const std::size_t home = index_for(slots_[cursor].key);
            const std::size_t cursor_distance = (cursor - home) & mask_;
            const std::size_t hole_distance = (cursor - hole) & mask_;
            if (cursor_distance >= hole_distance) {
                slots_[hole] = slots_[cursor];
                hole = cursor;
            }
            cursor = (cursor + 1) & mask_;
        }
        slots_[hole].occupied = false;
    }

    std::vector<Slot> slots_;
    std::size_t mask_ = 0;
    std::size_t size_ = 0;
    std::size_t max_entries_ = 0;
};

} // namespace trading::memory
