// Copyright (c) 2022 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include <memory>
#include <vector>

#include <src/cast.h>
#include <src/ir.h>

#include <WasmCounter/Exceptions.hpp>

#include "Block.hpp"
#include "Classification.hpp"
#include "make_unique.hpp"

namespace WasmCounter
{

inline BrType CheckContBlockBrType(
	const std::vector<BrBinding>& scopeStack,
	size_t contBlockLvl
)
{
	// search backwards
	bool passLoop = false;
	size_t idx = contBlockLvl;
	for (
		auto it = scopeStack.rbegin();
		(it != scopeStack.rend()) && (idx < scopeStack.size());
		++it, ++idx
	)
	{
		passLoop = passLoop ||
			((it->m_dest.m_blk != nullptr) && (it->m_dest.m_blk->m_isLoopHead));
	}

	return passLoop ? BrType::OutOfLoop : BrType::Normal;
}

inline BlockChild CreateBlockChildByBrDest(
	const std::vector<BrBinding>& scopeStack,
	const BrBinding& brBinding,
	bool passLoop
)
{
	if (brBinding.m_dest.m_blk == nullptr)
	{
		// the head is the end of the func, the destination is empty
		BrType brType = passLoop ? BrType::OutOfLoop : BrType::Normal;
		BrType cntType = BrType::Normal;
		return BlockChild(brType, cntType, nullptr);
	}
	else
	{
		BrType brType = (brBinding.m_dest.m_blk->m_isLoopHead) ? BrType::IntoLoop :
			(passLoop ? BrType::OutOfLoop : BrType::Normal);
		BrType cntType = (brBinding.m_dest.m_blk->m_isLoopHead) ? BrType::IntoLoop :
			CheckContBlockBrType(scopeStack, brBinding.m_dest.m_blkLvl);

		return BlockChild(brType, cntType, brBinding.m_dest.m_blk);
	}
}

inline BlockChild FindBrDestination(
	const std::vector<BrBinding>& scopeStack,
	const wabt::Index& var
)
{
	bool passLoop = false;

	wabt::Index idx = var;

	for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it, --idx)
	{
		if (idx == 0)
		{
			return CreateBlockChildByBrDest(scopeStack, *it, passLoop);
		}

		passLoop = passLoop || (it->m_dest.m_blk->m_isLoopHead);
	}

	throw Exception("Branch to an index that is out of range");
}

inline BlockChild FindBrDestination(
	const std::vector<BrBinding>& scopeStack,
	const std::string& name
)
{
	bool passLoop = false;

	for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it)
	{
		if (it->m_name == name)
		{
			return CreateBlockChildByBrDest(scopeStack, *it, passLoop);
		}

		passLoop = passLoop || (it->m_dest.m_blk->m_isLoopHead);
	}

	throw Exception("Branch to an name that is not found");
}

inline BlockChild FindBrDestination(
	const std::vector<BrBinding>& scopeStack,
	const wabt::Var& var
)
{
	if (var.is_index())
	{
		return FindBrDestination(scopeStack, var.index());
	}
	else if (var.is_name())
	{
		return FindBrDestination(scopeStack, var.name());
	}
	else
	{
		throw Exception("Unkown var type");
	}
}

/**
 * @brief Add a continuous block (head) as a child of the given block
 *
 * @param blkPtr
 * @param scopeStack
 * @param contBlock
 */
inline void AddContBlockAsChild(
	BrType brType,
	Block* blkPtr,
	const std::vector<BrBinding>& scopeStack,
	const BrDest& contBlock
)
{
	blkPtr->AddChild(
		BlockChild(
			brType,
			CheckContBlockBrType(scopeStack, contBlock.m_blkLvl),
			contBlock.m_blk
		)
	);
}

Block* GenerateGraph(
	BlockType blkType,
	wabt::ExprList& exprList,
	BlockStorage& storage,
	std::vector<BrBinding>& scopeStack,
	const BrDest& contBlock
);

