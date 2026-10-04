#include <stdint.h>

#define SYSTICK_BASE  0xE000E010UL
#define STK_CTRL      (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define STK_LOAD      (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))

#define RCC_BASE      0x40021000UL
#define RCC_APB2ENR   (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1ENR   (*(volatile uint32_t *)(RCC_BASE + 0x1C))

#define GPIOA_CRL     (*(volatile uint32_t *)(0x40010800 + 0x00))
#define GPIOA_CRH     (*(volatile uint32_t *)(0x40010800 + 0x04))
#define GPIOA_ODR     (*(volatile uint32_t *)(0x40010800 + 0x0C))

#define GPIOB_CRL     (*(volatile uint32_t *)(0x40010C00 + 0x00))
#define GPIOB_CRH     (*(volatile uint32_t *)(0x40010C00 + 0x04))
#define GPIOB_IDR     (*(volatile uint32_t *)(0x40010C00 + 0x08))
#define GPIOB_ODR     (*(volatile uint32_t *)(0x40010C00 + 0x0C))

#define AFIO_EXTICR2  (*(volatile uint32_t *)(0x40010000 + 0x0C))
#define EXTI_IMR      (*(volatile uint32_t *)(0x40010400 + 0x00))
#define EXTI_FTSR     (*(volatile uint32_t *)(0x40010400 + 0x0C))
#define EXTI_PR       (*(volatile uint32_t *)(0x40010400 + 0x14))

#define TIM2_CR1      (*(volatile uint32_t *)(0x40000000 + 0x00))
#define TIM2_DIER     (*(volatile uint32_t *)(0x40000000 + 0x0C))
#define TIM2_SR       (*(volatile uint32_t *)(0x40000000 + 0x10))
#define TIM2_EGR      (*(volatile uint32_t *)(0x40000000 + 0x14))
#define TIM2_CNT      (*(volatile uint32_t *)(0x40000000 + 0x24))
#define TIM2_PSC      (*(volatile uint32_t *)(0x40000000 + 0x28))
#define TIM2_ARR      (*(volatile uint32_t *)(0x40000000 + 0x2C))

#define NVIC_ISER0    (*(volatile uint32_t *)(0xE000E100 + 0x00))

