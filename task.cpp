#include <TaskScheduler.h>
#include "tftUtil.h"
#include "net.h"
#include "common.h"

enum CurrentPage currentPage = SETTING;
// String scrollText[5];        // 轮播天气的信息
int currentIndex = 0;        // 轮播索引
int currnetImgAnimIndex = 0; // 太空人动画索引
int tipsIndex = 0;           // 获取数据时，动态文字的索引
NowWeather nowWeather;       // 记录查询到的实况天气数据
FutureWeather futureWeather; // 记录查询到的七日天气数据
// unsigned int timerCount = 0; // 计数器的值(ms)
extern bool isLowPower; // 低功耗模式

void drawWeatherContent();
void drawFutureWeatherPage();
// void doScrollTextData(String win, int air, String text, String feelsLike, String vis);
void drawWeaImage(int icon);
void drawWeaBackground(int icon);
void drawFWeaImage(int icon, int temperature);
void drawFutureWeaImage(int wea_img, int x, int y);
void drawCityAir(String city, int air);
void drawTHProgressBar(int temperature, int humidity, String vis, String win);
void drawDateWeek();
void drawZhuYe();
// void drawNumsByCount(unsigned int count);
String getWea(int icon);
String week(int tm_wday);
String monthDay(int tm_mon, int tm_mday);
void doHourMinuteSecond(int hour, int minute, int second, int *hourH, int *hourL, int *minuteH, int *minuteL, int *secondH, int *secondL);
void drawTime();

//////////////////////////////// 多任务区域/////////////////////////////
// void tAnimCallback();
// void tScrollTextCallback();
void tQueryWeatherCallback();
void tQueryFutureWeatherCallback();
void tCheckWiFiCallback();
void tCheckTimeCallback();
Scheduler runner;
// Task tAnim(30, TASK_FOREVER, &tAnimCallback, &runner, true);                                          // 30毫秒播放一帧太空人动画
// Task tScrollText(5000, TASK_FOREVER, &tScrollTextCallback, &runner, true);                            // 5秒轮播一条天气情况
// Task tQueryWeather(60 * 60 * 1000, TASK_FOREVER, &tQueryWeatherCallback, &runner, false);             // 60分钟查询一次实况天气情况
// Task tQueryFutureWeather(71 * 60 * 1000, TASK_FOREVER, &tQueryFutureWeatherCallback, &runner, false); // 71分钟查询一次一周天气情况
// Task tCheckWiFi(5 * 60 * 1000, TASK_FOREVER, &tCheckWiFiCallback, &runner, true);                     // 5分钟检查一次网络状态
// Task tCheckTime(58 * 60 * 1000, TASK_FOREVER, &tCheckTimeCallback, &runner, true);                    // 58分钟进行一次NTP对时
// 启动runner
void startRunner()
{
  runner.startNow();
}
// 执行runner
void executeRunner()
{
  runner.execute();
}

////////////////////////////////////////////////////////////////////////

//////////////// 定时器相关区域//////////////////////////////////////////
// 定义定时器
hw_timer_t *timerShowTips = NULL;
// hw_timer_t *timerQueryWeather = NULL;

// void IRAM_ATTR onTimerQueryWeather()
// {
//   // 使能查询天气的多线程任务
//   tQueryWeather.enable();
//   tQueryFutureWeather.enable();
// }

void IRAM_ATTR onTimerShowTips()
{
  // @L
  clk.createSprite(240, 180);
  clk.fillSprite(TFT_BLACK);
  clk.loadFont(city_wea_24);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREEN);
  clk.drawString("//synchronous weather", 120, 75);
  clk.pushSprite(0, 70);
  clk.deleteSprite();
  clk.unloadFont();
}

