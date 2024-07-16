// Copyright (c) 2022 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include <src/ir.h>

#include <WasmCounter/Exceptions.hpp>

namespace WasmCounter
{


template<typename _RetType>
inline _RetType ThrowUnimplementedFeature(wabt::ExprType exprType)
{
	throw Exception(
		"Unimplemented feature for expr type: " +
			std::string(wabt::GetExprTypeName(exprType))
	);
}


inline bool IsEffectiveControlFlowExpr(wabt::ExprType exprType)
{
	// TODO: double check these instruction types
	switch (exprType)
	{
	// TODO: check if these instruction has effect on the execution flow
	case wabt::ExprType::AtomicLoad:
	case wabt::ExprType::AtomicRmw:
	case wabt::ExprType::AtomicRmwCmpxchg:
	case wabt::ExprType::AtomicStore:
	case wabt::ExprType::AtomicNotify:
	case wabt::ExprType::AtomicFence:
	case wabt::ExprType::AtomicWait:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::Binary:
		return false;

	// control flow
	case wabt::ExprType::Block:
	case wabt::ExprType::Br:
	case wabt::ExprType::BrIf:
	case wabt::ExprType::BrTable:
		return true;

	// these ARE control flow expr, but it doesn't affect our block flow
	case wabt::ExprType::Call:
	case wabt::ExprType::CallIndirect:
	case wabt::ExprType::CallRef:
		return false;

	case wabt::ExprType::CodeMetadata:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::Compare:
	case wabt::ExprType::Const:
	case wabt::ExprType::Convert:
	case wabt::ExprType::Drop:
	case wabt::ExprType::GlobalGet:
	case wabt::ExprType::GlobalSet:
		return false;

	// control flow
	case wabt::ExprType::If:
		return true;

	// non-control flow
	case wabt::ExprType::Load:
	case wabt::ExprType::LocalGet:
	case wabt::ExprType::LocalSet:
	case wabt::ExprType::LocalTee:
		return false;

	// control flow
	case wabt::ExprType::Loop:
		return true;

	case wabt::ExprType::MemoryCopy:
	case wabt::ExprType::DataDrop:
	case wabt::ExprType::MemoryFill:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::MemoryGrow:
		return false;

	case wabt::ExprType::MemoryInit:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::MemorySize:
	case wabt::ExprType::Nop:
		return false;

	case wabt::ExprType::RefIsNull:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::RefFunc:
		return false;

	case wabt::ExprType::RefNull:
		return ThrowUnimplementedFeature<bool>(exprType);

	// TODO: check if these instruction has effect on the execution flow
	case wabt::ExprType::Rethrow:
		return ThrowUnimplementedFeature<bool>(exprType);

	// control flow
	case wabt::ExprType::Return:
		return true;

	// TODO: check if these instruction has effect on the execution flow
	case wabt::ExprType::ReturnCall:
	case wabt::ExprType::ReturnCallIndirect:
		return ThrowUnimplementedFeature<bool>(exprType);

	// select instruction doesn't affect the execution flow
	case wabt::ExprType::Select:
		return false;

	case wabt::ExprType::SimdLaneOp:
	case wabt::ExprType::SimdLoadLane:
	case wabt::ExprType::SimdStoreLane:
	case wabt::ExprType::SimdShuffleOp:
	case wabt::ExprType::LoadSplat:
	case wabt::ExprType::LoadZero:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::Store:
		return false;

	case wabt::ExprType::TableCopy:
	case wabt::ExprType::ElemDrop:
	case wabt::ExprType::TableInit:
	case wabt::ExprType::TableGet:
	case wabt::ExprType::TableGrow:
	case wabt::ExprType::TableSize:
	case wabt::ExprType::TableSet:
	case wabt::ExprType::TableFill:
	case wabt::ExprType::Ternary:
		return ThrowUnimplementedFeature<bool>(exprType);

	// TODO: check if these instruction has effect on the execution flow
	case wabt::ExprType::Throw:
	case wabt::ExprType::Try:
		return ThrowUnimplementedFeature<bool>(exprType);

	// non-control flow
	case wabt::ExprType::Unary:
	case wabt::ExprType::Unreachable:
		return false;

	default:
		return ThrowUnimplementedFeature<bool>(exprType);
	}
}

inline bool IsBlockLikeDecl(wabt::ExprType exprType)
{
	switch (exprType)
	{
	case wabt::ExprType::AtomicLoad:
	case wabt::ExprType::AtomicRmw:
	case wabt::ExprType::AtomicRmwCmpxchg:
	case wabt::ExprType::AtomicStore:
	case wabt::ExprType::AtomicNotify:
	case wabt::ExprType::AtomicFence:
	case wabt::ExprType::AtomicWait:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Binary:
		return false;

	// block of expressions
	case wabt::ExprType::Block:
		return true;

	case wabt::ExprType::Br:
	case wabt::ExprType::BrIf:
	case wabt::ExprType::BrTable:
	case wabt::ExprType::Call:
	case wabt::ExprType::CallIndirect:
	case wabt::ExprType::CallRef:
		return false;

	case wabt::ExprType::CodeMetadata:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Compare:
	case wabt::ExprType::Const:
	case wabt::ExprType::Convert:
	case wabt::ExprType::Drop:
	case wabt::ExprType::GlobalGet:
	case wabt::ExprType::GlobalSet:
		return false;

	// block of expressions
	case wabt::ExprType::If:
		return true;

	case wabt::ExprType::Load:
	case wabt::ExprType::LocalGet:
	case wabt::ExprType::LocalSet:
	case wabt::ExprType::LocalTee:
		return false;

	// block of expressions
	case wabt::ExprType::Loop:
		return true;

	case wabt::ExprType::MemoryCopy:
	case wabt::ExprType::DataDrop:
	case wabt::ExprType::MemoryFill:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::MemoryGrow:
		return false;

	case wabt::ExprType::MemoryInit:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::MemorySize:
	case wabt::ExprType::Nop:
		return false;

	case wabt::ExprType::RefIsNull:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::RefFunc:
		return false;

	case wabt::ExprType::RefNull:
	case wabt::ExprType::Rethrow:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Return:
		return false;

	case wabt::ExprType::ReturnCall:
	case wabt::ExprType::ReturnCallIndirect:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Select:
		return false;

	case wabt::ExprType::SimdLaneOp:
	case wabt::ExprType::SimdLoadLane:
	case wabt::ExprType::SimdStoreLane:
	case wabt::ExprType::SimdShuffleOp:
	case wabt::ExprType::LoadSplat:
	case wabt::ExprType::LoadZero:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Store:
		return false;

	case wabt::ExprType::TableCopy:
	case wabt::ExprType::ElemDrop:
	case wabt::ExprType::TableInit:
	case wabt::ExprType::TableGet:
	case wabt::ExprType::TableGrow:
	case wabt::ExprType::TableSize:
	case wabt::ExprType::TableSet:
	case wabt::ExprType::TableFill:
	case wabt::ExprType::Ternary:
	case wabt::ExprType::Throw:
	case wabt::ExprType::Try:
		return ThrowUnimplementedFeature<bool>(exprType);

	case wabt::ExprType::Unary:
	case wabt::ExprType::Unreachable:
		return false;

	default:
		return ThrowUnimplementedFeature<bool>(exprType);
	}
}

} // namespace WasmCounter
