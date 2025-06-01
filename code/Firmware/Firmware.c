/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <HAL/Audio.h>
#include <HAL/I2C.h>
#include <HAL/SD.h>
#include <HAL/Timer.h>

#include "Firmware/ELF.h"
#include "Runtime/CRT.h"
#include "Runtime/File.h"
#include "Runtime/Runtime.h"

typedef void (*call_fn_t)();

void __register_exitproc(void) {}
void __call_exitprocs(void) {}

static int32_t launch_elf(const char* filename)
{
	int32_t fd = file_open(filename, FILE_MODE_READ);
	if (fd <= 0)
		return 1;

	char tmp[256] = {};
	uint32_t jstart = 0;

	ELF32_Header hdr = {};
	file_read(fd, (uint8_t*)&hdr, sizeof(hdr));
	if (hdr.e_machine != 0xf3)
		return 2;

	for (uint32_t i = 0; i < hdr.e_shnum; ++i)
	{
		ELF32_SectionHeader shdr = {};
		file_seek(fd, hdr.e_shoff + i * sizeof(ELF32_SectionHeader), 0);
		file_read(fd, (uint8_t*)&shdr, sizeof(shdr));

		if (
			shdr.sh_type == 0x01 ||	// SHT_PROGBITS
			shdr.sh_type == 0x0e ||	// SHT_INIT_ARRAY
			shdr.sh_type == 0x0f	// SHT_FINI_ARRAY
		)
		{
			if ((shdr.sh_flags & 0x02) == 0x02)	// SHF_ALLOC
			{
				file_seek(fd, shdr.sh_offset, 0);
				for (uint32_t i = 0; i < shdr.sh_size; i += 512)
				{
					uint32_t nb = shdr.sh_size - i;
					if (nb > 512)
						nb = 512;
					if (file_read(fd, (void*)(shdr.sh_addr + i), nb) != nb)
						return 3;
				}
			}
		}
		else if (shdr.sh_type == 0x02)	// SHT_SYMTAB
		{
			ELF32_SectionHeader shdr_link;
			file_seek(fd, hdr.e_shoff + shdr.sh_link * sizeof(ELF32_SectionHeader), 0);
			file_read(fd, (uint8_t*)&shdr_link, sizeof(shdr_link));

			for (int32_t j = 0; j < shdr.sh_size; j += sizeof(ELF32_Sym))
			{
				ELF32_Sym sym = {};
				file_seek(fd, shdr.sh_offset + j, 0);
				file_read(fd, (uint8_t*)&sym, sizeof(sym));

				file_seek(fd, shdr_link.sh_offset + sym.st_name, 0);
				file_read(fd, tmp, sym.st_size);

				tmp[sym.st_size] = 0;

				if (strcmp(tmp, "_start") == 0)
				{
					jstart = sym.st_value;
					break;
				}
			}
		}
	}

	file_close(fd);

	if (jstart != 0)
	{
		const uint32_t sp = 0x20000000 + /*sysreg_read(SR_REG_RAM_SIZE)*/ 0x01000000 - 0x8;
		__asm__ volatile (
			"fence					\n"
			"mv		sp, %0			\n"
			:
			: "r" (sp)
		);
		((call_fn_t)jstart)();
	}
	
	return 4;
}




#define TLV320DAC3100_I2CADDR_DEFAULT 0x18 ///< Default I2C address

