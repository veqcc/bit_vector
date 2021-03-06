#include <iostream>
#include <random>
#include <vector>

using namespace std;

const int n = 1 << 16;
const int word_size = 8;
vector<uint8_t> bits(n / word_size);

const int block_count = 16 * 16 - 2;
const int tree_total_size = 1 + 4 + 16 + 64 + 256 + 1024 + 4096 + 4096 * 4;
vector<bool> is_big;
vector<uint16_t> ptr;
vector<uint16_t> original_idx;
vector<vector<uint16_t>> big_block;
vector<vector<uint8_t>> tree;

const int lookup_table_size = 1 << word_size;
vector<vector<uint8_t>> lookup_table;

int select1_naive(int idx) {
    int sum = -1, ans = -1;
    for (int i = 0; i < n; i++) {
        if (sum == idx) {
            ans = i;
            break;
        } else {
            if (bits[i / word_size] & (1 << (i % word_size))) {
                sum++;
            }
        }
    }
    return ans;
}

void preprocess() {
    int sum = 0, len = 0, big_idx = 0, small_idx = 0;
    for (int i = 0; i < n; i++) {
        if (bits[i / word_size] & (1 << (i % word_size))) {
            sum++;
        }
        len++;
        if (sum == block_count) {
            original_idx.push_back(i - len);
            if (len >= block_count * block_count) {
                is_big.push_back(true);
                ptr.push_back(big_idx);
                big_block.push_back({});
                int base = i - len;
                for (int j = 0; j < len; j++) {
                    if (bits[(base + j) / word_size] & (1 << ((base + j) % word_size))) {
                        big_block[big_idx].push_back(base + j);
                    }
                }
                big_idx++;
            } else {
                is_big.push_back(false);
                ptr.push_back(small_idx);
                tree.push_back(vector<uint8_t>(tree_total_size, 0));
                int base = i - len;
                int tree_leaf = 1 + 4 + 16 + 64 + 256 + 1024 + 4096;
                int s = 8;
                for (int j = 0; j <= len / s; j++) {
                    int cnt = 0;
                    for (int k = 0; k < s; k++) {
                        if (base + j * s + k > i) break;
                        if (bits[(base + j * s + k) / word_size] & (1 << ((base + j * s + k) % word_size))) {
                            cnt++;
                        }
                    }
                    int idx = tree_leaf + j;
                    while (true) {
                        tree[small_idx][idx] += cnt;
                        if (idx == 0) break;
                        idx = (idx - 1) / 4;
                    }
                }
                small_idx++;
            }
            sum = 0;
            len = 0;
        }
    }
}

void preprocess_lookup_table() {
    lookup_table.resize(lookup_table_size);
    for (int i = 0; i < lookup_table_size; i++) {
        int cur = 0;
        lookup_table[i].resize(word_size);
        for (int j = 0; j < word_size; j++) {
            if (i & (1 << j)) {
                lookup_table[i][cur] = j;
                cur++;
            }
        }
    }
}

int select1_efficient(int idx) {
    int block_idx = idx / block_count;
    idx %= block_count;
    if (idx == 0) {
        return original_idx[block_idx] - 1;
    }

    if (is_big[block_idx]) {
        int big_idx = ptr[block_idx];
        return big_block[big_idx][idx];
    } else {
        int tree_idx = ptr[block_idx];
        int leaf_idx = 0;
        int cur = -1;
        for (int i = 0; i < 7; i++) {
            for (int j = 1; j <= 4; j++) {
                int child = leaf_idx * 4 + j;
                if (tree[tree_idx][child] + cur <= idx) {
                    cur += tree[tree_idx][child];
                } else {
                    leaf_idx = child;
                    break;
                }
            }
        }

        int leaf_start = 1 + 4 + 16 + 64 + 256 + 1024 + 4096;

        int ans = 0;
        ans += original_idx[block_idx];
        ans += (leaf_idx - leaf_start) * word_size;

        int pos = original_idx[block_idx] + (leaf_idx - leaf_start) * word_size;
        while (cur <= idx) {
            if (bits[pos / word_size] & (1 << (pos % word_size))) {
                cur++;
            }
            pos++;
            ans++;
        }

        return ans;
    }
}

int main() {
    mt19937 generator(1234);

    for (int i = 0; i < n; i++) {
        uniform_int_distribution<int> distribution(0, 1 << 8);
        bits[i] = static_cast<uint8_t>(distribution(generator));
    }

    preprocess();
    preprocess_lookup_table();

    cout << "ptr:" << ptr.size() * 16 << endl;
    cout << "tree_total_size: " << tree_total_size << endl;
    cout << "big_block : " << big_block.size() << endl;
    cout << "tree : " << tree.size() << endl;

    for (int i = 0; i < 10; i++) {
        uniform_int_distribution<int> distribution(0, n - 1);
        auto idx = distribution(generator) / 5;
        cout << "\nselect " << idx << "th" << endl;

        cout << "naive:     " << select1_naive(idx) << endl;
        cout << "efficient: " << select1_efficient(idx) << endl;
    }

    return 0;
}