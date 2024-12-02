MEMORY {
    rom (rx) : ORIGIN = 0x08000000, LENGTH = 128K
    ram (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
INCLUDE /home/ilya/stm32_2024/libs/libopencm3/lib/cortex-m-generic.ld
