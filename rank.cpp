#include <iostream>
#include <random>
#include <vector>

using namespace std;

const int n = 1 << 16;
const int word_size = 8;
vector<uint8_t> bits(n / word_size);

const int big_block_size = 16 * 16;
vector<uint16_t> big_block(n / big_block_size);

const int small_block_size = word_size;
vector<vector<uint8_t>> small_block(n / big_block_size, vector<uint8_t>(big_block_size / small_block_size));

const int lookup_table_size = 1 << word_size;
vector<vector<uint8_t>> lookup_table(lookup_table_size, vector<uint8_t>(word_size));

// time: O(N / lgN)
int rank1_naive(int idx) {
    int rank = 0;
    for (int i = 0; i < idx / word_size; i++) {
        rank += __builtin_popcount(bits[i]);
    }
    for (int i = 0; i < idx % word_size; i++) {
        if (bits[idx / word_size] & (1 << i)) {
            rank++;
        }
    }
    return rank;
}

void preprocess_big_block() {
    int rank = 0;
    for (int i = 0; i < n / big_block_size; i++) {
        big_block[i] = rank;
        for (int j = 0; j < big_block_size / word_size; j++) {
            rank += __builtin_popcount(bits[i * big_block_size / word_size + j]);
        }
    }
}

// time: O(lgN)
int rank1_with_big_block(int idx) {
    int rank = big_block[idx / big_block_size];

    int base = idx / big_block_size * big_block_size / word_size;
    for (int i = 0; i < idx % big_block_size / word_size; i++) {
        rank += __builtin_popcount(bits[base + i]);
    }
    for (int i = 0; i < idx % word_size; i++) {
        if (bits[idx / word_size] & (1 << i)) {
            rank++;
        }
    }
    return rank;
}

void preprocess_small_block() {
    for (int i = 0; i < n / big_block_size; i++) {
        int rank = 0;
        for (int j = 0; j < big_block_size / small_block_size; j++) {
            small_block[i][j] = rank;
            rank += __builtin_popcount(bits[i * big_block_size / word_size + j]);
        }
    }
}

// time: O(lgN)
int rank1_with_small_block(int idx) {
    int rank = big_block[idx / big_block_size];
    rank += small_block[idx / big_block_size][idx % big_block_size / small_block_size];
    for (int i = 0; i < idx % word_size; i++) {
        if (bits[idx / word_size] & (1 << i)) {
            rank++;
        }
    }
    return rank;
}

void preprocess_lookup_table() {
    for (int i = 0; i < lookup_table_size; i++) {
        int rank = 0;
        for (int j = 0; j < word_size; j++) {
            lookup_table[i][j] = rank;
            if (i & (1 << j)) {
                rank++;
            }
        }
    }
}

// time: O(1)
int rank1_with_lookup(int idx) {
    int rank = big_block[idx / big_block_size];
    rank += small_block[idx / big_block_size][idx % big_block_size / small_block_size];
    rank += lookup_table[bits[idx / word_size]][idx % word_size];
    return rank;
}

int main() {
    std::random_device device;
    std::mt19937 generator(device());

    for (int i = 0; i < n / 8; i++) {
        std::uniform_int_distribution<int> distribution(0, 1 << 8);
        bits[i] = static_cast<uint8_t>(distribution(generator));
    }

    preprocess_big_block();
    preprocess_small_block();
    preprocess_lookup_table();

    cout << "size of" << endl;
    cout << "  bit vector:   " << n  << endl;
    cout << "  big block:    " << n / big_block_size * 16 << endl;
    cout << "  small block:  " << n / small_block_size * 8 << endl;
    cout << "  lookup table: " << lookup_table_size * word_size * 8 << endl;

    for (int i = 0; i < 10; i++) {
        std::uniform_int_distribution<int> distribution(0, n - 1);
        auto idx = distribution(generator);
        std::cout << "\nrange [0.." << int(idx) << ')' << std::endl;

        cout << "naive:        " << rank1_naive(idx) << endl;
        cout << "big block:    " << rank1_with_big_block(idx) << endl;
        cout << "small block:  " << rank1_with_small_block(idx) << endl;
        cout << "lookup table: " << rank1_with_lookup(idx) << endl;
    }

    return 0;
}