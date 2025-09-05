#include <OneButton.h>
#include "net.h"
#include "common.h"
#include "PreferencesUtil.h"
#include "tftUtil.h"
#include "task.h"
#include <Arduino.h>
#include <WiFi.h>

/**
Dudu天气时钟  版本2.0

本次更新内容：

1.配网页面更新，增加了城市的上级行政区划，用于排除重名城市。
(之前输入普陀，可能是浙江舟山市普陀区，也可能是上海市普陀区，有了上级行政区划，就可以进行精确划分了)

2.更改了天气接口，由于易客天气现在只免费3个月，所以这个版本将天气接口改为了和风天气，功能更强大、接口更稳定且永久免费。

3.增加了空气质量页面，包含了pm10颗粒物，pm2.5颗粒物，no2二氧化氮，so2二氧化硫，co一氧化碳以及o3臭氧。
*/

unsigned int prevDisplay = 0; // 实况天气页面上次显示的时间
// unsigned int preTimerDisplay = 0; // 计数器页面上次显示的毫秒数/10,即10毫秒显示一次
// unsigned long startMillis = 0;    // 开始计数时的毫秒数
int synDataRestartTime = 60; // 同步NTP时间和天气信息时，超过多少秒就重启系统，防止网络不好时，傻等
// bool isCouting = false;           // 计时器是否正在工作
bool isBright = false;
OneButton myButton(BUTTON, true);
int prevMinute = -1;              // 初始化为 -1，表示尚未记录分钟
uint8_t defaultBrightness = 200;  // 默认亮度
uint8_t lowBrightness = 5;        // 夜间亮度
unsigned long lastActiveTime = 0; // 上次操作时间
bool isLowPower = false;          // 是否已进入低功耗
static bool wasLowPower = false;
bool ignoreNextButtonTick = false;

void setup()
{
  Serial.begin(115200);

  tftInit();
  // 显示系统启动文字
  // drawText("Hello Molly");
  // 设置默认亮度
  setBrightness(defaultBrightness);
  // 显示启动界面
  tft.pushImage(0, 0, 240, 320, KaiJi);

  delay(1000);
  lastActiveTime = millis();

  // //   // 测试的时候，先写入WiFi信息，省的配网，生产环境请注释掉
  // setInfo4Test();
  // 查询是否有配置过Wifi，没有->进入Wifi配置页面（0），有->进入天气时钟页面（1）
  getWiFiCity();
  // nvs中没有WiFi信息，进入配置页面
  if (ssid.length() == 0 || pass.length() == 0 || city.length() == 0)
  {
    currentPage = SETTING; // 将页面置为配置页面
    wifiConfigBySoftAP();  // 开启SoftAP配置WiFi
  }
  else
  {                        // 有WiFi信息，连接WiFi后进入时钟页面
    currentPage = WEATHER; // 将页面置为时钟页面
    // 连接WiFi,30秒超时重启并恢复出厂设置
    connectWiFi(30);
    // // 查询是否有城市id，如果没有，就利用city和adm查询出城市id，并保存为location
    // if (location.equals(""))
    // {
    //   getCityID();
    // }
    // 初始化一些列数据:NTP对时、实况天气、一周天气
    initDatas();
    // 绘制实况天气页面
    drawWeatherPage();
    // 多任务启动
    startRunner();
    // // 初始化定时器，让查询天气的多线程任务在一小时后再使能
    // startTimerQueryWeather();
    // 初始化按键监控
    myButton.attachClick(click);
    myButton.attachDoubleClick(doubleclick);
    myButton.attachLongPressStart(longclick);
    myButton.setPressMs(2000); // 设置长按时间
    // myButton.setClickMs(300); //设置单击时间
    myButton.setDebounceMs(10); // 设置消抖时长
  }
}
////////////////////////////// 添加功能 //////////////////////////////
// 自动根据时间调整亮度
void autoAdjustBrightnessByTime()
{
  time_t now;
  time(&now);
  struct tm *timeinfo = localtime(&now);
  if ((timeinfo->tm_hour == 0 && timeinfo->tm_min >= 5 && timeinfo->tm_min < 60) || (timeinfo->tm_hour > 0 && timeinfo->tm_hour < 6))
  {
    if (defaultBrightness != lowBrightness)
    {
      defaultBrightness = lowBrightness;
      setBrightness(defaultBrightness);
      Serial.println("切换夜间亮度");
    }
  }
  else
  {
    if (defaultBrightness != 200)
    {
      defaultBrightness = 200;
      setBrightness(defaultBrightness);
      Serial.println("切换白天亮度");
    }
  }
}
void drawTips(String text)
{
  if (currentPage == WEATHER)
  {
    clk.setColorDepth(8);
    clk.loadFont(cityandwea_18);
    clk.setTextDatum(CC_DATUM);
    clk.createSprite(216, 46);
    clk.setTextColor(TFT_BLACK);
    clk.fillSprite(TFT_WHITE);
    clk.drawString(text, 108, 23);
    clk.pushSprite(12, 97);
    clk.deleteSprite();
    clk.unloadFont();
  }
}

