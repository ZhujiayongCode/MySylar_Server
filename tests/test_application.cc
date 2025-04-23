#include "Sylar/application.h"

int main(int argc, char** argv) {
    Sylar::Application app;
    if(app.init(argc, argv)) {
        return app.run();
    }
    return 0;
}
