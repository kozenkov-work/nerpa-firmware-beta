idf_component_register(
    SRCS
        "main.cpp"
        "../app/App.cpp"
        "../display/Display.cpp"
        "../input/Buttons.cpp"
        "../storage/ProgramStorage.cpp"
        "../host/Esp32Host.cpp"
        "../../stack.cpp"
        "../../value.cpp"
        "../../tokenizer.cpp"
        "../../vm.cpp"
    INCLUDE_DIRS
        "."
        "../app"
        "../display"
        "../input"
        "../storage"
        "../host"
        "../.."
    REQUIRES
        u8g2
        fatfs
        sdmmc
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
)

set_target_properties(${COMPONENT_LIB} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

option(NERPA_DISPLAY_DIAGNOSTIC "Run only OLED transport and test images" OFF)
if(NERPA_DISPLAY_DIAGNOSTIC)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE NERPA_DISPLAY_DIAGNOSTIC=1)
endif()
