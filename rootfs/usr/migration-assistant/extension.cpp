#include "extensions.hpp"

#include <utility>

namespace {
    std::vector<std::string> extensions = {
        ".cpp",
        ".hpp",
        ".h",
        ".txt"
    };
}

const std::vector<std::string>& get_extensions()
{
    return extensions;
}

void set_extensions(std::vector<std::string> nouvelles_extensions)
{
    extensions = std::move(nouvelles_extensions);
}