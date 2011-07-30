gcc -m32 -o os.o -c os.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o shell.o -c shell.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o refresher.o -c refresher.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o out.o -c out.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o keyboard.o -c keyboard.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o apps.o -c apps.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o vga.o -c vga.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o shell_api.o -c shell_api.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -m32 -o x86.o -c x86.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
as --32 loader.s -o loader.o
LDEMULATION="elf_i386" ld -T linker.ld -o kernel.bin loader.o os.o shell.o refresher.o out.o keyboard.o vga.o apps.o shell_api.o x86.o
cat  stage1 stage2 pad kernel.bin > kernel.img
