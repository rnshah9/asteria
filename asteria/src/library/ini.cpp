// This file is part of Asteria.
// Copyleft 2018 - 2022, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "ini.hpp"
#include "../runtime/argument_reader.hpp"
#include "../utils.hpp"

namespace asteria {
namespace {

constexpr char s_reject[]   = "[]=;#";
constexpr char s_comment[]  = ";#";
constexpr char s_space[]    = " \t";

tinyfmt&
do_format_key(tinyfmt& fmt, const cow_string& key)
  {
    if(key.empty())
      ASTERIA_THROW_RUNTIME_ERROR("empty key is not allowed");

    size_t pos = key.find_first_of(s_reject);
    if(pos != cow_string::npos)
      ASTERIA_THROW_RUNTIME_ERROR("key contains invalid characters: $1", key);

    pos = key.find_first_of(s_space);
    if(pos == 0)
      ASTERIA_THROW_RUNTIME_ERROR("key shall not begin with a space: $1", key);

    if(pos != cow_string::npos) {
      pos = key.find_last_of(s_space);
      if(pos == key.size() - 1)
        ASTERIA_THROW_RUNTIME_ERROR("key shall not end with a space: $1", key);
    }

    return fmt << key;
  }

bool
do_format_check_scalar(const Value& value)
  {
    switch(value.type()) {
      case type_null:
      case type_boolean:
      case type_integer:
      case type_real:
        // These values are always convertible to strings.
        return true;

      case type_string: {
        const auto& str = value.as_string();

        // Verify the string, as we will have to write it verbatim later.
        if(str.empty())
          return true;

        size_t pos = str.find_first_of(s_reject);
        if(pos != cow_string::npos)
          ASTERIA_THROW_RUNTIME_ERROR("value contains invalid characters: $1", str);

        pos = str.find_first_of(s_space);
        if(pos == 0)
          ASTERIA_THROW_RUNTIME_ERROR("value shall not begin with a space: $1", str);

        if(pos != cow_string::npos) {
          pos = str.find_last_of(s_space);
          if(pos == str.size() - 1)
            ASTERIA_THROW_RUNTIME_ERROR("value shall not end with a space: $1", str);
        }

        return true;
      }

      case type_opaque:
      case type_function:
      case type_array:
      case type_object:
        // these values are always ignored.
        return false;

      default:
        ASTERIA_TERMINATE("invalid value type (type `$1`)", value.type());
    }
  }

V_object
do_ini_parse(tinybuf& buf)
  {
    V_object root;
    V_object* sink = &root;

    // Read source file in lines.
    cow_string line;
    size_t nlines = 0;
    while(getline(line, buf)) {
      nlines++;

      // Convert CR LF pairs to LF characters.
      if(line.ends_with(sref("\r")))
        line.pop_back();

      // Remove comments.
      size_t pos = line.find_first_of(s_comment);
      if(pos != cow_string::npos)
        line.erase(pos);

      // Remove leading and trailing spaces.
      // Empty lines are ignored.
      pos = line.find_first_not_of(s_space);
      if(pos == cow_string::npos)
        continue;

      line.erase(0, pos);
      pos = line.find_last_not_of(s_space);
      line.erase(pos + 1);

      // if the line begins with an open bracket, it shall start a section.
      if(line.front() == '[') {
        if(line.back() != ']')
          ASTERIA_TERMINATE("invalid section name on line $1", nlines);

        // Trim the section name.
        pos = line.find_first_not_of(1, s_space);
        if(pos == line.size() - 1)
          ASTERIA_TERMINATE("empty section name on line $1", nlines);

        // Make a copy of the section name that is just large enough.
        cow_string key;
        key.assign(line, 1, line.size() - 2);

        // Insert a new section.
        auto& sub = root.try_emplace(::std::move(key), V_object()).first->second;
        ROCKET_ASSERT(sub.is_object());
        sink = &(sub.open_object());
        continue;
      }

      // Otherwise, it shall be a property.
      cow_string key;
      cow_string value;

      size_t eqpos = line.find('=');
      if(eqpos != cow_string::npos) {
        pos = line.find_last_not_of(eqpos - 1, s_space);
        if(pos == cow_string::npos)
          ASTERIA_TERMINATE("empty property name on line $1", nlines);

        // Make copies of the key and value that are just large enough.
        key.assign(line, 0, pos + 1);
        pos = line.find_first_not_of(eqpos + 1, s_space);
        value.assign(line, pos, line.size());
      }
      else
        key.assign(line, 0, line.size());

      // Insert a new value.
      sink->insert_or_assign(::std::move(key), ::std::move(value));
    }

    return root;
  }

}  // namespace

V_string
std_ini_format(optV_object value)
  {
    if(!value)
      return sref("");

    // Write top-level non-object values first.
    ::rocket::tinyfmt_str fmt;
    size_t nlines = 0;

    // Write top-level properties.
    for(const auto& r : *value) {
      if(!do_format_check_scalar(r.second))
        continue;

      // Write a key-value pair.
      do_format_key(fmt, r.first);
      fmt << "=";
      r.second.print(fmt, false);
      fmt << "\r\n";
      nlines++;
    }

    // Write sections.
    for(const auto& ro : *value) {
      if(!ro.second.is_object())
        continue;

      if(nlines != 0)
        fmt << "\r\n";

      // Write a section header.
      fmt << "[";
      do_format_key(fmt, ro.first);
      fmt << "]\r\n";
      nlines++;

      // Write all properties in this section.
      for(const auto& r : ro.second.as_object()) {
        if(!do_format_check_scalar(r.second))
          continue;

        // Write a key-value pair.
        do_format_key(fmt, r.first);
        fmt << "=";
        r.second.print(fmt, false);
        fmt << "\r\n";
        nlines++;
      }
    }

    return fmt.extract_string();
  }

V_object
std_ini_parse(V_string text)
  {
    // Parse characters from the string.
    ::rocket::tinybuf_str cbuf;
    cbuf.set_string(text, tinybuf::open_read);
    return do_ini_parse(cbuf);
  }

V_object
std_ini_parse_file(V_string path)
  {
    // Try opening the file.
    ::rocket::unique_posix_file fp(::fopen(path.safe_c_str(), "rb"), ::fclose);
    if(!fp)
      ASTERIA_THROW_RUNTIME_ERROR(
          "could not open file '$2'\n"
          "[`fopen()` failed: $1]", format_errno(), path);

    // Parse characters from the file.
    ::rocket::tinybuf_file cbuf(::std::move(fp));
    return do_ini_parse(cbuf);
  }

void
create_bindings_ini(V_object& result, API_Version /*version*/)
  {
    result.insert_or_assign(sref("format"),
      ASTERIA_BINDING(
        "std.ini.format", "[object]",
        Argument_Reader&& reader)
      {
        optV_object object;

        reader.start_overload();
        reader.optional(object);
        if(reader.end_overload())
          return (Value) std_ini_format(object);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("parse"),
      ASTERIA_BINDING(
        "std.ini.parse", "text",
        Argument_Reader&& reader)
      {
        V_string text;

        reader.start_overload();
        reader.required(text);
        if(reader.end_overload())
          return (Value) std_ini_parse(text);

        reader.throw_no_matching_function_call();
      });

    result.insert_or_assign(sref("parse_file"),
      ASTERIA_BINDING(
        "std.ini.parse_file", "path",
        Argument_Reader&& reader)
      {
        V_string path;

        reader.start_overload();
        reader.required(path);
        if(reader.end_overload())
          return (Value) std_ini_parse_file(path);

        reader.throw_no_matching_function_call();
      });
  }

}  // namespace asteria