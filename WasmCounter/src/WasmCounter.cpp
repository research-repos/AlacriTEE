// Copyright (c) 2022 WasmCounter
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#include <WasmCounter/WasmCounter.hpp>

#include <src/error.h>
#include <src/feature.h>
#include <src/result.h>
#include <src/shared-validator.h>
#include <src/validator.h>

#include "BlockGenerator.hpp"
#include "CodeInjector.hpp"
#include "WeightCalculator.hpp"

namespace WasmCounter
{

static std::unique_ptr<Graph> InstrumentFunc(
	wabt::Func& func,
	const ImportFuncInfo& funcInfo,
	const InjectedSymbolInfo& symInfo
)
{
	// Generate block flow graph
	std::unique_ptr<Graph> gr = GenerateGraph(func);

	// Calculate weight for each block
	WeightCalculator wCalc(GetDefaultExprWeightCalcMap(), 0);
	wCalc.CalcWeight(gr->m_head, funcInfo);

	// Inject counting code
	InjectCountingBlocks(gr->m_head, symInfo);

	return gr;
}

static void PostValidateModule(const wabt::Module& mod)
{
	wabt::Features features;
	wabt::ValidateOptions options(features);
	wabt::Errors errors;
	wabt::Result result = wabt::ValidateModule(&mod, &errors, options);
	if (!wabt::Succeeded(result))
	{
		std::string errMsg;
		for (const auto& err : errors)
		{
			errMsg += (err.message + '\n');
		}
		throw Exception(
			"Failed to validate the generated module:\n" +
			errMsg);
	}
}

static ImportFuncListType GetImportFuncList(
	const std::vector<wabt::Import*>& imps
)
{
	ImportFuncListType list;
	for (const wabt::Import* imp : imps)
	{
		if (imp->kind() == wabt::ExternalKind::Func)
		{
			const wabt::FuncImport* funcImp =
				wabt::cast<const wabt::FuncImport>(imp);
			list.emplace_back(funcImp->module_name, funcImp->field_name);
		}
	}

	return list;
}

} // namespace WasmCounter

void WasmCounter::Instrument(
	wabt::Module& mod,
	std::vector<Internal::InCmpPtr<Graph> >* outGraphs
)
{
	// Inject counter and functions
	auto symInfo = PreliminaryCheckAndInject(mod);

	// Generate import function info
	auto impFuncList = GetImportFuncList(mod.imports);
	ImportFuncInfo funcInfo{ mod.func_bindings, impFuncList };

	// Instrument code
	size_t funcIdx = 0;
	for (wabt::ModuleField& field : mod.fields)
	{
		switch (field.type())
		{
		case wabt::ModuleFieldType::Func:
			if (funcIdx != symInfo.m_funcIncrId)
			{
				wabt::Func& func =
					wabt::cast<wabt::FuncModuleField>(&field)->func;
				auto gr = InstrumentFunc(func, funcInfo, symInfo);
				if (outGraphs != nullptr)
				{
					outGraphs->emplace_back(std::move(gr));
				}
			}
			++funcIdx;
			break;
		case wabt::ModuleFieldType::Import:
			if (
				wabt::cast<wabt::ImportModuleField>(&field)->import->kind() ==
					wabt::ExternalKind::Func
			)
			{
				++funcIdx;
			}
			break;
		default:
			break;
		}
	}

	// Post injection
	PostInject(mod, symInfo);

	// validate generated module
	PostValidateModule(mod);
}


template<>
WasmCounter::Internal::InCmpPtr<WasmCounter::Graph>::~InCmpPtr()
{}

