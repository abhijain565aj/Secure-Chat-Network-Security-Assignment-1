#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <ctime>

std::time_t get_timestamp();
int parse_port(const std::string& value);

#endif // HELPERS_HPP
