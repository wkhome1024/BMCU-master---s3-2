
#include <sht30.h>

//TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
//#define TFT_SCLK 2  // Clock out  
//#define TFT_MOSI 3  // Data out
//#define TFT_RST  10      
//#define TFT_DC   6     
//#define TFT_CS   7
//______TFT_BL   不接 
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 12
#define TFT_CS   10  // Chip select control pin
#define TFT_DC   9  // Data Command control pin
#define TFT_RST  14  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   21  // LED back-light control pin

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);  


float Temp = 0;
float Humidity = 0;
bool SHT30_flag = true;

void Sht30_init()
{
    Wire.begin(SDA_PIN, SCL_PIN, 100000);
    Wire.beginTransmission(Addr_SHT30);
    Wire.write(0x2C);
    Wire.write(0x06);
    Wire.endTransmission();
    SHT30_flag = true;
}

std::pair<float, float> Sht30_read()
{
  unsigned int data[6];
  if (SHT30_flag)
  {
    Wire.requestFrom(Addr_SHT30, 6);
    if (Wire.available() == 6)
    {
      data[0] = Wire.read();
      data[1] = Wire.read();
      data[2] = Wire.read();
      data[3] = Wire.read();
      data[4] = Wire.read();
      data[5] = Wire.read();
    }

    Wire.beginTransmission(Addr_SHT30);
    Wire.write(0x2C);
    Wire.write(0x06);
    Wire.endTransmission();
    SHT30_flag = true;
  }
  else
  {
    SHT30_flag = true;
    Wire.beginTransmission(Addr_SHT30);
    Wire.write(0x2C);
    Wire.write(0x06);
    Wire.endTransmission();
    return {Temp, Humidity};
  } 
   

  Temp = ((((data[0] * 256.0) + data[1]) * 175.0) / 65535.0) - 55;   //修正 -10
  Humidity = ((((data[3] * 256.0) + data[4]) * 100.0) / 65535.0) + 10;  //修正 +10

  if (data[0] == 0xFF && data[1] == 0xFF)
  {
    Temp = 0;
    Humidity = 0;
  }

  return {Temp, Humidity};

}

String Sht30_read_mqtt()
{
  char tempBuf[10];
  char HumiBuf[10];
  auto data = Sht30_read();
  sprintf(tempBuf,"%4.1f", data.first);
  sprintf(HumiBuf,"%4.1f", data.second);
  String json = ("{\"Temp\":\"" + (String)tempBuf + "\",\"Humidity\":\"" + (String)HumiBuf + "\"}");
  return json;
}

String Sht30_tft()
{
  char tempBuf[10];
  char HumiBuf[10];
  sprintf(tempBuf,"%4.1f", Temp);
  sprintf(HumiBuf,"%4.1f", Humidity);
  return "Temp: " + String(tempBuf) + "Humi: " + String(HumiBuf);
}


void tft_init()
{
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.initR(INITR_GREENTAB); 
  tft.setRotation(0);
  tft.setTextColor(ST7735_RED);
  tft.setTextSize(2);
  tft.fillScreen(ST7735_CYAN);
  tft.setCursor(0, 32);
  tft.println("Hello,I'm Bmcu-hub");
}


void tft_print()
{
    tft.setCursor(0, 32);
    tft.fillScreen(ST7735_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(get_tay_color(0));
    tft.println(get_tay_map(0));
    tft.setCursor(0, 52);
    tft.setTextColor(get_tay_color(1));
    tft.println(get_tay_map(1));
    tft.setCursor(0, 72);
    tft.setTextColor(get_tay_color(2));
    tft.println(get_tay_map(2));
    tft.setCursor(0, 92);
    tft.setTextColor(get_tay_color(3));
    tft.println(get_tay_map(3)); 
    tft.setCursor(0, 122);
    tft.setTextColor(ST7735_BLACK);
    tft.println(Sht30_tft());
}