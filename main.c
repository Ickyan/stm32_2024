#include <stdint.h>
// #include <stm32f10x.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/spi.h>
#include <libopencmsis/core_cm3.h>
#include <MFRC522.h>

#define LCD_H 8
#define LCD_W 128

/*
RC522 PIN   STM32 PIN
---------   ---------
SDA         PA4       // NSS (CS, выбор устройства)
SCK         PA5       // SPI1_SCK (тактовый сигнал)
MOSI        PA7       // SPI1_MOSI (данные от мастера к RC522)
MISO        PA6       // SPI1_MISO (данные от RC522 к мастеру)
IRQ         -         // Не используется в базовом подключении
RST         PA3       // Сброс RC522
GND         GND       // Земля
3V3         3.3V      // Питание
*/

//задачи
/*
1. стянуть 2 файла
2. добавить в cmake
3. переписать 2 функции write/read через libopencm3
подкл lib gpio переписать 
4. протестировать коммуникацию (посмотреть индуский майн.си)
5. считать данные с помощью модуля
6. 
*/

void delay_us(uint32_t us) {
	__asm volatile (
		"push {r0}\r\n"
		"mov R0, %0\r\n"
		"_loop:\r\n" //approx. 8ticks/iteration
			"cmp R0, #0\r\n"     //1
			"beq _exit\r\n"      //1 or 1+P (when condition is True)
			"sub R0, R0, #1\r\n" //1
			"nop\r\n" //1 allignment
			"b _loop\r\n" //1+P (pipeline refill) ~4 cycle
		"_exit:\r\n"
		"pop {r0}\r\n"
		:: "r"(9 * us) //for 72Mhz
	);
}
/*
void spi_setup(void) {
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);
	

    // Настройка пинов SCK, MISO, MOSI
	// настройка порта PA6 в MISO на приём от RCC522
	gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

	// настройка порта PA5 PA7 PA4 как SLK MOSI NSS// скорость уточнить
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5 | GPIO7);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);

    // Настройка SPI
	spi_set_baudrate_prescaler(SPI1, SPI_CR1_BAUDRATE_FPCLK_DIV_8);
	spi_set_master_mode(SPI1);
	spi_set_standard_mode(SPI1, 0);
	//spi_set_standard_mode(SPI1, ~SPI_CR1_CPOL | ~SPI_CR1_CPHA);
	spi_enable_software_slave_management(SPI1);
	
	spi_enable(SPI1);
}
*/

void spi_setup(void) {
    // Включаем тактирование SPI1 и GPIOA
    rcc_periph_clock_enable(RCC_SPI1);
    rcc_periph_clock_enable(RCC_GPIOA);

    // Настройка пинов GPIO для SPI
    // PA6 (MISO) - входной пин
    gpio_set_mode(GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_FLOAT, GPIO6);

    // PA5 (SCK) и PA7 (MOSI) - альтернативный выход push-pull
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO5);
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);

    // PA4 (NSS) - программное управление, push-pull
    gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO4);

    // Отключаем SPI перед настройкой
    spi_disable(SPI1);

    // Настраиваем SPI
    spi_set_baudrate_prescaler(SPI1, SPI_CR1_BR_FPCLK_DIV_64);          // Предделитель 64
    spi_set_master_mode(SPI1);                                              // Режим мастер
	//spi_set_clock_polarity_1(SPI1);
    spi_set_clock_polarity_0(SPI1);                                         // Полярность 0 (SCK низкий в режиме ожидания)
    spi_set_clock_phase_1(SPI1);                                            // Фаза 1 (данные захватываются на переднем фронте)
    //spi_set_clock_phase_0(SPI1);
	spi_set_dff_8bit(SPI1);                                                 // Данные 8 бит
    spi_send_msb_first(SPI1);                                               // MSB первым

    // Программное управление NSS
    spi_enable_software_slave_management(SPI1);                             // Включаем программное управление NSS
    spi_set_nss_high(SPI1);                                                 // NSS высокий (неактивен)

    // Включаем SPI
    spi_enable(SPI1);
}




int main(void){
	rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO3);

	spi_setup();
	//опускаем PA3 в 0
	gpio_clear(GPIOA, GPIO3);
	delay_us(1000);
	//устанавливаем reset PA3 в 1 
	gpio_set(GPIOA, GPIO3);	


	MFRC522_Init();

	uint8_t status = Read_MFRC522(VersionReg);
	uint8_t buffer[128] = {0};


	while(1){
		gpio_toggle(GPIOC, GPIO13);
		delay_us(100000);
		MFRC522_Read(0, &buffer[0]);

	}

}