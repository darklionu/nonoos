#pragma once

#include <string>
#include <vector>

const std::vector<std::string>& get_extensions();

void set_extensions(std::vector<std::string> extensions);

void demander_extensions();