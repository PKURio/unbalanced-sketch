/*
    use different hash func
*/
#ifndef _UBSKETCH_H
#define _UBSKETCH_H

#include "params.h"
#include "BOBHash32.h"
#include "SPA.h"
#include <cstring>
#include <algorithm>

using namespace std;
#define UpperLimit(t) ((1 << gsiz[t]) - 1)

extern int gsiz[20]; // groupsize : 20 is enough

// g groups 
template <int memory_in_bytes, int g> 
class UbSketch: public SPA 
{
private:
    static constexpr int w = memory_in_bytes * 8 / (32 * g);
    // use 32bit to store
    uint32_t counters[g][w + 3]; // to be relatively prime
    int actual_counters_length[g];
    BOBHash32 * hash[g];
    int hashsiz[g];
public:
    UbSketch() {
        memset(counters, 0, sizeof(counters));
        for (int i = 0; i < g; ++i) // g <= 4
            actual_counters_length[i] = w + i;

        for (int j = 0; j < g; ++j)
        // each group has the same hash func
            hash[j] = new BOBHash32(j + 750);
        hashsiz[0] = 1, hashsiz[1] = 1, hashsiz[2] = 1, hashsiz[3] = 1;
    }

    void print_basic_info() {
        for (int j = 0; j < g; ++j)
            printf("gsiz[%d] = %d\n", j, gsiz[j]);
        printf("Ub sketch\n");
        for (int j = 0; j < g; ++j)
            printf("No.%d counters : %d\n", j, actual_counters_length[j] * 32 / gsiz[j]);
        printf("g = %d\n", g);
        printf("\tMemory: %.6lfKB\n", g * w * 4.0 / 1024);
    }

    virtual ~UbSketch() {
        for (int j = 0; j < g; ++j)
            delete hash[j];
    }

    inline void update(uint32_t bit_idx, int j){
        // int int_idx = bit_idx >> 5; // index for int
        // offset tells the offset in a 32-bit
        int offset = ((bit_idx % 32) / gsiz[j]) * gsiz[j];
        int val  = UpperLimit(j) & (counters[j][bit_idx >> 5] >> offset);

        // update
        counters[j][bit_idx >> 5] += val == UpperLimit(j) ? 0 : (1 << offset);
    }

    // f为变化量
    void insert(uint32_t key, int f = 1)
    {
        for (int j = g - 1; j >= 0; --j) {
            uint32_t bit_idx = 0; 

            bit_idx = (hash[j]->run((const char *)&key, 4)) % (actual_counters_length[j] << 5); // run计算hash值
            // update(bit_idx_array, j); // update
            int offset = ((bit_idx % 32) / gsiz[j]) * gsiz[j];
            int val  = UpperLimit(j) & (counters[j][bit_idx >> 5] >> offset);
            if (val != UpperLimit(j))
                counters[j][bit_idx >> 5] += (1 << offset);
            else 
                break;
        }
    }
    // for Delete
    inline int op1(uint32_t bit_idx, int j) {
        int offset = ((bit_idx % 32) / gsiz[j]) * gsiz[j];
        counters[j][bit_idx >> 5] -= (1 << offset);
        return UpperLimit(j) & (counters[j][bit_idx >> 5] >> offset);
    }

    void Delete(uint32_t key){
        // delete from the last group
        for (int j = g - 1; j >= 0; --j) {

            uint32_t bit_idx = (hash[j]->run((const char *)&key, 4)) % (actual_counters_length[j] << 5);
            int ret = op1(bit_idx, j);
            
            if(j > 0 && ret < UpperLimit(j - 1))
                continue;
            // if val is too big
            else
                break;
        }

    }
    // for query
    // inline int op2(uint32_t bit_idx, int j){
    //     // int int_idx = bit_idx >> 5;
    //     // int val = UpperLimit(j);
    //     int offset = ((bit_idx % 32) / gsiz[j]) * gsiz[j];
    //     // return (counters[j][int_idx] >> offset) & val;
    //     return (counters[j][bit_idx >> 5] >> offset) & (UpperLimit(j));
    // }
    //  查询是否存在
    int query(uint32_t key)
    {
        uint32_t ret = 0;
        for (int j = 0; j < g; ++j){
            uint32_t bit_idx = (hash[j]->run((const char *)&key, 4)) % (actual_counters_length[j] << 5);
            int offset = ((bit_idx % 32) / gsiz[j]) * gsiz[j];
            int ret = (counters[j][bit_idx >> 5] >> offset) & (UpperLimit(j));
            // int ret = op2(bit_idx, j);
            if(ret == UpperLimit(j)) // maybe not actual result
                continue;
            else
                return ret;
        }
        return ret;
    }
};

#endif //_UBSKETCH_H