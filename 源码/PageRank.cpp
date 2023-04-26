#include "PageRank.h"
#include <vector>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <mutex>

#define MULTITHREAD

void PageRank::process_data()
{
    if (block_num == 1)
    {
        std::ifstream input;
        input.open(input_file);
        while (!input.eof())
        {
            int from_node, to_node;
            input >> from_node >> to_node;
            // std::cout << from_node << ' ' << to_node << std::endl;
            graph[from_node].insert(to_node);
            graph[to_node];
        }
        input.close();
    }
    else
    {
        /**
         * TODO()
         */
    }
}

#ifdef MULTITHREAD

std::map<int, std::mutex> node2mutex;

static void update_rank(std::unordered_map<int, std::unordered_set<int>> &graph,
                        std::unordered_map<int, rank_type> &old_rank,
                        std::unordered_map<int, rank_type> &new_rank,
                        int thread_id, int all_threads, double beta)
{
    /* 计算每个线程处理的矩阵的范围 */
    int step = graph.size() / all_threads;
    int begin = thread_id * step;
    int end = ((thread_id == all_threads - 1) ? graph.size() : begin + step);
    auto begin_it = graph.begin();
    std::advance(begin_it, begin);
    auto end_it = graph.begin();
    std::advance(end_it, end);

    /* 先把rank加到一个局部map */
    std::unordered_map<int, rank_type> next_rank;
    for (begin_it; begin_it != end_it; begin_it++)
    {
        int from_node = (*begin_it).first;
        int out_degree = (*begin_it).second.size();
        for (auto to_node : (*begin_it).second)
        {
            next_rank[to_node] += beta * (old_rank[from_node] / out_degree);
        }
    }

    /* 把rank合并到全局map */
    for (auto &[node, rank] : next_rank)
    {
        node2mutex[node].lock();
        new_rank[node] += rank;
        node2mutex[node].unlock();
    }
}

#endif

void PageRank::iter(std::unordered_map<int, rank_type> &next_rank)
{
    if (thread_num == 1)
    {
        for (auto &[from_node, to_nodes] : graph)
        {
            int out_degree = to_nodes.size();
            for (auto to_node : to_nodes)
            {
                next_rank[to_node] += beta * (node_rank[from_node] / out_degree);
            }
        }
    }
    else if (thread_num > 0)
    {
        std::vector<std::thread *> threads;
        threads.resize(thread_num);
        for (int thread_id = 0; thread_id < thread_num; thread_id++)
        {
            threads[thread_id] = new std::thread(update_rank, std::ref(graph), std::ref(node_rank), std::ref(next_rank), thread_id, thread_num, beta);
        }
        for (int thread_id = 0; thread_id < thread_num; thread_id++)
        {
            threads[thread_id]->join();
            delete threads[thread_id];
        }
    }
    else
    {
        std::cout << "thread num illegal" << std::endl;
        exit(-1);
    }
}

void PageRank::page_rank()
{
    int nr_node = graph.size();
    /* 初始时每个节点的rank值为1/N */
    for (auto &[from_node, to_nodes] : graph)
    {
        node_rank[from_node] = 1.0 / nr_node;
#ifdef MULTITHREAD
        node2mutex[from_node];
#endif
    }
    std::unordered_map<int, rank_type> next_rank;
    for (int i = 0; i < iteration; i++)
    {
        /* next_ran清零 */
        for (auto &[node, rank] : node_rank)
            next_rank[node] = 0;

        if (block_num == 1)
        {
            /* 更新rank */
            iter(next_rank);
        }
        else
        {
            /**
             * TODO()
             */
        }
        /* 远程传播 */
        rank_type sum = 0, inc = 0;
        for (auto &[node, rank] : next_rank)
            sum += rank;
        inc = (1 - sum) / nr_node;
        for (auto &[node, rank] : next_rank)
            rank += inc;
        node_rank = next_rank;
        std::cout << "iteration: " << i + 1 << " over" << std::endl;
    }
}

void PageRank::save_result()
{
    /* 先把结果都排个序 */
    auto cmp = [](std::pair<int, rank_type> &a, std::pair<int, rank_type> &b)
    {
        if (a.second == b.second)
            return a.first < b.first;
        return a.second > b.second;
    };
    std::vector<std::pair<int, rank_type>> res_rank(node_rank.begin(), node_rank.end());
    std::sort(res_rank.begin(), res_rank.end(), cmp);

    /* 存到文件里 */
    int order = 1;
    std::ofstream output;
    output.open(output_file);
    for (auto &[node, rank] : res_rank)
    {
        if (order > topn)
            break;
        output << std::setw(4) << node << ": " << std::setprecision(10) << rank << std::endl;
        order++;
    }
    output.close();
}

PageRank::PageRank(double beta, int block_num, int iteration,
                   int topn, int thread_num,
                   std::string input_file,
                   std::string output_file)
    : beta(beta), block_num(block_num), iteration(iteration),
      topn(topn), thread_num(thread_num),
      input_file(input_file), output_file(output_file)
{
}

void PageRank::process()
{
    process_data();
    auto t1 = std::chrono::high_resolution_clock::now();
    page_rank();
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> fp_ms = t2 - t1;
    std::cout << "Page rank cal time: " << fp_ms.count() << "ms" << std::endl;
    save_result();
}
