// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include "WamrUniquePtr.hpp"

#include <memory>
#include <string>

#include <wasm_export.h>

#include "Exception.hpp"
#include "FuncUtils.hpp"
#include "Logging.hpp"
#include "SystemIO.hpp"
#include "WasmModuleInstance.hpp"


namespace WasmRuntime
{


class ExecEnvUserData;


struct WasmExecEnvDestroy
{
	void operator()(wasm_exec_env_t ptr) noexcept
	{
		wasm_runtime_destroy_exec_env(ptr);
	}
}; // struct WasmExecEnvDestroy


class WasmExecEnv :
	public WamrUniquePtr<
		typename std::remove_pointer<wasm_exec_env_t>::type,
		WasmExecEnvDestroy
	>
{
public: // static members

	using Self = WasmExecEnv;
	using Base = WamrUniquePtr<
		typename std::remove_pointer<wasm_exec_env_t>::type,
		WasmExecEnvDestroy
	>;
	typedef typename Base::pointer        pointer;
	typedef typename Base::const_pointer  const_pointer;

	static WasmExecEnv Create(
		std::shared_ptr<WasmModuleInstance> moduleInst,
		uint32_t stackSize
	)
	{
		wasm_exec_env_t ptr = wasm_runtime_create_exec_env(
			moduleInst->get(),
			stackSize
		);

		if (ptr == nullptr)
		{
			throw Exception("Failed to create execution environment");
		}

		return WasmExecEnv(ptr, moduleInst);
	}

	static Self& FromUserData(pointer exec_env)
	{
		return *static_cast<Self*>(wasm_runtime_get_user_data(exec_env));
	}

	static const Self& FromUserData(const_pointer exec_env)
	{
		wasm_exec_env_t tmpPtr = const_cast<wasm_exec_env_t>(exec_env);
		return *static_cast<const Self*>(wasm_runtime_get_user_data(tmpPtr));
	}

	static const Self& FromConstUserData(pointer exec_env)
	{
		return *static_cast<Self*>(wasm_runtime_get_user_data(exec_env));
	}

public:

	WasmExecEnv(
		wasm_exec_env_t ptr,
		std::shared_ptr<WasmModuleInstance> moduleInst
	) :
		Base(ptr), // base constructor is noexcept,
		m_logger(LoggerFactory::GetLogger("WasmRuntime::WasmExecEnv")),
		m_moduleInst(moduleInst), // shared_ptr copy is noexcept
		m_userData()
	{
		wasm_runtime_set_user_data(get(), this);
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WasmExecEnv(const WasmExecEnv&) = delete;

	/**
	 * @brief Construct a new WASM Execution Environment object by moving.
	 *
	 * @param other Another WasmExecEnv object.
	 */
	WasmExecEnv(WasmExecEnv&& other) :
		Base(std::move(other)), // base move is noexcept
		m_logger(std::move(other.m_logger)),
		m_moduleInst(std::move(other.m_moduleInst)), // shared_ptr move is noexcept
		m_userData(std::move(other.m_userData))
	{
		wasm_runtime_set_user_data(get(), this);
	}

	virtual ~WasmExecEnv()
	{
		// wasm_runtime_destroy_exec_env(get());
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WasmExecEnv& operator=(const WasmExecEnv&) = delete;

	/**
	 * @brief Move assignment operator.
	 *
	 * @param other The other WasmExecEnv object.
	 * @return This WasmExecEnv object after moving.
	 */
	WasmExecEnv& operator=(WasmExecEnv&& other)
	{
		Base::operator=(std::move(other));
		if (this != &other)
		{
			// free the current object and then move the other object
			m_logger = std::move(other.m_logger);
			m_moduleInst = std::move(other.m_moduleInst); // shared_ptr move is noexcept
			m_userData = std::move(other.m_userData);

			wasm_runtime_set_user_data(get(), this);
		}
		return *this;
	}

	template<typename _RetTuple, typename... _Args>
	inline _RetTuple ExecFunc(
		const std::string& funcName,
		_Args&&... args
	)
	{
		static constexpr size_t sk_numRes = std::tuple_size<_RetTuple>::value;

		_RetTuple retVals;
		wasm_val_t wasmRes[sk_numRes + 1];

		auto wasmArg = ToWasmValVector(std::forward<_Args>(args)...);

		wasm_module_inst_t   moduleInst = wasm_runtime_get_module_inst(get());
		wasm_function_inst_t targetFunc =
			wasm_runtime_lookup_function(moduleInst, funcName.c_str(), nullptr);
		if (targetFunc == nullptr)
		{
			throw Exception(
				"Could not find the function named " +
				funcName +
				" in the given WASM module"
			);
		}

		bool execRes = wasm_runtime_call_wasm_a(
			get(), targetFunc,
			static_cast<uint32_t>(sk_numRes), wasmRes,
			static_cast<uint32_t>(wasmArg.size()), wasmArg.data()
		);

		if (!execRes)
		{
			throw WasmRuntimeException(wasm_runtime_get_exception(moduleInst));
		}

		WasmValListToTuple<_RetTuple, sk_numRes>::Read(retVals, wasmRes);

		return retVals;
	}

	void SetUserData(std::unique_ptr<ExecEnvUserData> userData)
	{
		m_userData = std::move(userData);
	}

	ExecEnvUserData& GetUserData()
	{
		return *m_userData;
	}

	const ExecEnvUserData& GetUserData() const
	{
		return *m_userData;
	}

	WasmModuleInstance& GetModuleInstance()
	{
		return *m_moduleInst;
	}

	const WasmModuleInstance& GetModuleInstance() const
	{
		return *m_moduleInst;
	}

	virtual void NativePrintCStr(const char* str) const
	{
		NativePrintStr(str);
	}

	virtual void NativePrintStr(const std::string& str) const
	{
		// The logger should add new lines to the end of each call
		// so we want to remove spaces or new lines at the end of the string
		// to avoid extra new lines.

		size_t endPos = str.find_last_not_of(" \t\n\r");

		m_logger.Info(
			(endPos != std::string::npos) ?
				str.substr(0, endPos + 1) :
				str
		);
	}

	const SystemIO& GetSystemIO() const
	{
		return m_moduleInst->GetSystemIO();
	}

private:

	Logger m_logger;

	std::shared_ptr<WasmModuleInstance> m_moduleInst;
	std::unique_ptr<ExecEnvUserData> m_userData;

}; // class WasmExecEnv


} // namespace WasmRuntime

