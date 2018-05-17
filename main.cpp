#include "CUSketch.h"
#include "UbSketch.h"

#include <ctime>
#include <set>
#include <unordered_map>
#include <iostream>
#include <ctime>
#include <time.h>

#define MAX_INSERT_PACKAGE 32000000
#define testcycles 10

unordered_map<uint32_t, int> ground_truth;
uint32_t insert_data[MAX_INSERT_PACKAGE];
uint32_t query_data[MAX_INSERT_PACKAGE];
int gsiz[20] = {0};
// 载入文件
int load_data(const char *filename) {
    FILE *pf = fopen(filename, "rb");
    if (!pf) {
        cerr << filename << " not found." << endl;
        exit(-1);
    }

    ground_truth.clear();

    char ip[8];
    int ret = 0; // total size
    // 注意数据输入的格式 每8字节代表一个uint32_t的数字
    while (fread(ip, 1, 8, pf)) {
        uint32_t key = *(uint32_t *) ip;
        insert_data[ret] = key;
        ground_truth[key]++;
        ret++;
        if (ret == MAX_INSERT_PACKAGE)
            break;
    }
    fclose(pf);

    int i = 0;
    for (auto itr: ground_truth) {
        query_data[i++] = itr.first;
    }

    //printf("Total stream size = %d\n", ret);
    //printf("Distinct item number = %d\n", ground_truth.size());

    int max_freq = 0;
    for (auto itr: ground_truth) {
        max_freq = std::max(max_freq, itr.second);
    }
    //printf("Max frequency = %d\n", max_freq);

    return ret;
}

void demo_cm_ub(int packet_num, const char *writefile)
{
    //printf("\nExp for frequency estimation:\n");
    //printf("\tAllocate 10KB memory for each algorithm\n");
    FILE *wf = fopen(writefile, "a");
    if (!wf) {
        cerr << writefile << "not found." << endl;
        exit(-1);
    }

    gsiz[0] = 1, gsiz[1] = 4, gsiz[2] = 8, gsiz[3] = 16;
     //---------------------insert-------------------------
    timespec time1, time2;
    long long resns;

    clock_gettime(CLOCK_MONOTONIC, &time1);
    for (int j = 0; j < testcycles; ++j){
        auto cu = new CUSketch<10 * 1024, 4>(); // 10*1024 = 10KB
        for (int i = 0; i < packet_num; ++i) {
            cu->insert(insert_data[i], 1);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_cu = (double)1000.0 * testcycles * packet_num / resns;
    fprintf(wf, "throughput of CU (insert): %.6lf Mips\n", throughput_cu);

    clock_gettime(CLOCK_MONOTONIC, &time1);

    for (int j = 0; j < testcycles; ++j){
        // memory_in_bytes, d, g
        auto ub = new UbSketch<10 * 1024, 4>();
        for (int i = 0; i < packet_num; ++i) {
            ub->insert(insert_data[i], 1);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    double throughput_ub = (double)1000.0 * testcycles * packet_num / resns;
    fprintf(wf, "throughput of Ub (insert): %.6lf Mips\n", throughput_ub);

    //---------------------query---------------------------
    clock_gettime(CLOCK_MONOTONIC, &time1);
    for (int j = 0; j < testcycles; ++j){
        auto cu = new CUSketch<10 * 1024, 4>(); // 10*1024 = 10KB
        for (auto itr: ground_truth) {
            cu->query(itr.first);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    throughput_cu = (double)1000.0 * testcycles * packet_num / resns;
    fprintf(wf, "throughput of CU (query): %.6lf Mips\n", throughput_cu);

    clock_gettime(CLOCK_MONOTONIC, &time1);

    for (int j = 0; j < testcycles; ++j){
        auto ub = new UbSketch<10 * 1024, 4>();
        for (auto itr: ground_truth) {
            ub->query(itr.first);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &time2);
    resns = (long long)(time2.tv_sec - time1.tv_sec) * 1000000000LL + (time2.tv_nsec - time1.tv_nsec);
    throughput_ub = (double)1000.0 * testcycles * packet_num / resns;
    fprintf(wf, "throughput of Ub (query): %.6lf Mips\n", throughput_ub);
    //----------------------accuracy----------------------
/*
    auto cu = new CUSketch<10 * 1024, 3>(); // 10*1024 = 10KB
    auto ub = new UbSketch<10 * 1024, 4>(); // 4 means 4 groups


    int tot_aae;
    double tot_are; 

    // build and query for cu
    for (int i = 0; i < packet_num; ++i) {
        cu->insert(insert_data[i], 1);
    }

    //cu->print_basic_info();
    tot_aae = 0;
    tot_are = 0;
    for (auto itr: ground_truth) {
        int report_val = cu->query(itr.first);
        tot_aae += abs(report_val - itr.second);
        tot_are += abs(report_val - itr.second) / (itr.second * 1.0);
    }

    fprintf(wf, "\tCU AAE: %lf\n", (double)tot_aae / ground_truth.size());
    fprintf(wf, "\tCU ARE: %lf\n", tot_are / ground_truth.size());

    // build and query for ub
    for (int i = 0; i < packet_num; ++i) {
        ub->insert(insert_data[i]);
    }
    //ub->print_basic_info();
    tot_aae = 0;
    tot_are = 0;
    for (auto itr: ground_truth) {
        int report_val = ub->query(itr.first);
        tot_aae += abs(report_val - itr.second);
        tot_are += abs(report_val - itr.second) / (itr.second * 1.0);
    }


    fprintf(wf, "\tUb AAE: %lf\n", (double)tot_aae / ground_truth.size());
    fprintf(wf, "\tUb ARE: %lf\n", tot_are / ground_truth.size());
*/
    wf.close();
}



int main()
{
    int packet_num = load_data("../data/sample.dat");
    demo_cm_ub(packet_num, "../bin/out.txt");
    //demo_ss(packet_num);
    //demo_flow_radar(packet_num);
    return 0;
}