
#include <IO_init.h>

/*
// TFT_eSPI tft = TFT_eSPI(); // Invoke custom library
// #define TFT_SCLK 2  // Clock out
// #define TFT_MOSI 3  // Data out
// #define TFT_RST  10
// #define TFT_DC   6
// #define TFT_CS   7
//______TFT_BL   不接



#define TFT_MOSI 11 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 12
#define TFT_CS 10  // Chip select control pin
#define TFT_DC 9   // Data Command control pin
#define TFT_RST 14 // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL 21  // LED back-light control pin

//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCLK, TFT_RST);
*/

#define sht30_en false

// MotorMCPWMConfig hw{Motor_H_pin, Motor_L_pin, -1, MCPWM_UNIT_1, MCPWM_TIMER_0, MCPWM0A, MCPWM0B};
// Motor motor;

#define test_pin 3
#define test_pin2 45
#define test_channel 5
//PWM_Analyzer Buf_pwm(Bufio_pin, 1);
// PWM_Analyzer Buf_pwm(test_pin2, 1);
// float buf_voltage = 0.0;
float Buf_pwm_read()
{
  //float duty_cycle = Buf_pwm.Get_PWM_duty_cycle();
  float duty_cycle = -1;
  if (duty_cycle == -1)
  {
    duty_cycle = 60.0;
    //Buf_pwm.Restart();
  }
  return duty_cycle;
}
uint32_t Buf_pwm_frequency()
{
  //uint32_t frequency = Buf_pwm.Get_PWM_frequency();
  return 0;
}

void ADC_init()
{
  analogReadResolution(12);       // 设置ADC分辨率为12位
  analogSetAttenuation(ADC_11db); // 设置衰减为11dB，适用于0-3.3V范围
  adcAttachPin(Pull_pin);
  adcAttachPin(Online_pin);
  //Buf_pwm.Restart();
}
std::pair<float, float> ADC_read()
{
  // 定义ADC参考电压和最大值常量
  const float REFERENCE_VOLTAGE = 3.3;
  const float ADC_MAX_VALUE = 4095.0;

  // 读取拉力传感器ADC值
  int pull_adc_value = analogRead(Pull_pin);
  // 将ADC值转换为电压值
  float pull_voltage = (pull_adc_value / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;

  // 读取在线状态传感器ADC值
  int online_adc_value = analogRead(Online_pin);
  // 将ADC值转换为电压值
  float online_voltage = (online_adc_value / ADC_MAX_VALUE) * REFERENCE_VOLTAGE;

  return {pull_voltage, online_voltage};

  // pinMode(Bufio_pin, INPUT);
  // buf_voltage = digitalRead(Bufio_pin) ? REFERENCE_VOLTAGE : 0.0;

  // 根据具体需求处理电压值
  // my_printf("ADC Voltage: %.2f V\n", pull_voltage);
}

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
  if (!sht30_en)
    return;
  Wire.begin(SDA_PIN, SCL_PIN, 100000);
  Wire.beginTransmission(Addr_SHT30);
  Wire.write(0x2C);
  Wire.write(0x06);
  Wire.endTransmission();
  sht30_state = Sht30State::WAITING_FOR_DATA;
}

std::pair<float, float> Sht30_read()
{
  if (!sht30_en)
    return {0, 0};
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
      Temp = ((((sht30_data[0] * 256.0) + sht30_data[1]) * 175.0) / 65535.0) - 45;
      Humidity = ((((sht30_data[3] * 256.0) + sht30_data[4]) * 100.0) / 65535.0);
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
/*

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

void tft_print(bool flag)
{
  if (flag)
  {
    tft.fillScreen(ST7735_CYAN);
    tft.setTextSize(2);
    tft.setTextColor(ST7735_RED);
    tft.setCursor(12, 60);
    tft.println("Bmcu-hub");
    tft.setCursor(2, 82);
    tft.println("catch mode");
  }
  else
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
}
*/

bool Temp_read(int temp1)
{
  if (temp1 < int(Temp))
  {
    return true;
  }
  else
  {
    return false;
  }
}
void Set_fan_t(int temp1)
{
  if (!sht30_en)
  {
    pinMode(FAN_PIN, OUTPUT);
    Set_fan(true);
  }
  else if (temp1 - 5 > int(Temp))
  {
    digitalWrite(FAN_PIN, LOW);
    // analogWrite(FAN_PIN, 0);
  }
  else if (temp1 < int(Temp))
  {
    int dutyCycle = 0;
    dutyCycle = (int(Temp) * 2) + 100;
    // analogWrite(FAN_PIN, dutyCycle);
    ledcWrite(FAN_channel, dutyCycle);
    // digitalWrite(FAN_PIN, HIGH);
  }
}

bool ENable24 = true;

bool enable_24()
{
  return ENable24;
}
void Set_24(bool enable)
{
  ENable24 = enable;
  if (enable)
  {
    digitalWrite(EN_24, HIGH);
  }
  else
  {
    digitalWrite(EN_24, LOW);
  }
}
void Set_fan(bool enable)
{
  if (enable)
  {
    digitalWrite(FAN_PIN, HIGH);
  }
  else
  {
    digitalWrite(FAN_PIN, LOW);
  }
}
void IO_init()
{
  pinMode(EN_24, OUTPUT);
  ENable24 = true;
  digitalWrite(EN_24, HIGH);
  if (!sht30_en)
  {
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
  }
  else
  {
    ledcSetup(FAN_channel, 4000, 8);     // 4kHz, 8-bit
    ledcAttachPin(FAN_PIN, FAN_channel); // 将FAN_PIN引脚连接到通道7
    ledcWrite(FAN_channel, 0);           // 初始占空比为 0
  }

  ADC_init();
}

uint8_t sw_read()
{
  uint8_t sw = 0;
  /*
  if (digitalRead(ONline_1) == LOW)
  {
    sw |= 0x01;
  }
  if (digitalRead(ONline_2) == LOW)
  {
    sw |= 0x02;
  }
  if (digitalRead(ONline_3) == LOW)
  {
    sw |= 0x04;
  }
  if (digitalRead(ONline_4) == LOW)
  {
    sw |= 0x08;
  }
  */

  return sw;
}