// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <type_traits>
#include <utility>


namespace WasmRuntime
{

template<typename _T, typename _Deleter>
class WamrUniquePtr
{
public: // static members

	typedef _T                                             element_type;
	typedef _Deleter                                       deleter_type;
	typedef typename std::add_const<element_type>::type    const_element_type;
	typedef typename std::add_pointer<element_type>::type  pointer;
	typedef typename std::add_pointer<const_element_type>::type const_pointer;
	typedef typename std::add_const<deleter_type>::type    const_deleter_type;

	typedef typename std::add_lvalue_reference<deleter_type>::type
						deleter_reference_type;
	typedef typename std::add_lvalue_reference<const_deleter_type>::type
						const_deleter_reference_type;

public:

	/**
	 * @brief Construct a new WASM runtime object by giving a pointer
	 *        to a fully initialized WASM module instance.
	 *
	 * @param ptr Pointer to a fully initialized WASM module instance.
	 */
	WamrUniquePtr(pointer ptr) noexcept : m_ptr(ptr), m_del() {}

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WamrUniquePtr(const WamrUniquePtr&) = delete;

	/**
	 * @brief Construct a new WASM runtime object by moving.
	 *
	 * @param other Another WASM runtime object.
	 */
	WamrUniquePtr(WamrUniquePtr&& other) noexcept :
		m_ptr(other.m_ptr), // pointer copy is noexcept
		m_del(std::move(other.m_del)) // deleter move is noexcept
	{
		other.m_ptr = nullptr; // pointer move is noexcept
	}

	virtual ~WamrUniquePtr() { reset(); }

	/**
	 * @brief Copy is prohibited.
	 *
	 */
	WamrUniquePtr& operator=(const WamrUniquePtr&) = delete;

	/**
	 * @brief Move assignment.
	 *
	 * @param other Another WASM runtime object.
	 * @return This WASM runtime object.
	 */
	WamrUniquePtr& operator=(WamrUniquePtr&& other)
	{
		reset(other.m_ptr);
		m_del = std::move(other.m_del);
		other.m_ptr = nullptr;
		return *this;
	}

	/**
	 * @brief Destroy the current WASM runtime object and set the pointer
	 *        to another fully initialized WASM module instance.
	 *
	 * @param ptr
	 */
	void reset(pointer ptr = nullptr)
	{
		if (m_ptr != nullptr)
		{
			get_deleter()(m_ptr);
		}
		m_ptr = ptr;
	}

	const_pointer get() const noexcept { return m_ptr; }

	deleter_reference_type get_deleter() noexcept { return m_del; }

	const_deleter_reference_type get_deleter() const noexcept { return m_del; }

protected:

	pointer get() noexcept { return m_ptr; }

private:

	pointer m_ptr;
	deleter_type m_del;

}; // class WamrUniquePtr


} // namespace WasmRuntime