// 初始化定时器，让查询天气的多线程任务在一小时后再使能
// void startTimerQueryWeather()
// {
//   timerQueryWeather = timerBegin(0, 80, true); // 1us计数一次
//   timerAttachInterrupt(timerQueryWeather, &onTimerQueryWeather, true);
//   timerAlarmWrite(timerQueryWeather, 3600000000, false); // 执行完一次后就取消这个定时器 (3600000000)3600秒，即60分钟后使能
//   timerAlarmEnable(timerQueryWeather);
//

// 初始化定时器，在获取初始数据时，给用户提示
void startTimerShowTips()
{
  timerShowTips = timerBegin(1, 80, true); // 1us计数一次
  timerAttachInterrupt(timerShowTips, &onTimerShowTips, true);
  timerAlarmWrite(timerShowTips, 1000000, true); // 每秒执行一次
  timerAlarmEnable(timerShowTips);
}
///////////////////////////////////////////////////////////////////////

/////////// 绘制整个页面区域////////////////////////////////////////////

// 重写的 drawZhuYe 函数
void drawZhuYe()
{
  // L@主页上部
  tft.pushImage(0, 134, 240, 186, ZhuYe2);
}
// 绘制时间、日期、星期
void drawDateWeek()
{
  // 获取RTC时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("获取RTC时间失败");
    return;
  }

  // 解析时分秒
  int hourH, hourL, minuteH, minuteL, secondH, secondL;
  doHourMinuteSecond(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                     &hourH, &hourL, &minuteH, &minuteL, &secondH, &secondL);

  // 时、分 时间L@
  clk.setColorDepth(8);
  clk.createSprite(50, 42);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(time_52);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(hourH, 14, 22);
  clk.drawNumber(hourL, 39, 22);
  clk.unloadFont();
  clk.pushSprite(29, 102);
  clk.deleteSprite();

  // clk.drawString(":", 80, 32);
  clk.createSprite(50, 42);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(time_52);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(0x8C31);
  clk.drawNumber(minuteH, 14, 22);
  clk.drawNumber(minuteL, 39, 22);
  clk.unloadFont();
  clk.pushSprite(95, 102);
  clk.deleteSprite();
  // 秒
  clk.createSprite(50, 42);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(time_52);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(secondH, 14, 22);
  clk.drawNumber(secondL, 39, 22);
  clk.unloadFont();
  clk.pushSprite(158, 102);
  clk.deleteSprite();

  // 底部日期、星期
  clk.loadFont(cityandwea_18);

  // 星期L@
  clk.createSprite(54, 17);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(CR_DATUM);
  clk.drawString(week(timeinfo.tm_wday), 53, 10);
  clk.pushSprite(158, 148);
  clk.deleteSprite();

  // 月日
  clk.createSprite(54, 17);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(CC_DATUM);
  clk.drawString(monthDay(timeinfo.tm_mon, timeinfo.tm_mday), 27, 10);

  clk.pushSprite(95, 148);
  clk.deleteSprite();

  clk.unloadFont();
}

