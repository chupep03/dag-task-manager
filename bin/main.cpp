#include <iostream>
#include "TTaskScheduler.hpp"
#include "TTask.hpp"
#include "TFuture.hpp"
#include "App.hpp"

int main(int argc, char** argv) {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    int rc = 0;
    try {
        App app;
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        rc = 1;
    }

    curl_global_cleanup();
    return rc;
}