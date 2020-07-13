/* 
 *      
 */
#include <Wire.h>
/* Reference : "../documents/GRID_EYE_SPECS.pdf"
 */
/* I2C Address */

#define AMG_I2C_ADDRESS 0x69

/*AMG8833 Raw Value to Degrees Conversion Constant*/
#define AMG8833_TEMP_CONVERSION_CONSTANT 0.25        

/*In arduino it works with 32 continuous 1 byte read from I2C bus*/
#define AMG_CONTINUOUS_READ_CHUNK 32 /* For some mcu it is 16*/

/* AMG8833 Register Address */
#define AMG8833_PIXEL_DATA_REGISTER_ADDRESS_START 0x80
#define MASK_EXTRACT_11_BITS_FROM_LSB   0x7FF
#define MASK_EXTRACT_ONLY_12TH_BIT      0x800
#define BIT_12_MASK      0x800
#define  AMG_PCTL 	0x00
#define  AMG_RST 	0x01
#define  AMG_FPSC  	0x02
#define  AMG_INTC  	0x03
#define  AMG_STAT  	0x04
#define  AMG_SCLR  	0x05
#define  AMG_AVE  	0x07
#define  AMG_INTHL  0x08
#define  AMG_INTHH  0x09
#define  AMG_INTLL  0x0A
#define  AMG_INTLH  0x0B
#define  AMG_IHYSL  0x0C
#define  AMG_IHYSH  0x0D
#define  AMG_TTHL  	0x0E
#define  AMG_TTHH  	0x0F
#define  AMG_INT_OFFSET  	0x010

/* AMG8833 Operation Modes */
#define AMG_NORMAL_MODE  		0x00
#define AMG_SLEEP_MODE  		0x01
#define AMG_STAND_BY_MODE_60  	0x20
#define AMG_STAND_BY_MODE_10  	0x21

#define AMG_FLAG_RESET			0x30
#define AMG_INITIAL_RESET		0x3F

/* Frames Per Second */
#define AMG_FPS_10 0x00 
#define AMG_FPS_1  0x01

void i2c_init()
{
    Wire.begin();   
}

void i2c_write16(uint16_t data)
{
    uint8_t msb = (data >> 8) & 0xFF;
    uint8_t lsb = data & 0xFF;
    Wire.write(msb);
    Wire.write(lsb);
}

void i2c_write8(uint8_t data)
{
    Wire.write(data);
}

uint8_t i2c_read8(uint8_t address)
{
    Wire.requestFrom(address, (uint8_t) 1);
    return Wire.read();
}


void AMG8833_INIT_REGISTERS()
{
    AMG8833_WRITE_BYTE(AMG_I2C_ADDRESS, AMG_PCTL, AMG_NORMAL_MODE);
    AMG8833_WRITE_BYTE(AMG_I2C_ADDRESS, AMG_RST, AMG_INITIAL_RESET);

    /* Disable Interrupt */
    AMG8833_WRITE_BYTE(AMG_I2C_ADDRESS, AMG_INTC, 0x0);

    /*Frame Rate 10 frames per second*/
    AMG8833_WRITE_BYTE(AMG_I2C_ADDRESS, AMG_FPSC, AMG_FPS_10);
    
    delay(250);

}

void AMG8833_WRITE_BYTE(uint8_t i2c_slave_address, uint8_t register_address, uint8_t data) 
{
    Wire.beginTransmission(i2c_slave_address);
    i2c_write8(register_address);
    i2c_write8(data);
    Wire.endTransmission();
}


/*  arr[0] = lsb
 *  arr[1] = msb
 */
uint16_t merge_lsb_n_msb(uint8_t lsb, uint8_t msb)
{
    uint16_t value = ( (uint16_t) msb << 8 ) | lsb ;
    return value;
}

void zero_buffer_double(double *buffer, int size)
{
    int i;
    
    if(!buffer){
        return;
    }

    for(i=0; i<size; i++){
        buffer[i] = 0x0;
    }
}


void zero_buffer8(uint8_t *buffer, int size)
{
    int i;
    
    if(!buffer){
        return;
    }

    for(i=0; i<size; i++){
        buffer[i] = 0x0;
    }
}


/* Extract valid 12 bits ( sensor value ) with sign - from 16 bit number
 */
float extract_12_bits(uint16_t val){
  int16_t  v1 = (val << 4);
  float  v2 = (v1 >> 4);
  return v2;
}

int16_t convert_sensor_value_from2s_comp(uint16_t abs_value)
{
    /* Bit 12 contains the sign bit*/
    uint16_t s = ( 1 << 12);
    uint16_t c = ~(s - 1);

    /* remove one signed bit and 4 unused bits at the msb*/
    c = (c << 5);
    c = (c >> 5);
  
    uint16_t o = c |  abs_value;
    
    /*Add - before converted absolute value*/
    int16_t value_in_decimal = 0 - ((int16_t) o);  
    return value_in_decimal;
}

