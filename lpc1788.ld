ENTRY(Reset_Handler)

MEMORY
{
	sram	:	ORIGIN = 0x10000000, LENGTH = 64k
	flash	:	ORIGIN = 0x00000000, LENGTH = 512k
}

_end_stack = 0x10007FF0;

SECTIONS {
	. = ORIGIN(flash);

	vectors :
	{
		*(.vectors)
	} >flash

	.text :
	{
		*(.text)
		*(.rodata)
		*(.rodata*)
		*(.ARM.extab* .gnu.linkonce.armextab.*)
		*(.eh_frame_hdr)
		*(.eh_frame)
	} >flash

	__exidx_start = .;
	.ARM.exidx :
	{
		*(.ARM.exidx* .gnu.linkonce.armexidx.*)
	} >flash
	__exidx_end = .;

        _end_text = .;

	.data :
	{
		_start_data = .;
		*(.data)
		_end_data = .;
	} >sram AT>flash

	.bss :
	{
		_start_bss = .;
		*(.bss)
		_end_bss = .;
	} >sram

	. = ALIGN(4);

	_start_stack = .;
}

_end = .;
PROVIDE(end = .);