char access_code[] = "159357";
char code_saisir[6];
volatile uint8_t compteur_touches = 0;
volatile uint8_t reset_demande = 0;
const uint8_t _7_segment[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

void delay_ms(uint32_t ms)
{
    STK_LOAD = 8000 - 1;
    STK_CTRL = 5;

    for (uint32_t i = 0; i < ms; i++) {
        while ((STK_CTRL & (1 << 16)) == 0) {
        }
    }

    STK_CTRL = 0;
}

void LCD_Command(uint8_t cmd)
{
    GPIOA_ODR = (GPIOA_ODR & 0xFFC0) | ((cmd >> 4) & 0x0F);
    GPIOA_ODR &= ~(1 << 4);
    GPIOA_ODR |= (1 << 5); delay_ms(1); GPIOA_ODR &= ~(1 << 5);

    GPIOA_ODR = (GPIOA_ODR & 0xFFC0) | (cmd & 0x0F);
    GPIOA_ODR |= (1 << 5); delay_ms(1); GPIOA_ODR &= ~(1 << 5);
    delay_ms(2);
}

void LCD_Data(uint8_t data)
{
    GPIOA_ODR = (GPIOA_ODR & 0xFFC0) | ((data >> 4) & 0x0F) | (1 << 4);
    GPIOA_ODR |= (1 << 5); delay_ms(1); GPIOA_ODR &= ~(1 << 5);

    GPIOA_ODR = (GPIOA_ODR & 0xFFC0) | (data & 0x0F) | (1 << 4);
    GPIOA_ODR |= (1 << 5); delay_ms(1); GPIOA_ODR &= ~(1 << 5);

    delay_ms(2);
}

void LCD_Init(void)
{
    delay_ms(20);
    LCD_Command(0x02);
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
}

void LCD_Print(char *str)
{
    while (*str) {
        LCD_Data(*str++);
    }
}

void LCD_Clear(void)
{
    LCD_Command(0x01);
    delay_ms(2);
}

void Reset_Systeme(void)
{
    GPIOA_ODR &= ~((1 << 8) | (1 << 9) | (1 << 10));
    GPIOB_ODR &= ~(0x7F);
    LCD_Clear();
    compteur_touches = 0;
    TIM2_CR1 &= ~(1 << 0);
    TIM2_CNT = 0;
}

void TIM2_IRQHandler(void)
{
    if (TIM2_SR & (1 << 0)) {
        TIM2_SR &= ~(1 << 0);
        TIM2_CR1 &= ~(1 << 0);
        reset_demande = 1;
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI_PR & (1 << 7)) {
        EXTI_PR = (1 << 7);
        reset_demande = 1;
    }
}

int main(void)
{
    RCC_APB2ENR |= 0x0D;
    RCC_APB1ENR = 1;

    GPIOA_CRL &= 0x00000000;
    GPIOA_CRH &= 0xFFFFF000;
    GPIOA_CRH |= 0x00000333;
    GPIOA_CRL |= 0x33333333;

    GPIOB_CRL &= 0x00000000;
    GPIOB_CRH &= 0x00000000;
    GPIOB_CRL |= 0x83333333;
    GPIOB_ODR |= (1 << 7);

    GPIOB_CRH |= 0x3338888;
    GPIOB_ODR |= (0xF << 8);

    AFIO_EXTICR2 |= (0x01 << 12);
    EXTI_IMR |= (1 << 7);
    EXTI_FTSR |= (1 << 7);
    NVIC_ISER0 |= (1 << 23);

    TIM2_PSC = 7999;
    TIM2_ARR = 2285;
    TIM2_EGR = 1;
    TIM2_SR = 0;
    TIM2_DIER |= (1 << 0);
    NVIC_ISER0 |= (1 << 28);

    LCD_Init();
    LCD_Print("TEST");

    while (1) {
        if (reset_demande) {
            reset_demande = 0;
            Reset_Systeme();
        }

        char touche_detectee = 0;

        for (int col = 0; col < 3; col++) {
            GPIOB_ODR |= (0x7 << 12);
            GPIOB_ODR &= ~(1 << (12 + col));
            delay_ms(2);

            uint16_t lignes = (GPIOB_IDR >> 8) & 0x0F;
            if (lignes != 0x0F) {
                delay_ms(20);
                if (((GPIOB_IDR >> 8) & 0x0F) == lignes) {
                    if      (!(lignes & 1)) touche_detectee = "123"[col];
                    else if (!(lignes & 2)) touche_detectee = "456"[col];
                    else if (!(lignes & 4)) touche_detectee = "789"[col];
                    else if (!(lignes & 8)) touche_detectee = "*0#"[col];

                    while (((GPIOB_IDR >> 8) & 0x0F) != 0x0F);
                }
            }
        }

        if (touche_detectee >= '0' && touche_detectee <= '9' && compteur_touches < 6) {
            code_saisir[compteur_touches] = touche_detectee;
            GPIOB_ODR = (GPIOB_ODR & ~0x7F) | _7_segment[touche_detectee - '0'];
            compteur_touches += 1;

            if (compteur_touches == 6) {
                uint8_t code_correct = 1;
                for (uint8_t i = 0; i < 6; i++) {
                    if (code_saisir[i] != access_code[i]) code_correct = 0;
                }
                LCD_Clear();

                if (code_correct) {
                    GPIOA_ODR |= (1 << 8);
                    LCD_Print("AIT LAHCEN AYOUB");
                } else {
                    GPIOA_ODR |= (1 << 9);
                    GPIOA_ODR |= (1 << 10);
                    LCD_Print("CODE ERRONE !");
                }

                TIM2_CNT = 0;
                TIM2_CR1 |= (1 << 0);
            }
        }
    }
}
