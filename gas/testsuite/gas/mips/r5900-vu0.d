#objdump: -dr --prefix-addresses --show-raw-insn -M gpr-names=numeric -mmips:5900
#name: MIPS R5900 VU0
#as: -march=r5900

.*: +file format .*mips.*

Disassembly of section \.text:
[0-9a-f]+ <[^>]*> d8000000 	lqc2	\$0,0\(\$0\)
[0-9a-f]+ <[^>]*> d8217fff 	lqc2	\$1,32767\(\$1\)
[0-9a-f]+ <[^>]*> d9088000 	lqc2	\$8,-32768\(\$8\)
[0-9a-f]+ <[^>]*> dbffffff 	lqc2	\$31,-1\(\$31\)
[0-9a-f]+ <[^>]*> 3c010001 	lui	\$1,0x1
[0-9a-f]+ <[^>]*> 00220821 	addu	\$1,\$1,\$2
[0-9a-f]+ <[^>]*> d8208000 	lqc2	\$0,-32768\(\$1\)
[0-9a-f]+ <[^>]*> 3c01ffff 	lui	\$1,0xffff
[0-9a-f]+ <[^>]*> 003f0821 	addu	\$1,\$1,\$31
[0-9a-f]+ <[^>]*> d8287fff 	lqc2	\$8,32767\(\$1\)
[0-9a-f]+ <[^>]*> 3c01f123 	lui	\$1,0xf123
[0-9a-f]+ <[^>]*> 00240821 	addu	\$1,\$1,\$4
[0-9a-f]+ <[^>]*> d83f4567 	lqc2	\$31,17767\(\$1\)
[0-9a-f]+ <[^>]*> f8000000 	sqc2	\$0,0\(\$0\)
[0-9a-f]+ <[^>]*> f8217fff 	sqc2	\$1,32767\(\$1\)
[0-9a-f]+ <[^>]*> f9088000 	sqc2	\$8,-32768\(\$8\)
[0-9a-f]+ <[^>]*> fbffffff 	sqc2	\$31,-1\(\$31\)
[0-9a-f]+ <[^>]*> 3c010001 	lui	\$1,0x1
[0-9a-f]+ <[^>]*> 00220821 	addu	\$1,\$1,\$2
[0-9a-f]+ <[^>]*> f8208000 	sqc2	\$0,-32768\(\$1\)
[0-9a-f]+ <[^>]*> 3c01ffff 	lui	\$1,0xffff
[0-9a-f]+ <[^>]*> 003f0821 	addu	\$1,\$1,\$31
[0-9a-f]+ <[^>]*> f8287fff 	sqc2	\$8,32767\(\$1\)
[0-9a-f]+ <[^>]*> 3c01f123 	lui	\$1,0xf123
[0-9a-f]+ <[^>]*> 00240821 	addu	\$1,\$1,\$4
[0-9a-f]+ <[^>]*> f83f4567 	sqc2	\$31,17767\(\$1\)
[0-9a-f]+ <[^>]*> 48400000 	cfc2	\$0,\$0
[0-9a-f]+ <[^>]*> 4840f800 	cfc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48400001 	cfc2.i	\$0,\$0
[0-9a-f]+ <[^>]*> 4840f801 	cfc2.i	\$0,\$31
[0-9a-f]+ <[^>]*> 48400000 	cfc2	\$0,\$0
[0-9a-f]+ <[^>]*> 4840f800 	cfc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48c00000 	ctc2	\$0,\$0
[0-9a-f]+ <[^>]*> 48c0f800 	ctc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48c00001 	ctc2.i	\$0,\$0
[0-9a-f]+ <[^>]*> 48c0f801 	ctc2.i	\$0,\$31
[0-9a-f]+ <[^>]*> 48c00000 	ctc2	\$0,\$0
[0-9a-f]+ <[^>]*> 48c0f800 	ctc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48200000 	qmfc2	\$0,\$0
[0-9a-f]+ <[^>]*> 4820f800 	qmfc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48200001 	qmfc2.i	\$0,\$0
[0-9a-f]+ <[^>]*> 4820f801 	qmfc2.i	\$0,\$31
[0-9a-f]+ <[^>]*> 48200000 	qmfc2	\$0,\$0
[0-9a-f]+ <[^>]*> 4820f800 	qmfc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48a00000 	qmtc2	\$0,\$0
[0-9a-f]+ <[^>]*> 48a0f800 	qmtc2	\$0,\$31
[0-9a-f]+ <[^>]*> 48a00001 	qmtc2.i	\$0,\$0
[0-9a-f]+ <[^>]*> 48a0f801 	qmtc2.i	\$0,\$31
[0-9a-f]+ <[^>]*> 48a00000 	qmtc2	\$0,\$0
[0-9a-f]+ <[^>]*> 48a0f800 	qmtc2	\$0,\$31
[0-9a-f]+ <[^>]*> 4900ffff 	bc2f	[0-9a-f]+ <branch_label>
[0-9a-f]+ <[^>]*> 00000000 	nop
[0-9a-f]+ <[^>]*> 4902fffd 	bc2fl	[0-9a-f]+ <branch_label>
[0-9a-f]+ <[^>]*> 00000000 	nop
[0-9a-f]+ <[^>]*> 4901fffb 	bc2t	[0-9a-f]+ <branch_label>
[0-9a-f]+ <[^>]*> 00000000 	nop
[0-9a-f]+ <[^>]*> 4903fff9 	bc2tl	[0-9a-f]+ <branch_label>
[0-9a-f]+ <[^>]*> 00000000 	nop
	\.\.\.