void GenerateGraphForIf(
	std::unique_ptr<Block>& blk,
	BlockStorage& storage,
	std::vector<BrBinding>& scopeStack,
	BrDest& head
)
{
	// utilize the blk as a dummy block, which has two children
	// one for the if body, one for the else body
	blk->m_type = BlockType::If;
	Block* ifBlkPtr = blk.get();
	wabt::IfExpr* ifExpr =
		wabt::cast<wabt::IfExpr>(&(*(blk->m_blkBegin)));
	const std::string& ifBlkLabel = ifExpr->true_.label;
	wabt::ExprList& thenBlkExprList = ifExpr->true_.exprs;
	wabt::ExprList& elseBlkExprList = ifExpr->false_;

	// keep the dummy if block
	storage.Append(std::move(blk));

	// Get the continuation block (head) of the if block
	BrDest thenDest = head;
	// for if block, br/br_if 0 works as if the regular `block` expr
	scopeStack.emplace_back(ifBlkLabel, static_cast<const BrDest&>(head));
	thenDest.m_blk = GenerateGraph(
		BlockType::IfThen,
		thenBlkExprList,
		storage,
		scopeStack,
		head
	);
	scopeStack.pop_back();
	if (thenDest.m_blk != head.m_blk)
	{
		// head is updated
		thenDest.m_blkLvl = scopeStack.size();
	}

	// Get the continuation block (head) of the else block
	BrDest elseDest = head;
	// for if block, br/br_if 0 works as if the regular `block` expr
	scopeStack.emplace_back(ifBlkLabel, static_cast<const BrDest&>(head));
	elseDest.m_blk = GenerateGraph(
		BlockType::IfElse,
		elseBlkExprList,
		storage,
		scopeStack,
		head
	);
	scopeStack.pop_back();
	if (elseDest.m_blk != head.m_blk)
	{
		// head is updated
		elseDest.m_blkLvl = scopeStack.size();
	}

	// Add them as children of the dummy if block
	AddContBlockAsChild(BrType::Normal, ifBlkPtr, scopeStack, thenDest);
	AddContBlockAsChild(BrType::Normal, ifBlkPtr, scopeStack, elseDest);

	// make this dummy if block as the continuation block (head) of the outter
	// layer
	head.m_blk = ifBlkPtr;
	head.m_blkLvl = scopeStack.size();
}

void GenerateGraphForBlock(
	std::unique_ptr<Block>& blk,
	BlockStorage& storage,
	std::vector<BrBinding>& scopeStack,
	BrDest& head
)
{
	wabt::BlockExpr* blkExpr =
		wabt::cast<wabt::BlockExpr>(&(*(blk->m_blkBegin)));
	wabt::ExprList& blkExprList = blkExpr->block.exprs;
	const std::string& blkLabel = blkExpr->block.label;

	// For block, br/br_if 0 should points to head
	scopeStack.emplace_back(blkLabel, static_cast<const BrDest&>(head));

	Block* tmpHead = GenerateGraph(
		BlockType::Block,
		blkExprList,
		storage,
		scopeStack,
		head
	);

	scopeStack.pop_back();

	if (tmpHead != head.m_blk)
	{
		// head is updated
		head.m_blk = tmpHead;
		head.m_blkLvl = scopeStack.size();
	}
}

void GenerateGraphForLoop(
	std::unique_ptr<Block>& blk,
	BlockStorage& storage,
	std::vector<BrBinding>& scopeStack,
	BrDest& head
)
{
	wabt::LoopExpr* lpExpr =
		wabt::cast<wabt::LoopExpr>(&(*(blk->m_blkBegin)));
	wabt::ExprList& lpExprList = lpExpr->block.exprs;
	const std::string& blkLabel = lpExpr->block.label;

	// For loop, br/br_if 0 should points to loop itself
	blk->m_isLoopHead = true;
	Block* lpPtr = blk.get();
	storage.Append(std::move(blk)); // keep this loop block
	// !!! blk is invalid after this point !!!
	scopeStack.emplace_back(blkLabel, lpPtr, scopeStack.size());

	Block* tmpHead = GenerateGraph(
		BlockType::Loop,
		lpExprList,
		storage,
		scopeStack,
		head
	);

	scopeStack.pop_back();

	if (tmpHead != head.m_blk)
	{
		// head is updated
		head.m_blk = tmpHead;
		head.m_blkLvl = scopeStack.size();

		// the loop body is not empty
		// add the loop body as a child of the loop head
		lpPtr->AddChild(
			BlockChild(
				BrType::Normal,
				BrType::Normal,
				tmpHead
			)
		);
	}
}