// 绘制一周天气页面
void drawFutureWeatherPage()
{
  drawZhuYe();
  drawCityAir(city, nowWeather.air);
  drawWeaBackground(nowWeather.icon);
  // 绘制标题
  clk.createSprite(226, 141);
  clk.fillSprite(0xef7d);
  clk.fillRoundRect(0, 0, 226, 141, 8, TFT_WHITE);
  clk.pushSprite(7, 171);
  clk.deleteSprite();

  // // 绘制天气图案
  drawFutureWeaImage(futureWeather.day1wea_img, 27, 200);
  drawFutureWeaImage(futureWeather.day2wea_img, 103, 200);
  drawFutureWeaImage(futureWeather.day3wea_img, 179, 200);
  drawFutureWeaImage(futureWeather.day4wea_img, 27, 270);
  drawFutureWeaImage(futureWeather.day5wea_img, 103, 270);
  drawFutureWeaImage(futureWeather.day6wea_img, 179, 270);
  // // 绘制上部分文字
  clk.loadFont(futureweather_14);
  tft.loadFont(futureweather_14);
  clk.setTextDatum(TC_DATUM);
  clk.createSprite(215, 25);
  clk.setTextColor(0x8c92);
  clk.fillSprite(TFT_WHITE);
  // day1
  clk.drawString(futureWeather.day1wea, 32, 1);
  clk.drawString(String(futureWeather.day1tem_night) + "-" + String(futureWeather.day1tem_day) + "℃", 32, 15);
  // day2
  clk.drawString(futureWeather.day2wea, 108, 1);
  clk.drawString(String(futureWeather.day2tem_night) + "-" + String(futureWeather.day2tem_day) + "℃", 108, 15);
  // day3
  clk.drawString(futureWeather.day3wea, 184, 1);
  clk.drawString(String(futureWeather.day3tem_night) + "-" + String(futureWeather.day3tem_day) + "℃", 184, 15);

  clk.pushSprite(13, 175);
  clk.deleteSprite();
  // 绘制上部日期
  // clk.createSprite(215, 10);
  // clk.fillSprite(TFT_WHITE);
  tft.setTextColor(0x8c92, TFT_WHITE);
  tft.drawString(futureWeather.day1date.substring(5, 7), 10, 205);
  tft.drawString(futureWeather.day1date.substring(8, 10), 10, 219);
  tft.drawString(futureWeather.day2date.substring(5, 7), 87, 205);
  tft.drawString(futureWeather.day2date.substring(8, 10), 87, 219);
  tft.drawString(futureWeather.day3date.substring(5, 7), 163, 205);
  tft.drawString(futureWeather.day3date.substring(8, 10), 163, 219);
  // clk.pushSprite(13, 236);
  // clk.deleteSprite();
  // 绘制下部分文字
  clk.createSprite(215, 25);
  clk.setTextDatum(TC_DATUM);
  clk.setTextColor(0x8c92);
  clk.fillSprite(TFT_WHITE);
  // day4
  clk.drawString(futureWeather.day4wea, 32, 1);
  clk.drawString(String(futureWeather.day4tem_night) + "-" + String(futureWeather.day4tem_day) + "℃", 32, 15);
  // day5
  clk.drawString(futureWeather.day5wea, 108, 1);
  clk.drawString(String(futureWeather.day5tem_night) + "-" + String(futureWeather.day5tem_day) + "℃", 108, 15);
  // day6
  clk.drawString(futureWeather.day6wea, 184, 1);
  clk.drawString(String(futureWeather.day6tem_night) + "-" + String(futureWeather.day6tem_day) + "℃", 184, 15);

  clk.pushSprite(13, 246);
  clk.deleteSprite();

  // 绘制下部日期
  tft.drawString(futureWeather.day4date.substring(5, 7), 10, 279);
  tft.drawString(futureWeather.day4date.substring(8, 10), 10, 293);
  tft.drawString(futureWeather.day5date.substring(5, 7), 87, 279);
  tft.drawString(futureWeather.day5date.substring(8, 10), 87, 293);
  tft.drawString(futureWeather.day6date.substring(5, 7), 163, 279);
  tft.drawString(futureWeather.day6date.substring(8, 10), 163, 293);

  clk.unloadFont();
  tft.unloadFont();
  drawFWeaImage(nowWeather.icon, nowWeather.temp);
}
// 绘制空气质量页面
void drawAirPage()
{
  // 屏幕上部
  drawZhuYe();
  drawCityAir(city, nowWeather.air);
  drawWeaBackground(nowWeather.icon);
  drawFWeaImage(nowWeather.icon, nowWeather.temp);

  // 绘制背景框
  clk.createSprite(226, 141);
  clk.fillSprite(0xef7d);
  clk.fillRoundRect(0, 0, 226, 141, 8, TFT_WHITE);
  clk.pushSprite(7, 171);
  clk.deleteSprite();

  // 绘制空气质量文字
  // 第一排
  clk.loadFont(cityandwea_18);
  clk.setTextDatum(TL_DATUM);
  clk.createSprite(215, 25);
  clk.setTextColor(0x8c92);
  clk.fillSprite(TFT_WHITE);
  clk.drawString("PM10", 14, 1);
  clk.drawString("PM2.5", 90, 1);
  clk.drawString("NO2", 166, 1);
  clk.pushSprite(13, 175);
  clk.deleteSprite();
  // 第二排
  clk.createSprite(215, 25);
  clk.fillSprite(TFT_WHITE);
  clk.drawString("SO2", 14, 1);
  clk.drawString("CO", 90, 1);
  clk.drawString("O3", 166, 1);
  clk.pushSprite(13, 246);
  clk.deleteSprite();
  clk.unloadFont();

  // 绘制空气质量数据
  // 第一排
  clk.loadFont(city_wea_24);
  clk.createSprite(215, 40);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(TC_DATUM);
  clk.drawString(nowWeather.pm10, 32, 8);
  clk.drawString(nowWeather.pm2p5, 108, 8);
  clk.drawString(nowWeather.no2, 184, 8);
  clk.pushSprite(13, 200);
  clk.deleteSprite();
  // 第二排
  clk.createSprite(215, 40);
  clk.fillSprite(TFT_WHITE);
  clk.drawString(nowWeather.so2, 32, 8);
  clk.drawString(nowWeather.co, 108, 8);
  clk.drawString(nowWeather.o3, 184, 8);
  clk.pushSprite(13, 271);
  clk.deleteSprite();
  clk.unloadFont();
}

