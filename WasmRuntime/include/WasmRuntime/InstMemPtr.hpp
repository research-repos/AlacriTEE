// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <memory>
#include <type_traits>

#include <wasm_export.h>

#include "Exception.hpp"
#include "WasmModuleInstance.hpp"


namespace WasmRuntime
{


template<typename _ValType>
class InstMemPtrBase
{

public: // static members:

	using Self = InstMemPtrBase<_ValType>;

	/**
	 * @brief The type of the value that is going to be stored in the WASM memory
	 *
	 */
	typedef _ValType  value_type;
	typedef typename std::add_const<_ValType>::type  const_value_type;

	/**
	 * @brief The type of the pointer points to the value stored in the heap
	 *        used as WASM memory
	 *
	 */
	typedef typename std::add_pointer<value_type>::type  pointer;
	typedef typename std::add_pointer<const_value_type>::type  const_pointer;

	typedef typename std::add_lvalue_reference<value_type>::type  reference;
	typedef typename std::add_lvalue_reference<const_value_type>::type  const_reference;

	/**
	 * @brief The type of the pointer used by WASM
	 *
	 */
	typedef uint32_t  wasm_pointer;

	/**
	 * @brief The type of size values used by WASM
	 *
	 */
	typedef uint32_t  wasm_size;

	static Self Malloc(
		std::shared_ptr<WasmModuleInstance> modInst,
		size_t size
	)
	{
		if (size > std::numeric_limits<wasm_size>::max())
		{
			throw Exception("Requested size is larger than wasm_size");
		}
		wasm_size wasmSize = static_cast<wasm_size>(size);

		pointer nativePtr = nullptr;
		void* rawNativePtr = nullptr;
		wasm_pointer wasmPtr =
			wasm_runtime_module_malloc(modInst->get(), wasmSize, &rawNativePtr);
		if (wasmPtr == 0)
		{
			throw Exception("Failed to allocate memory in WASM");
		}
		nativePtr = reinterpret_cast<pointer>(rawNativePtr);

		return Self(
			wasmPtr,
			nativePtr,
			wasmSize,
			modInst
		);
	}

public:

	InstMemPtrBase(
		wasm_pointer                        wasmPtr,
		pointer                             nativePtr,
		wasm_size                           wasmSize,
		std::shared_ptr<WasmModuleInstance> modInst
	) noexcept :
		m_wasmPtr(wasmPtr), // int copy is noexcept
		m_nativePtr(nativePtr), // pointer copy is noexcept
		m_wasmSize(wasmSize), // int copy is noexcept
		m_modInst(modInst) // shared_ptr copy is noexcept
	{}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrBase(const InstMemPtrBase&) = delete;

	InstMemPtrBase(InstMemPtrBase&& other) :
		m_wasmPtr(other.m_wasmPtr), // int copy is noexcept
		m_nativePtr(other.m_nativePtr), // pointer copy is noexcept
		m_wasmSize(other.m_wasmSize), // int copy is noexcept
		m_modInst(std::move(other.m_modInst)) // shared_ptr move is noexcept
	{
		other.m_wasmPtr = 0;
		other.m_nativePtr = nullptr;
		other.m_wasmSize = 0;
	}

