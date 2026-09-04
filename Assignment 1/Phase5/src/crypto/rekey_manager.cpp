#include "rekey_manager.hpp"

RekeyManager::RekeyManager()
    : nextRotation(std::chrono::steady_clock::now() + ROTATION_INTERVAL) {}

bool RekeyManager::due() const {
    return std::chrono::steady_clock::now() >= nextRotation;
}

void RekeyManager::schedule_next() {
    nextRotation = std::chrono::steady_clock::now() + ROTATION_INTERVAL;
}