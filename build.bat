@echo off
echo 正在编译霍夫曼压缩工具...
gcc -Wall -O2 -D_WIN32_WINNT=0x0600 main.c huffman.c -o myzip.exe
if %errorlevel% == 0 (
	echo 编译成功！生成 myzip.exe
	) else (
		echo 编译失败！
		)
