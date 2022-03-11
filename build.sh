echo "Compiling..."

i686-elf-gcc -std=gnu99 -ffreestanding -g -c boot.s -o boot.o
i686-elf-gcc -std=gnu99 -ffreestanding -g -c kernel.c -o kernel.o

i686-elf-gcc -ffreestanding -nostdlib -g -T linker.ld boot.o kernel.o -o kernel.elf -lgcc

if [ $? -ne 0 ]; then
  echo "Compilation failed!"
  exit 1
else
  echo "Compilation succeeded!"
fi

echo "Creating ISO..."

cp kernel.elf isoroot/boot/kernel.elf
grub-mkrescue isoroot -o snakeos.iso -quiet

if [ $? -ne 0 ]; then
  echo "Create ISO failed!"
  exit 1
else
  echo "Create ISO succeeded!"
fi