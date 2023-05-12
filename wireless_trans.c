#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "sx1280-hal.h"
#include "sx1280.h"
#include "radio.h"
#include "spi.h"
#include "wireless_trans.h"
#include "beep.h"
#include "adc.h"
#include "oled.h"


const bool isMaster = true;


/*!
 * Select mode of operation for the Ping Ping application
 */
//#define MODE_BLE
#define MODE_LORA
//#define MODE_GFSK
//#define MODE_FLRC


#define RF_BL_ADV_CHANNEL_38                        2478000000 // Hz

/*!
 * \brief Defines the nominal frequency
 */
#define RF_FREQUENCY                                RF_BL_ADV_CHANNEL_38 // Hz

/*!
 * \brief Defines the output power in dBm
 *
 * \remark The range of the output power is [-18..+13] dBm
 */
#define TX_OUTPUT_POWER                             0

/*!
 * \brief Defines the buffer size, i.e. the payload size
 */
#define BUFFER_SIZE                                 32

#define PACKET_LENGTH                               16

/*!
 * \brief Number of tick size steps for tx timeout
 */
#define TX_TIMEOUT_VALUE                            1000 // ms

/*!
 * \brief Number of tick size steps for rx timeout
 */
#define RX_TIMEOUT_VALUE                            1000 // ms

/*!
 * \brief Size of ticks (used for Tx and Rx timeout)
 */
#define RX_TIMEOUT_TICK_SIZE                        RADIO_TICK_SIZE_1000_US


/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( void );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( IrqErrorCode_t );


typedef enum
{
    APP_LOWPOWER,
    APP_RX,
    APP_RX_TIMEOUT,
    APP_RX_ERROR,
    APP_TX,
    APP_TX_TIMEOUT,
}AppStates_t;


RadioCallbacks_t Callbacks =
{
    &OnTxDone,        // txDone
    &OnRxDone,        // rxDone
    NULL,             // syncWordDone
    NULL,             // headerDone
    &OnTxTimeout,     // txTimeout
    &OnRxTimeout,     // rxTimeout
    &OnRxError,       // rxError
    NULL,             // rangingDone
    NULL,             // cadDone
};


/*!
 * \brief Mask of IRQs to listen to in rx mode
 */
uint16_t RxIrqMask = IRQ_RX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR;

/*!
 * \brief Mask of IRQs to listen to in tx mode
 */
uint16_t TxIrqMask = IRQ_TX_DONE | IRQ_RX_TX_TIMEOUT | IRQ_CRC_ERROR;

/*!
 * \brief The State of the application
 */
AppStates_t AppState = isMaster ? APP_TX : APP_RX;

uint16_t packetNum = 3000;

uint8_t Buffer[BUFFER_SIZE];

uint8_t BufferSize;

#if defined( MODE_BLE )
/*!
 * \brief In case of BLE, the payload must contain the header
 */
typedef union
{
    struct BleAdvHeaderField_s
    {
        uint8_t pduType: 4;
        uint8_t rfu1:2;
        uint8_t txAddr:1;
        uint8_t rxAddr:1;
        uint8_t length:6;
        uint8_t rfu2:2;
    } Fields;
    uint8_t Serial[ 2 ];
}BleAdvHeaders_t;
BleAdvHeaders_t ble_header_adv;
#endif // MODE_BLE

PacketParams_t packetParams;

PacketStatus_t packetStatus;

