#ifndef REKEY_MANAGER_HPP
#define REKEY_MANAGER_HPP

#include <chrono>

class RekeyManager {
    static constexpr std::chrono::seconds ROTATION_INTERVAL{60};
    std::chrono::steady_clock::time_point nextRotation;

public:
    RekeyManager();
    bool due() const;
    void schedule_next();
};

#endif