int16_t extract_11_bits(uint16_t input12_bits)
{
    /* Extract 11 bits from input */
    uint16_t absolute_value = ( input12_bits & MASK_EXTRACT_11_BITS_FROM_LSB ); 
    int16_t value = 0;

   if(input12_bits & MASK_EXTRACT_ONLY_12TH_BIT){ 
        /* Negative Value*/
        value = convert_sensor_value_from2s_comp(absolute_value);
    }else{
        /* Positive Value*/
        value =  (int16_t)  (absolute_value);
    }

    return value;
}

double extract_raw_sensor_value(uint16_t sensor_reading)
{
    float  sensor_value_12_bits;
    //int16_t   sensor_value_11_bits;
    double sensor_value;

    sensor_value_12_bits = extract_12_bits(sensor_reading);
    //sensor_value_11_bits = extract_11_bits(sensor_value_12_bits);  
    sensor_value = (double) sensor_value_12_bits;

    return sensor_value;
}

double get_sensor_value_in_degrees(double sensor_value)
{
    double v = (double) AMG8833_TEMP_CONVERSION_CONSTANT * sensor_value;
    return v;
}

void display_binary8(uint8_t value)
{

    if(value & 128)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 64)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 32)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 16)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 8)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 4)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 2)
        Serial.print("1");
    else
        Serial.print("0");

    if(value & 1)
        Serial.print("1");
    else
        Serial.print("0");

}

void display_sensor_raw_value_1(int index, uint8_t *buffer, uint8_t register_address)
{     
          if(index%2 == 1){
            Serial.print("SENSOR_VALUE [ ");  
            Serial.print(register_address, HEX); Serial.print(" + ");  Serial.print(index);
            Serial.print(" ]  :  "); display_binary8(buffer[index]);
            Serial.print("  "); display_binary8(buffer[index-1]);Serial.println("");
          }
  
}

void AMG8833_READ_PIXELS(double *pixels)
{
    if(!pixels){
        return;
    }

    zero_buffer_double(pixels, 64);

    uint8_t buffer[128];
    zero_buffer8(buffer, 128);
    int i = 0;
    uint8_t register_address = (uint8_t ) AMG8833_PIXEL_DATA_REGISTER_ADDRESS_START;

    while(1){
        
        if(i >= 128){
            break;
        }


        Wire.beginTransmission((uint8_t)AMG_I2C_ADDRESS);
        i2c_write8(register_address);
        Wire.endTransmission();
        Wire.requestFrom((uint8_t)AMG_I2C_ADDRESS, (uint8_t)AMG_CONTINUOUS_READ_CHUNK);

        /*On arduino we need to read from AMG I2C as chunks  */
        for(int j=0; j<AMG_CONTINUOUS_READ_CHUNK; j++){   
          buffer[i] = Wire.read();
#ifdef DEBUG
          display_sensor_raw_value_1(i, buffer, register_address);
#endif
          register_address++;
          i++;
        }
        
     }

#ifdef DEBUG
    delay(1000);
#endif

    uint16_t  merged_value;
    uint16_t  valid_sensor_reading;
    double sensor_value;
    
    /*append msb and lsb*/
    for(i=0; i<64; i++){
        uint8_t lsb = buffer[(i*2)];
        uint8_t msb = buffer[(i*2)+1];
        merged_value = merge_lsb_n_msb(lsb, msb);
        /* 0xf = 1111
         * BIT_12_MASK = 0x800 [ settign only 12th bit]
         */
       
        valid_sensor_reading = ( BIT_12_MASK & merged_value )? ( merged_value | ( 0xf << 12 )) : merged_value; 
        if( BIT_12_MASK & merged_value ){
            /*BUG: Negative Use Case is not tested */
            sensor_value = (double) 0.0 - (double) (valid_sensor_reading) ;
        }else{
            sensor_value = (double) (valid_sensor_reading) ;
        }

        pixels[i] = get_sensor_value_in_degrees(sensor_value);   
    }
    
}

void setup() {
    delay(1000);
    Serial.begin(9600);   
    i2c_init();
    Serial.println("setup begin"); 
    delay(100);
    AMG8833_INIT_REGISTERS();
    delay(1000);
	  Serial.println("setup complete"); 
}

void display_single_pixel(double px){
    Serial.print("Pixel Value ( degree celcius ): ");
    Serial.print(px, 3);
    Serial.println("");
}

void loop() {
    double pixels[64];
    AMG8833_READ_PIXELS(pixels);
    display_single_pixel(pixels[40]);
    delay(100);
}
