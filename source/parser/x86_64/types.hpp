// Copyright 2013-2021 Lawrence Livermore National Security, LLC and other
// Spack Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace smeagle::x86_64::types {

  namespace detail {
    /*
     *  This class has an intentially weird layout. We leave the members public to make it an
     * aggregate, but have accessors to satisfy the smeagle::parameter interface requirements.
     */
    struct param {
      std::string name_;
      std::string type_name_;
      std::string class_name_;
      std::string direction_;
      std::string location_;
      size_t size_in_bytes_;

      std::string name() const { return name_; }
      std::string type_name() const { return type_name_; }
      std::string class_name() const { return class_name_; }
      std::string direction() const { return direction_; }
      std::string location() const { return location_; }
      size_t size_in_bytes() const { return size_in_bytes_; }
    };

    void toJson(param const &p, std::ostream &out, int indent) {
      auto buf = std::string(indent, ' ');
      out << buf << "\"name\":\"" << p.name() << "\",\n"
          << buf << "\"type\":\"" << p.type_name() << "\",\n"
          << buf << "\"class\":\"" << p.class_name() << "\",\n"
          << buf << "\"location\":\"" << p.location() << "\",\n"
          << buf << "\"direction\":\"" << p.direction() << "\",\n"
          << buf << "\"size\":\"" << p.size_in_bytes() << "\"";
    }
  }  // namespace detail

  // Parse a parameter into a Smeagle parameter
  // Note that this function cannot be named toJson as overload resolution won't work
  bool makeJson(st::Type *param_type, std::string param_name, std::ostream &out, int indent);

  struct none_t final : detail::param {
    void toJson(std::ostream &out, int indent) const { out << "none"; }
  };

  struct scalar_t final : detail::param {
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << "\n" << buf << "}";
    }
  };
  struct union_t final : detail::param {
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << "\n" << buf << "}";
    }
  };
  template <typename T> struct struct_t final : detail::param {
    T *dyninst_obj;
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      auto fields = *dyninst_obj->getFields();

      // Only print if we have fields
      if (fields.size() > 0) {
        auto buf = std::string(indent + 2, ' ');
        out << ",\n" << buf << "\"fields\": [\n";

        for (auto *field : fields) {
          auto endcomma = (field == fields.back()) ? "" : ",";
          bool created = makeJson(field->getType(), field->getName(), out, indent + 3);
          if (created) {
            out << endcomma << "\n";
          }
        }
        out << buf << "]\n";
      }
      out << buf << "}";
    }
  };

  // NOTE: we need to be able to parse call sites to do arrays
  template <typename T> struct array_t final : detail::param {
    T *dyninst_obj;
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << "\n" << buf << "}";
    }
  };

  template <typename T> struct enum_t final : detail::param {
    T *dyninst_obj;
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << ",\n" << buf << "  \"constants\": {\n";

      // TODO: Dyninst does not provide information about underlying type
      // which we would need here
      auto constants = dyninst_obj->getConstants();
      for (auto const &c : constants) {
        auto endcomma = (c == constants.back()) ? "" : ",";
        out << buf << "    \"" << c.first << "\" : \"" << c.second << "\"" << endcomma << "\n";
      }
      out << buf << "}}";
    }
  };

  struct function_t final : detail::param {
    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << "\n" << buf << "}";
    }
  };
  template <typename T> struct pointer_t final : detail::param {
    int pointer_indirections;
    T underlying_type;

    void toJson(std::ostream &out, int indent) const {
      auto buf = std::string(indent, ' ');
      out << buf << "{\n";
      detail::toJson(*this, out, indent + 2);
      out << ",\n" << buf << "  \"indirections\":\"" << pointer_indirections << "\"";
      out << ",\n" << buf << "  \"underlying_type\": ";
      underlying_type.toJson(out, indent + 4);
      out << "}";
    }
  };

  // Parse a parameter into a Smeagle parameter
  bool makeJson(st::Type *param_type, std::string param_name, std::ostream &out, int indent) {
    auto [underlying_type, ptr_cnt] = unwrap_underlying_type(param_type);
    std::string direction = "";

    // Have we seen it before?
    // Keep track of all of the identifiers we've seen.
    // This is function local static so it's like a global variable within a function
    static std::unordered_set<std::string> seen;
    auto found = seen.find(param_type->getName());

    // If we don't find the name, continue
    if (found == seen.end()) {
      // Add to seen
      seen.insert(param_type->getName());

      // Scalar Type
      if (auto *t = underlying_type->getScalarType()) {
        auto param = smeagle::parameter{types::scalar_t{param_name, param_type->getName(), "Scalar",
                                                        direction, "", param_type->getSize()}};
        param.toJson(out, indent);

        // Structure Type
      } else if (auto *t = underlying_type->getStructType()) {
        using dyn_t = std::decay_t<decltype(*t)>;
        auto param = smeagle::parameter{types::struct_t<dyn_t>{
            param_name, param_type->getName(), "Struct", direction, "", param_type->getSize(), t}};
        param.toJson(out, indent);

        // Union Type
      } else if (auto *t = underlying_type->getUnionType()) {
        auto param = smeagle::parameter{types::union_t{param_name, param_type->getName(), "Union",
                                                       direction, "", param_type->getSize()}};
        param.toJson(out, indent);

        // Array Type
      } else if (auto *t = underlying_type->getArrayType()) {
        using dyn_t = std::decay_t<decltype(*t)>;
        auto param = smeagle::parameter{types::array_t<dyn_t>{
            param_name, param_type->getName(), "Array", direction, "", param_type->getSize()}};
        param.toJson(out, indent);

        // Enum Type
      } else if (auto *t = underlying_type->getEnumType()) {
        using dyn_t = std::decay_t<decltype(*t)>;
        auto param = smeagle::parameter{types::enum_t<dyn_t>{
            param_name, param_type->getName(), "Enum", direction, "", param_type->getSize(), t}};
        param.toJson(out, indent);

        // Function Type
      } else if (auto *t = underlying_type->getFunctionType()) {
        auto param = smeagle::parameter{types::function_t{
            param_name, param_type->getName(), "Function", direction, "", param_type->getSize()}};
        param.toJson(out, indent);

      } else {
        throw std::runtime_error{"Unknown type " + param_type->getName()};
      }

      return true;
    } else {
      return false;
    }
  }
}  // namespace smeagle::x86_64::types
