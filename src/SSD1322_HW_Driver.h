/**
 ****************************************************************************************
 *
 * \file SSD1322_HW_Driver.h
 *
 * \brief Hardware dependent functions declarations for SSD1322 OLED display.
 *
 * This file contains functions that rely on hardware of used MCU. In this example functions
 * are filled with STM32F411RE hardware implementation. To use this library on any other MCU
 * you just have to provide its hardware implementations of functions from this file and higher
 * level functions should work without modification.
 *
 * Copyright (C) 2020 Wojciech Klimek
 * MIT license:
 * https://github.com/wjklimek1/SSD1322_OLED_library
 *
 ****************************************************************************************
 */
#ifndef SSD1322_HW_DRIVER_H
#define SSD1322_HW_DRIVER_H

#include <Arduino.h> // Arduino-Kernfunktionen
#include <SPI.h>     // SPI-Bibliothek für Display-Kommunikation

#include <stdint.h>

class SSD1322_HW_DRIVER
{
private:
    int OLED_CS;
    int OLED_DC;
    int OLED_RESET;
    uint32_t _spi_clock;

public:
    SSD1322_HW_DRIVER(int OLED_CS_PIN, int OLED_DC_PIN, int OLED_RESET_PIN = -1, uint32_t spi_clock = 8000000);
    void setSPIClock(uint32_t clock_speed);
    uint32_t getSPIClock() const { return _spi_clock; }
    void SSD1322_HW_drive_CS_low();
    void SSD1322_HW_drive_CS_high();
    void SSD1322_HW_drive_DC_low();
    void SSD1322_HW_drive_DC_high();
    void SSD1322_HW_drive_RESET_low();
    void SSD1322_HW_drive_RESET_high();
    void SSD1322_HW_SPI_send_byte(uint8_t byte_to_transmit);
    void SSD1322_HW_SPI_send_array(uint8_t *array_to_transmit, uint32_t array_size);
    void SSD1322_HW_msDelay(uint32_t milliseconds);
};

#endif /* SSD1322_HW_DRIVER_H */
