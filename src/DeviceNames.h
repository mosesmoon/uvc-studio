// Display names for devices that share a name: the first keeps its name,
// later ones get " (2)", " (3)" … in enumeration order.
#pragma once
#include <map>
#include <string>

class DuplicateNamer {
 public:
  std::string operator()(const std::string& name) {
    int n = ++uses_[name];
    return n == 1 ? name : name + " (" + std::to_string(n) + ")";
  }

 private:
  std::map<std::string, int> uses_;
};
