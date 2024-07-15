// Copyright (c) 2024 WasmRuntime
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT.

#pragma once


#include <memory>

#include "WasmRuntime.hpp"


namespace WasmRuntime
{


template<typename _T>
class SharedObject
{
public:

	SharedObject(std::unique_ptr<_T> ptr) :
		m_ptr(std::move(ptr))
	{}

	SharedObject(const SharedObject& other) noexcept :
		m_ptr(other.m_ptr) // shared_ptr copy is noexcept
	{}

	SharedObject(SharedObject&& other) noexcept :
		m_ptr(std::move(other.m_ptr)) // shared_ptr move is noexcept
	{}

	virtual ~SharedObject() noexcept
	{}

	SharedObject& operator=(const SharedObject& other) noexcept
	{
		m_ptr = other.m_ptr; // shared_ptr copy is noexcept
		return *this;
	}

	SharedObject& operator=(SharedObject&& other) noexcept
	{
		m_ptr = std::move(other.m_ptr); // shared_ptr move is noexcept
		return *this;
	}

	std::shared_ptr<_T> get() noexcept
	{
		return m_ptr;
	}

	std::shared_ptr<const _T> get() const noexcept
	{
		return m_ptr;
	}

	_T* operator->() noexcept
	{
		return m_ptr.get();
	}

	const _T* operator->() const noexcept
	{
		return m_ptr.get();
	}

private:

	std::shared_ptr<_T> m_ptr;

}; // class SharedObject


} // namespace WasmRuntime



