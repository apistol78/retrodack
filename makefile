
ARCH=rv32imf
ABI=ilp32

C=riscv32-unknown-elf-gcc
OC=riscv32-unknown-elf-objcopy
OD=riscv32-unknown-elf-objdump
H2V=../klara-rv/bin/Hex2Verilog

C_FLAGS=-march=$(ARCH) -mabi=$(ABI) -mbranch-cost=8 -mstrict-align -fno-rtti -fno-exceptions -fpermissive -Wcast-align -Wno-multichar -Wno-shift-overflow -ffast-math -fdata-sections -ffunction-sections -Wl,--gc-sections -O3
LINK_FLAGS=-march=$(ARCH) -mabi=$(ABI) -Wl,-gc-sections

build/firmware/firmware.vmem : build/firmware/firmware.hex
	$(H2V) -word=32 build/firmware/firmware.hex -vmem=firmware.vmem -vmem-range=firmware.vmem-range

build/firmware/firmware.hex : build/firmware/firmware
	$(OC) -O ihex build/firmware/firmware $@

build/firmware/firmware : build/firmware/startup.o code/verify.ld
	$(C) $(LINK_FLAGS) -nostdlib build/firmware/startup.o -o $@ -Tcode/verify.ld
	$(OD) -D build/firmware/firmware > build/firmware/firmware.dump

build/firmware/startup.o : code/startup.c
	@mkdir -p build/firmware
	$(C) -c $(C_FLAGS) -nostdlib code/startup.c -o $@

clean :
	@rm -Rf build/firmware