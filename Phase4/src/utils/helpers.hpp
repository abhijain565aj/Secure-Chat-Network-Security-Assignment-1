#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <ctime>

std::time_t get_timestamp();
int parse_port(const std::string& value);
std::string to_hex(const std::string& s);
std::string describe_payload(const std::string& payload);

#endif // HELPERS_HPP
