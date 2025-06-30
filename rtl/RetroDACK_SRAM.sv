/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

`timescale 1ns/1ns

(* top *)
module RetroDACK_SRAM(
	`include "RetroDACK_IO.sv"
);
	wire clock;

	assign LED_R = 1'b0;
	assign LED_G = 1'b1;
	assign LED_B = 1'b0;

	// 36.72 MHz
	PLL_ECP5 #(
		.CLKI_DIV(86/2),
		.CLKFB_DIV(126),
		.CLKOP_DIV(11),
		.CLKOP_CPHASE(0)
	) pll_video(
		.i_clk(CLOCK),
		.o_clk1(clock),
		.o_clk2(),
		.o_clk_locked()
	);

    bit [2:0] strobe = 3'b0;

    always_ff @(posedge clock) begin
        strobe <= strobe + 1;
    end

    assign SRAM_WE = strobe[2];
    assign SRAM_A[6] = strobe[1];

endmodule