void RFGPIOInit()
{
    /****************************************
     RF_RST
    ****************************************/
    gpio_init(RADIO_nRESET_PIN);
    gpio_set_dir(RADIO_nRESET_PIN, GPIO_OUT);
    gpio_put(RADIO_nRESET_PIN, 1);
  
    /****************************************
     RF_DIO1
    ****************************************/
    gpio_init(RADIO_DIOx_PIN);
    gpio_set_dir(RADIO_DIOx_PIN, GPIO_IN);
    gpio_pull_down(RADIO_DIOx_PIN);

    /****************************************
     Radio_BUSY
    ****************************************/
    gpio_init(RADIO_BUSY_PIN);
    gpio_set_dir(RADIO_BUSY_PIN, GPIO_IN);
    gpio_pull_up(RADIO_BUSY_PIN);
  
    /****************************************
     RADIO_CTX
    ****************************************/
    gpio_init(RADIO_CTX_PIN);
    gpio_set_dir(RADIO_CTX_PIN, GPIO_OUT);

    /****************************************
     RADIO_CPS
    ****************************************/
    gpio_init(RADIO_CPS_PIN);
    gpio_set_dir(RADIO_CPS_PIN, GPIO_OUT);
  
    /****************************************
     RADIO_CSD
    ****************************************/
    gpio_init(RADIO_CSD_PIN);
    gpio_set_dir(RADIO_CSD_PIN, GPIO_OUT);

    /****************************************
     ANT_SEL
    ****************************************/
    gpio_init(ANT_SEL_PIN);
    gpio_set_dir(ANT_SEL_PIN, GPIO_OUT);
}

void SE243L_PA_Enable()
{
    gpio_put(RADIO_CPS_PIN, 0);  // CPS=0
    gpio_put(RADIO_CSD_PIN, 1);  // CSD=1
    gpio_put(RADIO_CTX_PIN, 1);  // CTX=1

    gpio_put(ANT_SEL_PIN, 1);
}

void SE243L_LNA_Enable()
{
    gpio_put(RADIO_CPS_PIN, 1);  // CPS=1
    gpio_put(RADIO_CSD_PIN, 1);  // CSD=1
    gpio_put(RADIO_CTX_PIN, 0);  // CTX=0

    gpio_put(ANT_SEL_PIN, 1);
}

void SE243L_SLEEP()
{
    gpio_put(RADIO_CPS_PIN, 0);  // CPS=0
    gpio_put(RADIO_CSD_PIN, 0);  // CSD=0
    gpio_put(RADIO_CTX_PIN, 0);  // CTX=0

    gpio_put(ANT_SEL_PIN, 1);
}

