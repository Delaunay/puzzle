#include "application/application.h"


int main() {
    puzzle::Application app(640, 480);

    try {
        app.run();
    } catch (const std::exception&) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

