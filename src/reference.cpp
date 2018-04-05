// This file is part of asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "reference.hpp"
#include "variable.hpp"
#include "stored_value.hpp"
#include "recycler.hpp"
#include "utilities.hpp"

namespace Asteria {

Reference::Reference(Reference &&) = default;
Reference &Reference::operator=(Reference &&) = default;
Reference::~Reference() = default;

Reference::Type get_reference_type(Spref<const Reference> reference_opt) noexcept {
	return reference_opt ? reference_opt->get_type() : Reference::type_null;
}

namespace {
	struct Dereference_once_result {
		Sptr<const Variable> rptr_opt;  // How to read a value through this reference?
		bool rvalue;                    // Is this reference an rvalue that must not be written into?
		Xptr<Variable> *wref_opt;       // How to write a value through this reference?
	};

	Dereference_once_result do_dereference(Spref<const Reference> reference_opt, bool create_if_not_exist){
		const auto type = get_reference_type(reference_opt);
		switch(type){
		case Reference::type_null: {
			Dereference_once_result res = { nullptr, true, nullptr };
			return std::move(res); }
		case Reference::type_rvalue_static: {
			const auto &params = reference_opt->get<Reference::S_rvalue_static>();
			Dereference_once_result res = { params.variable_opt, true, nullptr };
			return std::move(res); }
		case Reference::type_rvalue_dynamic: {
			const auto &params = reference_opt->get<Reference::S_rvalue_dynamic>();
			Dereference_once_result res = { params.variable_opt, true, nullptr };
			return std::move(res); }
		case Reference::type_lvalue_scoped_variable: {
			const auto &params = reference_opt->get<Reference::S_lvalue_scoped_variable>();
			auto &variable_opt = params.scoped_variable->variable_opt;
			Dereference_once_result res = { variable_opt, false, &variable_opt };
			return std::move(res); }
		case Reference::type_lvalue_array_element: {
			const auto &params = reference_opt->get<Reference::S_lvalue_array_element>();
			const auto array = params.variable->get_opt<D_array>();
			if(!array){
				ASTERIA_THROW_RUNTIME_ERROR("Only arrays can be indexed by integers, while the operand had type `", get_variable_type_name(params.variable), "`");
			}
			auto normalized_index = (params.index_bidirectional >= 0) ? params.index_bidirectional
			                                                          : static_cast<std::int64_t>(static_cast<std::uint64_t>(params.index_bidirectional) + array->size());
			auto size_current = static_cast<std::int64_t>(array->size());
			if(normalized_index < 0){
				if(!create_if_not_exist || params.immutable){
					ASTERIA_DEBUG_LOG("D_array index was before the front: index = ", params.index_bidirectional, ", size = ", size_current);
					Dereference_once_result res = { nullptr, false, nullptr };
					return std::move(res);
				}
				ASTERIA_DEBUG_LOG("Creating array elements automatically in the front: index = ", params.index_bidirectional, ", size = ", size_current);
				const auto count_to_prepend = 0 - static_cast<std::uint64_t>(normalized_index);
				if(count_to_prepend > array->max_size() - array->size()){
					ASTERIA_THROW_RUNTIME_ERROR("The array is too large and cannot be allocated: count_to_prepend = ", count_to_prepend);
				}
				array->reserve(array->size() + static_cast<std::size_t>(count_to_prepend));
				for(std::size_t i = 0; i < count_to_prepend; ++i){
					array->emplace_back();
				}
				std::move_backward(array->begin(), array->begin() + static_cast<std::ptrdiff_t>(size_current), array->end());
				normalized_index += static_cast<std::int64_t>(count_to_prepend);
				size_current += static_cast<std::int64_t>(count_to_prepend);
				ASTERIA_DEBUG_LOG("Resized array successfully: normalized_index = ", normalized_index, ", size_current = ", size_current);
			} else if(normalized_index >= size_current){
				if(!create_if_not_exist || params.immutable){
					ASTERIA_DEBUG_LOG("D_array index was after the back: index = ", params.index_bidirectional, ", size = ", size_current);
					Dereference_once_result res = { nullptr, false, nullptr };
					return std::move(res);
				}
				ASTERIA_DEBUG_LOG("Creating array elements automatically in the back: index = ", params.index_bidirectional, ", size = ", size_current);
				const auto count_to_append = static_cast<std::uint64_t>(normalized_index - size_current) + 1;
				if(count_to_append > array->max_size() - array->size()){
					ASTERIA_THROW_RUNTIME_ERROR("The array is too large and cannot be allocated: count_to_append = ", count_to_append);
				}
				array->reserve(array->size() + static_cast<std::size_t>(count_to_append));
				for(std::size_t i = 0; i < count_to_append; ++i){
					array->emplace_back();
				}
				size_current += static_cast<std::int64_t>(count_to_append);
				ASTERIA_DEBUG_LOG("Resized array successfully: normalized_index = ", normalized_index, ", size_current = ", size_current);
			}
			auto &variable_opt = array->at(static_cast<std::size_t>(normalized_index));
			Dereference_once_result res = { variable_opt, false, &variable_opt };
			return std::move(res); }
		case Reference::type_lvalue_object_member: {
			const auto &params = reference_opt->get<Reference::S_lvalue_object_member>();
			const auto object = params.variable->get_opt<D_object>();
			if(!object){
				ASTERIA_THROW_RUNTIME_ERROR("Only objects can be indexed by strings, while the operand had type `", get_variable_type_name(params.variable), "`");
			}
			auto it = object->find(params.key);
			if(it == object->end()){
				if(!create_if_not_exist || params.immutable){
					ASTERIA_DEBUG_LOG("D_object member was not found: key = ", params.key);
					Dereference_once_result res = { nullptr, false, nullptr };
					return std::move(res);
				}
				ASTERIA_DEBUG_LOG("Creating object member automatically: key = ", params.key);
				it = object->emplace(params.key, nullptr).first;
				ASTERIA_DEBUG_LOG("Created object member successfuly: key = ", params.key);
			}
			auto &variable_opt = it->second;
			Dereference_once_result res = { variable_opt, false, &variable_opt };
			return std::move(res); }
		default:
			ASTERIA_DEBUG_LOG("Unknown type enumeration: type = ", type);
			std::terminate();
		}
	}
}

Sptr<const Variable> read_reference_opt(Spref<const Reference> reference_opt){
	auto result = do_dereference(reference_opt, false);
	return std::move(result.rptr_opt);
}
Sptr<Variable> write_reference(Spref<Reference> reference_opt, Spref<Recycler> recycler, Stored_value &&value_opt){
	auto result = do_dereference(reference_opt, true);
	if(result.rvalue){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write to a temporary reference having type `", get_variable_type_name(result.rptr_opt), "`");
	}
	if(!(result.wref_opt)){
		ASTERIA_THROW_RUNTIME_ERROR("Attempting to write to a constant reference having type `", get_variable_type_name(result.rptr_opt), "`");
	}
	return recycler->set_variable(*(result.wref_opt), std::move(value_opt));
}

}
