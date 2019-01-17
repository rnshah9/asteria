// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "_test_init.hpp"
#include "../asteria/src/syntax/expression.hpp"
#include "../asteria/src/syntax/xpnode.hpp"
#include "../asteria/src/syntax/statement.hpp"
#include "../asteria/src/runtime/global_context.hpp"
#include "../asteria/src/runtime/executive_context.hpp"

using namespace Asteria;

int main()
  {
    Global_Context global;
    Executive_Context ctx(nullptr);

    const auto cond = rocket::make_refcnt<Variable>(D_null(), false);
    const auto dval = rocket::make_refcnt<Variable>(D_real(1.5), false);
    const auto ival = rocket::make_refcnt<Variable>(D_integer(3), false);
    const auto aval = rocket::make_refcnt<Variable>(D_array(), false);

    ctx.open_named_reference(std::ref("cond")) = Reference_Root::S_variable { cond };
    ctx.open_named_reference(std::ref("dval")) = Reference_Root::S_variable { dval };
    ctx.open_named_reference(std::ref("ival")) = Reference_Root::S_variable { ival };
    ctx.open_named_reference(std::ref("aval")) = Reference_Root::S_variable { aval };

    // Plain: aval[1] = !cond ? (dval++ + 0.25) : (ival * "hello,");
    // RPN:   aval 1 [] cond ! ?: =                    ::= expr
    //                         |\-- dval ++ 0.25 +     ::= branch_true
    //                         \--- ival "hello," *    ::= branch_false
    Cow_Vector<Xpnode> branch_true;
    {
      branch_true.emplace_back(Xpnode::S_named_reference { std::ref("dval") });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_postfix_inc, false });
      branch_true.emplace_back(Xpnode::S_literal { D_real(0.25) });
      branch_true.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_add, false });
    }
    Cow_Vector<Xpnode> branch_false;
    {
      branch_false.emplace_back(Xpnode::S_named_reference { std::ref("ival") });
      branch_false.emplace_back(Xpnode::S_literal { D_string("hello,") });
      branch_false.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_mul, false });
    }
    Cow_Vector<Xpnode> nodes;
    {
      nodes.emplace_back(Xpnode::S_named_reference { std::ref("aval") });
      nodes.emplace_back(Xpnode::S_literal { D_integer(1) });
      nodes.emplace_back(Xpnode::S_subscript { Cow_String() });
      nodes.emplace_back(Xpnode::S_named_reference { std::ref("cond") });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_prefix_notl, false });
      nodes.emplace_back(Xpnode::S_branch { std::move(branch_true), std::move(branch_false), false });
      nodes.emplace_back(Xpnode::S_operator_rpn { Xpnode::xop_infix_assign, false });
    }
    auto expr = Expression(std::move(nodes));

    Reference result;
    expr.evaluate(result, global, ctx);
    auto value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_real>() == 1.75);
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 1.75);

    cond->set_value(D_integer(42));
    expr.evaluate(result, global, ctx);
    value = dval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_real>() == 2.5);
    value = ival->get_value();
    ASTERIA_TEST_CHECK(value.check<D_integer>() == 3);
    value = aval->get_value();
    ASTERIA_TEST_CHECK(value.check<D_array>().at(1).check<D_string>() == "hello,hello,hello,");
    value = result.read();
    ASTERIA_TEST_CHECK(value.check<D_string>() == "hello,hello,hello,");
 }