void WRTInit(void)
{
    //static uint16_t test;
    RFGPIOInit();
    SPIInit();

    ModulationParams_t modulationParams;

    Radio.Init( &Callbacks );

    Radio.SetRegulatorMode( USE_DCDC ); // Can also be set in LDO mode but consume more power
    
    //test=Radio.GetFirmwareVersion();    // A9B5(43445) test spi
    //printf("firmware version: %x\n", test);
    
    #if defined( MODE_BLE )

        modulationParams.PacketType = PACKET_TYPE_BLE;
        modulationParams.Params.Ble.BitrateBandwidth = GFS_BLE_BR_1_000_BW_1_2;
        modulationParams.Params.Ble.ModulationIndex = GFS_BLE_MOD_IND_0_50;
        modulationParams.Params.Ble.ModulationShaping = RADIO_MOD_SHAPING_BT_0_5;
    
        packetParams.PacketType = PACKET_TYPE_BLE;
        packetParams.Params.Ble.BlePacketType = BLE_EYELONG_1_0;
        packetParams.Params.Ble.ConnectionState = BLE_ADVERTISER;
        packetParams.Params.Ble.CrcField = BLE_CRC_3B;
        packetParams.Params.Ble.Whitening = RADIO_WHITENING_ON;
    
    #elif defined( MODE_GFSK )
    
        modulationParams.PacketType = PACKET_TYPE_GFSK;
        modulationParams.Params.Gfsk.BitrateBandwidth = GFS_BLE_BR_0_125_BW_0_3;
        modulationParams.Params.Gfsk.ModulationIndex = GFS_BLE_MOD_IND_1_00;
        modulationParams.Params.Gfsk.ModulationShaping = RADIO_MOD_SHAPING_BT_1_0;
    
        packetParams.PacketType = PACKET_TYPE_GFSK;
        packetParams.Params.Gfsk.PreambleLength = PREAMBLE_LENGTH_32_BITS;
        packetParams.Params.Gfsk.SyncWordLength = GFS_SYNCWORD_LENGTH_5_BYTE;
        packetParams.Params.Gfsk.SyncWordMatch = RADIO_RX_MATCH_SYNCWORD_1;
        packetParams.Params.Gfsk.HeaderType = RADIO_PACKET_VARIABLE_LENGTH;
        packetParams.Params.Gfsk.PayloadLength = BUFFER_SIZE;
        packetParams.Params.Gfsk.CrcLength = RADIO_CRC_3_BYTES;
        packetParams.Params.Gfsk.Whitening = RADIO_WHITENING_ON;
    
    #elif defined( MODE_LORA )
    
        modulationParams.PacketType = PACKET_TYPE_LORA;
        modulationParams.Params.LoRa.SpreadingFactor = LORA_SF6;// Effective Data Rate 50.78Kb/s
        modulationParams.Params.LoRa.Bandwidth = LORA_BW_1600;
        modulationParams.Params.LoRa.CodingRate = LORA_CR_LI_4_7;
    
        packetParams.PacketType = PACKET_TYPE_LORA;
        packetParams.Params.LoRa.PreambleLength = 12;
        packetParams.Params.LoRa.HeaderType = LORA_PACKET_VARIABLE_LENGTH;
        packetParams.Params.LoRa.PayloadLength = BUFFER_SIZE;
        packetParams.Params.LoRa.CrcMode = LORA_CRC_ON;
        packetParams.Params.LoRa.InvertIQ = LORA_IQ_NORMAL;
    
    #elif defined( MODE_FLRC )
    
        modulationParams.PacketType = PACKET_TYPE_FLRC;
        modulationParams.Params.Flrc.BitrateBandwidth = FLRC_BR_0_260_BW_0_3;
        modulationParams.Params.Flrc.CodingRate = FLRC_CR_1_2;
        modulationParams.Params.Flrc.ModulationShaping = RADIO_MOD_SHAPING_BT_1_0;

        packetParams.PacketType = PACKET_TYPE_FLRC;
        packetParams.Params.Flrc.PreambleLength = PREAMBLE_LENGTH_32_BITS;
        packetParams.Params.Flrc.SyncWordLength = FLRC_SYNCWORD_LENGTH_4_BYTE;
        packetParams.Params.Flrc.SyncWordMatch = RADIO_RX_MATCH_SYNCWORD_1;
        packetParams.Params.Flrc.HeaderType = RADIO_PACKET_VARIABLE_LENGTH;
        packetParams.Params.Flrc.PayloadLength = BUFFER_SIZE;
        packetParams.Params.Flrc.CrcLength = RADIO_CRC_3_BYTES;
        packetParams.Params.Flrc.Whitening = RADIO_WHITENING_OFF;

    #else
        #error "Please select the mode of operation for the Ping Ping demo"
    #endif
    
    Radio.SetStandby( STDBY_RC );
    Radio.SetPacketType( modulationParams.PacketType );
    Radio.SetModulationParams( &modulationParams );
    Radio.SetPacketParams( &packetParams );
    Radio.SetRfFrequency( RF_FREQUENCY );
    Radio.SetBufferBaseAddresses( 0x00, 0x00 );
    Radio.SetTxParams( TX_OUTPUT_POWER, RADIO_RAMP_02_US );
    
    #if defined( MODE_BLE )
        // only used in GENERIC and BLE mode
        Radio.SetSyncWord( 1, ( uint8_t[] ){ 0xDD, 0xA0, 0x96, 0x69, 0xDD } );
        Radio.WriteRegister(0x9c7, 0x55 );
        Radio.WriteRegister(0x9c8, 0x55 );
        Radio.WriteRegister(0x9c9, 0x55 );
        //Radio.WriteRegister( 0x9c5, 0x33 );
        Radio.SetBleAdvertizerAccessAddress( );
        Radio.SetWhiteningSeed( 0x33 );
        ble_header_adv.Fields.length = PINGPONGSIZE + 2;
        ble_header_adv.Fields.pduType = 2;
    #endif // MODE_BLE
}

