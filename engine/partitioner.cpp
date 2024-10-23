#include "partitioner.h"
#include <cassert>
#include <algorithm>
#define IDXTYPEWIDTH 32
#define REALTYPEWIDTH 32
#include "metis.h"

struct MetisGraph {
    idx_t nvtxs;
    std::vector<idx_t> xadj;
    std::vector<idx_t> adjncy; //压缩图表示
    std::vector<idx_t> adjwgt; //边权重
};

void Partitioner::init(std::uint32_t num_node)
{
    node_id.resize(num_node);
    sort_to.resize(num_node);
    std::uint32_t i = 0;
    for (auto& x : node_id)
    {
        x = i, i++;
    }
    for (auto& x : sort_to)
    {
        x = i, i++;
    }
}

MetisGraph* to_metis_data(const Graph& graph)
{
    MetisGraph* g = new MetisGraph;
    g->nvtxs = graph.g.size();
    for (auto& mp : graph.g)
    {
        g->xadj.push_back(g->adjncy.size());
        for (auto [to, cost] : mp)
        {
            g->adjncy.push_back(to);
            g->adjwgt.push_back(cost);
        }
    }
    g->xadj.push_back(g->adjncy.size());
    return g;
}

std::uint32_t Partitioner::bisect_graph(MetisGraph* graph_data, MetisGraph* child_graphs[2],
    std::uint32_t start, std::uint32_t end)
{
    assert(end - start == graph_data->nvtxs);

    if (graph_data->nvtxs <= max_part_size)
    {
        ranges.push_back({ start, end });
        return end;
    }
    const std::uint32_t exp_part_size = (min_part_size + max_part_size) / 2;
    const std::uint32_t exp_num_parts = std::max(2u, (graph_data->nvtxs + exp_part_size - 1) / exp_part_size);

    std::vector<idx_t> swap_to(graph_data->nvtxs);
    std::vector<idx_t> part(graph_data->nvtxs);

    idx_t nw = 1, npart = 2, ncut = 0;
    real_t part_weight[] = {
        float(exp_num_parts >> 1) / exp_num_parts,
        1.0f - float(exp_num_parts >> 1) / exp_num_parts
    };
    int res = METIS_PartGraphRecursive(
        &graph_data->nvtxs,
        &nw,
        graph_data->xadj.data(),
        graph_data->adjncy.data(),
        nullptr, //vert weights
        nullptr, //vert size
        graph_data->adjwgt.data(),
        &npart,
        part_weight, //partition weight
        nullptr,
        nullptr, //options
        &ncut,
        part.data()
    );
    assert(res == METIS_OK);

    std::int32_t l = 0, r = graph_data->nvtxs - 1;
    while (l <= r)
    {
        while (l <= r && part[l] == 0) swap_to[l] = l, l++;
        while (l <= r && part[l] == 1) swap_to[r] = r, r--;
        if (l < r)
        {
            std::swap(node_id[start + l], node_id[start + r]);
            swap_to[l] = r, swap_to[r] = l;
            l++, r--;
        }
    }
    std::int32_t split = l;

    std::int32_t size[2] = { split, graph_data->nvtxs - split };
    assert(size[0] >= 1 && size[1] >= 1);

    if (size[0] <= max_part_size && size[1] <= max_part_size)
    {
        ranges.push_back({ start, start + split });
        ranges.push_back({ start + split, end });
    }
    else
    {
        for (size_t i = 0; i < 2; i++)
        {
            child_graphs[i] = new MetisGraph;
            child_graphs[i]->adjncy.reserve(graph_data->adjncy.size() >> 1);
            child_graphs[i]->adjwgt.reserve(graph_data->adjwgt.size() >> 1);
            child_graphs[i]->xadj.reserve(size[i] + 1);
            child_graphs[i]->nvtxs = size[i];
        }
        for (size_t i = 0; i < graph_data->nvtxs; i++)
        {
            std::uint32_t is_rs = (i >= child_graphs[0]->nvtxs);
            std::uint32_t u = swap_to[i];
            MetisGraph* ch = child_graphs[is_rs];
            ch->xadj.push_back(ch->adjncy.size());
            for (size_t j = graph_data->xadj[u]; j < graph_data->xadj[u + 1]; j++)
            {
                idx_t v = graph_data->adjncy[j];
                idx_t w = graph_data->adjwgt[j];
                v = swap_to[v] - (is_rs ? size[0] : 0);
                if (v >= 0 && v < size[is_rs])
                {
                    ch->adjncy.push_back(v);
                    ch->adjwgt.push_back(w);
                }
            }
        }
        child_graphs[0]->xadj.push_back(child_graphs[0]->adjncy.size());
        child_graphs[1]->xadj.push_back(child_graphs[1]->adjncy.size());
    }
    return start + split;
}

void Partitioner::recursive_bisect_graph(MetisGraph* graph_data,
    std::uint32_t start, std::uint32_t end)
{
    MetisGraph* child_graph[2] = { 0 };
    std::uint32_t split = bisect_graph(graph_data, child_graph, start, end);
    delete graph_data;

    if (child_graph[0] && child_graph[1])
    {
        recursive_bisect_graph(child_graph[0], start, split);
        recursive_bisect_graph(child_graph[1], split, end);
    }
    else
    {
        assert(!child_graph[0] && !child_graph[1]);
    }
}

void Partitioner::partition(const Graph& graph, std::uint32_t min_part_size,
    std::uint32_t max_part_size)
{
    init(graph.g.size());
    this->min_part_size = min_part_size;
    this->max_part_size = max_part_size;
    MetisGraph* graph_data = to_metis_data(graph);
    recursive_bisect_graph(graph_data, 0, graph_data->nvtxs);
    std::sort(ranges.begin(), ranges.end());
    for (size_t i = 0; i < node_id.size(); i++)
    {
        sort_to[node_id[i]] = i;
    }
}