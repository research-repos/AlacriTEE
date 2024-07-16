// Copyright (c) 2022 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once

#include <memory>
#include <vector>

#include <src/ir.h>
#include <src/cast.h>

#include <WasmCounter/Exceptions.hpp>

#include "Block.hpp"
#include "Classification.hpp"
#include "ExprIterater.hpp"
#include "make_unique.hpp"
#include "Utils.hpp"

namespace WasmCounter
{

struct InjectedSymbolInfo
{
	InjectedSymbolInfo() :
		m_thrId(),
		m_thrVar(wabt::Index(m_thrId)),
		m_ctrId(),
		m_ctrVar(wabt::Index(m_ctrId)),
		m_wrapFuncId(),
		m_wrapFuncVar(wabt::Index(m_wrapFuncId)),
		m_exceedFuncId(),
		m_exceedFuncVar(wabt::Index(m_exceedFuncId)),
		m_funcIncrId()
	{}

	void SetThresholdId(size_t id)
	{
		m_thrId = id;
		m_thrVar = wabt::Var(wabt::Index(m_thrId));
	}

	void SetCounterId(size_t id)
	{
		m_ctrId = id;
		m_ctrVar = wabt::Var(wabt::Index(m_ctrId));
	}

	void SetWrapFuncId(size_t id)
	{
		m_wrapFuncId = id;
		m_wrapFuncVar = wabt::Var(wabt::Index(m_wrapFuncId));
	}

	void SetExceedFuncId(size_t id)
	{
		m_exceedFuncId = id;
		m_exceedFuncVar = wabt::Var(wabt::Index(m_exceedFuncId));
	}

	size_t m_thrId;
	wabt::Var m_thrVar;

	size_t m_ctrId;
	wabt::Var m_ctrVar;

	size_t m_wrapFuncId;
	wabt::Var m_wrapFuncVar;

	size_t m_exceedFuncId;
	wabt::Var m_exceedFuncVar;

	size_t m_funcIncrId;
}; // struct InjectedSymbolInfo

inline bool IsFuncTypeFieldExist(
	const wabt::FuncSignature& sig,
	const std::vector<wabt::TypeEntry*>& types
)
{
	for (const auto& t : types)
	{
		if (t->kind() == wabt::TypeEntryKind::Func)
		{
			const wabt::FuncType* ft =
				wabt::cast<const wabt::FuncType>(t);

			if (ft->sig == sig)
			{
				return true;
			}
		}
	}
	return false;
}

inline void AddFuncTypeIfNotExist(
	wabt::Module& mod,
	const wabt::FuncSignature& sig
)
{
	if (!IsFuncTypeFieldExist(sig, mod.types))
	{
		std::unique_ptr<wabt::TypeModuleField> typeField =
			Internal::make_unique<wabt::TypeModuleField>();
		std::unique_ptr<wabt::FuncType> funcType =
			Internal::make_unique<wabt::FuncType>();

		funcType->sig = sig;

		typeField->type = std::move(funcType);
		mod.AppendField(std::move(typeField));
	}
}


inline void FixExceedFuncDeclare(
	wabt::Module& mod,
	InjectedSymbolInfo& info
)
{
	const std::string& modName = "env";
	const std::string& funcImpName = "enclave_wasm_counter_exceed";
	const std::string& fullImpName = modName + "." + funcImpName;

	// 1. find the import function
	wabt::FuncImport* funcImp =
		FindImport<wabt::ExternalKind::Func>::Find(mod, modName, funcImpName);
	if (funcImp == nullptr)
	{
		throw Exception("Couldn't find import to " + fullImpName + " function");
	}
	wabt::Func& func = funcImp->func;

	// 2. fix declaration
	// 2.1. not using function type
	func.decl.has_func_type = false;
	// 2.2. no parameters
	func.decl.sig.param_types.clear();
	func.decl.sig.param_type_names.clear();
	// 2.3. no return value
	func.decl.sig.result_types.clear();
	func.decl.sig.result_type_names.clear();

	// 3. clear up or throw for other unusual fields
	if (func.local_types.size() > 0)
	{
		throw Exception(
			"The import to " + fullImpName + " function contains local types"
		);
	}
	func.bindings.clear();
	func.exprs.clear();

	// 4. add function type if not exist
	AddFuncTypeIfNotExist(mod, func.decl.sig);

	// 5. find the index of the function
	info.SetExceedFuncId(FindFuncIdx(mod, &func));
}


inline void InjectExport(
	wabt::Module& mod,
	const std::string& name,
	wabt::ExternalKind kind,
	const wabt::Var& var
)
{
	std::unique_ptr<wabt::ExportModuleField> exp =
		Internal::make_unique<wabt::ExportModuleField>();
	exp->export_.name = name;
	exp->export_.kind = kind;
	exp->export_.var = var;

	mod.AppendField(std::move(exp));
}


enum class WabtType
{
	I32,
	I64,
}; // enum class WabtType


template<WabtType _WabtType>
struct WabtTypeTraits;

template<>
struct WabtTypeTraits<WabtType::I64>
{
	using PrimativeType = uint64_t;

