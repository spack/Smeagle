// Copyright 2013-2021 Lawrence Livermore National Security, LLC and other
// Spack Project Developers. See the top-level COPYRIGHT file for details.
//
// SPDX-License-Identifier: (Apache-2.0 OR MIT)

#include <smeagle/corpora.h>
#include <smeagle/smeagle.h>

#include <iostream>
#include <stdexcept>
#include <string>

#include "Function.h"
#include "Symtab.h"

using namespace Dyninst;
using namespace SymtabAPI;
using namespace smeagle;

Smeagle::Smeagle(std::string _library) : library(std::move(_library)) {}

// Determine if the library has exceptions with smeagle
bool Smeagle::has_exceptions() {
  Symtab *obj = NULL;
  std::vector<ExceptionBlock *> exceptions;

  // Read the library into the Symtab object, cut out early if there's error
  if (not Symtab::openFile(obj, library)) {
    throw std::runtime_error{"There was a problem reading from '" + library + "'"};
  }

  // Parse exceptions
  obj->getAllExceptions(exceptions);
  if (exceptions.size() == 0) {
    return false;
  }
  return true;
}

// Parse the library with smeagle
smeagle::Corpus Smeagle::parse() {
  // We are going to read functions and symbols
  Symtab *obj = NULL;
  std::vector<Symbol *> symbols;

  // Read the library into the Symtab object, cut out early if there's error
  if (not Symtab::openFile(obj, library)) {
    throw std::runtime_error{"There was a problem reading from '" + library + "'"};
  }

  // Get all functions in the library
  // Note: looping through this doesn't seem to work
  if (not obj->getAllSymbols(symbols)) {
    throw std::runtime_error{"There was a problem getting symbols from '" + library + "'"};
  }

  // Create a corpus
  Corpus corpus(library);

  // Loop through the vector and look at symbols
  for (auto &symbol : symbols) {
    // We are interested in symbols in the dynamic symbol table
    if (symbol->isInDynSymtab()) {
      // If It's a function, parse the parameters
      if (symbol->isFunction()) {
        corpus.parseFunctionABILocation(symbol, obj->getArchitecture());

        // If it's a variable and not a function
      } else if (symbol->isVariable()) {
        // Do we have a global variable?
        if (symbol->getLinkage() == Symbol::SL_GLOBAL) {
          corpus.parseVariableABILocation(symbol, obj->getArchitecture());
        }
      }

      // The symbol is something else (we likely want a subset of these?)

      // else {
      // std::cout << "symbol_notparsed(" <<  sname << ")" << "\n";
      //}
    }
  }

  // Return the corpus for further processing
  return corpus;
}
