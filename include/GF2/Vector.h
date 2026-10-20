#ifndef GF2_VECTOR_H
#define GF2_VECTOR_H

#include <vector>
#include <bit>

#include <concepts>
#include <memory>

#include <random>

#include <iostream>
#include <stdexcept>
#include <cassert>

namespace GF2
{
    template <std::unsigned_integral Block = uint64_t, typename Allocator = std::allocator<Block>>
    class Vector_
    {
    private:
        size_t vector_size;                         // Number of bits in vector
        std::vector<Block, Allocator> block_vector; // Packed bit representation of vector through blocks (integers)

        static constexpr size_t block_size = std::numeric_limits<Block>::digits; // Number of bits per block

        // Returns number of blocks needed to represent bit-vector.
        // Ensure round up by adding (block_size - 1) to numerator.
        static constexpr size_t blocks_needed(size_t vector_size) { return (block_size - 1 + vector_size) / block_size; }

        // Returns index of the block containing target bit.
        static constexpr size_t block_index(size_t i) { return i / block_size; }

        // Returns index of target bit inside the block.
        static constexpr size_t bit_index(size_t i) { return i % block_size; }

        // Returns bit-mask to isolate target bit inside the block.
        static constexpr Block bit_mask(size_t i) { return 1ULL << bit_index(i); }

        // Return a reference to the block containing target bit.
        constexpr Block &block_ref(size_t i) { return block_vector[block_index(i)]; }
        constexpr const Block &block_ref(size_t i) const { return block_vector[block_index(i)]; }

        // Set excess (unused bits of last block) bits to "zero".
        constexpr void cleanExcess()
        {
            size_t excess = vector_size % block_size;
            if (excess != 0)
                block_vector[block_vector.size() - 1] &= ~(std::numeric_limits<Block>::max() << excess);
        }

        // Enable cross-template friendship between bit-vector instances.
        template <std::unsigned_integral BlockType, typename AllocatorType>
        friend class Vector_;

    public:
        //-------------------------------------------------------------------------------------------------------------
        //                                  CONSTRUCTORS & FACTORIES
        //-------------------------------------------------------------------------------------------------------------

        // Default constructor for bit-vector.
        constexpr explicit Vector_(size_t vector_size = 0) : vector_size(vector_size), block_vector(blocks_needed(vector_size), 0) {};

        // Factory method for an "all-one" vector.
        static constexpr Vector_ Ones(size_t vector_size)
        {
            Vector_ vec(vector_size);
            vec.ones();
            return vec;
        }

        // Return uniform distribution used for generating random bit-vector.
        static constexpr std::uniform_int_distribution<Block> getDistribution()
        {
            return std::uniform_int_distribution<Block>(0, std::numeric_limits<Block>::max());
        }

        // Factory method to generate random bit-vector.
        static constexpr Vector_ Random(size_t vector_size)
        {
            Vector_ vec(vector_size);

            std::random_device rd;
            std::mt19937 rng(rd());
            auto dist = getDistribution();

            for (size_t block = 0; block < vec.block_vector.size(); ++block)
                vec.block_vector[block] = dist(rng);
            vec.cleanExcess();

            return vec;
        }

        // Factory method to generate random bit-vector by using RNG from outside of method.
        template <typename RNG>
        static constexpr Vector_ Random(size_t vector_size, RNG &rng)
        {
            Vector_ vec(vector_size);

            auto dist = getDistribution();

            for (size_t block = 0; block < vec.block_vector.size(); ++block)
                vec.block_vector[block] = dist(rng);
            vec.cleanExcess();

            return vec;
        }

        // Factory method to generate random bit-vector by using RNG and distribution from outside of method.
        // Requires careful implementation as distribution has to be initialized using GF2::Vector::getDistribution().
        template <typename RNG, typename Distribution>
        static constexpr Vector_ Random(size_t vector_size, RNG &rng, Distribution &dist)
        {
            Vector_ vec(vector_size);

            for (size_t block = 0; block < vec.block_vector.size(); ++block)
                vec.block_vector[block] = dist(rng);
            vec.cleanExcess();

            return vec;
        }

        //-------------------------------------------------------------------------------------------------------------
        //                                      MODIFIERS & ARITHMETICS
        //-------------------------------------------------------------------------------------------------------------

        // Resize bit-vector. Extend by adding "zeros".
        constexpr Vector_ &resize(size_t new_size)
        {
            // Check for change
            if (vector_size == new_size)
                return *this;

            vector_size = new_size;

            // Adjust number of blocks if needed
            size_t new_num_blocks = blocks_needed(new_size);
            if (block_vector.size() != new_num_blocks)
                block_vector.resize(new_num_blocks);

            cleanExcess();

            return *this;
        }

