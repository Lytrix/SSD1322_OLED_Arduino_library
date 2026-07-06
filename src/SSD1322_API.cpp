/**
 ****************************************************************************************
 *
 * \file SSD1322_API.c
 *
 * \brief API for SSD1322 display. Includes commands, options and initialization sequence.
 *
 *
 * Copyright (C) 2020 Wojciech Klimek
 * MIT license:
 * https://github.com/wjklimek1/SSD1322_OLED_library
 *
 ****************************************************************************************
 */

//====================== Includes ====================//
#include "SSD1322_API.h"
#include "SSD1322_Config.h"
#include "TsyDMASPI.h"

SSD1322_CONFIG config;

//====================== Constructor ========================//
SSD1322_API::SSD1322_API(SSD1322_HW_DRIVER *driver) : driver_instance(driver) {
	// Initialize the framebuffer to zeroes
	memset(framebuffer, 0, FRAMEBUFFER_SIZE);
}

void SSD1322_API::begin() {
	TsyDMASPI0.begin(config.OLED_CS_PIN, SPISettings(config.SPI_CLOCK, MSBFIRST, SPI_MODE0));
}

//====================== command ========================//
/**
 *  @brief Sends command byte to SSD1322
 */
void SSD1322_API::SSD1322_API_command(uint8_t command)
{
	driver_instance->SSD1322_HW_drive_CS_low();
	driver_instance->SSD1322_HW_drive_DC_low();
	driver_instance->SSD1322_HW_SPI_send_byte(command);
	driver_instance->SSD1322_HW_drive_CS_high();
}

//====================== data ========================//
/**
 *  @brief Sends data byte to SSD1322
 */
void SSD1322_API::SSD1322_API_data(uint8_t data)
{
	driver_instance->SSD1322_HW_drive_CS_low();
	driver_instance->SSD1322_HW_drive_DC_high();
	driver_instance->SSD1322_HW_SPI_send_byte(data);
	driver_instance->SSD1322_HW_drive_CS_high();
}

//====================== initialization sequence ========================//
/**
 *  @brief Initializes SSD1322 OLED display.
 */
void SSD1322_API::SSD1322_API_init()
{
	driver_instance->SSD1322_HW_drive_RESET_low();	// Reset pin low
	driver_instance->SSD1322_HW_msDelay(1);			// 1ms delay
	driver_instance->SSD1322_HW_drive_RESET_high(); // Reset pin high
	driver_instance->SSD1322_HW_msDelay(50);		// 50ms delay
	SSD1322_API_command(0xFD);						// set Command unlock
	SSD1322_API_data(0x12);
	SSD1322_API_command(0xAE); // set display off
	SSD1322_API_command(0xB3); // set display clock divide ratio
	SSD1322_API_data(0x91);
	SSD1322_API_command(0xCA); // set multiplex ratio
	SSD1322_API_data(0x3F);
	SSD1322_API_command(0xA2); // set display offset to 0
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xA1); // start display start line to 0
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xA0); // set remap and dual COM Line Mode
	SSD1322_API_data(0x14);
	SSD1322_API_data(0x11);
	SSD1322_API_command(0xB5); // disable IO input
	SSD1322_API_data(0x00);
	SSD1322_API_command(0xAB); // function select
	SSD1322_API_data(0x01);
	SSD1322_API_command(0xB4); // enable VSL extern
	SSD1322_API_data(0xA0);
	SSD1322_API_data(0xFD);
	SSD1322_API_command(0xC1); // set contrast current
	SSD1322_API_data(0xFF);
	SSD1322_API_command(0xC7); // set master contrast current
	SSD1322_API_data(0x0F);
	SSD1322_API_command(0xB9); // default grayscale
	SSD1322_API_command(0xB1); // set phase length
	SSD1322_API_data(0xE2);
	SSD1322_API_command(0xD1); // enhance driving scheme capability
	SSD1322_API_data(0x82);
	SSD1322_API_data(0x20);
	SSD1322_API_command(0xBB); // first pre charge voltage
	SSD1322_API_data(0x1F);
	SSD1322_API_command(0xB6); // second pre charge voltage
	SSD1322_API_data(0x08);
	SSD1322_API_command(0xBE); // VCOMH
	SSD1322_API_data(0x07);
	SSD1322_API_command(0xA6);				 // set normal display mode
	SSD1322_API_command(0xA9);				 // no partial mode
	driver_instance->SSD1322_HW_msDelay(10); // stabilize VDD
	SSD1322_API_command(0xAF);				 // display on
	driver_instance->SSD1322_HW_msDelay(50); // stabilize VDD
}

//====================== normal/inversion ========================//
/**
 *  @brief Sets display mode to normal/inverted/off/on
 *
 *  When mode is set to ON or OFF, display ignores GRAM data and is always on or off.
 */

