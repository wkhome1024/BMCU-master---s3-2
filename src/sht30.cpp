
#include <sht30.h>

// TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
// #define TFT_SCLK 2  // Clock out
// #define TFT_MOSI 3  // Data out
// #define TFT_RST  10
// #define TFT_DC   6
// #define TFT_CS   7
//______TFT_BL   不接
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 12
#define TFT_CS 10  // Chip select control pin
#define TFT_DC 9   // Data Command control pin
#define TFT_RST 14 // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL 21  // LED back-light control pin

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);

enum class Sht30State
{
  IDLE,
  REQUESTING_MEASUREMENT,
  WAITING_FOR_DATA,
  PROCESSING_DATA
};
Sht30State sht30_state = Sht30State::IDLE;

float Temp = 0;
float Humidity = 0;


void Sht30_init()
{
  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.beginTransmission(Addr_SHT30);
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();
  sht30_state = Sht30State::WAITING_FOR_DATA;
}

std::pair<float, float> Sht30_read()
{
  static uint8_t sht30_data[6];
  switch (sht30_state)
  {
  case Sht30State::IDLE:
    // 发送测量命令
    Wire.beginTransmission(Addr_SHT30);
    Wire.write(0x2C);
    Wire.write(0x06);
    Wire.endTransmission();
    sht30_state = Sht30State::WAITING_FOR_DATA;
    break;

  case Sht30State::WAITING_FOR_DATA:
    Wire.requestFrom(Addr_SHT30, 6);
    if (Wire.available() == 6)
    {
      for (int i = 0; i < 6; ++i)
      {
        sht30_data[i] = Wire.read();
      }
      sht30_state = Sht30State::PROCESSING_DATA;
    }
    break;

  case Sht30State::PROCESSING_DATA:
    if (sht30_data[0] == 0xFF && sht30_data[1] == 0xFF)
    {
      Temp = 0;
      Humidity = 0;
    }
    else
    {
      Temp = ((((sht30_data[0] * 256.0) + sht30_data[1]) * 175.0) / 65535.0) - 55;
      Humidity = ((((sht30_data[3] * 256.0) + sht30_data[4]) * 100.0) / 65535.0) + 10;
    }
    sht30_state = Sht30State::IDLE; // 下次再循环
    break;
  }

  return {Temp, Humidity};
}

String Sht30_read_mqtt()
{
  auto data = Sht30_read();
  String json = "{\"Temp\":\"" + String(data.first, 1) + "\",\"Humidity\":\"" + String(data.second, 1) + "\"}";
  return json;
}

String Sht30_tft()
{
  char tempBuf[10];
  char HumiBuf[10];
  sprintf(tempBuf, "%4.1f", Temp);
  sprintf(HumiBuf, "%4.1f", Humidity);
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