// This file is part of Asteria.
// Copyleft 2018 - 2019, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_
#define ASTERIA_LIBRARY_BINDINGS_CHRONO_HPP_

#include "../fwd.hpp"

namespace Asteria {

extern D_integer std_chrono_utc_now();
extern D_integer std_chrono_local_now();
extern D_real std_chrono_hires_now();
extern D_integer std_chrono_steady_now();

extern D_integer std_chrono_local_from_utc(const D_integer& time_utc);
extern D_integer std_chrono_utc_from_local(const D_integer& time_local);

extern D_string std_chrono_format_datetime(const D_integer& time_point, const Opt<D_boolean>& with_ms = rocket::nullopt);
extern D_string std_chrono_min_datetime(const Opt<D_boolean>& with_ms = rocket::nullopt);
extern D_string std_chrono_max_datetime(const Opt<D_boolean>& with_ms = rocket::nullopt);
extern Opt<D_integer> std_chrono_parse_datetime(const D_string& time_str);

// Create an object that is to be referenced as `std.chrono`.
extern D_object create_bindings_chrono();

}  // namespace Asteria

#endif
