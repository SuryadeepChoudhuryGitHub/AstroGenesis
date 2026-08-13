#include "core/Application.hpp"

int main(int argc, char** argv) {
    AstroGenesis::Application app;

    if (!app.initialize(1600, 900, "ASTROGENESIS  —  Space Simulation Engine")) {
        return -1;
    }

    app.run();
    return 0;
}
