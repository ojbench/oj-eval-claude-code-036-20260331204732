#ifndef DYNAMIC_BITSET_H
#define DYNAMIC_BITSET_H

#include <vector>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <algorithm>

struct dynamic_bitset {
private:
    std::vector<uint64_t> data;
    std::size_t bit_size;

    static constexpr std::size_t BITS_PER_BLOCK = 64;

    std::size_t num_blocks() const {
        return (bit_size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;
    }

public:
    // Default constructor
    dynamic_bitset() : bit_size(0) {}

    // Destructor
    ~dynamic_bitset() = default;

    // Copy constructor
    dynamic_bitset(const dynamic_bitset &) = default;

    // Copy assignment
    dynamic_bitset &operator = (const dynamic_bitset &) = default;

    // Initialize bitset with size n, all zeros
    dynamic_bitset(std::size_t n) : bit_size(n) {
        std::size_t blocks = num_blocks();
        data.resize(blocks, 0);
    }

    // Initialize from string (lowest bit first)
    dynamic_bitset(const std::string &str) : bit_size(str.size()) {
        std::size_t blocks = num_blocks();
        data.resize(blocks, 0);

        for (std::size_t i = 0; i < str.size(); ++i) {
            if (str[i] == '1') {
                std::size_t block = i / BITS_PER_BLOCK;
                std::size_t bit = i % BITS_PER_BLOCK;
                data[block] |= (1ULL << bit);
            }
        }
    }

    // Access bit at position n
    bool operator [] (std::size_t n) const {
        if (n >= bit_size) return false;
        std::size_t block = n / BITS_PER_BLOCK;
        std::size_t bit = n % BITS_PER_BLOCK;
        return (data[block] >> bit) & 1ULL;
    }

    // Set bit at position n to val
    dynamic_bitset &set(std::size_t n, bool val = true) {
        if (n >= bit_size) return *this;
        std::size_t block = n / BITS_PER_BLOCK;
        std::size_t bit = n % BITS_PER_BLOCK;

        if (val) {
            data[block] |= (1ULL << bit);
        } else {
            data[block] &= ~(1ULL << bit);
        }
        return *this;
    }

    // Push back a bit (add to high end)
    dynamic_bitset &push_back(bool val) {
        ++bit_size;
        if (num_blocks() > data.size()) {
            data.push_back(0);
        }
        set(bit_size - 1, val);
        return *this;
    }

    // Check if all bits are 0
    bool none() const {
        std::size_t full_blocks = bit_size / BITS_PER_BLOCK;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            if (data[i] != 0) return false;
        }

        std::size_t remaining = bit_size % BITS_PER_BLOCK;
        if (remaining > 0) {
            uint64_t mask = (1ULL << remaining) - 1;
            if ((data[full_blocks] & mask) != 0) return false;
        }

        return true;
    }

    // Check if all bits are 1
    bool all() const {
        std::size_t full_blocks = bit_size / BITS_PER_BLOCK;
        for (std::size_t i = 0; i < full_blocks; ++i) {
            if (data[i] != ~0ULL) return false;
        }

        std::size_t remaining = bit_size % BITS_PER_BLOCK;
        if (remaining > 0) {
            uint64_t mask = (1ULL << remaining) - 1;
            if ((data[full_blocks] & mask) != mask) return false;
        }

        return true;
    }

    // Return size
    std::size_t size() const {
        return bit_size;
    }