#define TLV320DAC3100_REG_PAGE_SELECT 0x00 ///< Page select register
#define TLV320DAC3100_REG_RESET 0x01       ///< Reset register
#define TLV320DAC3100_REG_OT_FLAG 0x03     ///< Over-temperature flag register
#define TLV320DAC3100_REG_CLOCK_MUX1 0x04  ///< Clock muxing control register 1
#define TLV320DAC3100_REG_PLL_PROG_PR 0x05 ///< PLL P and R values
#define TLV320DAC3100_REG_PLL_PROG_J 0x06  ///< PLL J value
#define TLV320DAC3100_REG_PLL_PROG_D_MSB 0x07   ///< PLL D value MSB
#define TLV320DAC3100_REG_PLL_PROG_D_LSB 0x08   ///< PLL D value LSB
#define TLV320DAC3100_REG_NDAC 0x0B             ///< NDAC divider value
#define TLV320DAC3100_REG_MDAC 0x0C             ///< MDAC divider value
#define TLV320DAC3100_REG_DOSR 0x0D             ///< DOSR divider value MSB/LSB
#define TLV320DAC3100_REG_DOSR_MSB 0x0D         ///< DOSR divider value MSB
#define TLV320DAC3100_REG_DOSR_LSB 0x0E         ///< DOSR divider value LSB
#define TLV320DAC3100_REG_CLKOUT_MUX 0x19       ///< CLKOUT MUX register
#define TLV320DAC3100_REG_CLKOUT_M 0x1A         ///< CLKOUT M divider value
#define TLV320DAC3100_REG_CODEC_IF_CTRL1 0x1B   ///< Codec Interface Control 1
#define TLV320DAC3100_REG_DATA_SLOT_OFFSET 0x1C ///< Data-slot offset register
#define TLV320DAC3100_REG_BCLK_N 0x1E           ///< BCLK N divider value
#define TLV320DAC3100_REG_DAC_FLAG 0x25         ///< DAC Flag register
#define TLV320DAC3100_REG_DAC_FLAG2 0x26        ///< DAC Flag register 2
#define TLV320DAC3100_REG_INT1_CTRL 0x30        ///< INT1 Control Register
#define TLV320DAC3100_REG_INT2_CTRL 0x31        ///< INT2 Control Register
#define TLV320DAC3100_REG_GPIO1_CTRL 0x33 ///< GPIO1 In/Out Pin Control Register
#define TLV320DAC3100_REG_DIN_CTRL 0x36   ///< DIN Pin Control Register
#define TLV320DAC3100_REG_DAC_PRB                                              \
  0x3C ///< DAC Processing Block Selection Register
#define TLV320DAC3100_REG_DAC_DATAPATH 0x3F ///< DAC Data-Path Setup Register
#define TLV320DAC3100_REG_DAC_VOL_CTRL 0x40 ///< DAC Volume Control Register
#define TLV320DAC3100_REG_DAC_LVOL 0x41 ///< DAC Left Volume Control Register
#define TLV320DAC3100_REG_DAC_RVOL 0x42 ///< DAC Right Volume Control Register
#define TLV320DAC3100_REG_HEADSET_DETECT 0x43 ///< Headset Detection Register
#define TLV320DAC3100_REG_BEEP_L 0x47         ///< Left Beep Generator Register
#define TLV320DAC3100_REG_BEEP_R 0x48         ///< Right Beep Generator Register
#define TLV320DAC3100_REG_BEEP_LEN_MSB 0x49   ///< Beep Length MSB Register
#define TLV320DAC3100_REG_BEEP_LEN_MID                                         \
  0x4A ///< Beep Length Middle Bits Register
#define TLV320DAC3100_REG_BEEP_LEN_LSB 0x4B ///< Beep Length LSB Register
#define TLV320DAC3100_REG_BEEP_SIN_MSB 0x4C ///< Beep Sin(x) MSB Register
#define TLV320DAC3100_REG_BEEP_SIN_LSB 0x4D ///< Beep Sin(x) LSB Register
#define TLV320DAC3100_REG_BEEP_COS_MSB 0x4E ///< Beep Cos(x) MSB Register
#define TLV320DAC3100_REG_BEEP_COS_LSB 0x4F ///< Beep Cos(x) LSB Register
#define TLV320DAC3100_REG_VOL_ADC_CTRL                                         \
  0x74 ///< VOL/MICDET-Pin SAR ADC Control Register