// 绘制实况天气页面
void drawWeatherPage()
{
  // // 清空屏幕
  // reflashTFT();

  // 绘制天气相关内容
  drawWeatherContent();
}
// 绘制计时器页面
// void drawTimerPage()
// {
//   // 清空屏幕
//   reflashTFT();
//   tft.pushImage(0, 0, 240, 320, KaiJi);

//   // 显示提示文字
//   // tft.setTextDatum(CC_DATUM);
//   // tft.loadFont(city_wea_24);
//   // tft.setTextColor(TFT_BLACK);
//   // tft.drawString("TIMER", 120, 40);
//   // tft.drawString("Click to start/stop the timing", 120, 250);
//   // tft.drawString("Long press to reset", 120, 280);
//   // tft.unloadFont();
//   // 根据计数器当前的数值进行绘制
//   drawNumsByCount(timerCount);
// }
// 绘制出厂设置页面
void drawResetPage()
{
  // 清空屏幕
  reflashTFT();
  tft.pushImage(0, 0, 240, 320, KaiJi);
  // 绘制标题
  clk.createSprite(240, 180);
  clk.fillSprite(TFT_BLACK);
  clk.loadFont(city_wea_24);
  clk.setTextDatum(CC_DATUM);
  clk.setTextColor(TFT_GREEN);
  clk.drawString("RESET", 120, 75);
  clk.drawString("Long press to reset", 120, 105);
  clk.pushSprite(0, 70);
  clk.deleteSprite();
  clk.unloadFont();
}
// 绘制大号时钟页面
void drawBigClock()
{
  // // 清空屏幕
  // reflashTFT();
  tft.pushImage(0, 0, 240, 320, JY);
  drawTime();
}
// 绘制时间
void drawTime()
{ // 获取RTC时间
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 0))
  {
    Serial.println("获取本地时间失败");
    return;
  }
  // 解析时分秒
  int hourH, hourL, minuteH, minuteL, secondH, secondL;
  doHourMinuteSecond(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec,
                     &hourH, &hourL, &minuteH, &minuteL, &secondH, &secondL);
  // 绘制时分
  clk.setColorDepth(8);
  clk.createSprite(45, 75);
  clk.setTextColor(TFT_BLACK);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(clock_num_big_64);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(hourH, 23, 40);
  clk.pushSprite(60, 53);
  clk.deleteSprite();
  clk.unloadFont();

  clk.createSprite(45, 75);
  clk.setTextColor(TFT_BLACK);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(clock_num_big_64);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(hourL, 23, 40);
  clk.pushSprite(136, 53);
  clk.deleteSprite();
  clk.unloadFont();

  clk.createSprite(45, 75);
  clk.setTextColor(TFT_BLACK);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(clock_num_big_64);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(minuteH, 23, 40);
  clk.pushSprite(60, 198);
  clk.deleteSprite();
  clk.unloadFont();

  clk.createSprite(45, 75);
  clk.setTextColor(TFT_BLACK);
  clk.fillSprite(TFT_WHITE);
  clk.loadFont(clock_num_big_64);
  clk.setTextDatum(CC_DATUM);
  clk.drawNumber(minuteL, 23, 40);
  clk.pushSprite(136, 198);
  clk.deleteSprite();
  clk.unloadFont();
}