////////////////////////////// 添加串口调试指令//////////////////////////////
void loop()
{

  // 串口指令处理逻辑
  if (Serial.available() > 0)
  {
    lastActiveTime = millis();                     // 有串口输入，刷新活跃时间
    String command = Serial.readStringUntil('\n'); // 读取串口输入直到换行符
    command.trim();                                // 去除首尾空格或换行符

    if (command == "1")
    {
      Serial.println("调用 click()");
      click(); // 调用单击操作
    }
    else if (command == "2")
    {
      Serial.println("调用 doubleclick()");
      doubleclick(); // 调用双击操作
    }
    else if (command == "3")
    {
      Serial.println("调用 longclick()");
      longclick(); // 调用长按操作
    }

    else if (command.startsWith("sb"))
    {
      int value = command.substring(2).toInt();
      setBrightness((uint8_t)value);
      Serial.printf("已设置亮度为: %d\n", value);
    }

    else
    {
      Serial.println("未知指令: " + command);
    }
  }
  // 按键操作也算活跃
  if (digitalRead(BUTTON) == LOW)
  {
    lastActiveTime = millis();
  }
  // 低功耗判断
  if (!isLowPower && (millis() - lastActiveTime > 30000) && currentPage != SETTING) // 30s无操作且不在配网页面
  {
    isLowPower = true;
    // 关闭WiFi
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // 降低亮度
    setBrightness(lowBrightness);
    Serial.println("已进入低功耗模式");
    // 如需进入轻度睡眠，可加 esp_light_sleep_start();
  }

  // 如果有操作，恢复正常
  if (isLowPower && (millis() - lastActiveTime <= 30000))
  {
    isLowPower = false;
    // 恢复WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());
    // 恢复亮度
    setBrightness(defaultBrightness);
    // 只在天气、未来天气、空气质量页面绘制提示信息
    Serial.println("已恢复正常模式");
    wasLowPower = true; // 标记刚刚唤醒
  }
  else if (!isLowPower && wasLowPower)
  {
    // 唤醒后立即执行一次定时任务
    // 唤醒后先检查WiFi连接
    tCheckWiFiCallback();
    drawTips("//Check wifi");
    int retry = 0;
    const int maxRetry = 30; // 最多等待30次（约15秒）
    while (WiFi.status() != WL_CONNECTED && retry < maxRetry)
    {
      delay(500); // 每次等待0.5秒
      retry++;
      Serial.println("等待WiFi连接中...");
    }
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("WiFi已连接,开始执行任务");

      drawTips("//Update data");
      tQueryWeatherCallback();
      tQueryFutureWeatherCallback();
      tCheckTimeCallback();
      drawTips("");
    }
    else
    {
      drawTips("//Update failed");
      drawTips("");
      Serial.println("WiFi连接失败,跳过任务");
    }
    // 等待按键松开，防止误触发longclick
    while (digitalRead(BUTTON) == LOW)
    {
      delay(10);
    }
    ignoreNextButtonTick = true; // 唤醒后忽略第一次tick
    wasLowPower = false;
  }
  // 按键处理逻辑
  if (ignoreNextButtonTick)
  {
    // 跳过本次tick
    ignoreNextButtonTick = false;
  }
  else
  {
    myButton.tick();
  }

  // 页面逻辑
  switch (currentPage)
  {
  case SETTING:
    doClient();
    break;
  case WEATHER:
    executeRunner();
    time_t now;
    time(&now);
    if (now != prevDisplay)
    {
      prevDisplay = now;
      drawDateWeek();
    }
    autoAdjustBrightnessByTime(); // 自动调整亮度
    break;
  case FUTUREWEATHER:
    executeRunner();
    autoAdjustBrightnessByTime();
    break;
  case AIR:
    executeRunner();
    autoAdjustBrightnessByTime();
    break;
  case BClock:
  {
    executeRunner();
    time_t now_min;
    time(&now_min);
    struct tm *timeinfo = localtime(&now_min);
    if (timeinfo->tm_min != prevMinute)
    {
      prevMinute = timeinfo->tm_min;
      drawTime();
    }
    autoAdjustBrightnessByTime();
    break;
  }
  // case TIMER:
  //   if (isCouting && (millis() / 10) != preTimerDisplay)
  //   {
  //     preTimerDisplay = millis() / 10;
  //     drawNumsByCount(timerCount + millis() - startMillis);
  //   }
  //   autoAdjustBrightnessByTime();
  //   break;
  case RESET:
    executeRunner();
    autoAdjustBrightnessByTime();
    break;
  default:
    autoAdjustBrightnessByTime();
    break;
  }
}

