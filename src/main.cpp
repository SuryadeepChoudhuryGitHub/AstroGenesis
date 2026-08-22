#include "core/Application.hpp"

int main(int argc, char** argv) {
    AstroGenesis::Application app;

    if (!app.initialize(1600, 900, "ASTROGENESIS  —  Space Simulation Engine")) {
        return -1;
    }

    app.run();
    return 0;
}

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return main(__argc, __argv);
}
#endif
