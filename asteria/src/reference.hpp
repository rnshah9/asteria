// This file is part of Asteria.
// Copyleft 2018, LH_Mouse. All wrongs reserved.

#ifndef ASTERIA_REFERENCE_HPP_
#define ASTERIA_REFERENCE_HPP_

#include "fwd.hpp"
#include "reference_root.hpp"
#include "reference_modifier.hpp"

namespace Asteria {

class Reference
  {
  private:
    Reference_root m_root;
    rocket::cow_vector<Reference_modifier> m_mods;

  public:
    Reference() noexcept
      : m_root(), m_mods()
      {
      }
    // This constructor does not accept lvalues.
    template<typename XrootT,
      typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
        Reference(XrootT &&xroot)
      : m_root(std::forward<XrootT>(xroot)), m_mods()
      {
      }
    // This assignment operator does not accept lvalues.
    template<typename XrootT,
      typename std::enable_if<(Reference_root::Variant::index_of<XrootT>::value || true)>::type * = nullptr>
        Reference & operator=(XrootT &&xroot)
      {
        this->m_root = std::forward<XrootT>(xroot);
        this->m_mods.clear();
        return *this;
      }
    ROCKET_COPYABLE_DESTRUCTOR(Reference);

  private:
    [[noreturn]] void do_throw_unset_no_modifier() const;

    Value do_read_with_modifiers() const;
    Value & do_mutate_with_modifiers() const;
    Value do_unset_with_modifiers() const;

  public:
    bool is_constant() const noexcept
      {
        return this->m_root.index() == Reference_root::index_constant;
      }
    bool is_temporary() const noexcept
      {
        return this->m_root.index() == Reference_root::index_temporary;
      }

    Value read() const
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          return this->m_root.dereference_const();
        }
        return this->do_read_with_modifiers();
      }
    template<typename ValueT>
      Value & write(ValueT &&value) const
      {
        if(ROCKET_EXPECT(this->m_mods.empty())) {
          return this->m_root.dereference_mutable() = std::forward<ValueT>(value);
        }
        return this->do_mutate_with_modifiers() = std::forward<ValueT>(value);
      }
    Value unset() const
      {
        if(ROCKET_UNEXPECT(this->m_mods.empty())) {
          this->do_throw_unset_no_modifier();
        }
        return this->do_unset_with_modifiers();
      }

    template<typename XmodT>
      Reference & zoom_in(XmodT &&mod)
      {
        // Append a modifier.
        this->m_mods.emplace_back(std::forward<XmodT>(mod));
        return *this;
      }
    Reference & zoom_out()
      {
        if(this->m_mods.empty()) {
          // If there is no modifier, set `*this` to a null reference.
          return *this = Reference_root::S_constant();
        }
        // Drop the last modifier.
        this->m_mods.pop_back();
        return *this;
      }

    void enumerate_variables(const Abstract_variable_callback &callback) const
      {
        this->m_root.enumerate_variables(callback);
      }
    void dispose_variable(Global_context &global) const noexcept
      {
        this->m_root.dispose_variable(global);
      }
  };

}

#endif
