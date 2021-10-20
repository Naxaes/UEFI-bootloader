ENTRY(start)

SECTIONS
{
	. = 0x10e000000;
	.text : ALIGN(512B)
	{
		*(.text)
	}
	.rodata : ALIGN(512B)
	{
		*(.rodata)
	}
	.data : ALIGN(512B)
	{
		*(.data)
	}
	.bss : ALIGN(512B)
	{
		*(COMMON)
		*(.bss)
	}
}
