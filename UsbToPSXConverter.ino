#include <usbhid.h>
#include <hiduniversal.h>
#include <SPI.h>

#include <vector>

#include "JoystickReportParser.h"

// #include "sd_diskio_spi.h"
#include <STM32SD.h>

#define SD_DETECT_PIN SD_DETECT_NONE

// SPI2
#define MOSI_PIN GPIO_PIN_15
#define MISO_PIN GPIO_PIN_14
#define SCK_PIN GPIO_PIN_13
#define SS_PIN GPIO_PIN_12
#define ACK_PIN GPIO_PIN_1

#define NOP asm("nop")
#define NOP10 asm("nop;nop;nop;nop;nop;nop;nop;nop;nop;nop;")

#define SPI_SLAVE_TIMEOUT 1

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

    uint16_t allData;
    uint8_t data[2];
    BitData bitData;
} sendData, oldData;

enum Key{
    KEY_O = 0,
    KEY_X,
    KEY_TRI,
    KEY_SQUARE,
    KEY_L1,
    KEY_R1,
    KEY_L2,
    KEY_R2,
    KEY_SELECT,
    KEY_START,
    Cnt
};

SPI_HandleTypeDef SPI_Handle;
SPI_HandleTypeDef SPI_Slave_Handle;

USB Usb;
HIDUniversal Hid(&Usb);
JoystickReportParser parser;

File myFile;

uint8_t keyTable[(uint8_t)Key::Cnt] = {
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,
    10,
    11
};

std::vector<String> split(const String &input, char delimiter)
{
    std::vector<String> result;
    String tmp;
    for (int i = 0; i < input.length(); i++)
    {
        char c = input.charAt(i);
        if (c == delimiter)
        {
            tmp.trim();
            result.push_back(tmp);
            
            tmp = "";
        }
        else
        {
            tmp += c;
        }
    }
    result.push_back(tmp);

    return result;
}

String readLine(File file)
{
  String line = "";
  while (myFile.available())
  {
    char c = myFile.read();
    if (c == '\n' || c == '\r')
    {
      break;
    }
    else
    {
      line += c;
    }
  }

  return line;
}

void setup()
{
    HAL_Init(); // Reset of all peripherals, Initializes the Flash interface and the Systick

    // Initialize all configured peripherals
    GPIO_Init();
    SPI_Master_Init();
    SPI_Slave_Init();

    // ----------------------------------------------
    // Serial
    // ----------------------------------------------
    Serial.begin(115200);
    while (!Serial)
        ;
    E_Notify("Start\n", 0x80);


    // ----------------------------------------------
    // keyconfig
    // ----------------------------------------------

    Serial.print("Initializing SD card...");
    while (SD.begin(SD_DETECT_PIN) != TRUE){
        delay(10);
    }
    Serial.println("initialization done.");

    // read file
    uint8_t data_offset = 0;
    myFile = SD.open("keyconfig.ini");
    if (myFile) {
        Serial.println("keyconfig.ini");

        while (myFile.available()){
        String line = readLine(myFile);

        auto s = split(line, '=');
        if (s.size() == 2){
            if (s[0] == "O"){
                keyTable[Key::KEY_O] = s[1].toInt();
            }
            else if (s[0] == "X"){
                keyTable[Key::KEY_X] = s[1].toInt();
            }
            else if (s[0] == "TRI"){
                keyTable[Key::KEY_TRI] = s[1].toInt();
            }
            else if (s[0] == "SQUARE"){
                keyTable[Key::KEY_SQUARE] = s[1].toInt();
            }
            else if (s[0] == "L1"){
                keyTable[Key::KEY_L1] = s[1].toInt();
            }
            else if (s[0] == "R1"){
                keyTable[Key::KEY_R1] = s[1].toInt();
            }
            else if (s[0] == "L2"){
                keyTable[Key::KEY_L2] = s[1].toInt();
            }
            else if (s[0] == "R2"){
                keyTable[Key::KEY_R2] = s[1].toInt();
            }
            else if (s[0] == "SELECT"){
                keyTable[Key::KEY_SELECT] = s[1].toInt();
            }
            else if (s[0] == "START"){
                keyTable[Key::KEY_START] = s[1].toInt();
            }
            
            else if (s[0] == "OFFSET"){
                data_offset = s[1].toInt();
            }

            Serial.print(s[0]);
            Serial.print(" = ");
            Serial.println(s[1].toInt());
        }
        }
        
        myFile.close();
    }
    else {
        Serial.println("error opening");
    }

    // ----------------------------------------------
    // USB
    // ----------------------------------------------
    if (Usb.Init() == -1)
    {
        E_Notify("OSC did not start.\n", 0x80);
    }

    delay(200);

    // 何かデータにオフセットが入ってることがあるので
    parser.setDataOffset(data_offset);

    if (!Hid.SetReportParser(0, &parser))
    {
        ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1);
    }
}

