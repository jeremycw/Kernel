gcc -o os.o -c os.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o shell.o -c shell.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o refresher.o -c refresher.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o out.o -c out.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o keyboard.o -c keyboard.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o apps.o -c apps.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o vga.o -c vga.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o shell_api.o -c shell_api.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
gcc -o x86.o -c x86.c -Wall -Wextra -nostdlib -nostartfiles -nodefaultlibs -fno-builtin -g
ld -T linker.ld -o kernel.bin loader.o os.o shell.o refresher.o out.o keyboard.o vga.o apps.o shell_api.o x86.o
cat  stage1 stage2 pad kernel.bin > kernel.img
