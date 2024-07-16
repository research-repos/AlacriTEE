// Copyright (c) 2023 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <type_traits>

#include <src/cast.h>
#include <src/common.h>
#include <src/ir.h>

#include "Classification.hpp"
#include "Utils.hpp"


namespace WasmCounter
{


template<typename _IrType, typename _T>
inline void IterateAllExpr(_IrType&, _T);


template<typename _T>
inline void IterateAllExpr(wabt::Block& blk, _T op)
{
	return IterateAllExpr(blk.exprs, op);
}


template<wabt::ExprType _BlkType, typename _T>
inline void IterateAllExpr(wabt::BlockExprBase<_BlkType>& blk, _T op)
{
	return IterateAllExpr(blk.block, op);
}


template<typename _T>
inline void IterateAllExpr(wabt::IfExpr& ite, _T op)
{
	IterateAllExpr(ite.true_, op);
	IterateAllExpr(ite.false_, op);
}


template<typename _T>
inline void IterateAllExpr(wabt::ExprList& expr, _T op)
{
	for (wabt::Expr& e : expr)
	{
		wabt::ExprType exprType = e.type();
		if (IsBlockLikeDecl(exprType))
		{
			switch (exprType)
			{
			case wabt::ExprType::Block:
				IterateAllExpr(wabt::cast<wabt::BlockExpr>(&e)->block, op);
				break;
			case wabt::ExprType::Loop:
				IterateAllExpr(wabt::cast<wabt::LoopExpr>(&e)->block, op);
				break;
			case wabt::ExprType::If:
				IterateAllExpr(*wabt::cast<wabt::IfExpr>(&e), op);
				break;

			default:
				throw Exception(
					"Unknown block-like expr type " +
						std::string(wabt::GetExprTypeName(exprType))
				);
			}
		}
		else
		{
			op(e);
		}
	}
}


template<typename _T>
inline void IterateAllExpr(wabt::ExprListVector& exprList, _T op)
{
	for (wabt::ExprList& expr : exprList)
	{
		IterateAllExpr(expr, op);
	}
}


template<typename _T>
inline void IterateAllExpr(wabt::Global& global, _T op)
{
	IterateAllExpr(global.init_expr, op);
}


template<typename _T>
inline void IterateAllExpr(wabt::ElemSegment& elem, _T op)
{
	IterateAllExpr(elem.offset, op);
	IterateAllExpr(elem.elem_exprs, op);
}


template<typename _T>
inline void IterateAllExpr(wabt::DataSegment& data, _T op)
{
	IterateAllExpr(data.offset, op);
}


template<typename _T>
inline void IterateAllExpr(
	wabt::Func& func,
	_T op
)
{
	return IterateAllExpr(func.exprs, op);
}


template<typename _T>
inline void IterateAllExpr(
	wabt::Module& mod,
	_T op
)
{
	for (wabt::ModuleField& field : mod.fields)
	{
		switch (field.type())
		{
		case wabt::ModuleFieldType::Func:
		{
			wabt::Func& func =
				wabt::cast<wabt::FuncModuleField>(&field)->func;
			IterateAllExpr(func, op);
			break;
		}
		case wabt::ModuleFieldType::Global:
		{
			wabt::Global& global =
				wabt::cast<wabt::GlobalModuleField>(&field)->global;
			IterateAllExpr(global, op);
			break;
		}
		case wabt::ModuleFieldType::ElemSegment:
		{
			wabt::ElemSegment& elem =
				wabt::cast<wabt::ElemSegmentModuleField>(&field)->elem_segment;
			IterateAllExpr(elem, op);
			break;
		}
		case wabt::ModuleFieldType::DataSegment:
		{
			wabt::DataSegment& data =
				wabt::cast<wabt::DataSegmentModuleField>(&field)->data_segment;
			IterateAllExpr(data, op);
			break;
		}
		case wabt::ModuleFieldType::Import:
		case wabt::ModuleFieldType::Export:
		case wabt::ModuleFieldType::Type:
		case wabt::ModuleFieldType::Table:
		case wabt::ModuleFieldType::Memory:
		case wabt::ModuleFieldType::Start:
			break;
		default:
			throw Exception(
				std::string("IterateAllExpr on module type ") +
					GetModuleFieldTypeName(field.type()) +
					" is not supported"
			);
		}
	}
}

template<bool _enable>
struct HasNameHelper;


template<>
struct HasNameHelper<false>
{
	template<typename _ModType>
	static bool HasName(const _ModType&, const std::string&) { return false; }
}; // struct HasNameHelper<false>


template<>
struct HasNameHelper<true>
{
	// Func
	// Global
	// Table
	// ElemSegment
	// Memory
	// DataSegment
	// TypeEntry
	template<typename _ModType>
	static const std::string& GetName(const _ModType& modField)
	{
		return modField.name;
	}