void WRSendPacket()
{
    //++packetNum;
    //if (packetNum == 0) {
    //    packetNum = 3000;
    //}
    for (uint8_t i = 0; i < PACKET_LENGTH; i++) {
        if (i == 0) {
            Buffer[2 * i] = 0x00;
            Buffer[2 * i + 1] = 0x00;
        }
        else if (i <= CH_NUM) {
            Buffer[2 * i] = (uint8_t)(pwmValues[i - 1] >> 8) & 0xFF;
            Buffer[2 * i + 1] = (uint8_t)pwmValues[i - 1] & 0xFF;
        }
        else {
            Buffer[2 * i] = 0xFF;
            Buffer[2 * i + 1] = 0xFF;
            /*
            if (i != PACKET_LENGTH - 1) {
                Buffer[2 * i] = 0xFF;
                Buffer[2 * i + 1] = 0xFF;
            }
            else {
                Buffer[2 * i] = (uint8_t)(packetNum >> 8) & 0xFF;
                Buffer[2 * i + 1] = (uint8_t)packetNum & 0xFF;
            }
            */
        }
    }
}

void StartSendPacket() {
    if(isMaster==true)
    {
        SE243L_PA_Enable();
        WRSendPacket();
        Radio.SetDioIrqParams( TxIrqMask, TxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
        Radio.SendPayload( Buffer, BUFFER_SIZE, ( TickTime_t ){ RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE } );
    }
    else
    {
        SE243L_LNA_Enable();
        Radio.SetDioIrqParams( RxIrqMask, RxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
        Radio.SetRx( ( TickTime_t ) { RX_TIMEOUT_TICK_SIZE, RX_TIMEOUT_VALUE } );
    }

    while(true)
    {
        SX1280HalWaitOnDIOx();
        
        SX1280ProcessIrqs();
        
        switch( AppState )
        {
            case APP_RX:
                Radio.GetPayload( Buffer, &BufferSize, BUFFER_SIZE );

                //OledOnVoltageChanged(Buffer[0], Buffer[1]);
  
                SE243L_PA_Enable();
                WRSendPacket();
                Radio.SetDioIrqParams( TxIrqMask, TxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
                Radio.SendPayload( Buffer, BUFFER_SIZE, ( TickTime_t ){ RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE } );
                
                break;

            case APP_TX:
                // TODO:
                //if (packetNum % 1024 != 0) {
                    SE243L_PA_Enable();
                    WRSendPacket();
                    Radio.SetDioIrqParams( TxIrqMask, TxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
                    Radio.SendPayload( Buffer, BUFFER_SIZE, ( TickTime_t ){ RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE } );
                //}
                //else {
                //    SE243L_LNA_Enable();
                //    Radio.SetDioIrqParams( RxIrqMask, RxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
                //    Radio.SetRx( ( TickTime_t ) { RX_TIMEOUT_TICK_SIZE, RX_TIMEOUT_VALUE } );
                //}
                
                break;

            case APP_RX_TIMEOUT:
            case APP_RX_ERROR:
            case APP_TX_TIMEOUT:
            default:
                SE243L_PA_Enable();
                WRSendPacket();
                Radio.SetDioIrqParams( TxIrqMask, TxIrqMask, IRQ_RADIO_NONE, IRQ_RADIO_NONE );
                Radio.SendPayload( Buffer, BUFFER_SIZE, ( TickTime_t ){ RX_TIMEOUT_TICK_SIZE, TX_TIMEOUT_VALUE } );

                break;
        }
    }

    SE243L_SLEEP();
}

void OnTxDone( void )
{
    //printf("Tx Done\n");
    AppState = APP_TX;
}

void OnRxDone( void )
{
    printf("Rx Done\n");
    AppState = APP_RX;
}

void OnTxTimeout( void )
{
    printf("Tx Timeout\n");
    BeepOnState(packetTxTimeout);
    AppState = APP_TX_TIMEOUT;
}

void OnRxTimeout( void )
{
    printf("Rx Timeout\n");
    BeepOnState(packetRxTimeout);
    AppState = APP_RX_TIMEOUT;
}

void OnRxError( IrqErrorCode_t errorCode )
{
    printf("Rx Error\n");
    BeepOnState(packetRxError);
    AppState = APP_RX_ERROR;
}

void OnRangingDone( IrqRangingCode_t val )
{
  
}

void OnCadDone( bool channelActivityDetected )
{
  
}
