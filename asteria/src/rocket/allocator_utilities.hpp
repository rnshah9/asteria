// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ROCKET_ALLOCATOR_UTILITIES_HPP_
#define ROCKET_ALLOCATOR_UTILITIES_HPP_

#include <memory> // std::allocator<>, std::allocator_traits<>
#include <type_traits> // std::conditional<>, std::false_type, std::true_type
#include <utility> // std::move()
#include "utilities.hpp"

namespace rocket
{

using ::std::allocator_traits;
using ::std::conditional;
using ::std::false_type;
using ::std::true_type;

namespace details_allocator_utilities
{
	template<typename typeT>
	struct is_final
#ifdef __cpp_lib_is_final
		: ::std::is_final<typeT>
#else
		: false_type
#endif
	{ };

	template<typename allocatorT>
	class final_wrapper
	{
	private:
		allocatorT m_alloc;

	public:
		explicit final_wrapper(const allocatorT &alloc) noexcept
			: m_alloc(alloc)
		{ }
		explicit final_wrapper(allocatorT &&alloc) noexcept
			: m_alloc(::std::move(alloc))
		{ }

	public:
		operator const allocatorT & () const noexcept
		{
			return this->m_alloc;
		}
		operator allocatorT & () noexcept
		{
			return this->m_alloc;
		}
	};
}

template<typename allocatorT>
struct allocator_wrapper_base_for
	: conditional<details_allocator_utilities::is_final<allocatorT>::value, details_allocator_utilities::final_wrapper<allocatorT>, allocatorT>
{ };

template<typename allocatorT, bool propagateT = allocator_traits<allocatorT>::propagate_on_container_copy_assignment::value>
struct allocator_copy_assigner
{
	void operator()(allocatorT & /*lhs*/, const allocatorT & /*rhs*/) const
	{
		// nothing to do
	}
};
template<typename allocatorT>
struct allocator_copy_assigner<allocatorT, true>
{
	void operator()(allocatorT &lhs, const allocatorT &rhs) const
	{
		lhs = rhs;
	}
};

template<typename allocatorT, bool propagateT = allocator_traits<allocatorT>::propagate_on_container_move_assignment::value>
struct allocator_move_assigner
{
	void operator()(allocatorT & /*lhs*/, allocatorT && /*rhs*/) const
	{
		// nothing to do
	}
};
template<typename allocatorT>
struct allocator_move_assigner<allocatorT, true>
{
	void operator()(allocatorT &lhs, allocatorT &&rhs) const
	{
		lhs = ::std::move(rhs);
	}
};

template<typename allocatorT, bool propagateT = allocator_traits<allocatorT>::propagate_on_container_swap::value>
struct allocator_swapper
{
	void operator()(allocatorT & /*lhs*/, allocatorT & /*rhs*/) const
	{
		// nothing to do
	}
};
template<typename allocatorT>
struct allocator_swapper<allocatorT, true>
{
	void operator()(allocatorT &lhs, allocatorT &rhs) const
	{
		noadl::adl_swap(lhs, rhs);
	}
};

template<typename allocatorT>
struct is_std_allocator
	: false_type
{ };
template<typename valueT>
struct is_std_allocator<::std::allocator<valueT>>
	: true_type
{ };

}

#endif
