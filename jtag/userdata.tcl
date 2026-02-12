# openocd -f userdata.tcl

adapter driver usb_blaster
usb_blaster lowlevel_driver ftdi

jtag newtap myfpga tap -irlen 4

init
scan_chain



irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x1
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "PC = $val"

set NAMES "ZERO RA SP GP TP T0 T1 T2 S0 S1 A0 A1 A2 A3 A4 A5 A6 A7 S2 S3 S4 S5 S6 S7 S8 S9 S10 S11 T3 T4 T5 T6"
for { set i 0 } { $i < [ llength $NAMES ] } { incr i } {

    irscan myfpga.tap 0x9
    drscan myfpga.tap 8 [expr {$i + 2}]
    irscan myfpga.tap 0x8
    set val [drscan myfpga.tap 32 0]

    set N [lindex $NAMES $i]
    puts "$N = $val"
}


irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x22
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "BUS REQUESTS = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x23
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "IBUS ADDR = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x24
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "DBUS ADDR = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x25
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "AUDIO DMA ADDR = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x26
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "DMA ADDR = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x27
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "PLIC IRQ = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x28
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "EPC = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x29
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "MSTATUS = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x2a
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "MIE = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x2b
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "MIP = $val"



irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x30
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "TRACE0 = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x31
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "TRACE1 = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x32
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "TRACE2 = $val"

irscan myfpga.tap 0x9
drscan myfpga.tap 8 0x33
irscan myfpga.tap 0x8
set val [drscan myfpga.tap 32 0]
puts "TRACE3 = $val"


shutdown