// Returns the head block of the given expr
// We only return 1 pointer to head block,
// since is only one entry point to Func/Block/Loop
inline Block* GenerateGraph(
	BlockType blkType,
	wabt::ExprList& exprList,
	BlockStorage& storage,
	std::vector<BrBinding>& scopeStack,
	const BrDest& contBlock
)
{
	auto exprBegin = exprList.begin();
	auto exprEnd = exprList.end();

	// # create a stack of blocks, where the last block is on the top
	//   so it will be processed first
	std::vector<std::unique_ptr<Block> > blockStack;

	// 1st iteration, from top to bottom
	auto it = exprBegin;
	while (it != exprEnd)
	{
		// Create new block
		std::unique_ptr<Block> blk =
			Internal::make_unique<Block>(blkType, exprList, it);

		// expand block
		blk->ExpandBlock();

		// Set the next begining to the end of this block
		it = blk->m_blkEnd;

		if (!blk->IsEmpty())
		{
			// We need to keep the block if:
			// !isEmpty || (!notEnd && isEffectiveCtrlFlow)
			blockStack.emplace_back(std::move(blk));
		}
	}

	// 2nd iteration, from bottom to top
	// Work from the stack top
	// check blkBegin is block/loop
	// -> if so, recursive call
	// -> if not, check blkEnd is br/br_if/return
	// -> -> if so, connect child
	//       (return has 0 child, br has 1, br_if has 2)
	// -> -> if not,

	// This should point to the block that we should flow to
	// when all current expr are executed
	BrDest head(contBlock.m_blk, contBlock.m_blkLvl);

	while (blockStack.size() > 0)
	{
		std::unique_ptr<Block> blk = std::move(blockStack.back());
		blockStack.pop_back();

		if (IsBlockLikeDecl(blk->m_blkFstExprType))
		{
			// It is some type of block
			switch (blk->m_blkFstExprType)
			{
			case wabt::ExprType::If:
				GenerateGraphForIf(
					blk,
					storage,
					scopeStack,
					head
				);
				break;
			case wabt::ExprType::Block:
				GenerateGraphForBlock(
					blk,
					storage,
					scopeStack,
					head
				);
				break;
			case wabt::ExprType::Loop:
				GenerateGraphForLoop(
					blk,
					storage,
					scopeStack,
					head
				);
				break;
			default:
				throw Exception("Unimplemented feature");
			}
		}
		else
		{
			// It is a list of expr
			if (blk->IsEmpty())
			{
				// empty block, skip
			}
			else
			{
				auto lastExpr = blk->GetBlkLastExpr(1);
				if (IsEffectiveControlFlowExpr(lastExpr->type()))
				{
					// block ends with control flow expr

					// Keep this block
					Block* blkPtr = blk.get();
					storage.Append(std::move(blk));

					switch (lastExpr->type())
					{
					case wabt::ExprType::Br:
					{
						// br always jump, so there is only 1 child
						const wabt::BrExpr* brExpr =
							wabt::cast<const wabt::BrExpr>(&(*lastExpr));

						// set up block flow link
						blkPtr->AddChild(
							FindBrDestination(scopeStack, brExpr->var)
						);

						head.m_blk = blkPtr;
						head.m_blkLvl = scopeStack.size();
						break;
					}
					case wabt::ExprType::BrIf:
					{
						// br_if may/may not jump, so there are 2 children
						const wabt::BrIfExpr* brExpr =
							wabt::cast<const wabt::BrIfExpr>(&(*lastExpr));

						// set up block flow link
						// -> flow when jump
						blkPtr->AddChild(
							FindBrDestination(scopeStack, brExpr->var)
						);
						// -> flow when not jump
						AddContBlockAsChild(BrType::Normal, blkPtr, scopeStack, head);

						head.m_blk = blkPtr;
						head.m_blkLvl = scopeStack.size();
						break;
					}
					case wabt::ExprType::BrTable:
					{
						// br_table always jump, the number of children depends
						// on the number of targets, but it's always >= 1
						const wabt::BrTableExpr* brTabExpr =
							wabt::cast<const wabt::BrTableExpr>(&(*lastExpr));

						// Iterate through all targets
						for (const auto& var : brTabExpr->targets)
						{
							blkPtr->AddChild(
								FindBrDestination(scopeStack, var)
							);
						}

						// Add default target
						blkPtr->AddChild(
							FindBrDestination(scopeStack, brTabExpr->default_target)
						);

						head.m_blk = blkPtr;
						head.m_blkLvl = scopeStack.size();
						break;
					}
					case wabt::ExprType::Return:
					{
						// return directly terminate the func,
						// so the child should be the very last head
						// (nullptr in current implementation)
						blkPtr ->AddChild(
							BlockChild(
								BrType::Normal,
								BrType::Normal,
								nullptr
							)
						);

						head.m_blk = blkPtr;
						head.m_blkLvl = scopeStack.size();
						break;
					}
					default:
						throw Exception("Unimplemented feature");
					}
				}
				else
				{
					// block ends with non-control-flow expr

					// Keep this block
					Block* blkPtr = blk.get();
					storage.Append(std::move(blk));

					// set up block flow link
					// -> flow to the previous head
					AddContBlockAsChild(BrType::Normal, blkPtr, scopeStack, head);

					head.m_blk = blkPtr;
					head.m_blkLvl = scopeStack.size();
				}
			}
		}
	}

	return head.m_blk;
}

inline std::unique_ptr<Graph> GenerateGraph(wabt::Func& func)
{
	std::unique_ptr<Graph> gr = Internal::make_unique<Graph>(func.name);

	BrDest endBlock(
		nullptr, // there is no blocks after all exprs in this func
		0        // continuous block level 0 i.e., it's outer most level
	);

	std::vector<BrBinding> scopeStack;
	gr->m_head = GenerateGraph(
		BlockType::Func,
		func.exprs, // expr list
		gr->m_storage, // Storage to keep blocks
		scopeStack, // scope stack for nested blocks/loops
		endBlock
	);

	return gr;
}

} // namespace WasmCounter
