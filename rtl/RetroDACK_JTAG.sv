/*
 RetroDÄCK
 Copyright (c) 2026 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

`timescale 1ns/1ns
`default_nettype none

`define FREQUENCY 100_000_000

(* top *)
module RetroDACK_JTAG(
	`include "RetroDACK_IO.sv"
);
	wire clock;
	wire clock_sdram;
	wire clock_video;
	wire clock_locked;

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
	// Reset
	wire reset;

	Reset rst(
		.i_clock(clock),
		.i_reset(!clock_locked),
		.o_reset(reset)
	);

	//====================================================

	bit tck_r = 1'b0;
	always_ff @(posedge clock) begin
		tck_r <= JTAG_TCK;
	end

    wire[7:0] jtag_userOp;
    wire[31:0] jtag_userData;
	wire jtag_userOp_ready;

	jtaglet #(
		.ID_PARTVER(4'h1),
		.ID_PARTNUM(16'hBEEF),
		.ID_MANF(11'h035)
	) jl (
		.tck(tck_r),
		.tms(JTAG_TMS),
		.tdi(JTAG_TDI),
		.tdo(JTAG_TDO),
		.trst(~reset),

		.userData_in(32'hcafe_babe),
		.userData_out(jtag_userData),
		.userOp(jtag_userOp),
		.userOp_ready(jtag_userOp_ready)
	);

	// JTAG_Simple jt(
	// 	.i_reset(reset),
	// 	.i_clock(clock),
	// 	.i_usercode(32'hcafe_babe),
	// 	.o_state(),
	// 	.TDI(JTAG_TDI),
	// 	.TDO(JTAG_TDO),
	// 	.TCK(JTAG_TCK),
	// 	.TMS(JTAG_TMS)
	// );


	assign JTAG_VREF = 1'b1;

	bit [31:0] tck_count = 0;

	always_ff @(posedge JTAG_TCK) begin
		tck_count <= tck_count + 1;
	end

	assign LED_R = tck_count[0];
	assign LED_G = 1'b0;
	assign LED_B = 1'b0;

endmodule
