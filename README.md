# *J1ST.IO Demo for FreeRTOS(ESP8266) in C * #

### 基于FreeRTOS的J1ST.IO演示程序
- 样例基于 [*ZE_FreeRTOS_SDK 1.0.0*](https://github.com/zenin-tech/HW.RaspberryPi)运行在FreeRTOS上
- 硬件设备：NodeMCU
- 硬件模块：PCF8591模块、MAX7219模块、GPIO模块
- GCC编译器：gcc 4.8.2 for xtensa-lx106
- 其它：乐鑫 [ESP8266 RTOS SDK v1.4.0](http://www.espressif.com/en/support/download/sdks-demos)
- 在模块的初始阶段，还需要一台iPhone或Android手机以配置系统

# 概要
*J1ST.IO Demo for FreeRTOS(ESP8266)* 旨在能让用户在运行FreeRTOS的NodeMCU系统（基于ESP8266芯片）上快速实现J1ST.IO的基本操作功能，并配合[Developer Console](http://developer.j1st.io/)控制台来实现属于自己的IoT产品。若您未使用过我们的产品，请先阅读[J1ST.IO入门(TODO)]()。

- *J1ST.IO Demo for FreeRTOS(ESP8266) in C* 提供的操作：
    - WiFi操作功能（由乐鑫的ESP8266 RTOS SDK提供）
        - ESP-TOUCH: 乐鑫的WiFi快连技术，不需路由器支持，即可在手机上将WiFi配置信息传送给待配置设备。
        - Wifi接入后台操作
    - J1ST.IO平台功能
        - 接入J1ST.IO平台及相关协议的勤务操作
        - 生成符合J1ST.IO规范的报文，并定时（或按需）上传平台
        - 订阅有关话题并解析平台下发指令
    - 硬件模块相关的功能
        - PCF8591模块
            - *数据采集*，采集四个模拟量，分别对应温度、光强、电位器位置与电压等数值
            - *远程控制*，调节模块上LED的亮度
        - MAX7219模块
            - *远程控制和状态显示*，提供8位LED数字显示
        - GPIO模块
            - *外部触发*，多至4个开关量，可用于系统状态输入或事件触发
            - *状态显示*，多至8个LED
- 硬件MQTT连接服务的参数配置：
    - J1ST.IO平台系统的参数（在`sPayload.c`中定义）

        | Section | Value |
        |--------|--------|
        | DEFAULTHOST | Use `"developer.j1st.io"`, which is provided by J1ST.IO |
        | gPort | Use `8883`, which is provided by J1ST.IO
        | DEFAULTAGENT | Use `agentId`, which is provided by J1ST.IO |
        | DEFAULTTOKEN | Use `agentToken`, which is provided by J1ST.IO |

    - 符合J1ST.IO规范的`PUBLISH`和`SUBSCRIBE`主题生成（在`sPayload.c`文件`SetParas()`函数中生成）

        ```
            PUBLISH:  agents/{agentId}/upstream
        ```

        ```
            SUBSCRIBE:  agents/{agentId}/downstream
        ```

- SDK的API，详细文档[在此](https://github.com/zenin-tech/HW.FreeRToS.SDK/wiki)：
    - J1ST.IO network functions
        - *jNetInit*
        - *jNetFree*
        - *jNetConnect*
        - *jNetDisconnect*
        - *jNetSubscribe*
        - *jNetYield*
        - *jNetPublish*
        - *jNetSubscribe*
    - J1ST.IO Timer utilities
        - *TmrInit*
        - *TmrStart*
        - *TmrStart_ms*
        - *TmrExpired*
        - *TmrLeft*
    - J1ST.IO cJSON utilities
        - *cJSON_Parse*
        - *cJSON_Delete*
        - *cJSON_GetObjectItem*
        - *cJSON_ArrayForEach*
        - *cJSON_CreateArray*
        - *cJSON_CreateObject*
        - *cJSON_AddItemToArray*
        - *cJSON_AddNumberToObject*
        - *cJSON_AddStringToObject*
        - *cJSON_Print*
        - *cJSON_PrintUnformatted*
    - Types and Data structures
        - *struct MessageData*
            {
            MQTTMessage\* message;
            MQTTString\* topicName;
            };
        - *typedef* void ( \*messageHandler)(MessageData\* );

### 相关链接

| Resource | Location |
| -------------- | -------------- |
| Developer Wiki       | https://github.com/zenin-tech/HW.FreeRTOS_SDK/wiki |
| API docs             | [ZE_Liunx_SDK api](https://github.com/zenin-tech/ZE_FreeRTOS_SDK/wiki) |
| Docs                 | [J1ST.IO docs(TODO)](https://github.com/zenin-tech/HW-freertos-esp8266/wiki) |
| Home                 | [J1ST.IO(Home)](http://j1st.io/) |
| Developer Console    | [J1ST.IO(Developer)](http://developer.j1st.io/) |

# 库说明
表格*Description*中所描述的功能请查看[概要]()。

### 文件夹结构

| Directory     | Description |
|--------       |--------|
| inc/          | Head files of Demo |
| user/         | Application and module source |
| driver/       | ESP8266外围接口的库文件 |
| eMqtt/        | MQTT协议库文件 |
| SSLPatch/     | Replace corresponding files under free-rtos-sdk |

### 8266 Demo user目录下文件简单说明

| File        | Description |
|--------     |--------|
| user_main.c | 用户入口代码，包含：入口函数，ESP8266外围设备及模块初始化（含FLASH键长按及短按处理），定时发送及LED灯操作 |
| smConn.c    | WiFi相关的勤务程序，包括ESP-TOUCH，连接至AP，连接完成后，启动J1ST.IO连接等 |
| sPayload.c  | J1ST.IO平台相关功能实现，包括J1ST.IO平台的连接与重连，SUBSCRIBE与PUBLISH操作，生成与解析J1ST.IO平台的JSON报文，相关传感器数值的读取等 |
| PCF8591.c MAX7219.c WS2812.c | 相应模块的驱动实现 |
| Makefile    | Linux下生成可执行文件的make文件 |

# Quickstart
本样例旨在能让用户快速理解和实现J1ST.IO的基本操作功能，并搭配Developer console开发工具来配合完成属于您自己的J1ST.IO的IoT产品。

## Hardware Support
系统基于NodeMCU，用户应同时准备PCF8591 AD/DA模块(I2C)、MAX7219 8位LED显示模块、GPIO模块，[在此(TODO)]()可以获得模块。为了方便接线，我们建议选购与NodeMCU配套的底板。
以下是NodeMCU与相关模块的接线表：

|PCF8591模块(I2C)   |NodeMCU|
|------             |------|
|VCC                |3V|
|GND                |GND|
|SDA                |D4|
|SDL                |D5|

|MAX7219模块         |NodeMCU|
|------             |------|
|VCC                |VUSB|
|GND                |GND|
|DIN                |D2|
|CS                 |D6|
|CLK                |D7|

|GPIO模块(GPIO)     |NodeMCU|
|------             |------|
|LED VCC            |3V|
|LED D7             |D0|
|KEY GND            |GND|
|KEY S4             |D1|

## 配置ESP8266开发环境
在[乐鑫官网](http://www.espressif.com)和[ESP8266社区](http://www.esp8266.com)(由全球ESP8266的爱好者们创建)有ESP8266开发所需的大量文档和代码。我们强烈建议用户首先按照[乐鑫官方的入门指导](http://www.espressif.com/zh-hans/support/explore/get-started/esp8266/getting-started-guide)，下载VirtualBox下的已配置完成开发系统工具集镜像（Linux系统），固件下载工具和RTOS SDK(当前最新版本为1.4.0，可以在[乐鑫官网](http://www.espressif.com/en/support/download/sdks-demos)或者[GitHub](http://www.espressif.com/zh-hans/support/explore/get-started/esp8266/getting-started-guide)上下载)。*建议用户安装配置完成后，首先按照相关手册，编译并下载运行一个系统提供的示例程序，以熟悉相关流程。*

## 下载示例程序
在乐鑫系统缺省配置下，在FreeRTOS SDK的app目录下，创建一个工作目录用来存放下载的Demo源码

```
git clone https://github.com/zenin-tech/HW-freertos-esp8266.git
```

## Get hardware connection permission

现在需要在我们的`Develop Console`控制台上创建一个新的`Product`用于此Demo的实现，您也可以在任意一个现有的`Product`下进行操作。下面给出的是新建一个`Product`的样例：
- 登陆之后点击右上角`Add`，填入`ProductName`，选择`Public`，确认`TimeZone`，点击确认
- 点击左侧菜单栏`Agents`，选择刚刚新建的`Product`，选择一个`网关SN`，点击`剪贴SN&Token`，选择合适的方式保存
- Linux系统下修改HW.ESP8266文件夹下的`sPayload.c`中硬件连接许可的参数配置，按照提示替换成属于您的`agentId` `agentToken`

```
cd HW-freertos-esp8266/user/
vi sPayload.c
```

```
#define DEFAULTHOST "developer.j1st.io"
#define DEFAULTAGENT "574beaf36097e967b84370c9"      //Replace here of agentId
#define DEFAULTTOKEN "TqPGtNVacEjMfNQMYnzMQrMpcRtXjOuZ"   //Replace here of agentToken
```

- 替换后按`[Esc]`退出编辑并输入`:wq`保存退出。

*注：`agentId` `agentToken`必须同时使用，在相同的时间仅能提供给一台硬件设备运行。*

## 编译和烧写
- 按照乐鑫的手册，进入app/HW-freertos-esp8266目录下，执行`./gen_misc.sh`以生成相关的二进制映像文件。
- 将上述生成的映像文件通过乐鑫提供的FLASH DOWNLOAD TOOL下载到NODEMCU中，具体操作方式请参见乐鑫的文档。
- 重新启动NODEMCU，可以看见相关LED以5Hz的频率快闪，同时计数值在迅速增加，表明设备处于配置状态。
- 在8位数码LED上，前6位是计数值，后两位是当前的光强（0-255，实际显示最后两位）

## 设备配置
- 在Android手机上启动ESP-TOUCH Android APK或在iPhone上启动EPS-TOUCH IOS IPA，相关APP可以在[乐鑫官网](http://www.espressif.com/zh-hans/support/download/apks)下载
- 在手机上输入AP的密码，即可以把AP的SSID和Password发送给同一区域所有处于配置状态的设备。
- 配置完成后，设备会使用刚才给出的SSID和PASSWORD启动并连入相关AP。连接成功后，LED灯闪烁频率变为1Hz，计数值也会以每秒增加2的速度变化。
- 如果配置错误或者要更改配置，可以长按FLASH键5秒以上，系统即清除AP配置的相关信息并重启。

## Use Developer Console
在Demo中你可以试着在控制台中实现我们提供的`LogStash`、`Fn`、`WebHook`等功能。

- LogStash：配合PCF8591模块获取光照度"Lumen"，可手动调整的可变电阻电压"Volt"以及程序产生的随机数值"temperature"。当模拟故障发生时（按键被按下时）"error"会被置为true。在本实现中，模拟了一个带有额外sensor的设备，agent本体仅上报模拟的温度及故障情况，额外sensor上报照度及电压。Agent本体和sensor上报的数据在同一条消息中传输。
    - upstream topic

    ```
    agents/570c63096097e943626e1142/upstream
    ```

    - upstream payload

    ```json
    [
       {
          "hwid": "570c63096097e943626e1142",
          "type": "AGENT",
          "values": {
             "temperature": 21,
             "error": false
          }
       },
       {
          "hwid": "ABCDEF0123456",
          "type": "SENSOR",
          "values": {
             "lumen": 181,
             "Volt": 151
          }
       }
    ]
    ```

- Fn："SetInterval"改变上传间隔（单位为秒），"SetRand"亮度（随机数的均值和方差），"SetDisplay"改变LED显示的最后两位数值（取0时显示照度的后两位）。下面，分别展示了修改单个或多个参数的命令。在实际使用中，这些命令是DevConsole指令J1ST.IO平台下发的，8266上的应用程序应能正确解析并完成相关操作。
    - downstream topic

    ```
    agents/570c63096097e943626e1142/downstream
    ```

    - downstream payload

    ```json
    {
       "SetRand": [
          {
             "HwID": "570c63096097e943626e1142",
             "mean": 20,
             "vari": 15
          }
       ]
    }
    ```

    ```json
    {
       "Setinterval": [
          {
             "HwID": "570c63096097e943626e1142",
             "seC": 90
          }
       ],
       "SetDisplay": [
          {
             "HwID": "570c63096097e943626e1142",
             "data": 77
          }
       ]
    }
    ```

- WebHook：


### 手机APP端配合Demo程序联合使用
- Dev页面设置player APP实现
- profile查看
- chart查看
- Fn实现

### Note for SSL
- Use jNetSInit to create an SSL environment instead of jNetInit for non-secure sys
- Make sure the port is 8883 for Mqtt on SSL
- Nothing more

### Note for certificate verification
- Does not check validity of full cert chains as we cannot / do not want to store all root certificates so self-signed certificate is allowed
- The integrity of the first certificate is checked.
- The DN can be checked by programmers in jVerifyCert
- It is strongly suggested that we check the fingerprint of the server otherwise the SSL is non-helpful

### Bugs (?)
- Memory leakage on connection broken?
