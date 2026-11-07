#include <cstdio>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef NERPA_DISPLAY_DIAGNOSTIC
#include "Display.h"
#else
#include "App.h"
#endif

extern "C" void app_main() {
#ifdef NERPA_DISPLAY_DIAGNOSTIC
    Display display;
    display.runDiagnostic();
#else
    std::printf("Stage: constructing App\n");
    App app;
    std::printf("Stage: App constructed\n");
    if (!app.setup()) {
        while (true) vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while (true) app.loop();
#endif
}
