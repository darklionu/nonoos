#pragma once

#include <string>

namespace chatia {

class RappelService {
public:
    std::string handleRequest(const std::string& input) const;
};

} // namespace chatia