#define TLV320DAC3100_REG_VOL_ADC_READ 0x75 ///< VOL/MICDET-Pin Gain Register

// Page 1
#define TLV320DAC3100_REG_BCLK_CTRL2 0x1D ///< BCLK Control Register 2
#define TLV320DAC3100_REG_HP_SPK_ERR_CTL                                       \
  0x1E ///< Headphone and Speaker Error Control Register
#define TLV320DAC3100_REG_HP_DRIVERS 0x1F ///< Headphone Drivers Register
#define TLV320DAC3100_REG_SPK_AMP 0x20 ///< Class-D Speaker Amplifier Register
#define TLV320DAC3100_REG_HP_POP                                               \
  0x21 ///< HP Output Drivers POP Removal Settings Register
#define TLV320DAC3100_REG_PGA_RAMP                                             \
  0x22 ///< Output Driver PGA Ramp-Down Period Control Register
#define TLV320DAC3100_REG_OUT_ROUTING                                          \
  0x23                                 ///< DAC Output Mixer Routing Register
#define TLV320DAC3100_REG_HPL_VOL 0x24 ///< Left Analog Volume to HPL Register
#define TLV320DAC3100_REG_HPR_VOL 0x25 ///< Right Analog Volume to HPR Register
#define TLV320DAC3100_REG_SPK_VOL 0x26 ///< Left Analog Volume to SPK Register
#define TLV320DAC3100_REG_HPL_DRIVER 0x28 ///< HPL Driver Register
#define TLV320DAC3100_REG_HPR_DRIVER 0x29 ///< HPR Driver Register
#define TLV320DAC3100_REG_SPK_DRIVER 0x2A ///< Class-D Speaker Driver Register
#define TLV320DAC3100_REG_HP_DRIVER_CTRL 0x2C ///< HP Driver Control Register
#define TLV320DAC3100_REG_MICBIAS 0x2E  ///< MICBIAS Configuration Register
#define TLV320DAC3100_REG_INPUT_CM 0x32 ///< Input Common Mode Settings Register
#define TLV320DAC3100_REG_TIMER_MCLK_DIV                                       \
  0x10 ///< Timer Clock MCLK Divider Register
#define TLV320DAC3100_REG_IRQ_FLAGS_STICKY                                     \
  0x2C                                   ///< Interrupt Flags - Sticky Register
#define TLV320DAC3100_REG_IRQ_FLAGS 0x2E ///< Interrupt Flags - DAC Register

// IRQ Flag bits
#define TLV320DAC3100_IRQ_HPL_SHORT                                            \
  0x80 ///< Short circuit detected at HPL / left class-D driver
#define TLV320DAC3100_IRQ_HPR_SHORT                                            \
  0x40 ///< Short circuit detected at HPR / right class-D driver
#define TLV320DAC3100_IRQ_BUTTON_PRESS 0x20 ///< Headset button pressed
#define TLV320DAC3100_IRQ_HEADSET_DETECT                                       \
  0x10 ///< Headset insertion detected (1) or removal detected (0)
#define TLV320DAC3100_IRQ_LEFT_DRC                                             \
  0x08 ///< Left DAC signal power greater than DRC threshold
#define TLV320DAC3100_IRQ_RIGHT_DRC                                            \
  0x04 ///< Right DAC signal power greater than DRC threshold


/*!
 * @brief Headset detection debounce time options
 */
typedef enum {
  TLV320_DEBOUNCE_16MS = 0b000,  ///< 16ms debounce (2ms clock)
  TLV320_DEBOUNCE_32MS = 0b001,  ///< 32ms debounce (4ms clock)
  TLV320_DEBOUNCE_64MS = 0b010,  ///< 64ms debounce (8ms clock)
  TLV320_DEBOUNCE_128MS = 0b011, ///< 128ms debounce (16ms clock)
  TLV320_DEBOUNCE_256MS = 0b100, ///< 256ms debounce (32ms clock)
  TLV320_DEBOUNCE_512MS = 0b101, ///< 512ms debounce (64ms clock)
} tlv320_detect_debounce_t;

