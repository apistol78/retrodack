/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

`timescale 1ns/1ns
`default_nettype none

`define FREQUENCY 100_000_000

(* top *)
module RetroDACK(
	`include "RetroDACK_IO.sv"
);
	wire clock;
	wire clock_sdram;
	wire clock_video;
	wire clock_locked;

	assign LED_R = cpu_fault;
	assign LED_G = cpu_external_interrupt; //~cpu_fault;
	assign LED_B = cpu_timer_interrupt; // sd_CARD;

	assign DEBUG_0 = 1'b0; //(I2C_SCL_direction == 1'b0 && I2C_SDA == 1'b0);
	assign DEBUG_1 = 1'b0; //I2C_SCL_direction;	// 1 = CPU own SCL
	assign DEBUG_2 = I2C_SDA_direction;	// 1 = CPU own SDA
	assign DEBUG_3 = I2C_SCL;
	assign DEBUG_4 = I2C_SDA;

	// Unimplemeneted pins.
	assign LCD_CS = 1'b0;
	assign USB_HCI_RESET_n = ~reset;

	// assign SRAM_BLE = 1'b0;
	// assign SRAM_BHE = 1'b0;

	// assign LCD_R[0] = I2S_SCLK;
	// assign LCD_R[1] = I2S_MCLK;
	// assign LCD_R[2] = I2S_LRCK;
	// assign LCD_R[3] = I2S_SDOUT;
	// assign LCD_R[4] = audio_busy;

	// assign LCD_R[1] = USB_HCI_MISO;
	// assign LCD_R[2] = USB_HCI_SS_n;
	// assign LCD_R[3] = USB_HCI_SCLK;
	// assign LCD_R[4] = USB_HCI_INT;

	// assign LCD_R[5] = 1'b0;
	


	//====================================================


	// https://blog.dave.tf/post/ecp5-pll/
	//
	// Input CLOCK is 25 MHz
	// 100 MHz
	// 100 MHz (7000 ps phase shift)
	// 33.3 MHz
	wire pll_locked;
	PLL_ECP5 #(
		.CLKI_DIV(1),
		.CLKFB_DIV(4),
		
		.CLKOP_DIV(4),
		.CLKOP_CPHASE(4 - 1),
		
		.CLKOS_DIV(4),
		.CLKOS_CPHASE(5),
		
		.CLKOS2_DIV(4*3),
		.CLKOS2_CPHASE(4*3 - 1)
	) pll (
		.i_clk(CLOCK),
		.o_clk1(clock),
		.o_clk2(clock_sdram),
		.o_clk3(clock_video),
		.o_clk_locked(pll_locked)
	);

	assign clock_locked = pll_locked;


	//====================================================


	// CLK need to be tunneled through a primitive
	// since it's not accessible as a user pin.
	wire FLASH_CLK;
	wire tristate = 1'b0;
	USRMCLK u1 (.USRMCLKI(FLASH_CLK), .USRMCLKTS(tristate));


	//====================================================
	// Reset
	wire reset;
	wire uart_soft_reset;

	Reset rst(
		.i_clock(clock),
		.i_reset(uart_soft_reset || !clock_locked),
		.o_reset(reset)
	);


	//====================================================
	// CPU
	wire cpu_timer_interrupt;
	wire cpu_external_interrupt;
	wire cpu_ibus_request;
	wire cpu_ibus_ready;
	wire [31:0] cpu_ibus_address;
	wire [31:0] cpu_ibus_rdata;
	wire cpu_dbus_rw;
	wire cpu_dbus_request;
	wire cpu_dbus_ready;
	wire [31:0] cpu_dbus_address;
	wire [31:0] cpu_dbus_rdata;
	wire [31:0] cpu_dbus_wdata;
	wire [3:0] cpu_dbus_wmask;
	wire cpu_fault;

	CPU #(
		.STACK_POINTER(32'h12000000 - 4),
		.FREQUENCY(`FREQUENCY),
		.DCACHE_SIZE(12),
		.DCACHE_REGISTERED(1),
		.DCACHE_WB_QUEUE(1),
		.ICACHE_SIZE(12),
		.ICACHE_REGISTERED(1)		
	) cpu(
		.i_reset(reset),
		.i_clock(clock),

		// Control
		.i_timer_interrupt(cpu_timer_interrupt),
		.i_external_interrupt(cpu_external_interrupt),

		// Instruction bus
		.o_ibus_request(cpu_ibus_request),
		.i_ibus_ready(cpu_ibus_ready),
		.o_ibus_address(cpu_ibus_address),
		.i_ibus_rdata(cpu_ibus_rdata),

		// Data bus
		.o_dbus_rw(cpu_dbus_rw),
		.o_dbus_request(cpu_dbus_request),
		.i_dbus_ready(cpu_dbus_ready),
		.o_dbus_address(cpu_dbus_address),
		.i_dbus_rdata(cpu_dbus_rdata),
		.o_dbus_wdata(cpu_dbus_wdata),
		.o_dbus_wmask(cpu_dbus_wmask),

		// Debug
		.o_execute_busy(),
		.o_memory_busy(),
		.o_fault(cpu_fault)
	);


	//===========================================================
	//===========================================================
	// Everything below is connected on the XBAR.
	//===========================================================
	//===========================================================


	//====================================================
	// SPI Flash
	wire spif_request;
	wire [31:0] spif_address;
	wire [31:0] spif_rdata;
	wire spif_ready;

	SPI_Flash spif(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(spif_request),
		.i_address({ 4'h0, spif_address[27:0] + 28'h100000 }),
		.o_rdata(spif_rdata),
		.o_ready(spif_ready),

		.SPI_nCS(FLASH_nCS),
		.SPI_CLK(FLASH_CLK),
		.SPI_MOSI(FLASH_MOSI),
		.SPI_MISO(FLASH_MISO)
	);


	//=====================================
    // SDRAM
	wire sdram_request;
	wire sdram_rw;
	wire [31:0] sdram_address;
	wire [31:0] sdram_rdata;
	wire [31:0] sdram_wdata;
	wire [3:0] sdram_wmask;
	wire sdram_ready;

	bit [15:0] it_sdram_data_r;
	wire [15:0] it_sdram_data_w;
	wire it_sdram_data_rw;

	assign SDRAM_DQ = it_sdram_data_rw ? it_sdram_data_w : 16'hz;
	assign it_sdram_data_r = SDRAM_DQ;

    SDRAM_controller #(
        .FREQUENCY(`FREQUENCY),
		.SDRAM_DATA_WIDTH(16)
    ) sdram(
	    .i_reset(reset),
	    .i_clock(clock),
		.i_clock_sdram(clock_sdram),

	    .i_request(sdram_request),
	    .i_rw(sdram_rw),
	    .i_address(sdram_address),
	    .i_wdata(sdram_wdata),
		.i_wmask(sdram_wmask),
	    .o_rdata(sdram_rdata),
	    .o_ready(sdram_ready),

	    .sdram_clk(SDRAM_CLK),
	    .sdram_clk_en(SDRAM_CKE),
	    .sdram_cas_n(SDRAM_CAS_n),
	    .sdram_cs_n(SDRAM_CE_n),
	    .sdram_ras_n(SDRAM_RAS_n),
	    .sdram_we_n(SDRAM_WE_n),
	    .sdram_dqm(SDRAM_DQM),
	    .sdram_bs(SDRAM_BA),
	    .sdram_addr(SDRAM_A),
		.sdram_rdata(it_sdram_data_r),
		.sdram_wdata(it_sdram_data_w),
		.sdram_data_rw(it_sdram_data_rw)
    );


	//====================================================
	// UART
	wire uart_request;
	wire uart_rw;
	wire [31:0] uart_address;
	wire [31:0] uart_wdata;
	wire [31:0] uart_rdata;
	wire uart_ready;

	UART #(
		.FREQUENCY(`FREQUENCY),
		.BAUDRATE(115200),
		.RX_FIFO_DEPTH(1024),
		.TX_FIFO_DEPTH(32)
	) uart(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(uart_request),
		.i_rw(uart_rw),
		.i_address(uart_address[3:2]),
		.i_wdata(uart_wdata),
		.o_rdata(uart_rdata),
		.o_ready(uart_ready),
		.o_interrupt(),
		.o_soft_reset(uart_soft_reset),
		// ---
		.UART_RX(UART_RX),
		.UART_TX(UART_TX)
	);


	//====================================================
	// I2C
	wire i2c_request;
	wire i2c_rw;
	wire [31:0] i2c_address;
	wire [31:0] i2c_wdata;
	wire [31:0] i2c_rdata;
	wire i2c_ready;

	// wire I2C_SCL_direction;
	// wire I2C_SCL_w;

	wire I2C_SDA_direction;
	wire I2C_SDA_w;

	// assign I2C_SCL = I2C_SCL_direction ? I2C_SCL_w : 1'bz;
	assign I2C_SDA = I2C_SDA_direction ? I2C_SDA_w : 1'bz;

	I2C_v2 #(
		// .DELAY_FAST(400),
		// .DELAY_SLOW(1400)
		.DELAY(4000)
	) i2c (
		.i_reset(reset),
		.i_clock(clock),
		.i_request(i2c_request),
		.i_rw(i2c_rw),
		.i_address(i2c_address[3:2]),
		.i_wdata(i2c_wdata),
		.o_rdata(i2c_rdata),
		.o_ready(i2c_ready),
		// ---

		.I2C_SCL(I2C_SCL),
		.I2C_SDA_direction(I2C_SDA_direction),
		.I2C_SDA_r(I2C_SDA),
		.I2C_SDA_w(I2C_SDA_w)

		// .I2C_SCL_direction(I2C_SCL_direction),
		// .I2C_SCL_r(I2C_SCL),
		// .I2C_SCL_w(I2C_SCL_w),

		// .I2C_SDA_direction(I2C_SDA_direction),
		// .I2C_SDA_r(I2C_SDA),
		// .I2C_SDA_w(I2C_SDA_w)
	);


	//====================================================
	// SD

	wire bd_sd_card;

	Debounce bd_sd(
		.i_clock(clock),
		.i_data(SD_EXTERNAL_CARD),
		.o_stable(bd_sd_card)
	);

	wire sd_request;
	wire sd_rw;
	wire [31:0] sd_address;
	wire [31:0] sd_wdata;
	wire [31:0] sd_rdata;
	wire sd_ready;

	wire sd_cmd_dir;
	wire sd_cmd_out;
	assign SD_EXTERNAL_CMD = sd_cmd_dir ? sd_cmd_out : 1'bz;

	wire sd_dat_dir;
	wire [3:0] sd_dat_out;
	assign SD_EXTERNAL_DAT = sd_dat_dir ? sd_dat_out : 4'bz; 

	SD sd(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(sd_request),
		.i_rw(sd_rw),
		.i_address(sd_address[1:0]),
		.i_wdata(sd_wdata),
		.o_rdata(sd_rdata),
		.o_ready(sd_ready),

		.SD_CLK(SD_EXTERNAL_CLK),
		.SD_CMD_dir(sd_cmd_dir),
		.SD_CMD_in(SD_EXTERNAL_CMD),
		.SD_CMD_out(sd_cmd_out),
		.SD_DAT_dir(sd_dat_dir),
		.SD_DAT_in(SD_EXTERNAL_DAT),
		.SD_DAT_out(sd_dat_out),
		.SD_CARD(bd_sd_card)
	);


	//====================================================
	// TIMER
	wire timer_request;
	wire timer_rw;
	wire [31:0] timer_address;
	wire [31:0] timer_wdata;
	wire [31:0] timer_rdata;
	wire timer_ready;

	Timer #(
		.FREQUENCY(`FREQUENCY)
	) timer(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(timer_request),
		.i_rw(timer_rw),
		.i_address(timer_address[5:2]),
		.i_wdata(timer_wdata),
		.o_rdata(timer_rdata),
		.o_ready(timer_ready),
		.o_interrupt(cpu_timer_interrupt)
	);


	//====================================================
	// AUDIO
	wire audio_output_sample_clock;
	wire [31:0] audio_output_sample_rate;
	wire [15:0] audio_output_sample_left;
	wire [15:0] audio_output_sample_right;
	wire audio_sdout;
	wire audio_sclk;
	wire audio_lrck;
	wire audio_mclk;

	AUDIO_i2s_output #(
		.FREQUENCY(`FREQUENCY)
	) audio_i2s_output(
		.i_clock(clock),
		.o_sample_clock(audio_output_sample_clock),
		.i_sample_rate(audio_output_sample_rate),
		.i_sample_left(audio_output_sample_left),
		.i_sample_right(audio_output_sample_right),
		.o_i2s_sdout(I2S_SDOUT),
		.o_i2s_sclk(I2S_SCLK),
		.o_i2s_lrck(I2S_LRCK),
		.o_i2s_mclk(I2S_MCLK)
	);

	wire audio_request;
	wire audio_rw;
	wire [31:0] audio_address;
	wire [31:0] audio_wdata;
	wire [31:0] audio_rdata;
	wire audio_ready;

	AUDIO_controller audio_controller(
		.i_reset(reset),
		.i_clock(clock),

		.i_request(audio_request),
		.i_rw(audio_rw),
		.i_address(audio_address[5:2]),
		.i_wdata(audio_wdata),
		.o_rdata(audio_rdata),
		.o_ready(audio_ready),

		.i_output_sample_clock(audio_output_sample_clock),
		.o_output_sample_rate(audio_output_sample_rate),
		.o_output_sample_left(audio_output_sample_left),
		.o_output_sample_right(audio_output_sample_right)
	);


	//====================================================
	// PLIC
	wire plic_interrupt;
	wire plic_request;
	wire plic_rw;
	wire [31:0] plic_address;
	wire [31:0] plic_wdata;
	wire [31:0] plic_rdata;
	wire plic_ready;

	bit [1:0] tbi = 2'b00;
	always_ff @(posedge clock) begin
		tbi <= { tbi[0], ~TRACKBALL_INTERRUPT };
	end

	bit [1:0] kpi = 2'b00;
	always_ff @(posedge clock) begin
		kpi <= { kpi[0], ~KEYPAD_INTERRUPT };
	end

	bit [1:0] vbi = 2'b00;
	always_ff @(posedge clock) begin
		vbi <= { vbi[0], vga_vblank };
	end

	bit [1:0] ubi = 2'b00;
	always_ff @(posedge clock) begin
		ubi <= { ubi[0], ~USB_HCI_INT };
	end

	CPU_PLIC plic(
		.i_reset(reset),
		.i_clock(clock),

		.i_interrupt_0((tbi == 2'b01) || (kpi == 2'b01)),
		.i_interrupt_1(1'b0),
		.i_interrupt_2(vbi == 2'b01),
		.i_interrupt_3(ubi == 2'b01),

		.i_interrupt_enable(1'b1),
		.o_interrupt(cpu_external_interrupt),

		.i_request(plic_request),
		.i_rw(plic_rw),
		.i_address(plic_address[23:0]),
		.i_wdata(plic_wdata),
		.o_rdata(plic_rdata),
		.o_ready(plic_ready)
	);


	//====================================================
	// VIDEO MEMORY

	wire video_sram_request;
	wire video_sram_rw;
	wire [31:0] video_sram_address;
	wire [31:0] video_sram_rdata;
	wire [31:0] video_sram_wdata;
	wire [3:0] video_sram_wmask;
	wire video_sram_ready;

	wire [15:0] video_sram_sram_d_w;
	wire [15:0] video_sram_sram_d_r;
	wire video_sram_sram_d_rw;

	assign SRAM_D = video_sram_sram_d_rw ? video_sram_sram_d_w : 16'hz;
	assign video_sram_sram_d_r = SRAM_D;

	SRAM_controller #(
		.FREQUENCY(`FREQUENCY),
		.SRAM_ADDRESS_WIDTH(20)
	) video_sram(
		.i_reset(reset),
		.i_clock(clock),
		
		.i_request(video_sram_request),
		.i_rw(video_sram_rw),

		.i_address(video_sram_address),
		.o_rdata(video_sram_rdata),
		.i_wdata(video_sram_wdata),
		.i_wmask(video_sram_wmask),
		.o_ready(video_sram_ready),

		.SRAM_A(SRAM_A),
		.SRAM_D_w(video_sram_sram_d_w),
		.SRAM_D_r(video_sram_sram_d_r),
		.SRAM_D_rw(video_sram_sram_d_rw),

		.SRAM_CE_n(),
		.SRAM_OE_n(SRAM_OE),
		.SRAM_WE_n(SRAM_WE),
		.SRAM_LB_n(SRAM_BLE),
		.SRAM_UB_n(SRAM_BHE)
	);


	//====================================================
	// VIDEO MEMORY PORT
	wire vram_pa_request;
	wire vram_pa_rw;
	wire [31:0] vram_pa_address;
	wire [31:0] vram_pa_rdata;
	wire [31:0] vram_pa_wdata;
	wire [3:0] vram_pa_wmask;
	wire vram_pa_ready;

	wire vram_pb_request;
	wire [31:0] vram_pb_address;
	wire [31:0] vram_pb_rdata;
	wire vram_pb_ready;

	DualPort vram_bus(
		.i_reset(reset),
		.i_clock(clock),

		.o_bus_rw(video_sram_rw),
		.o_bus_request(video_sram_request),
		.i_bus_ready(video_sram_ready),
		.o_bus_address(video_sram_address),
		.i_bus_rdata(video_sram_rdata),
		.o_bus_wdata(video_sram_wdata),
		.o_bus_wmask(video_sram_wmask),

		.i_pa_rw(1'b0),
		.i_pa_request(1'b0),
		.o_pa_ready(),
		.i_pa_address(32'h0),
		.o_pa_rdata(),
		.i_pa_wdata(32'h0),
		.i_pa_wmask(4'h0),

		// Video output access.
		.i_pb_rw(1'b0),
		.i_pb_request(vram_pb_request),
		.o_pb_ready(vram_pb_ready),
		.i_pb_address(vram_pb_address),
		.o_pb_rdata(vram_pb_rdata),
		.i_pb_wdata(32'h0),
		.i_pb_wmask(4'h0),

		// Video CPU access.
		.i_pc_rw(vram_pa_rw),
		.i_pc_request(vram_pa_request),
		.o_pc_ready(vram_pa_ready),
		.i_pc_address(vram_pa_address),
		.o_pc_rdata(vram_pa_rdata),
		.i_pc_wdata(vram_pa_wdata),
		.i_pc_wmask(vram_pa_wmask)
	);


	//====================================================

	bit bl_clk_l = 1'b0;
	wire bl_clk;

	ClockDivider #(
		.CLOCK_RATE(`FREQUENCY),
		.BAUD_RATE(100000)
	) clk_div(
		.i_reset(1'b0),
		.i_clock(clock),
		.o_clock(bl_clk)
	);

	bit [7:0] counter = 0;

	always_ff @(posedge clock) begin
		bl_clk_l <= bl_clk;
		if ({ bl_clk_l, bl_clk } == 2'b01) begin
			counter <= counter + 8'h1;
		end
	end

	assign LCD_BACKLIGHT_CTRL = (counter > 90) ? 1'b1 : 1'b0;
	
	//====================================================
	// VIDEO SIGNAL GENERATOR
	wire vga_hblank;
	wire vga_vblank;
	wire [10:0] vga_pos_x;
	wire [10:0] vga_pos_y;
	wire [31:0] video_dac_rdata;

	assign LCD_R = video_dac_rdata[7:0+2];
	assign LCD_G = video_dac_rdata[15:8+2];
	assign LCD_B = video_dac_rdata[23:16+2];

	VIDEO_VGA #(
		// 720 0 20 20 40 720 0 15 15 15 0 0 0 60 0 36720000 4
		.USE_CLOCK_OUT(0),
		.HLINE(720),	// horizontal pixels
		.HBACK(40),		// back porch
		.HFRONT(20),	// front porch
		.HPULSE(20),	// sync pulse
		.VLINE(720),	// vertical lines
		.VBACK(15),		// back porch
		.VFRONT(15),	// front porch
		.VPULSE(15),	// sync pulse
		.VSPOL(0),
		.HSPOL(0)
	) vga(
		.i_clock(clock_video),
		.i_clock_out(clock),
		.o_clock(LCD_CLK),
		.o_hsync(LCD_HSYNC),
		.o_vsync(LCD_VSYNC),
		.o_hblank(vga_hblank),
		.o_vblank(vga_vblank),
		.o_data_enable(LCD_ENABLE),
		.o_pos_x(vga_pos_x),
		.o_pos_y(vga_pos_y)
	);


	//====================================================
	// VIDEO SPRITE
	wire video_sprite_request;
	wire [31:0] video_sprite_address;
	wire [31:0] video_sprite_wdata;
	wire video_sprite_ready;

	wire [10:0] video_overlay_x;
	wire [10:0] video_overlay_y;
	wire [7:0] video_overlay_data;
	wire video_overlay_mask;

	VIDEO_sprite #(
		.WIDTH(64),
		.HEIGHT(64)
	) video_sprite (
		.i_clock(clock),

		.i_request(video_sprite_request),
		.i_address(video_sprite_address),
		.i_wdata(video_sprite_wdata),
		.o_ready(video_sprite_ready),

		.i_video_hblank(vga_hblank),
		.i_video_vblank(vga_vblank),
		.i_overlay_x(video_overlay_x),
		.i_overlay_y(video_overlay_y),
		.o_overlay_data(video_overlay_data),
		.o_overlay_mask(video_overlay_mask)
	);


	//====================================================
	// VIDEO CONTROLLER
	wire video_request;
	wire video_rw;
	wire [31:0] video_address;
	wire [31:0] video_rdata;
	wire [31:0] video_wdata;
	wire [3:0] video_wmask;
	wire video_ready;	

	VIDEO_controller #(
		.MAX_PITCH(720)
	) video_controller(
		.i_clock(clock),
		
		// CPU interface.
		.i_cpu_request(video_request),
		.i_cpu_rw(video_rw),
		.i_cpu_address(video_address),
		.o_cpu_rdata(video_rdata),
		.i_cpu_wdata(video_wdata),
		.i_cpu_wmask(video_wmask),
		.o_cpu_ready(video_ready),
		
		// Video signal interface.
		.i_video_hblank(vga_hblank),
		.i_video_vblank(vga_vblank),
		.i_video_pos_x(vga_pos_x),
		.i_video_pos_y(vga_pos_y),
		.o_video_rdata(video_dac_rdata),
		
		// Video RAM interface.
		.o_vram_pa_request(vram_pa_request),
		.o_vram_pa_rw(vram_pa_rw),
		.o_vram_pa_address(vram_pa_address),
		.i_vram_pa_rdata(vram_pa_rdata),
		.o_vram_pa_wdata(vram_pa_wdata),
		.o_vram_pa_wmask(vram_pa_wmask),
		.i_vram_pa_ready(vram_pa_ready),
		.o_vram_pb_request(vram_pb_request),
		.o_vram_pb_address(vram_pb_address),
		.i_vram_pb_rdata(vram_pb_rdata),
		.i_vram_pb_ready(vram_pb_ready),

		// Overlay
		.o_overlay_x(video_overlay_x),
		.o_overlay_y(video_overlay_y),
		.i_overlay_data(video_overlay_data),
		.i_overlay_mask(video_overlay_mask)
	);


	//====================================================
	// DMA
	wire dma_request;
	wire dma_rw;
	wire [31:0] dma_address;
	wire [31:0] dma_wdata;
	wire [31:0] dma_rdata;
	wire dma_ready;

	wire dma_bus_rw;
	wire dma_bus_request;
	wire dma_bus_ready;
	wire [31:0] dma_bus_address;
	wire [31:0] dma_bus_rdata;
	wire [31:0] dma_bus_wdata;

	DMA dma(
		.i_reset(reset),
		.i_clock(clock),

		.i_request(dma_request),
		.i_rw(dma_rw),
		.i_address(dma_address[27:0]),
		.i_wdata(dma_wdata),
		.o_rdata(dma_rdata),
		.o_ready(dma_ready),
		
		.o_bus_rw(dma_bus_rw),
		.o_bus_request(dma_bus_request),
		.i_bus_ready(dma_bus_ready),
		.o_bus_address(dma_bus_address),
		.i_bus_rdata(dma_bus_rdata),
		.o_bus_wdata(dma_bus_wdata)
	);


	//====================================================
	// SPI
	wire spi_request;
	wire spi_rw;
	wire [31:0] spi_address;
	wire [31:0] spi_wdata;
	wire [31:0] spi_rdata;
	wire spi_ready;

	SPI #(
		.DELAY(0)
	) spi (
		.i_reset(reset),
		.i_clock(clock),
		.i_request(spi_request),
		.i_rw(spi_rw),
		.i_address(spi_address[3:2]),
		.i_wdata(spi_wdata),
		.o_rdata(spi_rdata),
		.o_ready(spi_ready),
		// ---
		.SPI_SS_n(USB_HCI_SS_n),
		.SPI_SCLK(USB_HCI_SCLK),
		.SPI_MOSI(USB_HCI_MOSI),
		.SPI_MISO(USB_HCI_MISO)
	);


	//====================================================
	// XBAR

	XBAR_3_12 xbar(
		.i_reset(reset),
		.i_clock(clock),

		// CPU instruction bus
		.i_m0_rw(1'b0),
		.i_m0_request(cpu_ibus_request),
		.o_m0_ready(cpu_ibus_ready),
		.i_m0_address(cpu_ibus_address),
		.o_m0_rdata(cpu_ibus_rdata),
		.i_m0_wdata(32'h0),
		.i_m0_wmask(4'h0),

		// CPU data bus
		.i_m1_rw(cpu_dbus_rw),
		.i_m1_request(cpu_dbus_request),
		.o_m1_ready(cpu_dbus_ready),
		.i_m1_address(cpu_dbus_address),
		.o_m1_rdata(cpu_dbus_rdata),
		.i_m1_wdata(cpu_dbus_wdata),
		.i_m1_wmask(cpu_dbus_wmask),

		// DMA
		.i_m2_rw(dma_bus_rw),
		.i_m2_request(dma_bus_request),
		.o_m2_ready(dma_bus_ready),
		.i_m2_address(dma_bus_address),
		.o_m2_rdata(dma_bus_rdata),
		.i_m2_wdata(dma_bus_wdata),
		.i_m2_wmask(4'b1111),

		//

		// 32'h0xxx_xxxx : SPI Flash
		.o_s0_rw(),
		.o_s0_request(spif_request),
		.i_s0_ready(spif_ready),
		.o_s0_address(spif_address),
		.i_s0_rdata(spif_rdata),
		.o_s0_wdata(),
		.o_s0_wmask(),

		// 32'h1xxx_xxxx : SDRAM
		.o_s1_rw(sdram_rw),
		.o_s1_request(sdram_request),
		.i_s1_ready(sdram_ready),
		.o_s1_address(sdram_address),
		.i_s1_rdata(sdram_rdata),
		.o_s1_wdata(sdram_wdata),
		.o_s1_wmask(sdram_wmask),

		// 32'h2xxx_xxxx : UART
		.o_s2_rw(uart_rw),
		.o_s2_request(uart_request),
		.i_s2_ready(uart_ready),
		.o_s2_address(uart_address),
		.i_s2_rdata(uart_rdata),
		.o_s2_wdata(uart_wdata),
		.o_s2_wmask(),

		// 32'h3xxx_xxxx : I2C
		.o_s3_rw(i2c_rw),
		.o_s3_request(i2c_request),
		.i_s3_ready(i2c_ready),
		.o_s3_address(i2c_address),
		.i_s3_rdata(i2c_rdata),
		.o_s3_wdata(i2c_wdata),
		.o_s3_wmask(),

		// 32'h4xxx_xxxx : SD
		.o_s4_rw(sd_rw),
		.o_s4_request(sd_request),
		.i_s4_ready(sd_ready),
		.o_s4_address(sd_address),
		.i_s4_rdata(sd_rdata),
		.o_s4_wdata(sd_wdata),
		.o_s4_wmask(),

		// 32'h5xxx_xxxx : Timer
		.o_s5_rw(timer_rw),
		.o_s5_request(timer_request),
		.i_s5_ready(timer_ready),
		.o_s5_address(timer_address),
		.i_s5_rdata(timer_rdata),
		.o_s5_wdata(timer_wdata),
		.o_s5_wmask(),

		// 32'h6xxx_xxxx : Audio
		.o_s6_rw(audio_rw),
		.o_s6_request(audio_request),
		.i_s6_ready(audio_ready),
		.o_s6_address(audio_address),
		.i_s6_rdata(audio_rdata),
		.o_s6_wdata(audio_wdata),
		.o_s6_wmask(),

		// 32'h7xxx_xxxx : PLIC
		.o_s7_rw(plic_rw),
		.o_s7_request(plic_request),
		.i_s7_ready(plic_ready),
		.o_s7_address(plic_address),
		.i_s7_rdata(plic_rdata),
		.o_s7_wdata(plic_wdata),
		.o_s7_wmask(),

		// 32'h8xxx_xxxx : Video
		.o_s8_rw(video_rw),
		.o_s8_request(video_request),
		.i_s8_ready(video_ready),
		.o_s8_address(video_address),
		.i_s8_rdata(video_rdata),
		.o_s8_wdata(video_wdata),
		.o_s8_wmask(video_wmask),

		// 32'h9xxx_xxxx : DMA
		.o_s9_rw(dma_rw),
		.o_s9_request(dma_request),
		.i_s9_ready(dma_ready),
		.o_s9_address(dma_address),
		.i_s9_rdata(dma_rdata),
		.o_s9_wdata(dma_wdata),
		.o_s9_wmask(),

		// 32'haxxx_xxxx : Sprites
		.o_s10_rw(),
		.o_s10_request(video_sprite_request),
		.i_s10_ready(video_sprite_ready),
		.o_s10_address(video_sprite_address),
		.i_s10_rdata(32'h0),
		.o_s10_wdata(video_sprite_wdata),
		.o_s10_wmask(),

		// 32'hbxxx_xxxx : SPI
		.o_s11_rw(spi_rw),
		.o_s11_request(spi_request),
		.i_s11_ready(spi_ready),
		.o_s11_address(spi_address),
		.i_s11_rdata(spi_rdata),
		.o_s11_wdata(spi_wdata),
		.o_s11_wmask()
	);


endmodule
