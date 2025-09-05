# duduclock自用版本
  
1.修改了屏幕接线用来控制亮度  
![image](img/网购转接板接线.jpg)  
（粉色的线接到 GPIO 06 ）   
2.更改按键逻辑，按键单击下一页，双击上一页，长按关闭屏幕   
3.删除计时器  
4.增加低功耗模式，超过一定时间进入休眠，以增加电池的续航，低功耗关闭wifi，没有按键唤醒天气不会更新  
5.默认亮度defaultBrightness设置为200，进入低功耗模式的亮度lowBrightness为5      
6.目前已知bug，单击按键唤醒的时候有很低的概率会触发关闭屏幕，暂未找到原因    
7.修改字库文件city_18.h 把自己的城市写进去，添加了太多图片，内存不够把所有城市都写进去了  
  
参考视频
【DuDuclock 复刻 智能天气时钟】  
 https://www.bilibili.com/video/BV11ivTzeEBu/?share_source=copy_web&vd_source=0943c63425a90970e6fffb1efc6d5ff9
