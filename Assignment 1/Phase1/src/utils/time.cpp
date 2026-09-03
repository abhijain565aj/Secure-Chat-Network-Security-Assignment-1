#include "utils/time.hpp"

std::time_t get_timestamp() {
    return std::time(nullptr);
}