void SSD1322_API::SSD1322_API_set_display_mode(enum SSD1322_mode_e mode)
{
	switch (mode)
	{
	case SSD1322_MODE_NORMAL:
		SSD1322_API_command(SET_DISP_MODE_NORMAL);
		break;
	case SSD1322_MODE_INVERTED:
		SSD1322_API_command(SET_DISP_MODE_INVERTED);
		break;
	case SSD1322_MODE_ON:
		SSD1322_API_command(SET_DISP_MODE_ON);
		break;
	case SSD1322_MODE_OFF:
		SSD1322_API_command(SET_DISP_MODE_OFF);
		break;
	}
}

//====================== go to sleep ========================//
/**
 *  @brief Go to sleep mode.
 */
void SSD1322_API::SSD1322_API_sleep_on()
{
	SSD1322_API_command(SLEEP_MODE_ON);
}

//====================== wake up from sleep ========================//
/**
 *  @brief Wake up from sleep mode.
 */
void SSD1322_API::SSD1322_API_sleep_off()
{
	SSD1322_API_command(SLEEP_MODE_OFF);
}

//====================== contrast ========================//
/**
 *  @brief Sets contrast between brightest and darkest pixels.
 */
void SSD1322_API::SSD1322_API_set_contrast(uint8_t contrast)
{
	SSD1322_API_command(SET_CONTRAST_CURRENT);
	SSD1322_API_data(contrast);
}

//====================== brightness ========================//
/**
 *  @brief Should set brightness, but actual effect is similar to setting contrast.
 */
void SSD1322_API::SSD1322_API_set_brightness(uint8_t brightness)
{
	SSD1322_API_command(MASTER_CONTRAST_CURRENT);
	SSD1322_API_data(0x0F & brightness); // first 4 bits have to be 0
}

//====================== custom grayscale ========================//
/**
 *  @brief Upload custom grayscale table to SSD1322.
 *
 *  Uploads exact values for 4-bit grayscale levels. Grayscale levels can have values from 0 to 180
 *  and have to meet following condition:
 *
 *  G0 < G1 < G2 < ... < G14 < G15
 *
 *  Where Gx is value of grayscale level
 *
 *  @param[in] grayscale_tab array of 16 brightness values
 *
 *  @return 0 when levels are out of range, 1 if function has ended correctly
 */
uint8_t SSD1322_API::SSD1322_API_custom_grayscale(uint8_t *grayscale_tab)
{
	SSD1322_API_command(SET_GRAYSCALE_TABLE);
	for (int i = 0; i < 16; i++)
	{
		if (grayscale_tab[i] > 180)
			return 0;
		SSD1322_API_data(grayscale_tab[i]);
	}
	SSD1322_API_command(ENABLE_GRAYSCALE_TABLE);
	return 1;
}

//====================== default grayscale ========================//
/**
 *  @brief Reset grayscale levels to default (linear)
 */
void SSD1322_API::SSD1322_API_default_grayscale()
{
	SSD1322_API_command(SET_DEFAULT_GRAYSCALE_TAB);
}

//====================== window to draw into ========================//
/**
 *  @brief Sets range of pixels to write to.
 *
 *  @param[in] start_column
 *  @param[in] end_column
 *  @param[in] start_row
 *  @param[in] end_row
 */
void SSD1322_API::SSD1322_API_set_window(uint8_t start_column, uint8_t end_column, uint8_t start_row, uint8_t end_row)
{
	SSD1322_API_command(SET_COLUMN_ADDR); // set columns range
	SSD1322_API_data(28 + start_column);
	SSD1322_API_data(28 + end_column);
	SSD1322_API_command(SET_ROW_ADDR); // set rows range
	SSD1322_API_data(start_row);
	SSD1322_API_data(end_row);
}

//====================== send pixel data to display ========================//
/**
 *  @brief Sends pixels buffer to SSD1322 GRAM memory.
 *
 *  This function should be always preceded by SSD1322_API_set_window() to specify range of rows and columns.
 *
 *  @param[in] buffer array of pixel values
 *  @param[in] buffer_size amount of bytes in the array
 */
void SSD1322_API::SSD1322_API_send_buffer(uint8_t *buffer, uint32_t buffer_size)
{
	SSD1322_API_command(ENABLE_RAM_WRITE); // enable write of pixels
	driver_instance->SSD1322_HW_drive_CS_low();
	driver_instance->SSD1322_HW_drive_DC_high();
	driver_instance->SSD1322_HW_SPI_send_array(buffer, buffer_size);
	driver_instance->SSD1322_HW_drive_CS_high();
}

#ifdef __IMXRT1062__
/**
 *  @brief Sends pixel buffer to SSD1322 GRAM memory using DMA (Teensy 4.x only).
 *
 *  This function uses SSD1322_DMA to transfer the buffer via DMA for high-speed, non-blocking display updates.
 *  The function blocks until the DMA transfer is complete or times out.
 *
 *  @param[in] buffer      Array of pixel values to send
 *  @param[in] buffer_size Number of bytes in the buffer
 *  @param[in] dmaBuffer   Pointer to a DMA-capable buffer (must be at least buffer_size bytes)
 */