////////////////////////// 按键区///////////////////////
// 单击操作，用来切换各个页面
// 修改fadeBrightness函数，支持从任意亮度到任意亮度
void fadeBrightness(uint8_t from, uint8_t to, int stepDelay = 5)
{
  if (from == to)
    return;
  if (from < to)
  {
    for (uint8_t i = from; i <= to; i++)
    {
      setBrightness(i);
      delay(stepDelay);
    }
  }
  else
  {
    for (int i = from; i >= to; i--)
    {
      setBrightness(i);
      delay(stepDelay);
    }
  }
}

void click()
{
  switch (currentPage)
  {
  case WEATHER:
    drawFutureWeatherPage();
    currentPage = FUTUREWEATHER;
    break;
  case FUTUREWEATHER:
    drawAirPage();
    currentPage = AIR;
    break;
  case AIR:
    fadeBrightness(defaultBrightness, 0, 1);
    drawBigClock();
    currentPage = BClock;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  case BClock:
    fadeBrightness(defaultBrightness, 0, 1);
    drawResetPage();
    currentPage = RESET;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  case RESET:
    fadeBrightness(defaultBrightness, 0, 1);
    drawWeatherPage();
    currentPage = WEATHER;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  default:
    break;
  }
}

void doubleclick()
{
  // 切换前慢慢变暗

  switch (currentPage)
  {
  case RESET:
    fadeBrightness(defaultBrightness, 0, 1);
    drawBigClock();
    currentPage = BClock;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  case BClock:
    fadeBrightness(defaultBrightness, 0, 1);
    drawAirPage();
    currentPage = AIR;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  case AIR:
    drawFutureWeatherPage();
    currentPage = FUTUREWEATHER;
    break;
  case FUTUREWEATHER:
    drawWeatherPage();
    currentPage = WEATHER;
    break;
  case WEATHER:
    fadeBrightness(defaultBrightness, 0, 1);
    drawResetPage();
    currentPage = RESET;
    fadeBrightness(0, defaultBrightness, 1);
    break;
  default:
    break;
  }
  // 切换后慢慢变亮
}
void longclick()
{
  if (currentPage == RESET)
  {
    Serial.println("恢复出厂设置");
    // 恢复出厂设置并重启
    clearWiFiCity();
    restartSystem("已恢复出厂设置", false);
  }
  // else if (currentPage == TIMER)
  // {
  //   Serial.println("计数器归零");
  //   timerCount = 0;              // 计数值归零
  //   isCouting = false;           // 计数器标志位置为false
  //   drawNumsByCount(timerCount); // 重新绘制计数区域，提示区域不用变
  // }
  else
  {
    // 只有不在RESET和TIMER页面时才切换亮度
    isBright = !isBright;
    if (isBright)
    {
      lowBrightness = 0;
    }
    else
    {
      lowBrightness = 5;
    }
    setBrightness(isBright ? lowBrightness : defaultBrightness);
    Serial.printf("长按切换亮度为: %d\n", isBright ? lowBrightness : defaultBrightness);
  }
}
////////////////////////////////////////////////////////

// 初始化一些列数据:NTP对时、实况天气、一周天气
void initDatas()
{
  startTimerShowTips(); // 获取数据时，循环显示提示文字
  // 记录此时的时间，在同步数据时，超过一定的时间，就直接重启
  time_t start;
  time(&start);
  // 获取NTP并同步至RTC,第一次同步失败，就一直尝试同步
  getNTPTime();
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo))
  {
    time_t end;
    time(&end);
    if ((end - start) > synDataRestartTime)
    {
      restartSystem("同步数据失败", true);
    }
    Serial.println("时钟对时失败...");
    getNTPTime();
  }
  Serial.println("对时成功");
  char timeStr[64];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  Serial.print("当前时间：");
  Serial.println(timeStr);
  // @Ltest
  // 查询是否有城市id，如果没有，就利用city和adm查询出城市id，并保存为location
  if (location.equals("") || lat.equals("") || lon.equals(""))
  {
    getCityID();
  }
  // 第一次查询实况天气,如果查询失败，就一直反复查询
  getNowWeather();
  while (!queryNowWeatherSuccess)
  {
    time_t end;
    time(&end);
    if ((end - start) > synDataRestartTime)
    {
      restartSystem("同步数据失败", true);
    }
    getNowWeather();
  }
  // 第一次查询空气质量,如果查询失败，就一直反复查询
  getAir();
  while (!queryAirSuccess)
  {
    time_t end;
    time(&end);
    if ((end - start) > synDataRestartTime)
    {
      restartSystem("同步数据失败", true);
    }
    getAir();
  }
  // 第一次查询一周天气,如果查询失败，就一直反复查询
  getFutureWeather();
  while (!queryFutureWeatherSuccess)
  {
    time_t end;
    time(&end);
    if ((end - start) > synDataRestartTime)
    {
      restartSystem("同步数据失败", true);
    }
    getFutureWeather();
  }
  // 结束循环显示提示文字的定时器
  timerEnd(timerShowTips);
  // 将isStartQuery置为false,告诉系统，启动时查询天气已完成
  isStartQuery = false;
}
