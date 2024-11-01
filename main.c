#include <stdint.h>
#include <stm32f10x.h>

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

void SPI1_Write(uint8_t data){
	while( SPI1->SR & SPI_SR_BSY); 
	while( (SPI1->SR & SPI_SR_TXE) == 0);
	SPI1->DR = data;
	return;
}

uint8_t SPI1_Read(){
	while( (SPI1->SR & SPI_SR_RXNE) == 0 );
	return SPI1->DR;
}

void delay(uint32_t ticks) {
	for (int i=0; i<ticks; i++) {
		__NOP();
	}
}

void cmd(uint8_t data){ // Отправка команды
   GPIOA->ODR &= ~GPIO_ODR_ODR3; // A0=0 --указание на то, что отправляем команду
   GPIOA->ODR &= ~GPIO_ODR_ODR1; // CS=0 – указание дисплею, что данные адресованы ему
   SPI1_Write(data);
   while( SPI1->SR & SPI_SR_BSY);
   GPIOA->ODR |= GPIO_ODR_ODR1; // CS=1 – окончание передачи данных
}

void dat(uint8_t data){ // Отправка данных
   GPIOA->ODR |= GPIO_ODR_ODR3; // A0=1 --указание на то, что отправляем данные
   GPIOA->ODR &= ~GPIO_ODR_ODR1; // CS=0 – указание дисплею, что данные адресованы ему
   SPI1_Write(data);
   while( SPI1->SR & SPI_SR_BSY);
   GPIOA->ODR |= GPIO_ODR_ODR1; // CS=1 – окончание передачи данных
}

void SPI1_Init(){
	// Включить тактирование PORTA и SPI1
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

	// PA5 Alternate function Output
	GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	GPIOA->CRL |= GPIO_CRL_MODE5_0; // Mode='01' Output 10Mhz
	GPIOA->CRL |= GPIO_CRL_CNF5_1; // Alt function Push-pull

	// PA7 Alternate function Output
	GPIOA->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
	GPIOA->CRL |= GPIO_CRL_MODE7_0; // Mode='01' Output 10Mhz
	GPIOA->CRL |= GPIO_CRL_CNF7_1; // Alt function Push-pull

	// PA6 Alternate function Input
	GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
	GPIOA->CRL |= GPIO_CRL_CNF6_1; // Input with pull-up

	// SPI1_BR = '011' Freq=F/16
	SPI1->CR1 &= ~ SPI_CR1_BR;
	SPI1->CR1 |= SPI_CR1_BR_0 | SPI_CR1_BR_1;

	SPI1->CR1 |= SPI_CR1_MSTR; // Master mode
	SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI; // Software CS, Chip select = ON
	SPI1->CR1 |= SPI_CR1_CPOL; // CLK=1 when idle
	SPI1->CR1 |= SPI_CR1_CPHA; // Falling edge
	//SPI1->CR1 |= SPI_CR1_LSBFIRST; // Low significant bit first
	SPI1->CR1 &= ~SPI_CR1_DFF; // 8-bit data Frame

	// Включить SPI
	SPI1->CR1 |= SPI_CR1_SPE;
}

void Display_Init(){
	SPI1_Init();
	// Настройка PORTA, PA1..3 на выход, General purpose output push-pull
	RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; 
	GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1 |
	GPIO_CRL_CNF2 | GPIO_CRL_MODE2 |
	GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
	GPIOA->CRL |= GPIO_CRL_MODE1_0 |
		GPIO_CRL_MODE2_0 |
		GPIO_CRL_MODE3_0;
	// SPI1 CPOL=1, CPHA=1
	SPI1->CR1 |= SPI_CR1_CPOL; // CLK=1 when idle
	SPI1->CR1 |= SPI_CR1_CPHA; // Falling edge

   GPIOA->ODR &= ~GPIO_ODR_ODR1; // CS=0
   GPIOA->ODR &= ~GPIO_ODR_ODR2; // RESET=0 - аппаратный сброс
   delay_us(10000); // Wait for the power stabilized
   GPIOA->ODR |= GPIO_ODR_ODR2; // RESET=1
   delay_us(1000); // Wait <1ms

	/* LCD bias setting (11) */
	cmd(0b10100011); //1/7 bias
	/* ADC selection */
	cmd(0b10100000);
	/* Common output mode selection */
	cmd(0b11000000);
	/* Power control mode */
	cmd(0x28 | 0b111);  //0b111
	/* Vo regulator resistor ratio (check) */
	uint8_t resRatio = 0x04;
	cmd(0b00100000 | resRatio);
  	cmd(0xA6); // Normal color, A7 = inverse color
  	cmd(0xAF); // Display on
   /*cmd(0xA2); //LCD Drive set 1/9 bias
   cmd(0xA2); //LCD Drive set 1/9 bias
   cmd(0xA0); // RAM Address SEG Output normal
   cmd(0xC8); // Common output mode selection
   cmd(0x28 | 0x07); // Power control mode
   cmd(0x20 | 0x05); // Voltage regulator
   cmd(0xA6); // Normal color, A7 = inverse color
   cmd(0xAF); // Display on*/
   cmd(0xAF); // Display on

}




int __attribute((noreturn)) main(void) {
	//RCC->APB2ENR |= 0b10000;
	// Включает тактирование PORTC, PORTB
	RCC->APB2ENR |= (RCC_APB2ENR_IOPCEN | RCC_APB2ENR_IOPBEN);
	// Настройка PORTC, PC13 на выход, General purpose output push-pull
	GPIOC->CRH = ~(GPIO_CRH_CNF13 | GPIO_CRH_MODE13) | GPIO_CRH_MODE13_0; 
	// Настройка PORTB, PB9 на вход с подтягивающим резистором
	GPIOB->CRH = ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9) | GPIO_CRH_CNF9_1;
	// Включение pull-up на PB9
	GPIOB->ODR |= GPIO_ODR_ODR9;
	// Запись логического 0 на выход PC13 (LED ON)
	GPIOC->ODR &= ~GPIO_ODR_ODR13;
	// Инициализация SPI1
	SPI1_Init();
	// Инициализация дисплея
	Display_Init();
	cmd(0b10110000);
	cmd(0b00010000);
	cmd(0x00);
	for(int i=0; i<128; i++){
		dat(0xFF);
	}



    while (1) {
		if(GPIOB->IDR & GPIO_IDR_IDR9){ 
			GPIOC->ODR &= ~GPIO_ODR_ODR13; // LED ON
		} else { // Button is pressed
			GPIOC->ODR |= GPIO_ODR_ODR13; // LED OFF
		}
		// ToDo: delay_ms(1000)
    }

}