/*!
 * @brief Button press debounce time options
 */
typedef enum {
  TLV320_BTN_DEBOUNCE_0MS = 0b00,  ///< No debounce
  TLV320_BTN_DEBOUNCE_8MS = 0b01,  ///< 8ms debounce (1ms clock)
  TLV320_BTN_DEBOUNCE_16MS = 0b10, ///< 16ms debounce (2ms clock)
  TLV320_BTN_DEBOUNCE_32MS = 0b11, ///< 32ms debounce (4ms clock)
} tlv320_button_debounce_t;

/*!
 * @brief Headset detection status
 */
typedef enum {
  TLV320_HEADSET_NONE = 0b00,        ///< No headset detected
  TLV320_HEADSET_WITHOUT_MIC = 0b01, ///< Headset without microphone
  TLV320_HEADSET_WITH_MIC = 0b11,    ///< Headset with microphone
} tlv320_headset_status_t;

/*!
 * @brief DAC channel data path options
 */
typedef enum {
  TLV320_DAC_PATH_OFF = 0b00,     ///< DAC data path off
  TLV320_DAC_PATH_NORMAL = 0b01,  ///< Normal path (L->L or R->R)
  TLV320_DAC_PATH_SWAPPED = 0b10, ///< Swapped path (R->L or L->R)
  TLV320_DAC_PATH_MIXED = 0b11,   ///< Mixed L+R path
} tlv320_dac_path_t;

/*!
 * @brief DAC volume control soft stepping options
 */
typedef enum {
  TLV320_VOLUME_STEP_1SAMPLE = 0b00,  ///< One step per sample
  TLV320_VOLUME_STEP_2SAMPLE = 0b01,  ///< One step per two samples
  TLV320_VOLUME_STEP_DISABLED = 0b10, ///< Soft stepping disabled
} tlv320_volume_step_t;

/*!
 * @brief DAC volume control configuration options
 */
typedef enum {
  TLV320_VOL_INDEPENDENT = 0b00,   ///< Left and right channels independent
  TLV320_VOL_LEFT_TO_RIGHT = 0b01, ///< Left follows right volume
  TLV320_VOL_RIGHT_TO_LEFT = 0b10, ///< Right follows left volume
} tlv320_vol_control_t;

/*!
 * @brief Clock source options for CODEC_CLKIN
 */
typedef enum {
  TLV320DAC3100_CODEC_CLKIN_MCLK = 0b00,  ///< MCLK pin is the source
  TLV320DAC3100_CODEC_CLKIN_BCLK = 0b01,  ///< BCLK pin is the source
  TLV320DAC3100_CODEC_CLKIN_GPIO1 = 0b10, ///< GPIO1 pin is the source
  TLV320DAC3100_CODEC_CLKIN_PLL = 0b11,   ///< PLL_CLK pin is the source
} tlv320dac3100_codec_clkin_t;

/*!
 * @brief Clock source options for PLL_CLKIN
 */
typedef enum {
  TLV320DAC3100_PLL_CLKIN_MCLK = 0b00,  ///< MCLK pin is the source
  TLV320DAC3100_PLL_CLKIN_BCLK = 0b01,  ///< BCLK pin is the source
  TLV320DAC3100_PLL_CLKIN_GPIO1 = 0b10, ///< GPIO1 pin is the source
  TLV320DAC3100_PLL_CLKIN_DIN = 0b11    ///< DIN pin is the source
} tlv320dac3100_pll_clkin_t;

/*!
 * @brief Clock divider input source options
 */
