# Arduino T2

这是 [Tuya T2 开发板](https://developer.tuya.com/cn/docs/iot/t2-u-board?id=Kce6cq9e9vlmv)的 Arduino 仓库，可以使用  Arduino IDE 2 开发 T2 开发板。目前版本是基于 TuyaOS 3.3.3 进行适配开发的，所以  TuyaOS 3.3.3 有的功能这里都有的，关于  TuyaOS 3.3.4  有哪些功能，可以[点击查看详情](https://developer.tuya.com/cn/docs/iot-device-dev/TuyaOS-frame_iot_abi_map?id=Kc4clt7k7h62u)。

> 注意：该项目还处于测试阶段，还没有完全适配于 Arduino API。

## 如何安装

Arduino IDE 2 开发板管理器地址如下：

GitHub：

```
https://github.com/shiliu-yang/arduino-t2/releases/download/0.0.1/pack_tuya_t2_index.json
```

Gitee：
```
https://gitee.com/shiliu_yang/arduino-t2/releases/download/0.0.1/pack_tuya_t2_index.json
```

目前在 Linux 和 Windows 下测试都是可以运行的，可能因为部分编译或烧录工具导致**无法在 Windows 32 位下运行**。

## API 适配进度

和 Arduino 相关的 API 借助了 [ArduinoCore-AP](https://github.com/arduino/ArduinoCore-API)。

硬件功能相关 API 适配现状如下表所示：

|        功能         |  进度  |
| :-----------------: | :----: |
|        GPIO         |  完成  |
|         ADC         |  完成  |
|         PWM         |  完成  |
|       Serial        | 待完成 |
|         SPI         | 待完成 |
|         I2C         | 不支持 |
|        TIMER        | 待完成 |
| WiFi 与网络相关接口 | 待完成 |
|         BLE         | 待完成 |
