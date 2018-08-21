// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#include "precompiled.hpp"
#include "context.hpp"
#include "variadic_arguer.hpp"
#include "utilities.hpp"

namespace Asteria
{

Context::~Context()
  = default;

namespace
  {
    template<typename GeneratorT>
      void do_set_reference(Spref<Context> ctx_out, const String &name, GeneratorT &&generator)
        {
          Reference ref;
          if(ctx_out->is_feigned() == false) {
            ref = std::forward<GeneratorT>(generator)();
          }
          ctx_out->set_named_reference_opt(name, std::move(ref));
        }
  }

void initialize_function_context(Spref<Context> ctx_out, const Vector<String> &params, const String &file, Unsigned line, Reference self, Vector<Reference> args)
  {
    // Set up parameters.
    for(const auto &name : params) {
      Reference ref;
      if(args.empty() == false) {
        // Shift an argument.
        ref = std::move(args.mut_front());
        args.erase(args.begin());
      }
      if(name.empty() == false) {
        do_set_reference(ctx_out, name, [&] { return std::move(ref); });
      }
    }
    // Set up system variables.
    do_set_reference(ctx_out, String::shallow("__file"), [&] { return reference_constant(D_string(file)); });
    do_set_reference(ctx_out, String::shallow("__line"), [&] { return reference_constant(D_integer(line)); });
    do_set_reference(ctx_out, String::shallow("__this"), [&] { return std::move(self); });
    do_set_reference(ctx_out, String::shallow("__varg"), [&] { return reference_constant(D_function(allocate<Variadic_arguer>(file, line, std::move(args)))); });
  }

}