	template<typename _ModType>
	static bool HasName(const _ModType& modField, const std::string& name)
	{
		return GetName(modField) == name;
	}
}; // struct HasNameHelper<true>


inline bool HasNameFromImport(
	const wabt::Import& import,
	const std::string& name
)
{
	switch (import.kind())
	{
		case wabt::ExternalKind::Func:
			return HasNameHelper<true>::HasName(
				wabt::cast<wabt::FuncImport>(&import)->func,
				name
			);
		case wabt::ExternalKind::Table:
			return HasNameHelper<true>::HasName(
				wabt::cast<wabt::TableImport>(&import)->table,
				name
			);
		case wabt::ExternalKind::Memory:
			return HasNameHelper<true>::HasName(
				wabt::cast<wabt::MemoryImport>(&import)->memory,
				name
			);
		case wabt::ExternalKind::Global:
			return HasNameHelper<true>::HasName(
				wabt::cast<wabt::GlobalImport>(&import)->global,
				name
			);
		default:
			throw Exception(
				std::string("HasNameFromImport on import kind ") +
					wabt::GetKindName(import.kind()) +
					" is not supported"
			);
	}
}


template<
	bool _forAll,
	wabt::ModuleFieldType _specType = wabt::ModuleFieldType::Func
>
inline bool HasNameAtModLevel(const wabt::Module& mod, const std::string& name)
{
	for (const wabt::ModuleField& field : mod.fields)
	{
		switch (field.type())
		{
		case wabt::ModuleFieldType::Func:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::Func)>::HasName(
					wabt::cast<wabt::FuncModuleField>(&field)->func,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::Global:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::Global)>::HasName(
					wabt::cast<wabt::GlobalModuleField>(&field)->global,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::Table:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::Table)>::HasName(
					wabt::cast<wabt::TableModuleField>(&field)->table,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::ElemSegment:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::ElemSegment)>::HasName(
					wabt::cast<wabt::ElemSegmentModuleField>(&field)->elem_segment,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::Memory:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::Memory)>::HasName(
					wabt::cast<wabt::MemoryModuleField>(&field)->memory,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::DataSegment:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::DataSegment)>::HasName(
					wabt::cast<wabt::DataSegmentModuleField>(&field)->data_segment,
					name
				)
			)
			{
				return true;
			}
			break;
		case wabt::ModuleFieldType::Export:
			// nothing about export can be referenced by the code inside the module
			break;
		case wabt::ModuleFieldType::Type:
			if (
				HasNameHelper<_forAll || (_specType == wabt::ModuleFieldType::Type)>::HasName(
					*(wabt::cast<wabt::TypeModuleField>(&field)->type),
					name
				)
			)
			{
				return true;
			}
		case wabt::ModuleFieldType::Start:
			// nothing about start can be referenced by the code inside the module
			break;
		case wabt::ModuleFieldType::Import:
			if (
				HasNameFromImport(
					*wabt::cast<wabt::ImportModuleField>(&field)->import,
					name
				)
			)
			{
				return true;
			}
			break;
		default:
			throw Exception(
				std::string("IterateAllExpr on module type ") +
					GetModuleFieldTypeName(field.type()) +
					" is not supported"
			);
		}
	}
	return false;
}


template<bool _IsConst>
struct FieldIteratorHelper
{
	using Module = typename std::conditional<
		_IsConst,
		const wabt::Module,
		wabt::Module
	>::type;
	using ModuleRef = typename std::add_lvalue_reference<Module>::type;

	using ModuleField = typename std::conditional<
		_IsConst,
		const wabt::ModuleField,
		wabt::ModuleField
	>::type;
	using ModuleFieldRef = typename std::add_lvalue_reference<ModuleField>::type;

	template<typename _T>
	static void IterateExports(
		ModuleRef mod,
		_T op
	)
	{
		for (ModuleFieldRef field : mod.fields)
		{
			switch (field.type())
			{
			case wabt::ModuleFieldType::Func:
			case wabt::ModuleFieldType::Global:
			case wabt::ModuleFieldType::Table:
			case wabt::ModuleFieldType::ElemSegment:
			case wabt::ModuleFieldType::Memory:
			case wabt::ModuleFieldType::DataSegment:
			case wabt::ModuleFieldType::Type:
			case wabt::ModuleFieldType::Start:
			case wabt::ModuleFieldType::Import:
				// none of these will expose their API to the outside
				break;
			case wabt::ModuleFieldType::Export:
			{
				auto& realField = *wabt::cast<wabt::ExportModuleField>(&field);
				if (op(realField))
				{
					// stop iteration
					return;
				}
				break;
			}
			default:
				throw Exception(
					std::string("IterateExports on module type ") +
						GetModuleFieldTypeName(field.type()) +
						" is not supported"
				);
			}
		}
	}

