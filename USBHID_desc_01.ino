#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>
#include "pgmstrings.h"
#include <SPI.h>

#include "JoystickReportParser.h"

// SPI2
#define MOSI_PIN GPIO_PIN_15
#define MISO_PIN GPIO_PIN_14
#define SCK_PIN GPIO_PIN_13

#define NBYTES 256

SPI_HandleTypeDef SPI_Handle;
SPI_HandleTypeDef SPI_Slave_Handle;

int spi_flag = 0;
byte rx_buffer[NBYTES];

USB Usb;
HIDUniversal Hid(&Usb);
JoystickReportParser parser;

static void GPIO_Init(void);
static void SPI_Master_Init(void);
static void SPI_Slave_Init(void);
static void SPI_isr_func(void);

void setup()
{
    HAL_Init();           // Reset of all peripherals, Initializes the Flash interface and the Systick

    // Initialize all configured peripherals
    GPIO_Init();
    SPI_Master_Init();
    SPI_Slave_Init();

    Serial.begin(115200);
    while (!Serial)
        ; // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
    Serial.println("Start");

    // ----------------------------------------------
    // USB
    // ----------------------------------------------
    if (Usb.Init() == -1){
        Serial.println("OSC did not start.");
    }

    delay(200);

    if (!Hid.SetReportParser(0, &parser)){
        ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
    }
}

void loop()
{
    Usb.Task();
}

// SPI1 init function
void SPI_Master_Init(void) {
	SPI_Handle.Instance = SPI1;
	SPI_Handle.Init.Mode = SPI_MODE_MASTER;
	SPI_Handle.Init.Direction = SPI_DIRECTION_2LINES;
	SPI_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
	SPI_Handle.Init.CLKPolarity = SPI_POLARITY_LOW;
	SPI_Handle.Init.CLKPhase = SPI_PHASE_1EDGE;
	SPI_Handle.Init.NSS = SPI_NSS_SOFT;
	SPI_Handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	SPI_Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
	SPI_Handle.Init.TIMode = SPI_TIMODE_DISABLED;
	SPI_Handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
	SPI_Handle.Init.CRCPolynomial = 10;
	HAL_SPI_Init(&SPI_Handle);
}

void GPIO_Init(void)
{
    // GPIO Ports Clock Enable
    __GPIOA_CLK_ENABLE();
    __GPIOB_CLK_ENABLE();
    __GPIOC_CLK_ENABLE();

    // Initialize user button
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

// SPI1 init function
void SPI_Slave_Init(void)
{
    __GPIOB_CLK_ENABLE();
    __SPI2_CLK_ENABLE();

    // ---------------------------------------------------
    // SPIの設定
    // ---------------------------------------------------
    SPI_Slave_Handle.Instance = SPI2;
    SPI_Slave_Handle.Init.Mode = SPI_MODE_SLAVE;
    SPI_Slave_Handle.Init.Direction = SPI_DIRECTION_2LINES;
    SPI_Slave_Handle.Init.DataSize = SPI_DATASIZE_8BIT;
    SPI_Slave_Handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    SPI_Slave_Handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    SPI_Slave_Handle.Init.NSS = SPI_NSS_SOFT;
    SPI_Slave_Handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    SPI_Slave_Handle.Init.TIMode = SPI_TIMODE_DISABLED;
    SPI_Slave_Handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLED;
    SPI_Slave_Handle.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&SPI_Slave_Handle);

    // ---------------------------------------------------
    // MISO, MOSI, SCK, SSの設定
    // ---------------------------------------------------
    GPIO_InitTypeDef GPIO_InitStruct;

    // MISO
    GPIO_InitStruct.Pin = MISO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;  // Alternate function 出力
    GPIO_InitStruct.Pull = GPIO_PULLUP;      // プルアップ
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH; // ハイスピード
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function mappingを参照のこと
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SCK, MOSI
    GPIO_InitStruct.Pin = SCK_PIN | MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Alternate function ドレイン
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SSの設定
    stm32_interrupt_enable(GPIOA, GPIO_PIN_13, SPI_isr_func, GPIO_MODE_IT_FALLING);
}

void SPI_isr_func()
{
    // ココに処理を書かないこと！

    uint8_t tx = 0x00;
    for (int x = 0; x < NBYTES; x++)
    {
        uint8_t rv = 0;
        HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);

        rx_buffer[x] = rv;
        tx = rx_buffer[x];

        Serial.println(rx_buffer[x]);
        if (rx_buffer[x] == 0xFF)
        {
            break;
        }
    }
}

