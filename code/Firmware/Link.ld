OUTPUT_ARCH( "riscv" )
ENTRY( _start )

MEMORY
{
    rom	(rx)	: ORIGIN = 0x00000000, LENGTH = 0x00030000
	ram	(rwx)	: ORIGIN = 0x11E00000, LENGTH = 0x00100000
}

EXTERN(crt_init);

SECTIONS
{
	/*--------------------------------------------------------------------*/
	/* Code and read-only segment                                         */
	/*--------------------------------------------------------------------*/

    . = ORIGIN(rom);

	_ftext = .;
	PROVIDE( eprol = . );

	/* text: Program code section */
	.text : 
	{
		*(.text)
		*(.text.*)
		*(.gnu.linkonce.t.*)

		KEEP(*(.keep_rt_crt))
	} >rom

	/* init: Code to execute before main (called by crt0.S) */
	.init : 
	{
		KEEP( *(.init) )
	} >rom

	/* fini: Code to execute after main (called by crt0.S) */
	.fini : 
	{
		KEEP( *(.fini) )
	} >rom

	/* rodata: Read-only data */
	.rodata : 
	{
		*(.rdata)
		*(.rodata)
		*(.rodata.*)
		*(.gnu.linkonce.r.*)
	} >rom

	/* End of code and read-only segment */
	PROVIDE( etext = . );
	_etext = .;

    /*--------------------------------------------------------------------*/
	/* Global constructor/destructor segment                             */
	/*--------------------------------------------------------------------*/

	.preinit_array     :
	{
		PROVIDE_HIDDEN (__preinit_array_start = .);
		KEEP (*(.preinit_array))
		PROVIDE_HIDDEN (__preinit_array_end = .);
	} >rom

	.init_array     :
	{
		PROVIDE_HIDDEN (__init_array_start = .);
		KEEP (*(SORT(.init_array.*)))
		KEEP (*(.init_array ))
		PROVIDE_HIDDEN (__init_array_end = .);
	} >rom

	.fini_array     :
	{
		PROVIDE_HIDDEN (__fini_array_start = .);
		KEEP (*(SORT(.fini_array.*)))
		KEEP (*(.fini_array ))
		PROVIDE_HIDDEN (__fini_array_end = .);
	} >rom

	/*--------------------------------------------------------------------*/
	/* Other misc gcc segments (this was in idt32.ld)                     */
	/*--------------------------------------------------------------------*/
	/* I am not quite sure about these sections but it seems they are for
		 C++ exception handling. I think .jcr is for "Java Class
		 Registration" but it seems to end up in C++ binaries as well. */

	.eh_frame_hdr     : { *(.eh_frame_hdr)     } >rom
	.eh_frame         : { KEEP( *(.eh_frame) ) } >rom
	.gcc_except_table : { *(.gcc_except_table) } >rom
	.jcr              : { KEEP (*(.jcr))       } >rom

	/*--------------------------------------------------------------------*/
	/* Initialized data segment                                           */
	/*--------------------------------------------------------------------*/

    . = ORIGIN(ram);
	. = ALIGN(16);

	.data : 
	{
        INIT_DATA_VALUES = LOADADDR(.data);
        INIT_DATA_START = .;

		*(.data)
		*(.data.*)
		*(.gnu.linkonce.d.*)

		*(.sdata)
		*(.sdata.*)
		*(.srodata.*)
		*(.gnu.linkonce.s.*)

        INIT_DATA_END = .;
	} >ram AT >rom

	. = ALIGN(16);

	_gp = . + 0x800;
	__global_pointer$ = .;

	PROVIDE( edata = . );
	_edata = .;

	/*--------------------------------------------------------------------*/
	/* Uninitialized data segment                                         */
	/*--------------------------------------------------------------------*/

	/* Start of uninitialized data segment */
	. = ALIGN(16);
    
	__bss_start = .;
	.bss (NOLOAD) : 
	{
	    BSS_START = .;

		*(.bss)
		*(.bss.*)
		*(.gnu.linkonce.b.*)
		*(COMMON)

		*(.sbss)
		*(.sbss.*)
		*(.gnu.linkonce.sb.*)

	    BSS_END = .;
	} >ram

	. = ALIGN(16);

	PROVIDE( end = . );
	_end = ALIGN(8);
}