/////////////////////////////////////////////////////////////////////

/////////// 多任务协程回调区域 ////////////////////////////////////////
// NTP对时
void tCheckTimeCallback()
{
  if (isLowPower)
  {
    Serial.println("低功耗模式,跳过NTP对时");
    return;
  }
  getNTPTime();
}
// 检查Wifi状态,如果失败，重新连接
void tCheckWiFiCallback()
{
  if (isLowPower)
  {
    Serial.println("低功耗模式,跳过WiFi检查");
    return;
  }
  Serial.println("开始检查网络状态");
  checkWiFiStatus();
}

// 查询天气和空气质量
void tQueryWeatherCallback()
{
  if (isLowPower)
  {
    Serial.println("低功耗模式,跳过实况天气查询");
    return;
  }
  getNowWeather();
  getAir();
  if (queryNowWeatherSuccess || queryAirSuccess)
  {
    Serial.println("更新实时天气信息");
    // 如果是在实况天气页面，则绘制天气相关内容
    if (currentPage == WEATHER)
    {
      drawWeatherContent();
    }
  }
}
// 查询未来天气
void tQueryFutureWeatherCallback()
{
  if (isLowPower)
  {
    Serial.println("低功耗模式，跳过一周天气查询");
    return;
  }
  getFutureWeather();
  if (queryFutureWeatherSuccess)
  { // 查询一周天气成功，再继续下面的操作
    Serial.println("更新一周天气信息");
    // 如果是在一周天气页面，则绘制一周天气相关内容
    if (currentPage == FUTUREWEATHER)
    {
      drawFutureWeatherPage();
    }
  }
}
//////////////////////// 计时器  /////////////////////////////////////////////

