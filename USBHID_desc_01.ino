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

#define ACK_PIN GPIO_PIN_1

#define NOP asm("nop")
#define NOP10 asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;")

static void GPIO_Init(void);
static void SPI_Master_Init(void);
static void SPI_Slave_Init(void);
static void SPI_isr_func(void);
static void PSX_ACK(void);
static void delay_us(uint8_t us);

template <typename T>
static bool getBitData(T data, uint8_t i);

// active low
union PSX_EventData {
    struct BitData
    {
        uint8_t select : 1;
        uint8_t none : 2;
        uint8_t start : 1;
        uint8_t up : 1;
        uint8_t right : 1;
        uint8_t down : 1;
        uint8_t left : 1;
        uint8_t l2 : 1;
        uint8_t r2 : 1;
        uint8_t l1 : 1;
        uint8_t r1 : 1;
        uint8_t tri : 1;
        uint8_t o : 1;
        uint8_t x : 1;
        uint8_t square : 1;
    };

    uint8_t data[2];
    BitData bitData;
} sendData, oldData;

SPI_HandleTypeDef SPI_Handle;
SPI_HandleTypeDef SPI_Slave_Handle;

USB Usb;
HIDUniversal Hid(&Usb);
JoystickReportParser parser;

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

    JoystickDescParser::EventData receiveData = parser.getEventData();

    memset(sendData.data, 0, sizeof(sendData.data));
    sendData.bitData.tri = getBitData(receiveData.buttons, 1);
    sendData.bitData.o = getBitData(receiveData.buttons, 3);
    sendData.bitData.x = getBitData(receiveData.buttons, 2);
    sendData.bitData.square = getBitData(receiveData.buttons, 0);
    sendData.bitData.l1 = getBitData(receiveData.buttons, 4);
    sendData.bitData.r1 = getBitData(receiveData.buttons, 5);
    sendData.bitData.l2 = getBitData(receiveData.buttons, 6);
    sendData.bitData.r2 = getBitData(receiveData.buttons, 7);
    sendData.bitData.select = getBitData(receiveData.buttons, 10);
    sendData.bitData.start = getBitData(receiveData.buttons, 11);

    sendData.bitData.up = receiveData.hat == 7 || receiveData.hat == 0 || receiveData.hat == 1;
    sendData.bitData.right = receiveData.hat == 1 || receiveData.hat == 2 || receiveData.hat == 3;
    sendData.bitData.down = receiveData.hat == 3 || receiveData.hat == 4 || receiveData.hat == 5;
    sendData.bitData.left = receiveData.hat == 5 || receiveData.hat == 6 || receiveData.hat == 7;

    bool change = false;

    // active lowなので反転
    for (int i = 0; i < sizeof(sendData.data); i++){
        sendData.data[i] = ~sendData.data[i];

        if(sendData.data[i] != oldData.data[i]){
            change = true;
        }
    }

    if(change){
        for (int i = 0; i < sizeof(sendData.data); i++){
            PrintBin<uint8_t>(sendData.data[i], 0x80);
            Serial.println();
        }
    }

    oldData = sendData;
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

    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = ACK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOB, ACK_PIN, GPIO_PIN_SET);
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
    SPI_Slave_Handle.Init.FirstBit = SPI_FIRSTBIT_LSB;
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
    GPIO_InitStruct.Pin = GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT; // Alternate function ドレイン
    GPIO_InitStruct.Pull = GPIO_NOPULL;      // プルアップ
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    stm32_interrupt_enable(GPIOA, GPIO_PIN_13, SPI_isr_func, GPIO_MODE_IT_FALLING);
}

void SPI_isr_func()
{
    // ココに処理を書かないこと！

    uint8_t tx = 0;
    uint8_t rv = 0;

    // read [start command]
    HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);
    if(rv == 0x01){
        PSX_ACK();

        // send [id], read [request the data]
        tx = 0x41; // 0x41=Digital, 0x23=NegCon, 0x73=Analogue Red LED, 0x53=Analogue Green LED
        HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);
        if (rv == 0x42){
            PSX_ACK();

            // send [here comes the data]
            tx = 0x5A; 
            HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);
            PSX_ACK();

            // send
            // Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
            // SLCT           STRT UP   RGHT DOWN LEFT
            tx = sendData.data[0];
            HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);
            PSX_ACK();

            // send
            // Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
            //  L2   R2    L1  R1   /\   O    X    |_|
            tx = sendData.data[1];
            HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, HAL_MAX_DELAY);
            PSX_ACK();

            Serial.println("sent!!");
        }
    }
}

template <typename T>
bool getBitData(T data, uint8_t i)
{
    return (data >> i) & 0b0001;
}

void PSX_ACK(void){
    HAL_GPIO_WritePin(GPIOB, ACK_PIN, GPIO_PIN_RESET);
    delay_us(3);
    HAL_GPIO_WritePin(GPIOB, ACK_PIN, GPIO_PIN_SET);
}

void delay_us(uint8_t us){
    // 84 MHz CPU
    // 1 clock = 1/84MHz = 11.9ns
    // 1us / 11.9ns = 84

    for (uint8_t i = 0; i < us; i++){
        // delary(1us)
        NOP10;
        NOP10;
        NOP10;
        NOP10;
        NOP10;
        NOP10;
        NOP10;
        NOP10;
        NOP;
        NOP;
        NOP;
        NOP;
    }
}