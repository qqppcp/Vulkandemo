#pragma once
#include <vector>
#include <map>

struct Graph
{
	std::vector<std::map<std::uint32_t, std::int32_t>> g;
	
	void init(std::uint32_t n) { g.resize(n); }
	void add_node() { g.push_back({}); }
	void add_edge(std::uint32_t from, std::uint32_t to, std::int32_t cost)
	{
		g[from][to] = cost;
	}
	void increase_edge_cost(std::uint32_t from, std::uint32_t to, std::int32_t i_cost)
	{
		g[from][to] += i_cost;
	}
};

struct MetisGraph;

class Partitioner
{
	std::uint32_t bisect_graph(MetisGraph* graph_data, MetisGraph* child_graphs[2],
		std::uint32_t start, std::uint32_t end);
	void recursive_bisect_graph(MetisGraph* graoh_data, std::uint32_t start, std::uint32_t end);
public:
	void init(std::uint32_t num_node);
	void partition(const Graph& graph, std::uint32_t min_part_size, std::uint32_t max_part_size);
	std::vector<std::uint32_t> node_id; //将节点按划分编号排序
	std::vector<std::pair<std::uint32_t, std::uint32_t>> ranges; //分块的连续范围，范围内是相同划分
	std::vector<std::uint32_t> sort_to;
	std::uint32_t min_part_size;
	std::uint32_t max_part_size;
};