typedef enum {
  TLV320DAC3100_CDIV_CLKIN_MCLK = 0b000, ///< MCLK (device pin)
  TLV320DAC3100_CDIV_CLKIN_BCLK = 0b001, ///< BCLK (device pin)
  TLV320DAC3100_CDIV_CLKIN_DIN =
      0b010, ///< DIN (for systems where DAC is not required)
  TLV320DAC3100_CDIV_CLKIN_PLL = 0b011, ///< PLL_CLK (generated on-chip)
  TLV320DAC3100_CDIV_CLKIN_DAC =
      0b100, ///< DAC_CLK (DAC DSP clock - generated on-chip)
  TLV320DAC3100_CDIV_CLKIN_DAC_MOD = 0b101, ///< DAC_MOD_CLK (generated on-chip)
} tlv320dac3100_cdiv_clkin_t;

/*!
 * @brief Data length for I2S interface
 */
typedef enum {
  TLV320DAC3100_DATA_LEN_16 = 0b00, ///< 16 bits
  TLV320DAC3100_DATA_LEN_20 = 0b01, ///< 20 bits
  TLV320DAC3100_DATA_LEN_24 = 0b10, ///< 24 bits
  TLV320DAC3100_DATA_LEN_32 = 0b11, ///< 32 bits
} tlv320dac3100_data_len_t;

/*!
 * @brief Data format for I2S interface
 */
typedef enum {
  TLV320DAC3100_FORMAT_I2S = 0b00, ///< I2S format
  TLV320DAC3100_FORMAT_DSP = 0b01, ///< DSP format
  TLV320DAC3100_FORMAT_RJF = 0b10, ///< Right justified format
  TLV320DAC3100_FORMAT_LJF = 0b11, ///< Left justified format
} tlv320dac3100_format_t;

/*!
 * @brief GPIO1 pin mode options
 */
typedef enum {
  TLV320_GPIO1_DISABLED =
      0b0000, ///< GPIO1 disabled (input and output buffers powered down)
  TLV320_GPIO1_INPUT_MODE =
      0b0001, ///< Input mode (secondary BCLK/WCLK/DIN input or ClockGen)
  TLV320_GPIO1_GPI = 0b0010,      ///< General-purpose input
  TLV320_GPIO1_GPO = 0b0011,      ///< General-purpose output
  TLV320_GPIO1_CLKOUT = 0b0100,   ///< CLKOUT output
  TLV320_GPIO1_INT1 = 0b0101,     ///< INT1 output
  TLV320_GPIO1_INT2 = 0b0110,     ///< INT2 output
  TLV320_GPIO1_BCLK_OUT = 0b1000, ///< Secondary BCLK output for codec interface
  TLV320_GPIO1_WCLK_OUT = 0b1001, ///< Secondary WCLK output for codec interface
} tlv320_gpio1_mode_t;

/*!
 * @brief DIN pin mode options
 */
typedef enum {
  TLV320_DIN_DISABLED = 0b00, ///< DIN disabled (input buffer powered down)
  TLV320_DIN_ENABLED = 0b01,  ///< DIN enabled (for codec interface/ClockGen)
  TLV320_DIN_GPI = 0b10,      ///< DIN used as general-purpose input
} tlv320_din_mode_t;

/*!
 * @brief Volume ADC hysteresis options
 */
typedef enum {
  TLV320_VOL_HYST_NONE = 0b00, ///< No hysteresis
  TLV320_VOL_HYST_1BIT = 0b01, ///< ±1 bit hysteresis
  TLV320_VOL_HYST_2BIT = 0b10, ///< ±2 bit hysteresis
} tlv320_vol_hyst_t;

/*!
 * @brief Volume ADC throughput rates
 */
