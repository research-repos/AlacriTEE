// Copyright (c) 2023 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <SimpleObjects/SimpleObjects.hpp>
#include <SimpleJson/SimpleJson.hpp>

#include "Block.hpp"


namespace WasmCounter
{


inline void AddBlock2Nodes(
	Block* block,
	SimpleObjects::Dict& nodes,
	bool isEntry = false
)
{
	if (block == nullptr)
	{
		return;
	}

	SimpleObjects::UInt64 bId(static_cast<uint64_t>(
		reinterpret_cast<uintptr_t>(block)
	));
	if (nodes.find(bId) != nodes.end())
	{
		// We have visited this block before
		return;
	}

	SimpleObjects::Dict node;
	node[SimpleObjects::String("isEntry")] =
		SimpleObjects::Bool(isEntry);
	node[SimpleObjects::String("weight")] =
		SimpleObjects::UInt64(static_cast<uint64_t>(block->m_weight));
	node[SimpleObjects::String("isLoopHead")] =
		SimpleObjects::Bool(block->m_isLoopHead);

	SimpleObjects::List children;
	for (size_t i = 0; i < block->m_children.size(); ++i)
	{
		const auto& child = block->m_children[i];

		children.push_back(
			SimpleObjects::UInt64(static_cast<uint64_t>(
				reinterpret_cast<uintptr_t>(child.m_ptr)
			))
		);
	}
	node[SimpleObjects::String("children")] = std::move(children);
	// !!! children is invalid after this point

	nodes[bId] = std::move(node);
	// !!! node is invalid after this point

	// Recursively add children
	for (const auto& child : block->m_children)
	{
		AddBlock2Nodes(child.m_ptr, nodes);
	}
}


inline SimpleObjects::Object Block2AdjacencyJson(
	const Graph& graph
)
{
	SimpleObjects::Dict json;
	json[SimpleObjects::String("funcName")] =
		SimpleObjects::String(graph.m_funcName);

	SimpleObjects::Dict nodes;

	AddBlock2Nodes(graph.m_head, nodes, true);

	json[SimpleObjects::String("nodes")] = std::move(nodes);
	// !!! nodes is invalid after this point

	return std::move(json);
}


} // namespace WasmCounter

