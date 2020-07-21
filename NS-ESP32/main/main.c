#include "main.h"
#include "sys/queue.h"
#include "soc/sens_reg.h"
#include "soc/rtc_io_reg.h"

/**
 * GPIO 5,18,19,23 will be used to SPI comms; 5:cs;18:sck;19:miso;23:mosi
 * GPIO 21,22 will be used as I2C to read battery lvl
 * GPIO 25,26 will be used as DACs
 * GPIO 2,4,16,17 will be used as switches
*/

void app_main(){ // runs in cpu0
    
    
    //comment this if you want esp32 store ssid/password
    //clear_wifi_config();

    //enable debug mode here
    DEBUG_MODE_ENABLED = 1;

    SERVER_ON = false;
    SOCKET_PORT = 8888;
    BATTERY_UPDATE_TIME_INTERVAL = 10000; //10 second
    BATTERY_LEVEL = 0;
    CHANNEL_NUM = 1;
    MAX_FREQ = 100000;//10KHZ
    PHASE_ONE_TIME = 10;// default 10us
    PHASE_TWO_TIME = 10;// default 10us
    STIM_AMP = 2000;// default 0uA
    INTER_PHASE_GAP = 0;//default 0us
    INTER_STIM_DELAY = 100;//default 0us
    ANODIC_CATHODIC = 1;//default cathodic
    STIM_TYPE = 0;//default uniform stim
    PULSE_NUM = 0;//default 0 is forever in ms
    BURST_NUM = 0;// number of burst
    INTER_BURST_DELAY = 0;
    PULSE_NUM_IN_ONE_BURST = 0;
    RAMP_UP = 0;
    SHORT_ELECTRODE = 1;

    STIM_STATUS = 0;
    STIM_TASK_STATUS = 0;

    ENABLE_RECORD = false;
    RECORD_OFFSET = 0;

    wifi_init();
    //i2c_connection_status = battery_init();

    vTaskDelay(2000 / portTICK_PERIOD_MS);


    char list[512];
    vTaskList((char *)&list);
    printf("%s\n",list);
    
}


void STIM_START(){
    STIM_TASK_STATUS = 1;
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_2);
    if(ENABLE_RECORD){
        configure_i2s();
        xTaskCreatePinnedToCore(recording, "recording", 1024*4, NULL, 6, NULL, 0);
    }
    if(RECORD_OFFSET < 0){
        vTaskDelay( -RECORD_OFFSET / portTICK_PERIOD_MS);
    }
    xTaskCreatePinnedToCore(biphasic_loop, "biphasic_loop", 4096, NULL, 2, &STIM_TASK, 1);
    printf("started!\n");
}

void STIM_STOP(){
    STIM_STATUS = 0;
    printf("stopped!\n");
}

void IRAM_ATTR biphasic_loop(void *params)//may need to change to fit elec team's circuit
{
    STIM_STATUS = 1;//mark as stimulation begin
    dac_output_voltage(DAC_CHANNEL_2, 127);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL1_REG, SENS_SW_TONE_EN);
    CLEAR_PERI_REG_MASK(SENS_SAR_DAC_CTRL2_REG, SENS_DAC_CW_EN1_M);
    /* while(1){
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 255, RTC_IO_PDAC1_DAC_S);
    }  */
    while(STIM_STATUS){
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 255, RTC_IO_PDAC1_DAC_S);
        ets_delay_us(PHASE_ONE_TIME);
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 127, RTC_IO_PDAC1_DAC_S);
        ets_delay_us(INTER_PHASE_GAP);
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 0, RTC_IO_PDAC1_DAC_S);
        ets_delay_us(PHASE_TWO_TIME);
        SET_PERI_REG_BITS(RTC_IO_PAD_DAC1_REG, RTC_IO_PDAC1_DAC, 127, RTC_IO_PDAC1_DAC_S);
        ets_delay_us(INTER_STIM_DELAY);
        //printf("stim\n");
    }
    dac_output_voltage(DAC_CHANNEL_1,127);//may need to change to fit elec team's circuit
    STIM_TASK_STATUS = 0;//mark as stimulation task finish
    vTaskDelete(NULL);
}