typedef enum {
  TLV320_VOL_RATE_15_625HZ = 0b000, ///< 15.625 Hz (MCLK) or 10.68 Hz (RC)
  TLV320_VOL_RATE_31_25HZ = 0b001,  ///< 31.25 Hz (MCLK) or 21.35 Hz (RC)
  TLV320_VOL_RATE_62_5HZ = 0b010,   ///< 62.5 Hz (MCLK) or 42.71 Hz (RC)
  TLV320_VOL_RATE_125HZ = 0b011,    ///< 125 Hz (MCLK) or 85.2 Hz (RC)
  TLV320_VOL_RATE_250HZ = 0b100,    ///< 250 Hz (MCLK) or 170 Hz (RC)
  TLV320_VOL_RATE_500HZ = 0b101,    ///< 500 Hz (MCLK) or 340 Hz (RC)
  TLV320_VOL_RATE_1KHZ = 0b110,     ///< 1 kHz (MCLK) or 680 Hz (RC)
  TLV320_VOL_RATE_2KHZ = 0b111,     ///< 2 kHz (MCLK) or 1.37 kHz (RC)
} tlv320_vol_rate_t;

/*!
 * @brief Headphone common-mode settings
 */
typedef enum {
  TLV320_HP_COMMON_1_35V = 0b00, ///< Common-mode voltage 1.35V
  TLV320_HP_COMMON_1_50V = 0b01, ///< Common-mode voltage 1.50V
  TLV320_HP_COMMON_1_65V = 0b10, ///< Common-mode voltage 1.65V
  TLV320_HP_COMMON_1_80V = 0b11, ///< Common-mode voltage 1.80V
} tlv320_hp_common_t;

/*!
 * @brief Headphone driver power-on time options
 */
typedef enum {
  TLV320_HP_TIME_0US = 0b0000,   ///< 0 microseconds
  TLV320_HP_TIME_15US = 0b0001,  ///< 15.3 microseconds
  TLV320_HP_TIME_153US = 0b0010, ///< 153 microseconds
  TLV320_HP_TIME_1_5MS = 0b0011, ///< 1.53 milliseconds
  TLV320_HP_TIME_15MS = 0b0100,  ///< 15.3 milliseconds
  TLV320_HP_TIME_76MS = 0b0101,  ///< 76.2 milliseconds
  TLV320_HP_TIME_153MS = 0b0110, ///< 153 milliseconds
  TLV320_HP_TIME_304MS = 0b0111, ///< 304 milliseconds
  TLV320_HP_TIME_610MS = 0b1000, ///< 610 milliseconds
  TLV320_HP_TIME_1_2S = 0b1001,  ///< 1.22 seconds
  TLV320_HP_TIME_3S = 0b1010,    ///< 3.04 seconds
  TLV320_HP_TIME_6S = 0b1011,    ///< 6.1 seconds
} tlv320_hp_time_t;

/*!
 * @brief Headphone driver ramp-up step time options
 */
typedef enum {
  TLV320_RAMP_0MS = 0b00, ///< 0 milliseconds
  TLV320_RAMP_1MS = 0b01, ///< 0.98 milliseconds
  TLV320_RAMP_2MS = 0b10, ///< 1.95 milliseconds
  TLV320_RAMP_4MS = 0b11, ///< 3.9 milliseconds
} tlv320_ramp_time_t;

/*!
 * @brief Speaker power-up wait time options
 */
typedef enum {
  TLV320_SPK_WAIT_0MS = 0b000,  ///< 0 milliseconds
  TLV320_SPK_WAIT_3MS = 0b001,  ///< 3.04 milliseconds
  TLV320_SPK_WAIT_7MS = 0b010,  ///< 7.62 milliseconds
  TLV320_SPK_WAIT_12MS = 0b011, ///< 12.2 milliseconds
  TLV320_SPK_WAIT_15MS = 0b100, ///< 15.3 milliseconds
  TLV320_SPK_WAIT_19MS = 0b101, ///< 19.8 milliseconds
  TLV320_SPK_WAIT_24MS = 0b110, ///< 24.4 milliseconds
  TLV320_SPK_WAIT_30MS = 0b111, ///< 30.5 milliseconds
} tlv320_spk_wait_t;

/*!
 * @brief DAC output routing options
 */