// 根据计数器当前的数值进行绘制
// void drawNumsByCount(unsigned int count)
// {
//   clk.setColorDepth(8);
//   clk.setTextDatum(CC_DATUM);
//   clk.createSprite(240, 180);
//   clk.fillSprite(TFT_BLACK);
//   // 根据 count 计数器的毫秒值，计算对应的小时、分、秒、毫秒
//   int hour = count / 1000 / 60 / 60;
//   int minute = (count / 1000 / 60) % 60;
//   int second = (count / 1000) % 60;
//   int millisecond = count % 1000;
//   clk.loadFont(clock_num_big_64);
//   clk.setTextColor(0x03bf);
//   clk.drawString(String(hour) + "H", 120, 50);
//   clk.unloadFont();
//   clk.loadFont(clock_num_big_64);
//   clk.setTextColor(0x07ea);
//   clk.drawString(String(minute / 10), 20, 120);
//   clk.drawString(String(minute % 10), 55, 120);
//   // clk.drawString(":", 80, 120);
//   clk.setTextColor(0x07ea);
//   clk.drawString(String(second / 10), 105, 120);
//   clk.drawString(String(second % 10), 140, 120);
//   clk.unloadFont();
//   clk.loadFont(clock_num_big_64);
//   clk.setTextColor(0x07ea);
//   clk.drawString(String((millisecond / 10) / 10), 185, 120);
//   clk.drawString(String((millisecond / 10) % 10), 220, 120);
//   clk.unloadFont();
//   // Serial.print(String(millisecond));Serial.println("毫秒");
//   // Serial.print(String(second));Serial.println("秒");
//   // Serial.print(String(minute));Serial.println("分");
//   // Serial.print(String(hour));Serial.println("时");
//   clk.pushSprite(0, 70);
//   clk.deleteSprite();
// }
// 绘制天气相关内容
void drawWeatherContent()
{
  // 绘制主页背景
  drawZhuYe();
  // 绘制温湿度数据
  drawTHProgressBar(nowWeather.temp, nowWeather.humidity, nowWeather.vis, nowWeather.win);
  // 绘制城市、空气质量
  drawCityAir(city, nowWeather.air);
  // 绘制天气图案和文字
  drawWeaImage(nowWeather.icon);
  drawWeaBackground(nowWeather.icon);
  // 绘制时间、日期、星期
  drawDateWeek();
}
void drawWeaBackground(int icon)
{
  String s = getWea(icon);
  if (backColor == BACK_BLACK)

  {
    if (s.equals("雪"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_xue);
    }
    else if (s.equals("雷"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_lei);
    }
    else if (s.equals("沙尘"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_yin);
    }
    else if (s.equals("雾"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_wu);
    }
    else if (s.equals("冰雹"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_bin);
    }
    else if (s.equals("多云"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_yun);
    }
    else if (s.equals("雨"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_yu);
    }
    else if (s.equals("阴"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_yin);
    }
    else if (s.equals("晴"))
    {
      tft.pushImage(0, 0, 240, 134, zhuye_qing);
    }
  }
}

void drawWeaImage(int icon)
{
  String s = getWea(icon);
  // 绘制天气图案
  if (backColor == BACK_BLACK)
  {
    if (s.equals("雪"))
    {
      tft.pushImage(27, 200, 36, 36, xue);
    }
    else if (s.equals("雷"))
    {
      tft.pushImage(27, 200, 36, 36, lei);
    }
    else if (s.equals("沙尘"))
    {
      tft.pushImage(27, 200, 36, 36, shachen);
    }
    else if (s.equals("雾"))
    {
      tft.pushImage(27, 200, 36, 36, wu);
    }
    else if (s.equals("冰雹"))
    {
      tft.pushImage(27, 200, 36, 36, bingbao);
    }
    else if (s.equals("多云"))
    {
      tft.pushImage(27, 200, 36, 36, yun);
    }
    else if (s.equals("雨"))
    {
      tft.pushImage(27, 200, 36, 36, yu);
    }
    else if (s.equals("阴"))
    {
      tft.pushImage(27, 200, 36, 36, yin);
    }
    else if (s.equals("晴"))
    {
      tft.pushImage(27, 200, 36, 36, qing);
    }
  }
}