	static constexpr wabt::Type::Enum sk_wabtType = wabt::Type::I64;

	static wabt::Const ToConst(PrimativeType val)
	{
		return wabt::Const::I64(val);
	}
}; // struct WabtTypeTraits<wabt::Type::I64>


template<WabtType _T>
inline size_t InjectGlobalVar(
	wabt::Module& mod,
	typename WabtTypeTraits<_T>::PrimativeType val,
	std::string_view name = std::string_view()
)
{
	size_t id = mod.globals.size();

	std::unique_ptr<wabt::GlobalModuleField> global =
		Internal::make_unique<wabt::GlobalModuleField>(
			wabt::Location(),
			name
		);
	global->global.type = WabtTypeTraits<_T>::sk_wabtType;
	global->global.mutable_ = true;
	global->global.init_expr.push_back(
		Internal::make_unique<wabt::ConstExpr>(WabtTypeTraits<_T>::ToConst(val))
	);

	mod.AppendField(std::move(global));
	return id;
}


template<WabtType _T>
inline size_t InjectExportedGlobalVar(
	wabt::Module& mod,
	typename WabtTypeTraits<_T>::PrimativeType val,
	const std::string& expName,
	std::string_view varName = std::string_view(),
	bool expIdxBinding = true // wabt doesn't allow export name binding to name
)
{
	size_t id = InjectGlobalVar<_T>(mod, val, varName);

	std::unique_ptr<wabt::Var> var;
	if (expIdxBinding || varName.empty())
	{
		var = Internal::make_unique<wabt::Var>(static_cast<wabt::Index>(id));
	}
	else
	{
		var = Internal::make_unique<wabt::Var>(varName);
	}

	InjectExport(
		mod,
		expName,
		wabt::ExternalKind::Global,
		*var
	);

	return id;
}


/**
 * @brief Check if a global variable is referenced in the code by iterating all
 *        expressions in the module.
 *
 * @param mod Module to check
 * @param var Global variable to check
 * @return true If the global variable is referenced in the code
 */
inline bool HasRefGlobal(
	wabt::Module& mod,
	const wabt::Var& var
)
{
	size_t refCount = 0;
	IterateAllExpr(
		mod,
		[&var, &refCount](wabt::Expr& e)
		{
			if (e.type() == wabt::ExprType::GlobalGet)
			{
				wabt::GlobalGetExpr* getExpr = wabt::cast<wabt::GlobalGetExpr>(&e);
				const wabt::Var& getVar = getExpr->var;
				if (VarEq(getVar, var))
				{
					++refCount;
				}
			}
			else if (e.type() == wabt::ExprType::GlobalSet)
			{
				wabt::GlobalSetExpr* setExpr = wabt::cast<wabt::GlobalSetExpr>(&e);
				const wabt::Var& setVar = setExpr->var;
				if (VarEq(setVar, var))
				{
					++refCount;
				}
			}
		}
	);

	return refCount > 0;
}


inline size_t InjectFunc(
	wabt::Module& mod,
	std::unique_ptr<wabt::FuncModuleField> func
)
{
	// Add function type if not exist
	AddFuncTypeIfNotExist(mod, func->func.decl.sig);

	// store the index of the new function
	size_t id = mod.funcs.size();

	// Add function
	mod.AppendField(std::move(func));

	return id;
}


inline std::unique_ptr<wabt::FuncModuleField> BuildWrappingEntryFunc(
	const std::string& funcName,
	const wabt::Var& oriFuncVar,
	const InjectedSymbolInfo& info
)
{
	std::unique_ptr<wabt::FuncModuleField> func =
		Internal::make_unique<wabt::FuncModuleField>();

	// 1. set the function name
	func->func.name = funcName;

	// 2. 5 parameters
	func->func.decl.sig.param_types.push_back(wabt::Type::I32);
	func->func.decl.sig.param_types.push_back(wabt::Type::I32);
	func->func.decl.sig.param_types.push_back(wabt::Type::I64);

	// 3. 1 return value
	func->func.decl.sig.result_types.push_back(wabt::Type::I32);

	// 4. check if the threshold is set; return if it is set
	//  block
	//    global.get $threshold
	//    i64.eqz
	//    br_if 0 ;; branch if threshold is zero (i.e., not set)
	//    ;; the threshold is set, we must return an error code
	//    i32.const 1 ;; error code
	//    return
	//    unreachable
	//  end
	std::unique_ptr<wabt::BlockExpr> block =
		Internal::make_unique<wabt::BlockExpr>();
	block->block.exprs.push_back(
		Internal::make_unique<wabt::GlobalGetExpr>(info.m_thrVar)
	);
	block->block.exprs.push_back(
		Internal::make_unique<wabt::UnaryExpr>(wabt::Opcode::I64Eqz)
	);
	block->block.exprs.push_back(
		Internal::make_unique<wabt::BrIfExpr>(wabt::Var(wabt::Index(0)))
	);
	block->block.exprs.push_back(
		Internal::make_unique<wabt::ConstExpr>(wabt::Const::I32(1))
	);
	block->block.exprs.push_back(
		Internal::make_unique<wabt::ReturnExpr>()
	);
	block->block.exprs.push_back(
		Internal::make_unique<wabt::UnreachableExpr>()
	);
	func->func.exprs.push_back(std::move(block));

	// 5. set the threshold
	//  local.get 4 ;; the 5th parameter - threshold
	//  global.set $threshold
	func->func.exprs.push_back(
		Internal::make_unique<wabt::LocalGetExpr>(wabt::Var(wabt::Index(2)))
	);
	func->func.exprs.push_back(
		Internal::make_unique<wabt::GlobalSetExpr>(info.m_thrVar)
	);

	// 6. call the original function
	//  local.get 0 ;; the 1st parameter - eIdSecSize
	//  local.get 1 ;; the 2nd parameter - msgSecSize
	//  call $oriFunc
	func->func.exprs.push_back(
		Internal::make_unique<wabt::LocalGetExpr>(wabt::Var(wabt::Index(0)))
	);
	func->func.exprs.push_back(
		Internal::make_unique<wabt::LocalGetExpr>(wabt::Var(wabt::Index(1)))
	);
	func->func.exprs.push_back(
		Internal::make_unique<wabt::CallExpr>(oriFuncVar)
	);

	// 7. return the generated function
	return func;
}

inline void InjectWrappingEntryFunc(
	wabt::Module& mod,
	InjectedSymbolInfo& info
)
{
	static const std::string sk_oriExpName = "enclave_wasm_main";
	static const std::string sk_injFuncName = "$enclave_wasm_injected_main";
	static const std::string sk_injExpName = "enclave_wasm_injected_main";

	// 1. ensure the reserved function name is not used
	if (HasNameAtModLevel<true>(mod, sk_injFuncName))
	{
		throw Exception("Function name for wrapping entry function is used");
	}

	// 2. ensure the reserved export name is not used
	if (HasNameExported(mod, sk_injExpName))
	{
		throw Exception("Export name for wrapping entry function is used");
	}

	// 3. find the original entry function
	wabt::Var oriFuncVar =
		FindExportTarget(mod, sk_oriExpName, wabt::ExternalKind::Func);

	// 4. build the wrapping entry function
	std::unique_ptr<wabt::FuncModuleField> func =
		BuildWrappingEntryFunc(sk_injFuncName, oriFuncVar, info);

	// 5. inject the wrapping entry function
	info.SetWrapFuncId(InjectFunc(mod, std::move(func)));

	// 6. export the wrapping entry function
	InjectExport(
		mod,
		sk_injExpName,
		wabt::ExternalKind::Func,
		info.m_wrapFuncVar
	);
}


inline InjectedSymbolInfo PreliminaryCheckAndInject(wabt::Module& mod)
{
	static const std::string sk_thrName = "$enclave_wasm_threshold";
	static const std::string sk_ctrName = "$enclave_wasm_counter";
	static const std::string sk_thrExpName = "enclave_wasm_threshold";
	static const std::string sk_ctrExpName = "enclave_wasm_counter";

	InjectedSymbolInfo info;

	// 1. inject global variable for threshold
	// 1.1 ensure this name is not used
	if (HasNameAtModLevel<true>(mod, sk_thrName))
	{
		throw Exception("Global variable name for threshold is used");
	}
	// 1.2 ensure the reserved export name is not used
	if (HasNameExported(mod, sk_thrExpName))
	{
		throw Exception("Export name for threshold is used");
	}
	// 1.3 inject global variable
	info.SetThresholdId(
		InjectExportedGlobalVar<WabtType::I64>(mod, 0, sk_thrExpName, sk_thrName)
	);
	// 1.4 ensure this global var is not referenced in the code
	if (
		HasRefGlobal(mod, wabt::Var(static_cast<wabt::Index>(info.m_thrId))) ||
		HasRefGlobal(mod, wabt::Var(sk_thrName))
	)
	{
		throw Exception("Global variable for threshold is referenced in the code");
	}

	// 2. inject global variable for counter
	// 2.1 ensure this name is not used
	if (HasNameAtModLevel<true>(mod, sk_ctrName))
	{
		throw Exception("Global variable name for counter is used");
	}
	// 2.2 ensure the reserved export name is not used
	if (HasNameExported(mod, sk_ctrExpName))
	{
		throw Exception("Export name for counter is used");
	}
	// 2.3 inject global variable
	info.SetCounterId(
		InjectExportedGlobalVar<WabtType::I64>(mod, 0, sk_ctrExpName, sk_ctrName)
	);
	// 2.4 ensure this global var is not referenced in the code
	if (
		HasRefGlobal(mod, wabt::Var(static_cast<wabt::Index>(info.m_ctrId))) ||
		HasRefGlobal(mod, wabt::Var(sk_ctrName))
	)
	{
		throw Exception("Global variable for counter is referenced in the code");
	}

	// 3. fix the declaration of enclave_wasm_counter_exceed function
	FixExceedFuncDeclare(mod, info);

	return info;
}


inline void PostInject(
	wabt::Module& mod,
	InjectedSymbolInfo& info
)
{
	// 1. inject entry function
	InjectWrappingEntryFunc(mod, info);
}


inline std::unique_ptr<wabt::BlockExpr> BuildCountingBlock(
	size_t count,
	const InjectedSymbolInfo& enclaveSymInfo
)
{
	std::unique_ptr<wabt::BlockExpr> blkExpr =
		Internal::make_unique<wabt::BlockExpr>();
	wabt::Block& blk = blkExpr->block;

	// - -> block
	// - ->     ;; increment the counter
	// - ->     i64.const count
	// - ->     global.get $counter
	// - ->     i64.add
	// - ->     global.set $counter
	blk.exprs.push_back(
		Internal::make_unique<wabt::ConstExpr>(wabt::Const::I64(count))
	);
	blk.exprs.push_back(
		Internal::make_unique<wabt::GlobalGetExpr>(enclaveSymInfo.m_ctrVar)
	);
	blk.exprs.push_back(
		Internal::make_unique<wabt::BinaryExpr>(wabt::Opcode::I64Add)
	);
	blk.exprs.push_back(
		Internal::make_unique<wabt::GlobalSetExpr>(enclaveSymInfo.m_ctrVar)
	);

	// - ->     ;; check if the counter exceeds the threshold
	// - ->     global.get $counter
	// - ->     global.get $threshold
	// - ->     i64.lt_u
	blk.exprs.push_back(
		Internal::make_unique<wabt::GlobalGetExpr>(enclaveSymInfo.m_ctrVar)
	);
	blk.exprs.push_back(
		Internal::make_unique<wabt::GlobalGetExpr>(enclaveSymInfo.m_thrVar)
	);
	blk.exprs.push_back(
		Internal::make_unique<wabt::BinaryExpr>(wabt::Opcode::I64LeU)
	);

	// - ->     ;; counter < threshold ==> br to continue to the original code
	// - ->     br_if 0
	blk.exprs.push_back(
		Internal::make_unique<wabt::BrIfExpr>(wabt::Var(wabt::Index(0)))
	);

	// - ->     ;; otherwise ==> call the counter exceed function
	// - ->     call $ctr_exceed
	blk.exprs.push_back(
		Internal::make_unique<wabt::CallExpr>(enclaveSymInfo.m_exceedFuncVar)
	);
	// - -> end

	return blkExpr;
}


inline wabt::ExprList::iterator  InjectCountingBlockExpr(
	wabt::ExprList& exprList,
	wabt::ExprList::iterator exprIt,
	size_t count,
	const InjectedSymbolInfo& enclaveSymInfo
)
{
	exprIt = exprList.insert(
		exprIt,
		BuildCountingBlock(count, enclaveSymInfo)
	);

	return exprIt;
}


inline void InjectCountingBlocks(
	Block* head,
	const InjectedSymbolInfo& enclaveSymInfo
)
{
	if ((head != nullptr))
	{
		if (!head->m_isWeightCalc)
		{
			throw Exception("The block weight is not calculated");
		}

		if (!head->m_isCtrInjected)
		{
			head->m_isCtrInjected = true;

			if (head->m_weight > 0)
			{
				// Only inject if weight > 0

				if (head->m_type == BlockType::If)
				{
					// This is a dummy if block
					// we want to inject counter before the if expr,
					// instead of after the if expr
					// otherwise, the br expr inside the if expr may skip the counter
					InjectCountingBlockExpr(
						*head->m_exprList,
						head->m_blkBegin,
						head->m_weight,
						enclaveSymInfo
					);
				}
				else if (
					IsEffectiveControlFlowExpr(head->m_blkLstExprType) &&
					!IsBlockLikeDecl(head->m_blkLstExprType)
				)
				{
					// Last statement is a branch expr
					// we want to inject the counter before the br expr
					auto exprBeforeBr = head->GetBlkLastExpr(1);

					InjectCountingBlockExpr(
						*head->m_exprList,
						exprBeforeBr,
						head->m_weight,
						enclaveSymInfo
					);
				}
				else
				{
					// All other cases
					// we can inject the counter at the end of the codeblock
					InjectCountingBlockExpr(
						*head->m_exprList,
						head->m_blkEnd,
						head->m_weight,
						enclaveSymInfo
					);
				}
			}

			// Recursive on children
			for (auto& child : head->m_children)
			{
				InjectCountingBlocks(child.m_ptr, enclaveSymInfo);
			}
		}
	}
}


} // namespace WasmCounter