	virtual ~InstMemPtrBase()
	{
		reset();
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrBase& operator=(const InstMemPtrBase&) = delete;

	InstMemPtrBase& operator=(InstMemPtrBase&& other) noexcept
	{
		if (this != &other)
		{
			// deallocate the current value
			reset();

			// accept the value from the other
			m_wasmPtr = other.m_wasmPtr; // int copy is noexcept
			m_nativePtr = other.m_nativePtr; // pointer copy is noexcept
			m_wasmSize = other.m_wasmSize; // int copy is noexcept
			m_modInst = std::move(other.m_modInst); // shared_ptr move is noexcept

			// empty the other
			other.m_wasmPtr = 0;
			other.m_nativePtr = nullptr;
			other.m_wasmSize = 0;
		}
		return *this;
	}

	/**
	 * @brief Deallocate the value pointed by this pointer
	 *
	 */
	void reset() noexcept
	{
		if (m_wasmPtr != 0)
		{
			// ensure this pointer had not been emptied by a move operation
			wasm_runtime_module_free(m_modInst->get(), m_wasmPtr);
			m_wasmPtr = 0;
			m_nativePtr = nullptr;
		}
	}

	pointer get() noexcept { return m_nativePtr; }
	const_pointer get() const noexcept { return m_nativePtr; }

	reference operator*() noexcept { return *m_nativePtr; }
	const_reference operator*() const noexcept { return *m_nativePtr; }

	pointer operator->() noexcept { return m_nativePtr; }
	const_pointer operator->() const noexcept { return m_nativePtr; }

	wasm_pointer GetWasmPtr() const noexcept { return m_wasmPtr; }

	wasm_size size() const noexcept { return m_wasmSize; }

private:

	wasm_pointer                         m_wasmPtr;
	pointer                              m_nativePtr;
	wasm_size                            m_wasmSize;
	std::shared_ptr<WasmModuleInstance>  m_modInst;

}; // struct InstMemPtrBase


template<typename _T>
wasm_val_t ToWasmVal(const InstMemPtrBase<_T>& ptr)
{
	wasm_val_t wasmVal;
	wasmVal.kind = wasm_valkind_enum::WASM_I32;
	wasmVal.of.i32 = static_cast<int32_t>(ptr.GetWasmPtr());
	return wasmVal;
}


template<typename _DestIt, typename _T>
_DestIt PushWasmVal(_DestIt dest, const InstMemPtrBase<_T>& ptr)
{
	*dest++ = ToWasmVal(ptr);
	return dest;
}


template<typename _ValType>
class InstMemPtrObject :
	public InstMemPtrBase<_ValType>
{
public: // static members:

	using Base = InstMemPtrBase<_ValType>;

	typedef typename Base::value_type  value_type;

public:

	template<typename... _Args>
	InstMemPtrObject(
		Base&& base,
		_Args&&... args
	) :
		Base(std::forward<Base>(base))
	{
		new (Base::get()) value_type(std::forward<_Args>(args)...);
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrObject(const InstMemPtrObject&) = delete;

	/**
	 * @brief Move constructor.
	 *
	 */
	InstMemPtrObject(InstMemPtrObject&& other) :
		Base(std::forward<Base>(other))
	{}

	virtual ~InstMemPtrObject()
	{
		Base::get()->~value_type();
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrObject& operator=(const InstMemPtrObject&) = delete;

	/**
	 * @brief Move assignment operator.
	 *
	 */
	InstMemPtrObject& operator=(InstMemPtrObject&& other) noexcept
	{
		Base::operator=(std::forward<Base>(other));
		return *this;
	}
}; // class InstMemPtrObject


template<typename _ValType>
class InstMemPtrArray :
	public InstMemPtrBase<_ValType>
{
public: // static members:

	using Base = InstMemPtrBase<_ValType>;

	typedef typename Base::value_type  value_type;

public:

	InstMemPtrArray(Base&& base, size_t numItems) :
		Base(std::forward<Base>(base)),
		m_numItems(numItems)
	{}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrArray(const InstMemPtrArray&) = delete;

	/**
	 * @brief Move constructor.
	 *
	 */
	InstMemPtrArray(InstMemPtrArray&& other) :
		Base(std::forward<Base>(other))
	{}

	virtual ~InstMemPtrArray()
	{
		for (size_t i = 0; i < m_numItems; ++i)
		{
			Base::get()[i].~value_type();
		}
	}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	InstMemPtrArray& operator=(const InstMemPtrArray&) = delete;

	/**
	 * @brief Move assignment operator.
	 *
	 */
	InstMemPtrArray& operator=(InstMemPtrArray&& other) noexcept
	{
		Base::operator=(std::forward<Base>(other));
		return *this;
	}

	typename Base::reference operator[](size_t index) noexcept
	{
		return Base::get()[index];
	}

	typename Base::const_reference operator[](size_t index) const noexcept
	{
		return Base::get()[index];
	}

	template<typename _Container>
	void CopyContainer(const _Container& container)
	{
		if (container.size() != m_numItems)
		{
			throw Exception("Container size does not match");
		}

		std::copy(container.begin(), container.end(), Base::get());
	}

private:

	size_t m_numItems;

}; // class InstMemPtrArray


namespace Internal
{
	template<class _T>
	struct InstMemPtrIf
	{
		typedef InstMemPtrObject<_T> Single;
	}; // struct InstMemPtrIf

	template<class _T>
	struct InstMemPtrIf<_T[]>
	{
		typedef InstMemPtrArray<_T> ArrayUnknownBound;
	}; // struct InstMemPtrIf

	template<class _T, size_t _N>
	struct InstMemPtrIf<_T[_N]>
	{
		typedef void ArrayKnownBound;
	}; // struct InstMemPtrIf
} // namespace Internal


template<class _T, class... _Args>
inline typename Internal::InstMemPtrIf<_T>::Single
	MakeInstMemPtr(
		std::shared_ptr<WasmModuleInstance> modInst,
		_Args&&... args
	)
{
	using _MemType     = typename Internal::InstMemPtrIf<_T>::Single;
	using _MemBaseType = typename _MemType::Base;

	_MemBaseType base = _MemBaseType::Malloc(modInst, sizeof(_T));

	return _MemType(std::move(base), std::forward<_Args>(args)...);
}


template<class _T>
inline typename Internal::InstMemPtrIf<_T>::ArrayUnknownBound
	MakeInstMemPtr(
		std::shared_ptr<WasmModuleInstance> modInst,
		size_t n
	)
{
	using _MemType     = typename Internal::InstMemPtrIf<_T>::ArrayUnknownBound;
	using _MemBaseType = typename _MemType::Base;
	using value_type   = typename _MemBaseType::value_type;

	const size_t totalSize = n * sizeof(value_type);
	_MemBaseType base = _MemBaseType::Malloc(modInst, totalSize);

	return _MemType(std::move(base), n);
}


template<class _T, class... _Args>
typename Internal::InstMemPtrIf<_T>::ArrayKnownBound
	MakeInstMemPtr(
		std::shared_ptr<WasmModuleInstance> modInst,
		_Args&&... args
	) = delete;


} // namespace WasmRuntime

