/*
 RetroDÄCK
 Copyright (c) 2025 Anders Pistol.

 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at https://mozilla.org/MPL/2.0/.
*/

`timescale 1ns/1ns

(* top *)
module RetroDACK(
	`include "RetroDACK_IO.sv"
);
	wire clock;
	wire clock_sdram;
	wire reset = 1'b0;


	assign LED_R = pin_value[0];
	assign LED_G = pin_value[1];
	assign LED_B = pin_value[2];

	bit [2:0] pin_value = 1'b0;
	wire pin_select;
	bit pin_ready = 1'b0;

	always @(posedge clock) begin
		if (bus_request && pin_select) begin
			pin_value <= bus_wdata[2:0];
			pin_ready <= 1'b1;
		end
		else begin
			pin_ready <= 1'b0;
		end
	end


	/*
	// 125 MHz
	`define FREQUENCY 125_000_000
	PLL_ECP5 #(
		.CLKI_DIV(1),
		.CLKFB_DIV(5),
		.CLKOP_DIV(5),
		.CLKOP_CPHASE(0)
	) pll(
		.i_clk(CLOCK),
		.o_clk1(clock),
		.o_clk2(),
		.o_clk_locked()
	);
	*/

	// 100 MHz
	// 100 MHz (7000 ps phase shift)
	`define FREQUENCY 100_000_000
	PLL_ECP5 #(
		.CLKI_DIV(1),
		.CLKFB_DIV(4),
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


	//====================================================
	// ROM
	wire rom_select;
	wire [31:0] rom_address;
	wire [31:0] rom_rdata;
	wire rom_ready;

	RetroDACK_BROM rom(
		.i_clock(clock),
		.i_request(bus_request && rom_select),
		.i_address(rom_address),
		.o_rdata(rom_rdata),
		.o_ready(rom_ready)
	);


	//====================================================
	// RAM
	wire ram_select;
	wire [31:0] ram_address;
	wire [31:0] ram_wdata;
	wire [31:0] ram_rdata;
	wire ram_ready;

	BRAM #(
		.SIZE(32'h1000)
	) ram(
		.i_clock(clock),
		.i_request(bus_request && ram_select),
		.i_rw(bus_rw),
		.i_address(ram_address),
		.i_wdata(ram_wdata),
		.o_rdata(ram_rdata),
		.o_ready(ram_ready),
		.o_valid()
	);


	//=====================================
    // SDRAM ($20000000)
	wire sdram_select;
	wire [31:0] sdram_address;
	wire [31:0] sdram_wdata;
	wire [31:0] sdram_rdata;
	wire sdram_ready;

	wire [1:0] it_sdram_dqm;
	wire [1:0] it_sdram_ba;
	wire [12:0] it_sdram_addr;
	logic [15:0] it_sdram_data_r;
	wire [15:0] it_sdram_data_w;
	wire it_sdram_data_rw;

	assign SDRAM_DQM0 = it_sdram_dqm[0];
	assign SDRAM_DQM1 = it_sdram_dqm[1];
	assign SDRAM_BA0 = it_sdram_ba[0];
	assign SDRAM_BA1 = it_sdram_ba[1];
	assign SDRAM_A0 = it_sdram_addr[0];
	assign SDRAM_A1 = it_sdram_addr[1];
	assign SDRAM_A2 = it_sdram_addr[2];
	assign SDRAM_A3 = it_sdram_addr[3];
	assign SDRAM_A4 = it_sdram_addr[4];
	assign SDRAM_A5 = it_sdram_addr[5];
	assign SDRAM_A6 = it_sdram_addr[6];
	assign SDRAM_A7 = it_sdram_addr[7];
	assign SDRAM_A8 = it_sdram_addr[8];
	assign SDRAM_A9 = it_sdram_addr[9];
	assign SDRAM_A10 = it_sdram_addr[10];
	assign SDRAM_A11 = it_sdram_addr[11];
	assign SDRAM_A12 = it_sdram_addr[12];

	assign 
		{ SDRAM_DQ0, SDRAM_DQ1, SDRAM_DQ2, SDRAM_DQ3, SDRAM_DQ4, SDRAM_DQ5, SDRAM_DQ6, SDRAM_DQ7, SDRAM_DQ8, SDRAM_DQ9, SDRAM_DQ10, SDRAM_DQ11, SDRAM_DQ12, SDRAM_DQ13, SDRAM_DQ14, SDRAM_DQ15 } =
		it_sdram_data_rw ? it_sdram_data_w : 16'hz;

	assign it_sdram_data_r = { SDRAM_DQ0, SDRAM_DQ1, SDRAM_DQ2, SDRAM_DQ3, SDRAM_DQ4, SDRAM_DQ5, SDRAM_DQ6, SDRAM_DQ7, SDRAM_DQ8, SDRAM_DQ9, SDRAM_DQ10, SDRAM_DQ11, SDRAM_DQ12, SDRAM_DQ13, SDRAM_DQ14, SDRAM_DQ15 };

    SDRAM_controller #(
        .FREQUENCY(`FREQUENCY),
		.SDRAM_DATA_WIDTH(16)
    ) sdram(
	    .i_reset(reset),
	    .i_clock(clock),
		.i_clock_sdram(clock_sdram),

	    .i_request(sdram_select && bus_request),
	    .i_rw(bus_rw),
	    .i_address(sdram_address),
	    .i_wdata(sdram_wdata),
	    .o_rdata(sdram_rdata),
	    .o_ready(sdram_ready),

	    .sdram_clk(SDRAM_CLK),
	    .sdram_clk_en(SDRAM_CKE),
	    .sdram_cas_n(SDRAM_CAS_n),
	    .sdram_cs_n(SDRAM_CE_n),
	    .sdram_ras_n(SDRAM_RAS_n),
	    .sdram_we_n(SDRAM_WE_n),
	    .sdram_dqm(it_sdram_dqm),
	    .sdram_bs(it_sdram_ba),
	    .sdram_addr(it_sdram_addr),
		.sdram_rdata(it_sdram_data_r),
		.sdram_wdata(it_sdram_data_w),
		.sdram_data_rw(it_sdram_data_rw)
    );


	//====================================================
	// CPU chip select
	assign rom_select = bus_address[31:28] == 4'h0;
	assign rom_address = { 4'h0, bus_address[27:0] };

	assign ram_select = bus_address[31:28] == 4'h1;
	assign ram_address = { 4'h0, bus_address[27:0] };
	assign ram_wdata = bus_wdata;

	assign sdram_select = bus_address[31:28] == 4'h2;
	assign sdram_address = { 4'h0, bus_address[27:0] };
	assign sdram_wdata = bus_wdata;

	assign bridge_select = bus_address[31:28] == 4'h5;
	assign bridge_address = { 4'h0, bus_address[27:0] };
	assign bridge_wdata = bus_wdata;

	assign pin_select = bus_address[31:28] == 4'h4;

	assign bus_rdata =
		rom_select		? rom_rdata		:
		ram_select		? ram_rdata		:
		sdram_select	? sdram_rdata	:
		bridge_select	? bridge_rdata	:
		32'h00000000;

	assign bus_ready =
		rom_select		? rom_ready		:
		ram_select		? ram_ready		:
		sdram_select	? sdram_ready	:
		bridge_select	? bridge_ready	:
		pin_select		? pin_ready		:
		1'b0;


	//====================================================
	// CPU BusMux
	wire bus_rw;
	wire bus_request;
	wire bus_ready;
	wire [31:0] bus_address;
	wire [31:0] bus_rdata;
	wire [31:0] bus_wdata;

	CPU_BusMux #(
		.REGISTERED(1)
	) bus(
		.i_reset(reset),
		.i_clock(clock),

		.o_bus_rw(bus_rw),
		.o_bus_request(bus_request),
		.i_bus_ready(bus_ready),
		.o_bus_address(bus_address),
		.i_bus_rdata(bus_rdata),
		.o_bus_wdata(bus_wdata),

		.i_pa_request(cpu_ibus_request),
		.o_pa_ready(cpu_ibus_ready),
		.i_pa_address(cpu_ibus_address),
		.o_pa_rdata(cpu_ibus_rdata),

		.i_pb_rw(cpu_dbus_rw),
		.i_pb_request(cpu_dbus_request),
		.o_pb_ready(cpu_dbus_ready),
		.i_pb_address(cpu_dbus_address),
		.o_pb_rdata(cpu_dbus_rdata),
		.i_pb_wdata(cpu_dbus_wdata)
	);


	//====================================================
	// CPU
	wire cpu_ibus_request;
	wire cpu_ibus_ready;
	wire cpu_timer_interrupt;
	wire cpu_external_interrupt;
	wire [31:0] cpu_ibus_address;
	wire [31:0] cpu_ibus_rdata;
	wire cpu_dbus_rw;
	wire cpu_dbus_request;
	wire cpu_dbus_ready;
	wire [31:0] cpu_dbus_address;
	wire [31:0] cpu_dbus_rdata;
	wire [31:0] cpu_dbus_wdata;
	wire cpu_fault;

	CPU #(
		.FREQUENCY(`FREQUENCY),
		.DCACHE_SIZE(0),
		.DCACHE_REGISTERED(1),
		.ICACHE_SIZE(1),
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

		// Debug
		.o_icache_hit(),
		.o_icache_miss(),
		.o_dcache_hit(),
		.o_dcache_miss(),
		.o_execute_busy(),
		.o_memory_busy(),
		.o_fault(cpu_fault)
	);


	//===========================================================
	//===========================================================
	// Everything below is on "the bridge", ie far peripherials.
	//===========================================================
	//===========================================================


	//====================================================
	// UART
	wire uart_select;
	wire [1:0] uart_address;
	wire [31:0] uart_wdata;
	wire [31:0] uart_rdata;
	wire uart_ready;

	UART #(
		.FREQUENCY(`FREQUENCY),
		.BAUDRATE(115200),
		.RX_FIFO_DEPTH(16)
	) uart(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(bridge_far_request && uart_select),
		.i_rw(bridge_far_rw),
		.i_address(uart_address),
		.i_wdata(uart_wdata),
		.o_rdata(uart_rdata),
		.o_ready(uart_ready),
		.o_interrupt(),
		// ---
		.UART_RX(UART_RX),
		.UART_TX(UART_TX)
	);


	//====================================================
	// I2C
	wire i2c_select;
	wire [1:0] i2c_address;
	wire [31:0] i2c_wdata;
	wire [31:0] i2c_rdata;
	wire i2c_ready;

	wire I2C_SDA_direction;
	wire I2C_SDA_w;

	assign I2C_SDA = I2C_SDA_direction ? I2C_SDA_w : 1'bz;

	I2C i2c(
		.i_clock(clock),
		.i_request(bridge_far_request && i2c_select),
		.i_rw(bridge_far_rw),
		.i_wdata(i2c_wdata),
		.o_rdata(i2c_rdata),
		.o_ready(i2c_ready),
		// ---
		.I2C_SCL(I2C_SCL),
		.I2C_SDA_direction(I2C_SDA_direction),
		.I2C_SDA_r(I2C_SDA),
		.I2C_SDA_w(I2C_SDA_w),
	);


	//====================================================
	// TIMER
	wire timer_select;
	wire [3:0] timer_address;
	wire [31:0] timer_wdata;
	wire [31:0] timer_rdata;
	wire timer_ready;
	Timer #(
		.FREQUENCY(`FREQUENCY)
	) timer(
		.i_reset(reset),
		.i_clock(clock),
		.i_request(bridge_far_request && timer_select),
		.i_rw(bridge_far_rw),
		.i_address(timer_address),
		.i_wdata(timer_wdata),
		.o_rdata(timer_rdata),
		.o_ready(timer_ready),
		.o_interrupt(cpu_timer_interrupt)
	);


	//====================================================
	// PLIC
	wire plic_interrupt;
	wire plic_select;
	wire [23:0] plic_address;
	wire [31:0] plic_wdata;
	wire [31:0] plic_rdata;
	wire plic_ready;

	CPU_PLIC plic(
		.i_reset(reset),
		.i_clock(clock),

		.i_interrupt_0(~I2C_INTERRUPT),
		.i_interrupt_1(0),
		.i_interrupt_2(0),
		.i_interrupt_3(0),

		.i_interrupt_enable(1'b1),
		.o_interrupt(cpu_external_interrupt),

		.i_request(bridge_far_request && plic_select),
		.i_rw(bridge_far_rw),
		.i_address(plic_address),
		.i_wdata(plic_wdata),
		.o_rdata(plic_rdata),
		.o_ready(plic_ready)
	);


	//====================================================
	// Bridge
	wire bridge_select;
	wire [27:0] bridge_address;
	wire [31:0] bridge_wdata;
	wire [31:0] bridge_rdata;
	wire bridge_ready;

	wire bridge_far_request;
	wire bridge_far_rw;
	wire [27:0] bridge_far_address;
	wire [31:0] bridge_far_wdata;
	wire [31:0] bridge_far_rdata;
	wire bridge_far_ready;

	Bridge #(
		.REGISTERED(1)
	) bridge(
		.i_clock		(clock),
		.i_reset		(reset),

		// Near
		.i_request		(bridge_select && bus_request),
		.i_rw			(bus_rw),
		.i_address		(bridge_address),
		.i_wdata		(bridge_wdata),
		.o_rdata		(bridge_rdata),
		.o_ready		(bridge_ready),

		// Far
		.o_far_request	(bridge_far_request),
		.o_far_rw		(bridge_far_rw),
		.o_far_address	(bridge_far_address),
		.o_far_wdata	(bridge_far_wdata),
		.i_far_rdata	(bridge_far_rdata),
		.i_far_ready	(bridge_far_ready)
	);

	assign uart_select = bridge_far_address[27:24] == 4'h1;
	assign uart_address = bridge_far_address[3:2];
	assign uart_wdata = bridge_far_wdata;

	assign i2c_select = bridge_far_address[27:24] == 4'h3;
	assign i2c_address = bridge_far_address[3:2];
	assign i2c_wdata = bridge_far_wdata;

	assign timer_select = bridge_far_address[27:24] == 4'h5;
	assign timer_address = bridge_far_address[5:2];
	assign timer_wdata = bridge_far_wdata;

	assign plic_select = bridge_far_address[27:24] == 4'h8;
	assign plic_address = bridge_far_address[23:0];
	assign plic_wdata = bridge_far_wdata;

	assign bridge_far_rdata =
		uart_select	? uart_rdata	:
		i2c_select ? i2c_rdata		:
		timer_select ? timer_rdata	:
		plic_select ? plic_rdata	:
		32'h00000000;
	
	assign bridge_far_ready =
		uart_select	? uart_ready	:
		i2c_select ? i2c_ready		:
		timer_select ? timer_ready	:
		plic_select ? plic_ready	:
		1'b0;


endmodule