void loop()
{
    Usb.Task();

    JoystickDescParser::EventData receiveData = parser.getEventData();
    PSX_EventData tmp = {0};

    tmp.bitData.tri = getBitData(receiveData.buttons, keyTable[Key::KEY_TRI]);
    tmp.bitData.o = getBitData(receiveData.buttons, keyTable[Key::KEY_O]);
    tmp.bitData.x = getBitData(receiveData.buttons, keyTable[Key::KEY_X]);
    tmp.bitData.square = getBitData(receiveData.buttons, keyTable[Key::KEY_SQUARE]);
    tmp.bitData.l1 = getBitData(receiveData.buttons, keyTable[Key::KEY_L1]);
    tmp.bitData.r1 = getBitData(receiveData.buttons, keyTable[Key::KEY_R1]);
    tmp.bitData.l2 = getBitData(receiveData.buttons, keyTable[Key::KEY_L2]);
    tmp.bitData.r2 = getBitData(receiveData.buttons, keyTable[Key::KEY_R2]);
    tmp.bitData.select = getBitData(receiveData.buttons, keyTable[Key::KEY_SELECT]);
    tmp.bitData.start = getBitData(receiveData.buttons, keyTable[Key::KEY_START]);

    tmp.bitData.up = receiveData.hat == 7 || receiveData.hat == 0 || receiveData.hat == 1;
    tmp.bitData.right = receiveData.hat == 1 || receiveData.hat == 2 || receiveData.hat == 3;
    tmp.bitData.down = receiveData.hat == 3 || receiveData.hat == 4 || receiveData.hat == 5;
    tmp.bitData.left = receiveData.hat == 5 || receiveData.hat == 6 || receiveData.hat == 7;

    bool change = false;

    // active lowなので反転
    tmp.allData = ~tmp.allData;
    if (tmp.allData != oldData.allData)
    {
        change = true;
    }
    sendData.allData = tmp.allData;

    if (change)
    {
        for (int i = 0; i < sizeof(sendData.data); i++)
        {
            PrintBin<uint8_t>(sendData.data[i], 0x80);
            E_Notify(PSTR("\r\n"), 0x80);
        }
    }
    oldData = sendData;
}

// SPI1 init function
void SPI_Master_Init(void)
{
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
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
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
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;    // Alternate function 出力
    GPIO_InitStruct.Pull = GPIO_NOPULL;        // プルアップ
    GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;   // ハイスピード
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2; // Alternate function mappingを参照のこと
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SCK, MOSI
    GPIO_InitStruct.Pin = SCK_PIN | MOSI_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Alternate function ドレイン
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // プルアップ
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // SSの設定
    GPIO_InitStruct.Pin = SS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD; // Alternate function ドレイン
    GPIO_InitStruct.Pull = GPIO_PULLUP;     // プルアップ
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    stm32_interrupt_enable(GPIOB, SS_PIN, SPI_isr_func, GPIO_MODE_IT_FALLING);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
}


void SPI_isr_func()
{
    uint8_t tx = 0;
    uint8_t rv = 0;
    
    // フェイントSS対策
    delay_us(5);

    if (HAL_GPIO_ReadPin(GPIOB, SS_PIN) == GPIO_PIN_RESET)
    {
        // read [start command]
        HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, SPI_SLAVE_TIMEOUT);
        if (rv == 0x01)
        {
            PSX_ACK();

            // send [id], read [request the data]
            tx = 0x41; // 0x41=Digital, 0x23=NegCon, 0x73=Analogue Red LED, 0x53=Analogue Green LED
            HAL_SPI_TransmitReceive(&SPI_Slave_Handle, &tx, &rv, 1, SPI_SLAVE_TIMEOUT);
            if (rv == 0x42)
            {
                PSX_ACK();

                // send [here comes the data]
                tx = 0x5A;
                HAL_SPI_Transmit(&SPI_Slave_Handle, &tx, 1, SPI_SLAVE_TIMEOUT);
                PSX_ACK();

                // send
                // Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
                // SLCT           STRT UP   RGHT DOWN LEFT
                tx = sendData.data[0];
                HAL_SPI_Transmit(&SPI_Slave_Handle, &tx, 1, SPI_SLAVE_TIMEOUT);
                PSX_ACK();

                // send
                // Bit0 Bit1 Bit2 Bit3 Bit4 Bit5 Bit6 Bit7
                //  L2   R2    L1  R1   /\   O    X    |_|
                tx = sendData.data[1];
                HAL_SPI_Transmit(&SPI_Slave_Handle, &tx, 1, SPI_SLAVE_TIMEOUT);

                E_Notify("sent!!\n", 0x80);
            }
        }

        __HAL_SPI_DISABLE(&SPI_Slave_Handle);
    }
}

template <typename T>
bool getBitData(T data, uint8_t i)
{
    return (data >> i) & 0b0001;
}

void PSX_ACK(void)
{
    delay_us(10);
    HAL_GPIO_WritePin(GPIOB, ACK_PIN, GPIO_PIN_RESET);
    delay_us(2);
    HAL_GPIO_WritePin(GPIOB, ACK_PIN, GPIO_PIN_SET);
}

void delay_us(uint8_t us)
{
    // 84 MHz CPU
    // 1 clock = 1/84MHz = 11.9ns
    // 1us / 11.9ns = 84

    for (uint8_t i = 0; i < us; i++)
    {
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