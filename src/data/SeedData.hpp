#pragma once

#include "data/repositories/ObjectRepository.hpp"

namespace AstroGenesis {

class SeedData {
public:
    // Check if the database needs initialization with baseline seed data
    static bool isDatabaseSeeded(ObjectRepository& repo);

    // Populate database with high-precision baseline Solar System, Asteroid Belt, and Exoplanet systems
    static bool seedDefaultDatabase(ObjectRepository& repo);
};

} // namespace AstroGenesis