	template<typename _T>
	static void IterateImports(
		ModuleRef mod,
		_T op
	)
	{
		for (ModuleFieldRef field : mod.fields)
		{
			switch (field.type())
			{
			case wabt::ModuleFieldType::Func:
			case wabt::ModuleFieldType::Global:
			case wabt::ModuleFieldType::Table:
			case wabt::ModuleFieldType::ElemSegment:
			case wabt::ModuleFieldType::Memory:
			case wabt::ModuleFieldType::DataSegment:
			case wabt::ModuleFieldType::Type:
			case wabt::ModuleFieldType::Start:
			case wabt::ModuleFieldType::Export:
				break;
			case wabt::ModuleFieldType::Import:
			{
				auto& realField = *wabt::cast<wabt::ImportModuleField>(&field);
				if (op(realField))
				{
					// stop iteration
					return;
				}
				break;
			}
			default:
				throw Exception(
					std::string("IterateImports on module type ") +
						GetModuleFieldTypeName(field.type()) +
						" is not supported"
				);
			}
		}
	}

}; // struct FieldIteratorHelper


inline bool HasNameExported(
	const wabt::Module& mod,
	const std::string& name
)
{
	bool found = false;
	FieldIteratorHelper<true>::IterateExports(
		mod,
		[&found, &name](const wabt::ExportModuleField& expField) -> bool
		{
			const wabt::Export& exp = expField.export_;
			if (exp.name == name)
			{
				found = true; // record the result
				return true; // stop iteration
			}
			return false;
		}
	);
	return found;
}


inline wabt::Var FindExportTarget(
	const wabt::Module& mod,
	const std::string& name,
	wabt::ExternalKind kind
)
{
	bool found = false;
	wabt::Var var;

	FieldIteratorHelper<true>::IterateExports(
		mod,
		[&found, &var, &name, &kind](const wabt::ExportModuleField& expField) -> bool
		{
			const wabt::Export& exp = expField.export_;
			if ((kind == exp.kind) && (name == exp.name))
			{
				var = exp.var;
				found = true; // record the result
				return true; // stop iteration
			}
			return false;
		}
	);
	if (!found)
	{
		throw Exception(
			std::string("Exported ") +
				wabt::GetKindName(kind) +
				" " +
				name +
				" not found"
		);
	}
	return var;
}


inline wabt::Import* FindImportImpl(
	wabt::Module& mod,
	const std::string& modName,
	const std::string& fieldName,
	wabt::ExternalKind kind,
	bool throwOnDup = false
)
{
	wabt::Import* found = nullptr;

	FieldIteratorHelper<false>::IterateImports(
		mod,
		[&found, &modName, &fieldName, kind, throwOnDup]
			(wabt::ImportModuleField& impField) -> bool
		{
			if (
				(impField.import->kind() == kind) &&
				(impField.import->module_name == modName) &&
				(impField.import->field_name == fieldName)
			)
			{
				if (throwOnDup && (found != nullptr))
				{
					throw Exception(
						std::string("Duplicate import ") +
							wabt::GetKindName(kind) + " " +
							modName + "." + fieldName
					);
				}
				found = impField.import.get();
				return !throwOnDup; // stop iteration
			}
			return false;
		}
	);

	return found;
}


template<wabt::ExternalKind _kind>
struct FindImport;

template<>
struct FindImport<wabt::ExternalKind::Func>
{
	static wabt::FuncImport* Find(
		wabt::Module& mod,
		const std::string& modName,
		const std::string& fieldName,
		bool throwOnDup = false
	)
	{
		wabt::Import* found = FindImportImpl(
			mod,
			modName,
			fieldName,
			wabt::ExternalKind::Func,
			throwOnDup
		);

		return found == nullptr ?
			nullptr :
			wabt::cast<wabt::FuncImport>(found);
	}
}; // struct FindImport<wabt::ExternalKind::Func>


inline size_t FindFuncIdx(const wabt::Module& mod, const wabt::Func* func)
{
	for(size_t i = 0; i < mod.funcs.size(); ++i)
	{
		if (mod.funcs[i] == func)
		{
			return i;
		}
	}
	throw Exception("The given func is not in the module");
}


} // namespace WasmCounter