    // OR operation
    dynamic_bitset &operator |= (const dynamic_bitset &other) {
        std::size_t min_size = std::min(bit_size, other.bit_size);
        std::size_t min_blocks = (min_size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;

        for (std::size_t i = 0; i < min_blocks; ++i) {
            data[i] |= other.data[i];
        }

        return *this;
    }

    // AND operation
    dynamic_bitset &operator &= (const dynamic_bitset &other) {
        std::size_t min_size = std::min(bit_size, other.bit_size);
        std::size_t min_blocks = (min_size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;

        for (std::size_t i = 0; i < min_blocks; ++i) {
            data[i] &= other.data[i];
        }

        // Clear bits beyond the overlap
        if (other.bit_size < bit_size) {
            std::size_t overlap_bits = min_size % BITS_PER_BLOCK;
            if (overlap_bits > 0 && min_blocks > 0) {
                uint64_t mask = (1ULL << overlap_bits) - 1;
                data[min_blocks - 1] &= mask;
            }

            for (std::size_t i = min_blocks; i < data.size(); ++i) {
                data[i] = 0;
            }
        }

        return *this;
    }

    // XOR operation
    dynamic_bitset &operator ^= (const dynamic_bitset &other) {
        std::size_t min_size = std::min(bit_size, other.bit_size);
        std::size_t min_blocks = (min_size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;

        for (std::size_t i = 0; i < min_blocks; ++i) {
            data[i] ^= other.data[i];
        }

        return *this;
    }

    // Left shift
    dynamic_bitset &operator <<= (std::size_t n) {
        if (n == 0) return *this;

        std::size_t new_size = bit_size + n;
        std::size_t new_blocks = (new_size + BITS_PER_BLOCK - 1) / BITS_PER_BLOCK;

        data.resize(new_blocks, 0);

        std::size_t block_shift = n / BITS_PER_BLOCK;
        std::size_t bit_shift = n % BITS_PER_BLOCK;

        if (bit_shift == 0) {
            // Simple block shift
            for (std::size_t i = data.size(); i > block_shift; --i) {
                data[i - 1] = data[i - 1 - block_shift];
            }
            for (std::size_t i = 0; i < block_shift; ++i) {
                data[i] = 0;
            }
        } else {
            // Complex shift with bit offset
            for (std::size_t i = data.size(); i > block_shift + 1; --i) {
                data[i - 1] = (data[i - 1 - block_shift] << bit_shift) |
                              (data[i - 2 - block_shift] >> (BITS_PER_BLOCK - bit_shift));
            }
            if (block_shift < data.size()) {
                data[block_shift] = data[0] << bit_shift;
            }
            for (std::size_t i = 0; i < block_shift; ++i) {
                data[i] = 0;
            }
        }

        bit_size = new_size;
        return *this;
    }

    // Right shift
    dynamic_bitset &operator >>= (std::size_t n) {
        if (n >= bit_size) {
            bit_size = 0;
            data.clear();
            return *this;
        }

        std::size_t new_size = bit_size - n;
        std::size_t block_shift = n / BITS_PER_BLOCK;
        std::size_t bit_shift = n % BITS_PER_BLOCK;

        if (bit_shift == 0) {
            // Simple block shift
            for (std::size_t i = 0; i + block_shift < data.size(); ++i) {
                data[i] = data[i + block_shift];
            }
        } else {
            // Complex shift with bit offset
            for (std::size_t i = 0; i + block_shift < data.size(); ++i) {
                data[i] = data[i + block_shift] >> bit_shift;
                if (i + block_shift + 1 < data.size()) {
                    data[i] |= data[i + block_shift + 1] << (BITS_PER_BLOCK - bit_shift);
                }
            }
        }

        bit_size = new_size;
        std::size_t new_blocks = num_blocks();
        data.resize(new_blocks);

        return *this;
    }

    // Set all bits to 1
    dynamic_bitset &set() {
        for (auto &block : data) {
            block = ~0ULL;
        }

        // Clear extra bits in the last block
        std::size_t remaining = bit_size % BITS_PER_BLOCK;
        if (remaining > 0 && !data.empty()) {
            uint64_t mask = (1ULL << remaining) - 1;
            data.back() &= mask;
        }

        return *this;
    }

    // Flip all bits
    dynamic_bitset &flip() {
        for (auto &block : data) {
            block = ~block;
        }

        // Clear extra bits in the last block
        std::size_t remaining = bit_size % BITS_PER_BLOCK;
        if (remaining > 0 && !data.empty()) {
            uint64_t mask = (1ULL << remaining) - 1;
            data.back() &= mask;
        }

        return *this;
    }

    // Reset all bits to 0
    dynamic_bitset &reset() {
        for (auto &block : data) {
            block = 0;
        }
        return *this;
    }
};

#endif // DYNAMIC_BITSET_H