typedef enum {
  TLV320_DAC_ROUTE_NONE = 0b00,  ///< DAC not routed
  TLV320_DAC_ROUTE_MIXER = 0b01, ///< DAC routed to mixer amplifier
  TLV320_DAC_ROUTE_HP = 0b10,    ///< DAC routed directly to HP driver
} tlv320_dac_route_t;

/*!
 * @brief MICBIAS voltage options
 */
typedef enum {
  TLV320_MICBIAS_OFF = 0b00,  ///< MICBIAS powered down
  TLV320_MICBIAS_2V = 0b01,   ///< MICBIAS = 2V
  TLV320_MICBIAS_2_5V = 0b10, ///< MICBIAS = 2.5V
  TLV320_MICBIAS_AVDD = 0b11, ///< MICBIAS = AVDD
} tlv320_micbias_volt_t;

/*!
 * @brief Speaker amplifier gain options
 */
typedef enum {
  TLV320_SPK_GAIN_6DB = 0b00,  ///< 6 dB gain
  TLV320_SPK_GAIN_12DB = 0b01, ///< 12 dB gain
  TLV320_SPK_GAIN_18DB = 0b10, ///< 18 dB gain
  TLV320_SPK_GAIN_24DB = 0b11, ///< 24 dB gain
} tlv320_spk_gain_t;

/*!
 * @brief BCLK source settings
 */
typedef enum {
  TLV320DAC3100_BCLK_SRC_DAC_CLK = 0,
  TLV320DAC3100_BCLK_SRC_DAC_MOD_CLK = 1,
} tlv320dac3100_bclk_src_t;


void tlv320_setPage(int page)
{
	hal_i2c_write(TLV320DAC3100_I2CADDR_DEFAULT, TLV320DAC3100_REG_PAGE_SELECT, page);
}


uint8_t register_set(uint8_t* current, uint8_t value, int offset, int nbits)
{
	uint8_t mask = 0;
	for (int i = 0; i < nbits; ++i)
	{
		mask |= 1 << (i + offset);
	}

	*current &= ~mask;
	*current |= (value << offset) & mask;

	return *current;
}


void i2c_write_and_verify(uint8_t device, uint8_t addr, uint8_t value)
{
	hal_i2c_write(device, addr, value);

	uint8_t data = 0;
	hal_i2c_read(device, addr, &data, 1);
	if (data != value)
	{
		printf("NOT CORRECT VALUE, is 0x%02x, expect 0x%02x (addr 0x%02x)\n\r", data, value, addr);
	}
}



void tlv320_write(uint8_t reg, uint8_t data)
{
	hal_i2c_write(0x18, reg, data);
}


void main(int argc, const char** argv)
{
	// Initialize SP, since we hot restart and startup doesn't set SP.
	const uint32_t sp = 0x20000000 + 0x00800000;
	__asm__ volatile (
		"mv sp, %0	\n"
		:
		: "r" (sp)
	);

	// // Initialize segments when running from ROM.
	// {
	// 	extern uint8_t INIT_DATA_VALUES;
	// 	extern uint8_t INIT_DATA_START;
	// 	extern uint8_t INIT_DATA_END;
	// 	uint8_t* src = (uint8_t*)&INIT_DATA_VALUES;
	// 	uint8_t* dest = (uint8_t*)&INIT_DATA_START;
	// 	uint32_t len = (uint32_t)(&INIT_DATA_END - &INIT_DATA_START);
	// 	memcpy(dest, src, len);
	// }
	// {
	// 	extern uint8_t BSS_START;
	// 	extern uint8_t BSS_END;
    //     uint8_t* dest = (uint8_t*)&BSS_START;
    //     uint32_t len = (uint32_t)(&BSS_END - &BSS_START);
	// 	memset(dest, 0, len);
	// }

	crt_init();

	hal_sd_init(SD_INTERNAL_BASE, SD_MODE_SW);
	hal_sd_init(SD_EXTERNAL_BASE, SD_MODE_SW);

	file_init();

	launch_elf("Dashboard");

	for (;;);
}
