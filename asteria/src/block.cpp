// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "block.hpp"
#include "statement.hpp"
#include "reference.hpp"
#include "scope.hpp"
#include "utilities.hpp"

namespace Asteria {

Block::Block(Block &&) noexcept = default;
Block & Block::operator=(Block &&) noexcept = default;
Block::~Block() = default;

void bind_block_in_place(Vp<Block> &bound_block_out, Spr<Scope> scope_inout, Spr<const Block> block_opt){
	if(block_opt == nullptr){
		// Return a null block.
		bound_block_out.reset();
		return;
	}
	// Bind statements recursively.
	T_vector<Statement> bound_stmts;
	bound_stmts.reserve(block_opt->size());
	for(const auto &stmt : *block_opt){
		bind_statement_in_place(bound_stmts, scope_inout, stmt);
	}
	bound_block_out.emplace(std::move(bound_stmts));
}
Statement::Execution_result execute_block_in_place(Vp<Reference> &reference_out, Spr<Scope> scope_inout, Spr<Recycler> recycler_inout, Spr<const Block> block_opt){
	if(block_opt == nullptr){
		// Nothing to do.
		move_reference(reference_out, nullptr);
		return Statement::execution_result_next;
	}
	// Execute statements one by one.
	for(const auto &stmt : *block_opt){
		const auto result = execute_statement_in_place(reference_out, scope_inout, recycler_inout, stmt);
		if(result != Statement::execution_result_next){
			// Forward anything unexpected to the caller.
			return result;
		}
	}
	return Statement::execution_result_next;
}

void bind_block(Vp<Block> &bound_block_out, Spr<const Block> block_opt, Spr<const Scope> scope){
	if(block_opt == nullptr){
		// Return a null block.
		bound_block_out.reset();
		return;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_lexical, scope);
	bind_block_in_place(bound_block_out, scope_working, block_opt);
}
Statement::Execution_result execute_block(Vp<Reference> &reference_out, Spr<Recycler> recycler_inout, Spr<const Block> block_opt, Spr<const Scope> scope){
	if(block_opt == nullptr){
		// Nothing to do.
		move_reference(reference_out, nullptr);
		return Statement::execution_result_next;
	}
	const auto scope_working = std::make_shared<Scope>(Scope::purpose_plain, scope);
	return execute_block_in_place(reference_out, scope_working, recycler_inout, block_opt);
}

}