void drawFWeaImage(int icon, int temperature)
{
  String s = getWea(icon);
  // 绘制天气图案
  if (backColor == BACK_BLACK)
  {
    if (s.equals("雪"))
    {
      tft.pushImage(27, 104, 36, 36, xue);
    }
    else if (s.equals("雷"))
    {
      tft.pushImage(26, 104, 36, 36, lei);
    }
    else if (s.equals("沙尘"))
    {
      tft.pushImage(26, 104, 36, 36, shachen);
    }
    else if (s.equals("雾"))
    {
      tft.pushImage(26, 104, 36, 36, wu);
    }
    else if (s.equals("冰雹"))
    {
      tft.pushImage(26, 104, 36, 36, bingbao);
    }
    else if (s.equals("多云"))
    {
      tft.pushImage(26, 104, 36, 36, yun);
    }
    else if (s.equals("雨"))
    {
      tft.pushImage(26, 104, 36, 36, yu);
    }
    else if (s.equals("阴"))
    {
      tft.pushImage(26, 104, 36, 36, yin);
    }
    else if (s.equals("晴"))
    {
      tft.pushImage(26, 104, 36, 36, qing);
    }
  }

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("获取RTC时间失败");
    return;
  }

  clk.setColorDepth(8);
  clk.loadFont(cityandwea_18);
  // 星期L@
  clk.createSprite(54, 17);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(CR_DATUM);
  clk.drawString(week(timeinfo.tm_wday), 53, 10);
  clk.pushSprite(158, 148);
  clk.deleteSprite();

  // 月日
  clk.createSprite(54, 17);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(CC_DATUM);
  clk.drawString(monthDay(timeinfo.tm_mon, timeinfo.tm_mday), 27, 10);
  clk.pushSprite(95, 148);
  clk.deleteSprite();

  // 绘制天气文字

  clk.setTextDatum(TL_DATUM);

  clk.createSprite(63, 20);
  clk.fillSprite(TFT_WHITE);
  clk.setTextColor(0x8C31);
  clk.drawString(s, 2, 3);
  clk.pushSprite(76, 120);
  clk.deleteSprite();

  // 温度
  clk.createSprite(40, 20);
  clk.setTextColor(0x8C31);
  clk.fillSprite(TFT_WHITE);
  clk.setTextDatum(TL_DATUM);
  if (temperature < 0)
  {
    clk.drawNumber(temperature, 2, 3);
  }
  else
  {
    clk.drawNumber(temperature, 2, 3);
  }
  clk.drawString("℃", 20, 3);
  clk.pushSprite(145, 120);
  clk.deleteSprite();

  clk.unloadFont();
}

// 绘制一周天气图案
void drawFutureWeaImage(int wea_img, int x, int y)
{
  String s = getWea(wea_img);
  if (backColor == BACK_BLACK)
  {
    if (s.equals("雪"))
    {
      tft.pushImage(x, y, 36, 36, xue);
    }
    else if (s.equals("雷"))
    {
      tft.pushImage(x, y, 36, 36, lei);
    }
    else if (s.equals("沙尘"))
    {
      tft.pushImage(x, y, 36, 36, shachen);
    }
    else if (s.equals("雾"))
    {
      tft.pushImage(x, y, 36, 36, wu);
    }
    else if (s.equals("冰雹"))
    {
      tft.pushImage(x, y, 36, 36, bingbao);
    }
    else if (s.equals("多云"))
    {
      tft.pushImage(x, y, 36, 36, yun);
    }
    else if (s.equals("雨"))
    {
      tft.pushImage(x, y, 36, 36, yu);
    }
    else if (s.equals("阴"))
    {
      tft.pushImage(x, y, 36, 36, yin);
    }
    else if (s.equals("晴"))
    {
      tft.pushImage(x, y, 36, 36, qing);
    }
  }
}
// 绘制城市、空气质量
void drawCityAir(String city, int air)
{
  clk.setColorDepth(8);
  // clk.setTextDatum(CC_DATUM);
  // 城市L@
  clk.loadFont(cityandwea_18);
  clk.createSprite(57, 17);
  clk.fillSprite(TFT_WHITE);
  clk.setTextColor(0x8C31);
  clk.setTextDatum(CL_DATUM);
  clk.drawString(city, 1, 10);
  clk.pushSprite(26, 148);
  clk.deleteSprite();
  clk.unloadFont();
  // 空气质量， ≤50优（绿色），≤100良（蓝），≤150中（橙），≤200差（红）
  clk.loadFont(city_wea_24);
  String level;
  uint16_t color;
  if (air <= 50)
  {
    color = 0x0E27;
    level = "优";
  }
  else if (air <= 100)
  {
    color = 0x2C3E;
    level = "良";
  }
  else if (air <= 150)
  {
    color = 0xFC60;
    level = "中";
  }
  else
  {
    color = TFT_RED;
    level = "差";
  }
  clk.createSprite(40, 36);
  clk.fillSprite(TFT_WHITE);
  // clk.fillRoundRect(0, 0, 40, 28, 7, color); // 进度条填充(x方向,y方向,长度,高度,圆角,颜色)
  clk.setTextColor(color);
  clk.setTextDatum(CC_DATUM);
  clk.drawString(level, 17, 18);
  clk.pushSprite(103, 270);
  clk.deleteSprite();
  clk.unloadFont();
}
// 绘制温湿度进度条和温湿度数据
void drawTHProgressBar(int temperature, int humidity, String vis, String win)
{
  // 将显示温湿度文字的地方先置为白色，防止温度刷新时，屏幕有残影

  clk.createSprite(41, 36);
  clk.fillSprite(TFT_WHITE);
  clk.pushSprite(103, 200); // 湿度
  // clk.pushSprite(103, 270);//空气指数
  clk.pushSprite(25, 270);  // 温度
  clk.pushSprite(179, 270); // 风速
  clk.deleteSprite();

  clk.createSprite(51, 36);
  clk.fillSprite(TFT_WHITE);
  clk.pushSprite(172, 200); // 能见度
  clk.deleteSprite();
  // 显示温湿度数据
  tft.loadFont(city_wea_24);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(0x8C31, TFT_WHITE);
  if (temperature < 0)
  {
    tft.drawNumber(temperature, 26, 279);
  }
  else
  {
    tft.drawNumber(temperature, 26, 279);
  }
  tft.drawString("℃", 49, 279);
  if (humidity >= 100)
  {
    tft.drawNumber(humidity, 104, 210);
  }
  else
  {
    tft.drawNumber(humidity, 104, 210);
  }
  tft.drawString("%", 126, 210);

  // 绘制能见度L@
  tft.drawString(vis, 173, 210);

  // 绘制风速L@
  tft.drawString(win, 180, 279);

  tft.unloadFont();
}