void SSD1322_API::SSD1322_API_send_buffer_DMA(uint8_t *buffer, uint32_t buffer_size, uint8_t *dmaBuffer) {
	if (!dmaBuffer) {
		Serial.println("SSD1322_API: ERROR - DMA buffer is null!");
		// Fall back to regular SPI
		SSD1322_API_send_buffer(buffer, buffer_size);
		return;
	}

	// Never reuse dmaBuffer while a prior transfer is still reading it.
	uint32_t priorWaitStart = millis();
	while (TsyDMASPI0.remained() > 0) {
		if (millis() - priorWaitStart > 1000) {
			Serial.printf("SSD1322_API: ERROR - prior DMA still busy (%d bytes)\n",
			              TsyDMASPI0.remained());
			driver_instance->SSD1322_HW_drive_CS_high();
			break;
		}
		yield();
	}
	// if (!TsyDMASPI) {
	//  	Serial.println("SSD1322_API: ERROR - DMA SPI object not set!");
	//  	// Fall back to regular SPI
	//  	SSD1322_API_send_buffer(buffer, buffer_size);
	//  	return;
	// }

	// Start Measure DMA CPU busy time
	uint32_t startTime = 0;
	if (DEBUG) {
		startTime = micros();
	}

	// Copy buffer to DMA memory
	memcpy(dmaBuffer, buffer, buffer_size);
	// Send command and prepare for data
    SSD1322_API_set_window(0, 63, 0, 63);
    SSD1322_API_command(SSD1322_WRITE_RAM);
    digitalWrite(config.OLED_DC_PIN, HIGH);
    digitalWrite(config.OLED_CS_PIN, LOW);
	// Use the member dmaSpi pointer for DMA transfer
	if (DEBUG) {
		Serial.println("SSD1322_API: Queueing DMA transfer");
	}
	arm_dcache_flush((void*)dmaBuffer, buffer_size);
	noInterrupts();
	TsyDMASPI0.queue(dmaBuffer, buffer_size);
	interrupts();
	
	// End Measure DMA transfer time
	uint32_t endTime = 0;
	if (DEBUG) {
	   endTime = micros() - startTime;
	   Serial.print("time: ");
	   Serial.print((uint32_t)endTime);
	   Serial.println("uS");	
	   Serial.println("SSD1322_API: Waiting for DMA transfer");
	}
	// Wait for DMA to complete with timeout
	uint32_t timeout = millis() + 1000;
	uint32_t remained = 0;
	
	while ((remained = TsyDMASPI0.remained()) > 0) {
		if (millis() > timeout) {
			Serial.printf("SSD1322_API: ERROR - DMA transfer timeout! %d bytes remaining\n", remained);
			break;
		}
		yield(); // Allow other processing while waiting
	}
	
	// Complete the transfer
	driver_instance->SSD1322_HW_drive_CS_high();
	if (DEBUG) {
		Serial.println("SSD1322_API: DMA transfer complete");
	}
}
#endif

/**
 *  @brief Returns a pointer to the framebuffer.
 *  @return Pointer to the framebuffer array.
 */
uint8_t* SSD1322_API::getFrameBuffer() {
	return framebuffer;
}

/**
 *  @brief Returns the size of the framebuffer in bytes.
 *  @return Size of the framebuffer.
 */
size_t SSD1322_API::getFrameBufferSize() const {
	return FRAMEBUFFER_SIZE;
}

/**
 *  @brief Updates the display with the contents of the framebuffer.
 */
void SSD1322_API::display() {
	//Serial.println("SSD1322_API: Displaying buffer");
	SSD1322_API_set_window(0, 63, 0, 63); // Full window, adjust as needed
	//Serial.println("SSD1322_API: Window set");
#ifdef __IMXRT1062__
	//Serial.println("SSD1322_API: Sending buffer via DMA");
	if (dmaBuffer) {
		SSD1322_API_send_buffer_DMA(framebuffer + (0 * 256 / 2) + 0, FRAMEBUFFER_SIZE, dmaBuffer);
	//	Serial.println("SSD1322_API: DMA buffer sent");
	} else {
		Serial.println("SSD1322_API: ERROR - DMA buffer not initialized, falling back to regular SPI");
		SSD1322_API_send_buffer(framebuffer + (0 * 256 / 2) + 0, FRAMEBUFFER_SIZE);
	}
#else
	Serial.println("SSD1322_API: Sending buffer via SPI");
	SSD1322_API_send_buffer(framebuffer, FRAMEBUFFER_SIZE);
#endif
	//Serial.println("SSD1322_API: Display update complete");
}
