// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "simple_function.hpp"
#include "utilities.hpp"

namespace Asteria {

Simple_function::~Simple_function() = default;

const String & Simple_function::describe() const noexcept {
	return m_description;
}
void Simple_function::invoke(Xptr<Reference> &result_out, Spparam<Recycler> recycler, Xptr<Reference> &&this_opt, Xptr_vector<Reference> &&arguments_opt) const {
	return (*m_target)(result_out, recycler, std::move(this_opt), std::move(arguments_opt));
}

}