// 根据icon获得天气状况
String getWea(int icon)
{
  String s = "";
  switch (icon)
  {
  case 100:
  case 150:
    s = "晴";
    break;
  case 104:
    s = "阴";
    break;
  case 300:
  case 301:
  case 305:
  case 306:
  case 307:
  case 308:
  case 309:
  case 310:
  case 311:
  case 312:
  case 313:
  case 314:
  case 315:
  case 316:
  case 317:
  case 318:
  case 350:
  case 351:
  case 399:
    s = "雨";
    break;
  case 101:
  case 102:
  case 103:
  case 151:
  case 152:
  case 153:
    s = "多云";
    break;
  case 304:
    s = "冰雹";
    break;
  case 500:
  case 501:
  case 502:
  case 509:
  case 510:
  case 511:
  case 512:
  case 513:
  case 514:
  case 515:
    s = "雾";
    break;
  case 503:
  case 504:
  case 507:
  case 508:
    s = "沙尘";
    break;
  case 302:
  case 303:
    s = "雷";
    break;
  case 400:
  case 401:
  case 402:
  case 403:
  case 404:
  case 405:
  case 406:
  case 407:
  case 408:
  case 409:
  case 410:
  case 456:
  case 457:
  case 499:
    s = "雪";
    break;
  default:
    s = "晴";
    break;
  }
  return s;
}
// 处理星期
String week(int tm_wday)
{
  String wk[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  String s = wk[tm_wday];
  return s;
}
// 处理月日
String monthDay(int tm_mon, int tm_mday)
{
  String s = "";
  s = s + (tm_mon + 1);
  s = s + "." + tm_mday;
  return s;
}
// 处理时分秒
void doHourMinuteSecond(int hour, int minute, int second, int *hourH, int *hourL, int *minuteH, int *minuteL, int *secondH, int *secondL)
{
  *hourH = hour / 10;
  *hourL = hour % 10;
  *minuteH = minute / 10;
  *minuteL = minute % 10;
  *secondH = second / 10;
  *secondL = second % 10;
}