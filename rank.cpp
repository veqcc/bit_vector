#include <iostream>
#include <vector>
#include <random>
#include <bitset>

constexpr int n = 1000000;
std::bitset<n> bits;

constexpr int log2(int n) {
    int i = 1, cur = 1;
    for (; i < n; i++) {
        cur *= 2;
        if (cur >= n) break;
    }
    return i;
}

constexpr int pow2(int n) {
    int cur = 1;
    for (int i = 0; i < n; i++) {
        cur *= 2;
    }
    return cur;
}

constexpr int big_block_unit_size = log2(n) * log2(n);
constexpr int big_block_word_size = log2(n + 1);
constexpr int big_block_total_size = (n / big_block_unit_size + 1) * big_block_word_size;
std::bitset<big_block_total_size> big_block;

constexpr int small_block_unit_size = log2(n) / 2;
constexpr int small_block_word_size = log2(big_block_unit_size - small_block_unit_size + 1);
constexpr int small_block_total_size =
        (n / big_block_unit_size * (big_block_unit_size / small_block_unit_size + 1)) * small_block_word_size;
std::bitset<small_block_total_size> small_block;

constexpr int table_pattern_size = pow2(small_block_unit_size);
constexpr int table_word_size = log2(small_block_unit_size + 1);
constexpr int table_size = table_pattern_size * table_word_size;
std::bitset<table_size> table;

void rank1_naive(int idx) {
    int rank = 0;
    for (int i = 0; i < idx; i++) {
        rank += bits[i];
    }
    std::cout << "naive       : " << rank << std::endl;
}

void preprocess_big_block() {
    int rank = 0;
    for (int i = 1; i < n / big_block_unit_size; i++) {
        for (int j = 0; j < big_block_unit_size; j++) {
            rank += bits[(i - 1) * big_block_unit_size + j];
        }
        std::bitset<big_block_total_size> rank_bit(rank);
        big_block |= rank_bit << (i * big_block_word_size);
    }
}

void rank1_with_big_block(int idx) {
    std::bitset<big_block_total_size> mask(0);
    for (int i = 0; i < big_block_word_size; i++) {
        mask[i] = 1;
    }

    int rank = 0;
    rank += ((big_block >> (idx / big_block_unit_size * big_block_word_size)) & mask).to_ulong();

    for (int i = idx - (idx % big_block_unit_size); i < idx; i++) {
        rank += bits[i];
    }

    std::cout << "big block   : " << rank << std::endl;
}

void preprocess_small_block() {
    for (int i = 0; i < n / big_block_unit_size; i++) {
        int rank = 0;
        for (int j = 1; j < big_block_unit_size / small_block_unit_size; j++) {
            for (int k = 0; k < small_block_unit_size; k++) {
                rank += bits[i * big_block_unit_size + (j - 1) * small_block_unit_size + k];
            }
            std::bitset<small_block_total_size> rank_bit(rank);
            int shift = (i * (big_block_unit_size / small_block_unit_size + 1) + j) * small_block_word_size;
            small_block |= rank_bit << shift;
        }
    }
}

void rank1_with_small_block(int idx) {
    int rank = 0;

    std::bitset<small_block_total_size> mask_small_block(0);
    for (int i = 0; i < small_block_word_size; i++) {
        mask_small_block[i] = 1;
    }
    int shift = (idx / big_block_unit_size * (big_block_unit_size / small_block_unit_size + 1) + idx % big_block_unit_size / small_block_unit_size) * small_block_word_size;
    rank += ((small_block >> shift) & mask_small_block).to_ulong();

    std::bitset<big_block_total_size> mask_big_block(0);
    for (int i = 0; i < big_block_word_size; i++) {
        mask_big_block[i] = 1;
    }
    rank += ((big_block >> (idx / big_block_unit_size * big_block_word_size)) & mask_big_block).to_ulong();

    for (int i = idx - (idx % small_block_unit_size); i < idx; i++) {
        rank += bits[i];
    }

    std::cout << "small block : " << rank << std::endl;
}

void preprocess_lookup() {
    for (int i = 0; i < table_pattern_size; i++) {
        int rank = 0;
        for (int j = 0; j < small_block_unit_size; j++) {
            if (i & (1 << j)) rank++;
        }
        std::bitset<table_size> rank_bit(rank);
        int shift = i * table_word_size;
        table |= rank_bit << shift;
    }
}

void rank1_lookup(int idx) {
    int rank = 0;

    // big block
    std::bitset<big_block_total_size> mask_big_block(0);
    for (int i = 0; i < big_block_word_size; i++) {
        mask_big_block[i] = 1;
    }
    rank += ((big_block >> (idx / big_block_unit_size * big_block_word_size)) & mask_big_block).to_ulong();

    // small block
    std::bitset<small_block_total_size> mask_small_block(0);
    for (int i = 0; i < small_block_word_size; i++) {
        mask_small_block[i] = 1;
    }
    int shift = (idx / big_block_unit_size * (big_block_unit_size / small_block_unit_size + 1) + idx % big_block_unit_size / small_block_unit_size) * small_block_word_size;
    rank += ((small_block >> shift) & mask_small_block).to_ulong();

    // lookup table
    std::bitset<n> mask(0);
    for (int i = 0; i < idx % small_block_unit_size; i++) {
        mask[i] = 1;
    }
    int pattern = ((bits >> (idx - idx % small_block_unit_size)) & mask).to_ulong();
    // std::cout << pattern << std::endl;
    std::bitset<table_size> mask_table(0);
    for (int i = 0; i < table_word_size; i++) {
        mask_table[i] = 1;
    }
    shift = pattern * table_word_size;
    rank += ((table >> shift) & mask_table).to_ulong();

    std::cout << "lookup table: " << rank << std::endl;
}

int main() {
    std::random_device device;
    std::mt19937 generator(device());

    for (int i = 0; i < n; i++) {
        std::uniform_int_distribution<int> distribution(0, 1);
        bits[i] = distribution(generator);
    }

    preprocess_big_block();
    preprocess_small_block();
    preprocess_lookup();

    std::cout << "size of bit vector  : " << n << std::endl;
    std::cout << "size of big block   : " << big_block_total_size << std::endl;
    std::cout << "size of small block : " << small_block_total_size << std::endl;
    std::cout << "size of lookup table: " << table_size << std::endl;

    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> distribution(0, n - 1);
        int idx = distribution(generator);
        std::cout << "\nrange [0.." << idx << ')' << std::endl;

        rank1_naive(idx);
        rank1_with_big_block(idx);
        rank1_with_small_block(idx);
        rank1_lookup(idx);
    }

    return 0;
}