        // Set all bits of vector to "zero".
        constexpr Vector_ &zeros()
        {
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] = 0;
            return *this;
        }

        // Set all bits of vector to "one".
        template <std::unsigned_integral T = Block>
        constexpr Vector_ &ones()
        {
            Block max_integer = std::numeric_limits<T>::max();
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] = max_integer;
            return *this;
        }

        // Swap all bits of this vector with another.
        // Used in e.g. row permutations of GF2::Matrix.
        constexpr Vector_ &swapVectors(Vector_ &other_vec) noexcept
        {
            std::swap(vector_size, other_vec.vector_size);
            std::swap(block_vector, other_vec.block_vector);
            return *this;
        }

        // Swap two bits of a vector with each other.
        constexpr Vector_ &swapBits(size_t i_1, size_t i_2)
        {
            assert(("Indices must be within vector size.", i_1 < vector_size));
            assert(("Indices must be within vector size.", i_2 < vector_size));
            if (get(i_1) != get(i_2))
            {
                flipBit(i_1);
                flipBit(i_2);
            }
            return *this;
        }

        // Permute bits of vector.
        template <typename unsigned_int>
        constexpr Vector_ &permute(std::vector<unsigned_int> permutation)
        {
            static_assert((std::is_unsigned<unsigned_int>::value, "Input-vector must contain unsigned integers."));

            if (permutation.size() > vector_size)
                throw std::invalid_argument("Number of elements in input-vector can't exceed vector size.");

            Vector_ original_vec(*this);
            if (permutation.size() != vector_size)
                resize(permutation.size());
            for (size_t i = 0; i < vector_size; ++i)
                (*this)[i] = original_vec[permutation[i]];
            return *this;
        }

        // Append another bit-vector from the right.
        constexpr Vector_ &append(const Vector_ &other_vec)
        {
            size_t old_size = vector_size;    // Vector size before appending
            size_t old_blocks = num_blocks(); // Number of blocks before appending

            resize(old_size + other_vec.size());

            if (bit_index(old_size) == 0) // Case: Appended vector starts at new block
            {
                for (size_t block = 0; block < other_vec.num_blocks(); ++block)
                    block_vector[old_blocks + block] = other_vec.block_vector[block];
            }
            else // Case: Appended vector needs to fill in excess of last block
            {
                for (size_t block = 0; block < other_vec.num_blocks(); ++block)
                {
                    // Left-hand side of block fills excess of last block
                    auto blocks_lhs = static_cast<Block>(other_vec.block_vector[block] << bit_index(old_size));
                    block_vector[old_blocks + block - 1] |= blocks_lhs;

                    // Right-hand side of block fills first bits of next block (if there still is one)
                    auto blocks_rhs = static_cast<Block>(other_vec.block_vector[block] >> (block_size - bit_index(old_size)));
                    if (old_blocks + block < num_blocks())
                        block_vector[old_blocks + block] |= blocks_rhs;
                }
            }

            return *this;
        }

        // Slice sub-vector from vector.
        constexpr Vector_ slice(size_t i_start, size_t i_end) const
        {
            if (i_end > size())
                throw std::invalid_argument("Indices must be within vector size.");
            if (i_start > i_end)
                throw std::invalid_argument("Lower bound can't exceed upper bound.");

            size_t block_start = block_index(i_start);
            size_t block_end = block_index(i_end - 1);

            Vector_ sub_vec(i_end - i_start);

            if (bit_index(i_start) == 0) // Case: Entire blocks can be left-shifted
            {
                for (size_t block = block_start; block <= block_end; ++block)
                    sub_vec.block_vector[block - block_start] = block_vector[block];
            }
            else // Case: Bits need to be left-shifted across blocks
            {
                for (size_t block = block_start; block <= block_end; ++block)
                {
                    // Left-hand side of block is right-hand side of block in original vector
                    auto blocks_lhs = static_cast<Block>(block_vector[block] >> bit_index(i_start));
                    // Right-hand side of block is left-hand side of next block in original vector
                    Block blocks_rhs = 0;
                    if (block + 1 < block_vector.size())
                    {
                        blocks_rhs = static_cast<Block>(
                            block_vector[block + 1] << (block_size - bit_index(i_start)));
                    }
                    sub_vec.block_vector[block - block_start] = blocks_lhs | blocks_rhs;
                }

                // Clean discarded bits
                sub_vec.cleanExcess();
            }

            return sub_vec;
        }

        // Return true if vector is equal to another.
        constexpr bool isEqualTo(const Vector_ &other_vec) const
        {
            if (vector_size != other_vec.size())
                return false;

            for (size_t block = 0; block < num_blocks(); ++block)
                if (block_vector[block] != other_vec.block_vector[block])
                    return false;

            return true;
        }

        //-------------------------------------------------------------------------------------------------------------
        //                                      ACCESSORS
        //-------------------------------------------------------------------------------------------------------------

        // Return length of vector.
        constexpr size_t size() const { return vector_size; }

        // Return number of bits per block.
        constexpr size_t blockType() const { return block_size; }

        // Return number of blocks in vector.
        constexpr size_t num_blocks() const { return block_vector.size(); }

        // Return number of "ones" in vector.
        constexpr size_t degree() const
        {
            size_t degree = 0;
            for (size_t block = 0; block < block_vector.size(); ++block)
                // using popcount is valid, since excess bits are kept "zero"
                degree += static_cast<size_t>(std::popcount(block_vector[block]));
            return degree;
        }

        //-------------------------------------------------------------------------------------------------------------
        //                                      OUTPUTS
        //-------------------------------------------------------------------------------------------------------------

        // Overloads << operator to print bit-vector.
        friend std::ostream &operator<<(std::ostream &os, const Vector_ &vec)
        {
            for (size_t i = 0; i < vec.vector_size; ++i)
                if (vec.get(i))
                    os << 1 << " ";
                else
                    os << ". ";
            os << "\n";

            return os;
        }

        // Proxy class for write accessing a singular bit by reference.
        class Reference
        {
        private:
            Block target_bit_mask;   // Bit-mask to isolate target bit inside the block
            Block &target_block_ref; // Reference to the block containing target bit

        public:
            // Main constructor
            constexpr Reference(Vector_ &vec, size_t i) : target_bit_mask(vec.bit_mask(i)), target_block_ref(vec.block_ref(i)) {};

            // Copy constructor
            constexpr Reference(const Reference &) = default;

            // Move constructor
            constexpr Reference(Reference &&) noexcept = default;

            // Destructor
            ~Reference() = default;

            // Convert Reference into bool.
            constexpr bool convert_to_bool() const { return (target_block_ref & target_bit_mask); }

            // Enable use of Reference as bool.
            constexpr operator bool() const { return convert_to_bool(); }

            //-------------------------------------------------------------------------------------------------------------
            //                                      BITWISE MODIFIERS
            //-------------------------------------------------------------------------------------------------------------

            // Set bit to "one".
            constexpr Reference &setBit()
            {
                target_block_ref |= target_bit_mask;
                return *this;
            }

            // Set bit to "zero".
            constexpr Reference &clearBit()
            {
                target_block_ref &= ~target_bit_mask;
                return *this;
            }

            // Flip bit.
            constexpr Reference &flipBit()
            {
                target_block_ref ^= target_bit_mask;
                return *this;
            }

            // Set bit to given bool.
            constexpr Reference &assignBit(bool val)
            {
                val ? setBit() : clearBit();
                return *this;
            }

            //-------------------------------------------------------------------------------------------------------------
            //                                      BITWISE OPERATIONS
            //-------------------------------------------------------------------------------------------------------------

            // Assigns = operator to assignment mutator.
            constexpr Reference &operator=(bool val) { return assignBit(val); }                                   // Assign bool to target bit
            constexpr Reference &operator=(const Reference &bit) { return assignBit(bit.convert_to_bool()); }     // Copy assign bit to target bit
            constexpr Reference &operator=(Reference &&bit) noexcept { return assignBit(bit.convert_to_bool()); } // Move assign bit to traget bit

            // Assigns &= operator to logical AND.
            constexpr Reference &operator&=(bool val)
            {
                if (!val)
                    clearBit();
                return *this;
            }

            // Assigns |= operator to logical OR.
            constexpr Reference &operator|=(bool val)
            {
                if (val)
                    setBit();
                return *this;
            }

            // Assigns ^= operator to logical XOR.
            constexpr Reference &operator^=(bool val)
            {
                if (val)
                    flipBit();
                return *this;
            }

            // Assign ~ operator to logical NOT (Read-Only).
            constexpr bool operator~() const { return !convert_to_bool(); }
        };

        // Flip target bit. This method can be directly used on vetor.
        constexpr Vector_ &flipBit(size_t i)
        {
            assert(("Index must be within vector size.", i < vector_size));
            block_ref(i) ^= bit_mask(i);
            return *this;
        }

        //-------------------------------------------------------------------------------------------------------------
        //                                      BITWISE ACCESSORS
        //-------------------------------------------------------------------------------------------------------------

        // Read-only access target bit.
        constexpr const bool get(size_t i) const
        {
            assert(("Index must be within vector size.", i < vector_size));
            return block_ref(i) & bit_mask(i); // Returns only the value of target bit
        }

        // Read-write access target bit.
        constexpr Reference get(size_t i)
        {
            assert(("Index must be within vector size.", i < vector_size));
            return Reference(*this, i); // Returns a reference to target bit, enabling write access
        }

        // Assigns [] operator to accessor.
        constexpr const bool operator[](size_t i) const { return get(i); }
        constexpr Reference operator[](size_t i) { return get(i); }

        // Assigns () operator to accessor.
        constexpr const bool operator()(size_t i) const { return get(i); }
        constexpr Reference operator()(size_t i) { return get(i); }

        //-------------------------------------------------------------------------------------------------------------
        //                                      OPERATIONS
        //-------------------------------------------------------------------------------------------------------------

        // Return parity of vector.
        // Equivalent to XOR sum over all bits in vector.
        constexpr bool parity() const { return degree() % 2; }

        // Assign &= operator to logical AND of two vectors.
        constexpr Vector_ &operator&=(const Vector_ &rhs_vec)
        {
            if (vector_size != rhs_vec.size())
                throw std::invalid_argument("Vectors must be of same size.");
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] &= rhs_vec.block_vector[block];
            return *this;
        }

        // Assign ^= operator to logical XOR of two vectors.
        constexpr Vector_ &operator^=(const Vector_ &rhs_vec)
        {
            if (vector_size != rhs_vec.size())
                throw std::invalid_argument("Vectors must be of same size.");
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] ^= rhs_vec.block_vector[block];
            return *this;
        }

        // Assign |= operator to logical OR of two vectors.
        constexpr Vector_ &operator|=(const Vector_ &rhs_vec)
        {
            if (vector_size != rhs_vec.size())
                throw std::invalid_argument("Vectors must be of same size.");
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] |= rhs_vec.block_vector[block];
            return *this;
        }

        // Return logical NOT of bit-vector.
        constexpr Vector_ &flipVector()
        {
            for (size_t block = 0; block < block_vector.size(); ++block)
                block_vector[block] = ~block_vector[block];
            cleanExcess();
            return *this;
        }

        // Assign ~ operator to logical NOT of bit-vector (Read-only).
        constexpr Vector_ operator~() const
        {
            Vector_ x(*this);
            x.flipVector();
            return x;
        }

        // Return dot-product with another vector.
        constexpr bool dot(const Vector_ &rhs_vec) const
        {
            if (vector_size != rhs_vec.size())
                throw std::invalid_argument("Vectors must be of same size.");

            // Use logical AND on entire blocks
            Block result = 0;
            for (size_t block = 0; block < block_vector.size(); ++block)
                result ^= block_vector[block] & rhs_vec.block_vector[block];

            // Parity of vector (degree % 2) is equivalent to XOR sum
            return std::popcount(result) % 2;
        }
    };

    using Vector = Vector_<>;

    // Assign & operator to logical AND of two vectors.
    // x = a & b    (Equivalent to x = a * b)
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator&(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b)
    {
        Vector_<Block, Allocator> x(a);
        x &= b;
        return x;
    }

    // Assign ^ operator to logical XOR of two vectors.
    // x = a ^ b    (Equivalent to x = a + b or x = a - b)
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator^(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b)
    {
        Vector_<Block, Allocator> x(a);
        x ^= b;
        return x;
    }

    // Assign | operator to logical OR of two vectors.
    // x = a | b
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator|(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b)
    {
        Vector_<Block, Allocator> x(a);
        x |= b;
        return x;
    }

    // Assign * operatir to dot-product of two vectors.
    // x = a * b
    template <std::unsigned_integral Block, typename Allocator>
    constexpr bool operator*(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b)
    {
        Vector_<Block, Allocator> x(a);
        return x.dot(b);
    }

    // Assign + operator to addition of two vectors.
    // x = a + b    (Equivalent to x = a ^ b)
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator+(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b) { return (a ^ b); }

    // Assign - operator to addition of two vectors.
    // x = a - b    (Equivalent to x = a ^ b)
    template <std::unsigned_integral Block, typename Allocator>
    constexpr Vector_<Block, Allocator> operator-(const Vector_<Block, Allocator> &a, const Vector_<Block, Allocator> &b) { return (a ^ b); }

}

#endif // GF2_VECTOR_H
