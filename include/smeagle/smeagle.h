#pragma once

#include <string>

#include "Symtab.h"

using namespace Dyninst;
using namespace SymtabAPI;

namespace smeagle {

  /**  Output Format codes to be used with the Smeagle class */
  enum class FormatCode { Terminal, Json, Asp };

  /**
   * @brief A class for saying hello in multiple languages
   */
  class Smeagle {
    std::string library;

  public:
    /**
     * @brief Creates a new smeagle to parse the precious
     * @param name the name to greet
     */
    Smeagle(std::string library);

    /**
     * @brief Get the name of a symbol type from the enum int
     * @param symbol the symbol object
     */
    std::string getStringSymbolType(Symbol* symbol);

    /**
     * @brief Get a string representation of a location from a type
     * @param paramType the parameter (variable) type
     */
    std::string getStringLocationFromType(Type* paramType, int order);

    /**
     * @brief Parse a function into parameters, types, locations
     * @param symbol the symbol that is determined to be a function
     */
    void parseFunctionABILocation(Symbol* symbol);

    /**
     * @brief Parse the library with dyninst
     * @param fmt the format to print to the screen
     * @return a string containing the output
     */
    int parse(FormatCode fmt = FormatCode::Terminal);
  };

}  // namespace smeagle
