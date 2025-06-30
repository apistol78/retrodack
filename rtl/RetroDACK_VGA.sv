/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

`timescale 1ns/1ns

(* top *)
module RetroDACK_VGA(
	`include "RetroDACK_IO.sv"
);
	wire clock;
	wire clock_sdram;
	wire clock_video;

	assign LED_R = 1'b0;
	assign LED_G = 1'b0;
	assign LED_B = SD_EXTERNAL_CARD;

	// 100 MHz
	// 100 MHz (7000 ps phase shift)
	`define FREQUENCY 100_000_000
	PLL_ECP5 #(
		.CLKI_DIV(1),
		.CLKFB_DIV(4*2),
		.CLKOP_DIV(6),
		.CLKOP_CPHASE(0),
		.CLKOS_DIV(6),
		.CLKOS_CPHASE(5)
	) pll(
		.i_clk(CLOCK),
		.o_clk1(clock),
		.o_clk2(clock_sdram),
		.o_clk_locked()
	);

	// 36.72 MHz
	PLL_ECP5 #(
		.CLKI_DIV(86/2),
		.CLKFB_DIV(126),
		.CLKOP_DIV(11),
		.CLKOP_CPHASE(0)
	) pll_video(
		.i_clk(CLOCK),
		.o_clk1(clock_video),
		.o_clk2(),
		.o_clk_locked()
	);

	//====================================================
	// Reset
	wire reset;

	Reset rst(
		.i_clock(clock),
		.i_reset(uart_soft_reset),
		.o_reset(reset)
	);

	bit [15:0] cnt = 0;
	always_ff @(posedge clock) begin
		cnt <= cnt + 1;
	end


	bit [1:0] vs = 0;
	bit [5:0] voffset = 0;
	always_ff @(posedge clock) begin
		vs <= { vs[0], vga_vsync };
		if (vs == 2'b01) begin
			voffset <= voffset + 1;
		end
	end

	//====================================================
	// VIDEO SIGNAL GENERATOR
	wire vga_vsync;
	wire vga_data_enable;
	wire [10:0] vga_pos_x;
	wire [10:0] vga_pos_y;

	assign LCD_ENABLE = vga_data_enable;
	assign LCD_BACKLIGHT_CTRL = cnt[9];
	assign LCD_VSYNC = vga_vsync;
	
	assign { LCD_R5, LCD_R4, LCD_R3, LCD_R2, LCD_R1, LCD_R0 } = vga_pos_x[5:0] + voffset;
	assign { LCD_G5, LCD_G4, LCD_G3, LCD_G2, LCD_G1, LCD_G0 } = vga_pos_y[7:2] + voffset;
	assign { LCD_B5, LCD_B4, LCD_B3, LCD_B2, LCD_B1, LCD_B0 } = vga_pos_y[5:0];

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
		.o_vsync(vga_vsync),
		.o_data_enable(vga_data_enable),
		.o_pos_x(vga_pos_x),
		.o_pos_y(vga_pos_y)
	);

endmodule
