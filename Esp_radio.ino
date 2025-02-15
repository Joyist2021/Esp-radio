//******************************************************************************************
//*  Esp_radio -- 用于 ESP8266、（彩色）显示器和 VS1053 MP3 模块的 Webradio 接收器    *
//*               作者 Ed Smallenburg (ed@smallenburg.nl)                                    *
//*  ESP8266 以 80 MHz 运行，它能够处理高达 256 kb 的比特率。       *
//*  ESP8266 以 160 MHz 运行，它能够处理高达 320 kb 的比特率。      *
//******************************************************************************************
// 使用的 ESP8266 库：
//  - ESP8266WiFi       - ESP8266 Arduino 默认库的一部分。
//  - SPI               - Arduino 默认库的一部分。
//  - Adafruit_GFX      - https://github.com/adafruit/Adafruit-GFX-Library
//  - TFT_ILI9163C      - https://github.com/sumotoy/TFT_ILI9163C
//  - ESPAsyncTCP       - https://github.com/me-no-dev/ESPAsyncTCP
//  - ESPAsyncWebServer - https://github.com/me-no-dev/ESPAsyncWebServer
//  - FS - https://github.com/esp8266/arduino-esp8266fs-plugin/releases/download/0.2.0/ESP8266FS-0.2.0.zip
//  - ArduinoOTA        - ESP8266 Arduino 默认库的一部分。
//  - AsyncMqttClient   - https://github.com/marvinroger/async-mqtt-client
//  - TinyXML           - Fork https://github.com/adafruit/TinyXML
//
// VS1053（用于 ESP8266）的库不可用（或不容易找到）。 因此，
// 这个模块的一个类是从 maniacbug 库派生的，并集成在这个sketch草图中。
//
// 编译:设置SPIFS为3mb。设置IwIP变量为“V1.4更高带宽”。
// 有关合适的电台，请参见 http://www.internet-radio.com。
// 将您选择的电台添加到 .ini 文件中。
//
// 程序简要说明：
// 首先找到合适的 WiFi 网络并建立连接。
// 然后将与广播服务器建立连接。
// 服务器以可读 ascii 的标头中的一些信息开头，以双 CRLF 结尾，例如：
//  icy-name:佛罗里Florida达经典摇滚 - SHE Radio
//  icy-genre:经典摇滚 60 年代 70 年代 80 年代老歌 迈阿密 南佛罗里达
//  icy-url:http://www.ClassicRockFLorida.com
//  content-type:audio/mpeg
//  icy-pub:1
//  icy-metaint:32768          - MP3后元数据在32768字节
//  icy-br:128                 - 以 kb/sec 为单位（对于 Ogg，这就像 "icy-br=Quality 2"
//
// 收到 de double CRLF 后，服务器开始发送 mp3 或 Ogg 数据。 对于 mp3，
// 这个数据可能包含元数据(非mp3)在每个“元数据”mp3字节后。
// 在大多数情况下，元数据是空的，但如果有可用的元数据，则内容将显示在TFT上。
// 按下输入按钮，播放器就会选择ini文件中的下一个预设站。
//
// 使用的显示器TFT_ILI9163C中国1.8彩色TFT模块128 x 160像素。
// TFT_ILI9163C.h文件已被更改，以反映这个特定的模块。
// TFT_ILI9163C.cpp已被更改为使用全屏幕宽度，如果旋转到模式"3"。
// 现在每行有26个字符和16行。软件不需要安装显示器也能工作。
// 如果不使用TFT，你可以使用GPIO2和GPIO15作为控制按钮。参见下面“USE使用TFT”的定义。
// 开关被编程为：
// GPIO2 : "转到 站点1"
// GPIO0 : "下一站"
// GPIO15: "上一站".  请注意，当启动ESP8266时，GPIO15必须是LOW。
//         因此，GPIO15 的按钮必须连接到 VCC (3.3V) 而不是 GND。

//
// 对于 WiFi 网络的配置：请参阅进一步的全局数据部分。
//
// VS1053 和 TFT 的 SPI 接口使用硬件 SPI。
//
// 接线：
// NodeMCU  GPIO    Pin引脚编程     连接到 LCD           连接到 VS1053       连接到 rest
// -------  ------  --------------  ---------------     -------------------  ---------------------
// D0       GPIO16  16              -                   pin 1 DCS            -
// D1       GPIO5    5              -                   pin 2 CS             nodeMCU上的LED
// D2       GPIO4    4              -                   pin 4 DREQ           -
// D3       GPIO0    0 FLASH        -                   -                    控制按钮“下一站”
// D4       GPIO2    2              pin 3 (D/C)         -                    (OR)控制按钮“站点1”
// D5       GPIO14  14 SCLK         pin 5 (CLK)         pin 5 SCK            -
// D6       GPIO12  12 MISO         -                   pin 7 MISO           -
// D7       GPIO13  13 MOSI         pin 4 (DIN)         pin 6 MOSI           -
// D8       GPIO15  15              pin 2 (CS)          -                    (OR)控制按钮“上一站”
// D9       GPI03    3 RXD0         -                   -                    预留串行输入
// D10      GPIO1    1 TXD0         -                   -                    预留串行输出
// -------  ------  --------------  ---------------     -------------------  ---------------------
// GND      -        -              pin 8 (GND)         pin 8 GND            电源Power supply
// VCC 3.3  -        -              pin 6 (VCC)         -                    LDO 3.3 伏Volt
// VCC 5 V  -        -              pin 7 (BL)          pin 9 5V             Power supply
// RST      -        -              pin 1 (RST)         pin 3 RESET          复位电路
//
// 复位电路是一个带有 2 个连接到 GPIO5 和 GPIO16 的二极管和一个接地电阻 (wired OR gate有线或门) 的电路，
// 因为没有可用于此功能的空闲 GPIO 输出。
// 该电路包含在文档中。
// 问题:Issues:
// Webserver在一段时间后产生错误"LmacRxBlk:1"。在那之后，它会工作得很慢。
// 在这种情况下，程序将重置ESP8266。现在我们切换到async异步 webserver，
// 问题仍然存在，但程序将不再崩溃。
// 上传到ESP8266不可靠。
//
// 31-03-2016，ES：第一次设置。
// 01-04-2016，ES：开机检测缺失VS1053。
// 05-04-2016，es：通过80端口http服务器添加命令。
// 14-04-2016，ES：流错新增图标和开关预置。
// 2016-04-18，ES：增加webserver尖峰。
// 19-04-2016，ES：增加环缓冲区。
// 20-04-2016，ES：WiFi密码穿透SPIFF文件，开启OTA。
// 21-04-2016，ES：切换到异步Webserver。
// 27-04-2016，ES：保存设置，重启后使用相同的音量和预置。
// 03-05-2016，ES：添加低音/高音设置(另请参见new index.html)。
// 04-05-2016，es：允许“skonto.ls.lv：8002/mp3”这样的站点。
// 06-05-2016，es：如果这是唯一的.pw文件，则允许隐藏WiFi站。
// 07-05-2016，es：在webserver中增加预置选择。
// 2016-12-05-2016，ES：新增OGG-ENCODER支持。
// 2016-05-13，ES：更好的Ogg检测。
// 17-05-2016，ES：用于命令的模拟输入，如果不需要TFT，则附加按钮。
// 2016-05-26，ES：修复BUTTON3 bug(无TFT)。
// 27-05-2016，ES：修复重启时的恢复站。
//2016年04月07日，ES：WiFi.Disconnect现在清除旧连接(感谢Juppot)。
//23-09-2016，ES：通过MQTT和串行输入增加命令，AP模式Wifi设置。
//2016-04-10，es：.ini文件中的配置。不再使用EEPROM和.pw文件。
//11-10-2016，ES：允许头部没有码率的站点，如icecast.err.ee/raadio2.mp3。
//2016-14-10，ES：更新为Async-MQTT-Client-MASTER 0.5.0。
//2016-22-10，ES：修正静音/取消静音。
//2016-11-15，ES：支持.m3u播放列表。
//2016年12月22日，ES：支持localhost(从SPIFF播放)。
//2016-12-28，ES：实现Resume请求。
//31-12-2016，es：允许contentType“text/css”
//02-01-2017，ES：PROGMEM中的Webinterface。
//16-01-2017，ES：更正播放列表。
//17-01-2017，es：bugfix配置页面和播放列表。
//23-01-2017，es：bugfix播放列表。
//26-01-2017，ES：检查错误的冰彩。
//30-01-2017，es：允许分块传输编码。
//01-02-2017，es：修复文件上传。
//26-04-2017，ES：更好地输出预置变化的网页界面。
//03-05-2017，ES：无网络禁止启动inputstream。
//2017-04-05-2017，ES：集成iHeartRadio，感谢NonaSuomy。
//09-05-2017，es：修复abs问题。
//11-05-2017，es：显示前转换UTF8字符，感谢Everb313。
//24-05-2017，ES：更正。请勿跳过.mp3文件的第一部分。
//26-05-2017，ES：更正.m3u播放列表和LC/UC问题。
//31-05-2017，ES：TFT音量指示灯。
//02-02-2018，es：强制802.11n连接。
//18-04-2018，ES：无法使用wifi.connected()的解决方法。
//05-10-2018，ES：修复找不到网络异常。
//23-04-2018，es：检查低音设置。
//06-07-2021，ES：切换到LittleFS。
//06-07-2021，ES：新增SPI RAM(实验性)。
// 08-02-2022，es：新增重定向。
//
// 定义版本号，也用于 webserver 作为 Last-Modified 头：
#define VERSION "Fri, 11 Feb 2022 13:05:00 GMT"
// 实验性 SPI-RAM
//#define SPIRAM                                 // 使用 SPIRAM 作为环形缓冲区。 未定义 = 不使用
//#define SPIRAMDELAY 100000                     // 从 SPIRAM 读取之前的延迟(单位bytes)
// TFT.  如果需要，定义 USETFT。
#define USETFT
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncMqttClient.h>
#include <SPI.h>
#if defined ( USETFT )
  #include <Adafruit_GFX.h>
  #include <TFT_ILI9163C.h>
#endif
#include <Ticker.h>
#include <stdio.h>
#include <string.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <TinyXML.h>
#include <LittleFS.h>
#ifdef SPIRAM
  #include "spiram.hpp"
#endif
extern "C"
{
  #include "user_interface.h"
}

// 模拟输入上 3 个控制开关的定义
// 您可以通过按住开关并选择 /?analog=1 来测试模拟输入值
// 在网页界面中。 请参阅文档中的示意图。
// 开关分别编程为“Goto station 1”、“Next station”和“Previous station”。
// 如果不使用，请将这些值设置为 2000，或者将模拟输入接地。
#define NUMANA  3
//#define asw1    252
//#define asw2    334
//#define asw3    499
#define asw1    2000
#define asw2    2000
#define asw3    2000
//
// TFT 屏幕的颜色定义（如果使用）
#define BLACK   0x0000
#define BLUE    0xF800
#define RED     0x001F
#define GREEN   0x07E0
#define CYAN    GREEN | BLUE
#define MAGENTA RED | BLUE
#define YELLOW  RED | GREEN
// 使用的数字 I/O
// VS1053 模块的引脚
#define VS1053_CS     5
#define VS1053_DCS    16
#define VS1053_DREQ   4
// TFT 模块的 CS 和 DC 引脚（如果使用，请参见“USETFT”的定义）
#define TFT_CS 15
#define TFT_DC 2
//控制station站的控制按钮（GPIO）
#define BUTTON1 2
#define BUTTON2 0
#define BUTTON3 15
// 用于流畅播放的环形缓冲区。 20000 字节是 160 Kbits，在 128kb 比特率下大约需要 1.5 秒。
// 如果缓冲区太长，Web 界面将不再工作
#define RINGBFSIZ 18000
// 调试缓冲区大小
#define DEBUG_BUFFER_SIZE 100
//ini文件的名字
#define INIFILENAME "/radio.ini"
// 如果连接到 WiFi 网络失败，则接入点名称。也是 WiFi 和 OTA 的主机名。
// 并不是AP的密码必须至少有8个字符。
// 也用于其他命名。
#define NAME "Esp-radio"
// 放弃之前MQTT重新连接的最大数量
#define MAXMQTTCONNECTS 20
//
//******************************************************************************************
// 各种函数的转发声明                                               *
//******************************************************************************************
//void   displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color ) ;
void   showstreamtitle ( const char* ml, bool full = false ) ;
void   handlebyte ( uint8_t b, bool force = false ) ;
void   handlebyte_ch ( uint8_t b, bool force = false ) ;
void   handleFS ( AsyncWebServerRequest* request ) ;
void   handleFSf ( AsyncWebServerRequest* request, const String& filename ) ;
void   handleCmd ( AsyncWebServerRequest* request )  ;
void   handleFileUpload ( AsyncWebServerRequest* request, String filename,
                          size_t index, uint8_t* data, size_t len, bool final ) ;
char*  dbgprint( const char* format, ... ) ;
char*  analyzeCmd ( const char* str ) ;
char*  analyzeCmd ( const char* par, const char* val ) ;
String chomp ( String str ) ;
void   publishIP() ;
String xmlparse ( String mount ) ;
bool   connecttohost() ;
void XML_callback ( uint8_t statusflags, char* tagName, uint16_t tagNameLen,
                    char* data,  uint16_t dataLen ) ;



//
//******************************************************************************************
// 全局数据段。                                                                   *
//******************************************************************************************
// 有一个块 ini-data 包含一些配置。 配置数据为 *
// 通过网络界面保存在 SPIFFS 文件 radio.ini 中。 重新启动时，新数据将 *
// 从此文件中读取。 *
// ini_block 中的项目可以通过来自 webserver/MQTT/Serial 的命令进行更改。               *
//******************************************************************************************
struct ini_struct
{
  String         mqttbroker ;                              // MQTT 代理服务器的名称
  uint16_t       mqttport ;                                // 端口，默认 1883
  String         mqttuser ;                                // MQTT认证用户
  String         mqttpasswd ;                              // MQTT 认证密码
  String         mqtttopic ;                               // 要订阅的主题
  String         mqttpubtopic ;                            // 发布到 pubtop 的主题（IP 将被发布）
  uint8_t        reqvol ;                                  // 请求的音量
  uint8_t        rtone[4] ;                                // 请求的低音/高音设置
  int8_t         newpreset ;                               // 请求的预设
  String         ssid ;                                    // 要连接的 WiFi 网络的 SSID
  String         passwd ;                                  // WiFi网络密码
} ;

enum datamode_t { INIT = 1, HEADER = 2, DATA = 4,
                  METADATA = 8, PLAYLISTINIT = 16,
                  PLAYLISTHEADER = 32, PLAYLISTDATA = 64,
                  STOPREQD = 128, STOPPED = 256
                } ;        // State for datastream

// 全局变量
int              DEBUG = 1 ;
ini_struct       ini_block ;                               // 保存可配置的数据
WiFiClient       *mp3client = NULL ;                       // mp3 客户端的一个实例
AsyncWebServer   cmdserver ( 80 ) ;                        // 80 端口上的嵌入式 Web 服务器实例
AsyncMqttClient  mqttclient ;                              // MQTT 订阅者的客户端
IPAddress        mqtt_server_IP ;                          // MQTT代理的IP地址
char             cmd[130] ;                                // 来自 MQTT 或串行的命令
#if defined ( USETFT )
TFT_ILI9163C     tft = TFT_ILI9163C ( TFT_CS, TFT_DC ) ;
#endif
Ticker           tckr ;                                    // 用于计时 100 毫秒
TinyXML          xml;                                      // 用于XML解析器。
uint32_t         totalcount = 0 ;                          // 计数器 mp3 数据
datamode_t       datamode ;                                // State of datastream
int              metacount ;                               // Number of bytes in metadata
int              datacount ;                               // Counter databytes before metadata
String           metaline ;                                // Readable line in metadata
String           icystreamtitle ;                          // Streamtitle from metadata
String           icyname ;                                 // Icecast station name
int              bitrate ;                                 // Bitrate in kb/sec
int              metaint = 0 ;                             // Number of databytes between metadata
int8_t           currentpreset = -1 ;                      // Preset station playing
String           host ;                                    // The URL to connect to or file to play
String           playlist ;                                // The URL of the specified playlist
bool             xmlreq = false ;                          // Request for XML parse.
bool             hostreq = false ;                         // Request for new host
bool             reqtone = false ;                         // New tone setting requested
bool             muteflag = false ;                        // Mute output
uint8_t*         ringbuf ;                                 // Ringbuffer for VS1053
uint16_t         rbwindex = 0 ;                            // Fill pointer in ringbuffer
uint16_t         rbrindex = RINGBFSIZ - 1 ;                // Emptypointer in ringbuffer
uint16_t         rcount = 0 ;                              // Nr of bytes/chunks in ringbuffer/SPIRAM
uint16_t         analogsw[NUMANA] = { asw1, asw2, asw3 } ; // 3 levels of analog input
uint16_t         analogrest ;                              // Rest value of analog input
bool             resetreq = false ;                        // Request to reset the ESP8266
bool             NetworkFound ;                            // True if WiFi network connected
String           networks ;                                // Found networks
String           anetworks ;                               // Aceptable networks (present in .ini file)
String           presetlist ;                              // List for webserver
uint8_t          num_an ;                                  // Number of acceptable networks in .ini file
String           testfilename = "" ;                       // File to test (SPIFFS speed)
uint16_t         mqttcount = 0 ;                           // Counter MAXMQTTCONNECTS
int8_t           playlist_num = 0 ;                        // Nonzero for selection from playlist
File             mp3file  ;                                // File containing mp3 on SPIFFS
bool             localfile = false ;                       // Play from local mp3-file or not
bool             chunked = false ;                         // Station provides chunked transfer
int              chunkcount = 0 ;                          // Counter for chunked transfer
uint8_t          prcwinx ;                                 // Index in pwchunk (see putring)
uint8_t          prcrinx ;                                 // Index in prchunk (see getring)
#ifdef SPIRAM
  int32_t        spiramdelay = SPIRAMDELAY ;               // Delay before reading from SPIRAM
#endif
// XML parse globals.
const char* xmlhost = "playerservices.streamtheworld.com" ;// XML data source
const char* xmlget =  "GET /api/livestream"                // XML get parameters
                      "?version=1.5"                       // API Version of IHeartRadio
                      "&mount=%sAAC"                       // MountPoint with Station Callsign
                      "&lang=en" ;                         // Language
int         xmlport = 80 ;                                 // XML Port
uint8_t     xmlbuffer[150] ;                               // For XML decoding
String      xmlOpen ;                                      // Opening XML tag
String      xmlTag ;                                       // Current XML tag
String      xmlData ;                                      // Data inside tag
String      stationServer( "" ) ;                          // Radio stream server
String      stationPort( "" ) ;                            // Radio stream port
String      stationMount( "" ) ;                           // Radio stream Callsign

//******************************************************************************************
// End of global data section.                                                             *
//******************************************************************************************

//******************************************************************************************
// Pages and CSS for the webinterface.                                                     *
//******************************************************************************************
#include "about_html.h"
#include "config_html.h"
#include "index_html.h"
#include "radio_css.h"
#include "favicon_ico.h"
//
//******************************************************************************************
// VS1053 stuff.  Based on maniacbug library.                                              *
//******************************************************************************************
// VS1053 class definition.                                                                *
//******************************************************************************************
class VS1053
{
  private:
    uint8_t       cs_pin ;                        // CS 线连接的引脚
    uint8_t       dcs_pin ;                       // DCS 线连接的引脚
    uint8_t       dreq_pin ;                      // DREQ 线连接的引脚
    uint8_t       curvol ;                        // 当前音量设置 0..100%
    const uint8_t vs1053_chunk_size = 32 ;
    // SCI 寄存器
    const uint8_t SCI_MODE          = 0x0 ;
    const uint8_t SCI_BASS          = 0x2 ;
    const uint8_t SCI_CLOCKF        = 0x3 ;
    const uint8_t SCI_AUDATA        = 0x5 ;
    const uint8_t SCI_WRAM          = 0x6 ;
    const uint8_t SCI_WRAMADDR      = 0x7 ;
    const uint8_t SCI_AIADDR        = 0xA ;
    const uint8_t SCI_VOL           = 0xB ;
    const uint8_t SCI_AICTRL0       = 0xC ;
    const uint8_t SCI_AICTRL1       = 0xD ;
    const uint8_t SCI_num_registers = 0xF ;
    // SCI_MODE bits
    const uint8_t SM_SDINEW         = 11 ;        // Bitnumber in SCI_MODE always on
    const uint8_t SM_RESET          = 2 ;         // Bitnumber in SCI_MODE soft reset
    const uint8_t SM_CANCEL         = 3 ;         // Bitnumber in SCI_MODE cancel song
    const uint8_t SM_TESTS          = 5 ;         // Bitnumber in SCI_MODE for tests
    const uint8_t SM_LINE1          = 14 ;        // Bitnumber in SCI_MODE for Line input
    SPISettings   VS1053_SPI ;                    // SPI settings for this slave
    uint8_t       endFillByte ;                   // Byte to send when stopping song
  protected:
    inline void await_data_request() const
    {
      while ( !digitalRead ( dreq_pin ) )
      {
        yield() ;                                 // Very short delay
      }
    }

    inline void control_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( dcs_pin, HIGH ) ;            // Bring slave in control mode
      digitalWrite ( cs_pin, LOW ) ;
    }

    inline void control_mode_off() const
    {
      digitalWrite ( cs_pin, HIGH ) ;             // End control mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    inline void data_mode_on() const
    {
      SPI.beginTransaction ( VS1053_SPI ) ;       // Prevent other SPI users
      digitalWrite ( cs_pin, HIGH ) ;             // Bring slave in data mode
      digitalWrite ( dcs_pin, LOW ) ;
    }

    inline void data_mode_off() const
    {
      digitalWrite ( dcs_pin, HIGH ) ;            // End data mode
      SPI.endTransaction() ;                      // Allow other SPI users
    }

    uint16_t read_register ( uint8_t _reg ) const ;
    void     write_register ( uint8_t _reg, uint16_t _value ) const ;
    void     sdi_send_buffer ( uint8_t* data, size_t len ) ;
    void     sdi_send_fillers ( size_t length ) ;
    void     wram_write ( uint16_t address, uint16_t data ) ;
    uint16_t wram_read ( uint16_t address ) ;

  public:
    // Constructor.  Only sets pin values.  Doesn't touch the chip.  Be sure to call begin()!
    VS1053 ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin ) ;
    void     begin() ;                                   // Begin operation.  Sets pins correctly,
    // and prepares SPI bus.
    void     startSong() ;                               // Prepare to start playing. Call this each
    // time a new song starts.
    void     playChunk ( uint8_t* data, size_t len ) ;   // Play a chunk of data.  Copies the data to
    // the chip.  Blocks until complete.
    void     stopSong() ;                                // Finish playing a song. Call this after
    // the last playChunk call.
    void     setVolume ( uint8_t vol ) ;                 // Set the player volume.Level from 0-100,
    // higher is louder.
    void     setTone ( uint8_t* rtone ) ;                // Set the player baas/treble, 4 nibbles for
    // treble gain/freq and bass gain/freq
    uint8_t  getVolume() ;                               // Get the currenet volume setting.
    // higher is louder.
    void     printDetails ( const char *header ) ;       // Print configuration details to serial output.
    void     softReset() ;                               // Do a soft reset
    bool     testComm ( const char *header ) ;           // Test communication with module
    inline bool data_request() const
    {
      return ( digitalRead ( dreq_pin ) == HIGH ) ;
    }
    void     AdjustRate ( long ppm2 ) ;                  // Fine tune the datarate
} ;

//******************************************************************************************
// VS1053 class implementation.                                                            *
//******************************************************************************************

VS1053::VS1053 ( uint8_t _cs_pin, uint8_t _dcs_pin, uint8_t _dreq_pin ) :
  cs_pin(_cs_pin), dcs_pin(_dcs_pin), dreq_pin(_dreq_pin)
{
}

uint16_t VS1053::read_register ( uint8_t _reg ) const
{
  uint16_t result ;

  control_mode_on() ;
  SPI.write ( 3 ) ;                                // Read operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  // Note: transfer16 does not seem to work
  result = ( SPI.transfer ( 0xFF ) << 8 ) |        // Read 16 bits data
           ( SPI.transfer ( 0xFF ) ) ;
  await_data_request() ;                           // Wait for DREQ to be HIGH again
  control_mode_off() ;
  return result ;
}

void VS1053::write_register ( uint8_t _reg, uint16_t _value ) const
{
  control_mode_on( );
  SPI.write ( 2 ) ;                                // Write operation
  SPI.write ( _reg ) ;                             // Register to write (0..0xF)
  SPI.write16 ( _value ) ;                         // Send 16 bits data
  await_data_request() ;
  control_mode_off() ;
}

void VS1053::sdi_send_buffer ( uint8_t* data, size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    SPI.writeBytes ( data, chunk_length ) ;
    data += chunk_length ;
  }
  data_mode_off() ;
}

void VS1053::sdi_send_fillers ( size_t len )
{
  size_t chunk_length ;                            // Length of chunk 32 byte or shorter

  data_mode_on() ;
  while ( len )                                    // More to do?
  {
    await_data_request() ;                         // Wait for space available
    chunk_length = len ;
    if ( len > vs1053_chunk_size )
    {
      chunk_length = vs1053_chunk_size ;
    }
    len -= chunk_length ;
    while ( chunk_length-- )
    {
      SPI.write ( endFillByte ) ;
    }
  }
  data_mode_off();
}

void VS1053::wram_write ( uint16_t address, uint16_t data )
{
  write_register ( SCI_WRAMADDR, address ) ;
  write_register ( SCI_WRAM, data ) ;
}

uint16_t VS1053::wram_read ( uint16_t address )
{
  write_register ( SCI_WRAMADDR, address ) ;            // Start reading from WRAM
  return read_register ( SCI_WRAM ) ;                   // Read back result
}

bool VS1053::testComm ( const char *header )
{
  // Test the communication with the VS1053 module.  The result wille be returned.
  // If DREQ is low, there is problably no VS1053 connected.  Pull the line HIGH
  // in order to prevent an endless loop waiting for this signal.  The rest of the
  // software will still work, but readbacks from VS1053 will fail.
  int       i ;                                         // Loop control
  uint16_t  r1, r2, cnt = 0 ;
  uint16_t  delta = 300 ;                               // 3 for fast SPI

  if ( !digitalRead ( dreq_pin ) )
  {
    dbgprint ( "VS1053 not properly installed!" ) ;
    // Allow testing without the VS1053 module
    pinMode ( dreq_pin,  INPUT_PULLUP ) ;               // DREQ is now input with pull-up
    return false ;                                      // Return bad result
  }
  // Further TESTING.  Check if SCI bus can write and read without errors.
  // We will use the volume setting for this.
  // Will give warnings on serial output if DEBUG is active.
  // A maximum of 20 errors will be reported.
  if ( strstr ( header, "Fast" ) )
  {
    delta = 3 ;                                         // Fast SPI, more loops
  }
  dbgprint ( header ) ;                                 // Show a header
  for ( i = 0 ; ( i < 0xFFFF ) && ( cnt < 20 ) ; i += delta )
  {
    write_register ( SCI_VOL, i ) ;                     // Write data to SCI_VOL
    r1 = read_register ( SCI_VOL ) ;                    // Read back for the first time
    r2 = read_register ( SCI_VOL ) ;                    // Read back a second time
    if  ( r1 != r2 || i != r1 || i != r2 )              // Check for 2 equal reads
    {
      dbgprint ( "VS1053 error retry SB:%04X R1:%04X R2:%04X", i, r1, r2 ) ;
      cnt++ ;
      delay ( 10 ) ;
    }
    yield() ;                                           // Allow ESP firmware to do some bookkeeping
  }
  return ( cnt == 0 ) ;                                 // Return the result
}

void VS1053::begin()
{
  pinMode      ( dreq_pin,  INPUT ) ;                   // DREQ is an input
  pinMode      ( cs_pin,    OUTPUT ) ;                  // The SCI and SDI signals
  pinMode      ( dcs_pin,   OUTPUT ) ;
  digitalWrite ( dcs_pin,   HIGH ) ;                    // Start HIGH for SCI en SDI
  digitalWrite ( cs_pin,    HIGH ) ;
  delay ( 100 ) ;
  dbgprint ( "Reset VS1053..." ) ;
  digitalWrite ( dcs_pin,   LOW ) ;                     // Low & Low will bring reset pin low
  digitalWrite ( cs_pin,    LOW ) ;
  delay ( 2000 ) ;
  dbgprint ( "End reset VS1053..." ) ;
  digitalWrite ( dcs_pin,   HIGH ) ;                    // Back to normal again
  digitalWrite ( cs_pin,    HIGH ) ;
  delay ( 500 ) ;
  // Init SPI in slow mode ( 0.2 MHz )
  VS1053_SPI = SPISettings ( 200000, MSBFIRST, SPI_MODE0 ) ;
  //printDetails ( "Right after reset/startup" ) ;
  delay ( 20 ) ;
  //printDetails ( "20 msec after reset" ) ;
  testComm ( "Slow SPI,Testing VS1053 read/write registers..." ) ;
  // Most VS1053 modules will start up in midi mode.  The result is that there is no audio
  // when playing MP3.  You can modify the board, but there is a more elegant way:
  wram_write ( 0xC017, 3 ) ;                            // GPIO DDR = 3
  wram_write ( 0xC019, 0 ) ;                            // GPIO ODATA = 0
  delay ( 100 ) ;
  //printDetails ( "After test loop" ) ;
  softReset() ;                                         // Do a soft reset
  // Switch on the analog parts
  write_register ( SCI_AUDATA, 44100 + 1 ) ;            // 44.1kHz + stereo
  // The next clocksetting allows SPI clocking at 5 MHz, 4 MHz is safe then.
  write_register ( SCI_CLOCKF, 6 << 12 ) ;              // Normal clock settings multiplyer 3.0 = 12.2 MHz
  //SPI Clock to 4 MHz. Now you can set high speed SPI clock.
  VS1053_SPI = SPISettings ( 4000000, MSBFIRST, SPI_MODE0 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_LINE1 ) ) ;
  testComm ( "Fast SPI, Testing VS1053 read/write registers again..." ) ;
  delay ( 10 ) ;
  await_data_request() ;
  endFillByte = wram_read ( 0x1E06 ) & 0xFF ;
  dbgprint ( "endFillByte is %X", endFillByte ) ;
  //printDetails ( "After last clocksetting" ) ;
  delay ( 100 ) ;
}

void VS1053::setVolume ( uint8_t vol )
{
  // Set volume.  Both left and right.
  // Input value is 0..100.  100 is the loudest.
  // Clicking reduced by using 0xf8 to 0x00 as limits.
  uint16_t value ;                                      // Value to send to SCI_VOL

  if ( vol != curvol )
  {
    curvol = vol ;                                      // Save for later use
    value = map ( vol, 0, 100, 0xF8, 0x00 ) ;           // 0..100% to one channel
    value = ( value << 8 ) | value ;
    write_register ( SCI_VOL, value ) ;                 // Volume left and right
  }
}

void VS1053::setTone ( uint8_t *rtone )                 // Set bass/treble (4 nibbles)
{
  // Set tone characteristics.  See documentation for the 4 nibbles.
  uint16_t value = 0 ;                                  // Value to send to SCI_BASS
  int      i ;                                          // Loop control

  for ( i = 0 ; i < 4 ; i++ )
  {
    value = ( value << 4 ) | rtone[i] ;                 // Shift next nibble in
  }
  write_register ( SCI_BASS, value ) ;                  // Tone settings
  value = read_register ( SCI_BASS ) ;                  // Read back
  dbgprint ( "BASS settings is %04X", value ) ;         // Print for TEST
}

uint8_t VS1053::getVolume()                             // Get the currenet volume setting.
{
  return curvol ;
}

void VS1053::startSong()
{
  sdi_send_fillers ( 10 ) ;
}

void VS1053::playChunk ( uint8_t* data, size_t len )
{
  sdi_send_buffer ( data, len ) ;
}

void VS1053::stopSong()
{
  uint16_t modereg ;                     // Read from mode register
  int      i ;                           // Loop control

  sdi_send_fillers ( 2052 ) ;
  delay ( 10 ) ;
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_CANCEL ) ) ;
  for ( i = 0 ; i < 200 ; i++ )
  {
    sdi_send_fillers ( 32 ) ;
    modereg = read_register ( SCI_MODE ) ;  // Read status
    if ( ( modereg & _BV ( SM_CANCEL ) ) == 0 )
    {
      sdi_send_fillers ( 2052 ) ;
      dbgprint ( "Song stopped correctly after %d msec", i * 10 ) ;
      return ;
    }
    delay ( 10 ) ;
  }
  printDetails ( "Song stopped incorrectly!" ) ;
}

void VS1053::softReset()
{
  write_register ( SCI_MODE, _BV ( SM_SDINEW ) | _BV ( SM_RESET ) ) ;
  delay ( 10 ) ;
  await_data_request() ;
}

void VS1053::printDetails ( const char *header )
{
  uint16_t     regbuf[16] ;
  uint8_t      i ;

  dbgprint ( header ) ;
  dbgprint ( "REG   Contents" ) ;
  dbgprint ( "---   -----" ) ;
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    regbuf[i] = read_register ( i ) ;
  }
  for ( i = 0 ; i <= SCI_num_registers ; i++ )
  {
    delay ( 5 ) ;
    dbgprint ( "%3X - %5X", i, regbuf[i] ) ;
  }
}

void VS1053::AdjustRate ( long ppm2 )
{
  write_register ( SCI_WRAMADDR, 0x1e07 ) ;
  write_register ( SCI_WRAM,     ppm2 ) ;
  write_register ( SCI_WRAM,     ppm2 >> 16 ) ;
  // oldClock4KHz = 0 forces  adjustment calculation when rate checked.
  write_register ( SCI_WRAMADDR, 0x5b1c ) ;
  write_register ( SCI_WRAM,     0 ) ;
  // Write to AUDATA or CLOCKF checks rate and recalculates adjustment.
  write_register ( SCI_AUDATA,   read_register ( SCI_AUDATA ) ) ;
}


// The object for the MP3 player
VS1053 vs1053player (  VS1053_CS, VS1053_DCS, VS1053_DREQ ) ;

//******************************************************************************************
// End VS1053 stuff.                                                                       *
//******************************************************************************************



//******************************************************************************************
// Ringbuffer (fifo) routines.                                                             *
//******************************************************************************************
//******************************************************************************************
//                              R I N G S P A C E                                          *
//******************************************************************************************
inline bool ringspace()
{
  #ifdef SPIRAM
    return spaceAvailable() ;         // True if at least 1 chunk available
  #else
    return ( rcount < RINGBFSIZ ) ;   // True is at least one byte of free space is available
  #endif
}


//******************************************************************************************
//                              R I N G A V A I L                                          *
//******************************************************************************************
inline uint16_t ringavail()
{
  #ifdef SPIRAM
    return dataAvailable() ;          // Return number of chunks filled
  #else
    return rcount ;                     // Return number of bytes available
  #endif
}


//******************************************************************************************
//                                P U T R I N G                                            *
//******************************************************************************************
// No check on available space.  See ringspace()                                           *
//******************************************************************************************
void putring ( uint8_t b )              // Put one byte in the ringbuffer
{

  #ifdef SPIRAM
    static uint8_t pwchunk[32] ;        // Buffer for one chunk
    pwchunk[prcwinx++] = b ;            // Store in local chunk
    if ( prcwinx == sizeof(pwchunk) )   // Chunk full?
    {
      bufferWrite ( pwchunk ) ;         // Yes, store in SPI RAM
      prcwinx = 0 ;                     // Index to begin of chunk
    }
  #else
    *(ringbuf + rbwindex) = b ;         // Put byte in ringbuffer
    if ( ++rbwindex == RINGBFSIZ )      // Increment pointer and
    {
      rbwindex = 0 ;                    // wrap at end
    }
    rcount++ ;                          // Count number of bytes in the
#endif
}


//******************************************************************************************
//                                G E T R I N G                                            *
//******************************************************************************************
// Assume there is always something in the bufferpace.  See ringavail().                   *
//******************************************************************************************
uint8_t getring()
{
  #ifdef SPIRAM
    static uint8_t prchunk[32] ;          // Buffer for one chunk
    if ( prcrinx >= sizeof(prchunk) )     // End of buffer reached?
    {
      prcrinx = 0 ;                       // Yes, reset index to begin of buffer
      bufferRead ( prchunk ) ;            // And read new buffer
    }
    return ( prchunk[prcrinx++] ) ;
  #else
    if ( ++rbrindex == RINGBFSIZ )        // Increment pointer and
    {
      rbrindex = 0 ;                      // wrap at end
    }
    rcount-- ;                            // Count is now one less
    return *(ringbuf + rbrindex) ;        // return the oldest byte
  #endif
}


//******************************************************************************************
//                               E M P T Y R I N G                                         *
//******************************************************************************************
void emptyring()
{
  rbwindex = 0 ;                      // Reset ringbuffer administration
  rbrindex = RINGBFSIZ - 1 ;
  rcount = 0 ;
  prcwinx = 0 ;
  prcrinx = 32 ;                       // Set buffer to empty
}


//******************************************************************************************
//                              U T F 8 A S C I I                                          *
//******************************************************************************************
// UTF8-Decoder: convert UTF8-string to extended ASCII.                                    *
// Convert a single Character from UTF8 to Extended ASCII.                                 *
// Return "0" if a byte has to be ignored.                                                 *
//******************************************************************************************
byte utf8ascii ( byte ascii )
{
  static const byte lut_C3[] = 
         { "AAAAAAACEEEEIIIIDNOOOOO#0UUUU###aaaaaaaceeeeiiiidnooooo##uuuuyyy" } ;
  static byte       c1 ;              // Last character buffer
  byte              res = 0 ;         // Result, default 0

  if ( ascii <= 0x7F )                // Standard ASCII-set 0..0x7F handling
  {
    c1 = 0 ;
    res = ascii ;                     // Return unmodified
  }
  else
  {
    switch ( c1 )                     // Conversion depending on first UTF8-character
    {   
      case 0xC2: res = '~' ;
                 break ;
      case 0xC3: res = lut_C3[ascii-128] ;
                 break ;
      case 0x82: if ( ascii == 0xAC )
                 {
                    res = 'E' ;       // Special case Euro-symbol
                 }
    }
    c1 = ascii ;                      // Remember actual character
  }
  return res ;                        // Otherwise: return zero, if character has to be ignored
}


//******************************************************************************************
//                              U T F 8 A S C I I                                          *
//******************************************************************************************
// In Place conversion UTF8-string to Extended ASCII (ASCII is shorter!).                  *
//******************************************************************************************
void utf8ascii ( char* s )
{
  int  i, k = 0 ;                     // Indexes for in en out string
  char c ;

  for ( i = 0 ; s[i] ; i++ )          // For every input character
  {
    c = utf8ascii ( s[i] ) ;          // Translate if necessary
    if ( c )                          // Good translation?
    {
      s[k++] = c ;                    // Yes, put in output string
    }
  }
  s[k] = 0 ;                          // Take care of delimeter
}


//******************************************************************************************
//                                  D B G P R I N T                                        *
//******************************************************************************************
// Send a line of info to serial output.  Works like vsprintf(), but checks the BEDUg flag.*
// Print only if DEBUG flag is true.  Always returns the the formatted string.             *
//******************************************************************************************
char* dbgprint ( const char* format, ... )
{
  static char sbuf[DEBUG_BUFFER_SIZE] ;                // For debug lines
  va_list varArgs ;                                    // For variable number of params

  va_start ( varArgs, format ) ;                       // Prepare parameters
  vsnprintf ( sbuf, sizeof(sbuf), format, varArgs ) ;  // Format the message
  va_end ( varArgs ) ;                                 // End of using parameters
  if ( DEBUG )                                         // DEBUG on?
  {
    Serial.print ( "D: " ) ;                           // Yes, print prefix
    Serial.println ( sbuf ) ;                          // and the info
  }
  return sbuf ;                                        // Return stored string
}


//******************************************************************************************
//                             G E T E N C R Y P T I O N T Y P E                           *
//******************************************************************************************
// Read the encryption type of the network and return as a 4 byte name                     *
//*********************4********************************************************************
const char* getEncryptionType ( int thisType )
{
  switch (thisType)
  {
    case ENC_TYPE_WEP:
      return "WEP " ;
    case ENC_TYPE_TKIP:
      return "WPA " ;
    case ENC_TYPE_CCMP:
      return "WPA2" ;
    case ENC_TYPE_NONE:
      return "None" ;
    case ENC_TYPE_AUTO:
      return "Auto" ;
  }
  return "????" ;
}


//******************************************************************************************
//                                L I S T N E T W O R K S                                  *
//******************************************************************************************
// List the available networks and select the strongest.                                   *
// Acceptable networks are those who have an entry in "anetworks".                         *
// SSIDs of available networks will be saved for use in webinterface.                      *
//******************************************************************************************
void listNetworks()
{
  int         maxsig = -1000 ;   // Used for searching strongest WiFi signal
  int         newstrength ;
  byte        encryption ;       // TKIP(WPA)=2, WEP=5, CCMP(WPA)=4, NONE=7, AUTO=8
  const char* acceptable ;       // Netwerk is acceptable for connection
  int         i ;                // Loop control
  String      sassid ;           // Search string in anetworks

  ini_block.ssid = String ( "none" ) ;                   // No selceted network yet
  // scan for nearby networks:
  dbgprint ( "* Scan Networks *" ) ;
  int numSsid = WiFi.scanNetworks() ;
  if ( numSsid == -1 )
  {
    dbgprint ( "Couldn't get a wifi connection" ) ;
    return ;
  }
  // print the list of networks seen:
  dbgprint ( "Number of available networks: %d",
             numSsid ) ;
  // Print the network number and name for each network found and
  // find the strongest acceptable network
  for ( i = 0 ; i < numSsid ; i++ )
  {
    acceptable = "" ;                                    // Assume not acceptable
    newstrength = WiFi.RSSI ( i ) ;                      // Get the signal strenght
    sassid = WiFi.SSID ( i ) + String ( "|" ) ;          // For search string
    if ( anetworks.indexOf ( sassid ) >= 0 )             // Is this SSID acceptable?
    {
      acceptable = "Acceptable" ;
      if ( newstrength > maxsig )                        // This is a better Wifi
      {
        maxsig = newstrength ;
        ini_block.ssid = WiFi.SSID ( i ) ;               // Remember SSID name
      }
    }
    encryption = WiFi.encryptionType ( i ) ;
    dbgprint ( "%2d - %-25s Signal: %3d dBm Encryption %4s  %s",
               i + 1, WiFi.SSID ( i ).c_str(), WiFi.RSSI ( i ),
               getEncryptionType ( encryption ),
               acceptable ) ;
    // Remember this network for later use
    networks += WiFi.SSID ( i ) + String ( "|" ) ;
  }
  dbgprint ( "--------------------------------------" ) ;
}


//******************************************************************************************
//                                  T I M E R 1 0 S E C                                    *
//******************************************************************************************
// Extra watchdog.  Called every 10 seconds.                                               *
// If totalcount has not been changed, there is a problem and playing will stop.           *
// Note that a "yield()" within this routine or in called functions will cause a crash!    *
//******************************************************************************************
void timer10sec()
{
  static uint32_t oldtotalcount = 7321 ;          // Needed foor change detection
  static uint8_t  morethanonce = 0 ;              // Counter for succesive fails
  static uint8_t  t600 = 0 ;                      // Counter for 10 minutes

  if ( datamode & ( INIT | HEADER | DATA |        // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    if ( totalcount == oldtotalcount )            // Still playing?
    {
      dbgprint ( "No data input" ) ;              // No data detected!
      if ( morethanonce > 10 )                    // Happened too many times?
      {
        dbgprint ( "Going to restart..." ) ;
        ESP.restart() ;                           // Reset the CPU, probably no return
      }
      if ( datamode & ( PLAYLISTDATA |            // In playlist mode?
                        PLAYLISTINIT |
                        PLAYLISTHEADER ) )
      {
        playlist_num = 0 ;                        // Yes, end of playlist
      }
      if ( ( morethanonce > 0 ) ||                // Happened more than once?
           ( playlist_num > 0 ) )                 // Or playlist active?
      {
        datamode = STOPREQD ;                     // Stop player
        ini_block.newpreset++ ;                   // Yes, try next channel
        dbgprint ( "Trying other station/file..." ) ;
      }
      morethanonce++ ;                            // Count the fails
    }
    else
    {
      if ( morethanonce )                         // Recovered from data loss?
      {
        dbgprint ( "Recovered from dataloss" ) ;
        morethanonce = 0 ;                        // Data see, reset failcounter
      }
      oldtotalcount = totalcount ;                // Save for comparison in next cycle
    }
    if ( t600++ == 60 )                           // 10 minutes over?
    {
      t600 = 0 ;                                  // Yes, reset counter
      dbgprint ( "10 minutes over" ) ;
      publishIP() ;                               // Re-publish IP
    }
  }
}


//******************************************************************************************
//                                  A N A G E T S W                                        *
//******************************************************************************************
// Translate analog input to switch number.  0 is inactive.                                *
// Note that it is adviced to avoid expressions as the argument for the abs function.      *
//******************************************************************************************
uint8_t anagetsw ( uint16_t v )
{
  int      i ;                                    // Loop control
  int      oldmindist = 1000 ;                    // Detection least difference
  int      newdist ;                              // New found difference
  uint8_t  sw = 0 ;                               // Number of switch detected (0 or 1..3)

  if ( v > analogrest )                           // Inactive level?
  {
    for ( i = 0 ; i < NUMANA ; i++ )
    {
      newdist = analogsw[i] - v ;                  // Compute difference
      newdist = abs ( newdist ) ;                  // Make it absolute
      if ( newdist < oldmindist )                  // New least difference?
      {
        oldmindist = newdist ;                     // Yes, remember
        sw = i + 1 ;                               // Remember switch
      }
    }
  }
  return sw ;                                      // Return active switch
}


//******************************************************************************************
//                               T E S T F I L E                                           *
//******************************************************************************************
// Test the performance of SPIFFS read.                                                    *
//******************************************************************************************
void testfile ( String fspec )
{
  String   path ;                                      // Full file spec
  File     tfile ;                                     // File containing mp3
  uint32_t len, savlen ;                               // File length
  uint32_t t0, t1, told ;                              // For time test
  uint32_t t_error = 0 ;                               // Number of slow reads

  dbgprint ( "Start test of file %s", fspec.c_str() ) ;
  t0 = millis() ;                                      // Timestamp at start
  t1 = t0 ;                                            // Prevent uninitialized value
  told = t0 ;                                          // For report
  path = String ( "/" ) + fspec ;                      // Form full path
  tfile = LittleFS.open ( path, "r" ) ;                // Open the file
  if ( tfile )
  {
    len = tfile.available() ;                          // Get file length
    savlen = len ;                                     // Save for result print
    while ( len-- )                                    // Any data left?
    {
      t1 = millis() ;                                  // To meassure read time
      tfile.read() ;                                   // Read one byte
      if ( ( millis() - t1 ) > 5 )                     // Read took more than 5 msec?
      {
        t_error++ ;                                    // Yes, count slow reads
      }
      if ( ( len % 100 ) == 0 )                        // Yield reguarly
      {
        yield() ;
      }
      if ( ( ( t1 - told ) / 1000 ) > 0 || len == 0 )
      {
        // Show results for debug
        dbgprint ( "Read %s, length %d/%d took %d seconds, %d slow reads",
                   fspec.c_str(), savlen - len, savlen, ( t1 - t0 ) / 1000, t_error ) ;
        told = t1 ;
      }
      if ( ( t1 - t0 ) > 100000 )                      // Give up after 100 seconds
      {
        dbgprint ( "Give up..." ) ;
        break ;
      }
    }
    tfile.close() ;
    dbgprint ( "EOF" ) ;                               // End of file
  }
}


//******************************************************************************************
//                                  T I M E R 1 0 0                                        *
//******************************************************************************************
// Examine button every 100 msec.                                                          *
//******************************************************************************************
void timer100()
{
  static int     count10sec = 0 ;                 // Counter for activatie 10 seconds process
  static int     oldval2 = HIGH ;                 // Previous value of digital input button 2
#if ( not ( defined ( USETFT ) ) )
  static int     oldval1 = HIGH ;                 // Previous value of digital input button 1
  static int     oldval3 = HIGH ;                 // Previous value of digital input button 3
#endif
  int            newval ;                         // New value of digital input switch
  uint16_t       v ;                              // Analog input value 0..1023
  static uint8_t aoldval = 0 ;                    // Previous value of analog input switch
  uint8_t        anewval ;                        // New value of analog input switch (0..3)

  if ( ++count10sec == 100  )                     // 10 seconds passed?
  {
    timer10sec() ;                                // Yes, do 10 second procedure
    count10sec = 0 ;                              // Reset count
  }
  else
  {
    newval = digitalRead ( BUTTON2 ) ;            // Test if below certain level
    if ( newval != oldval2 )                      // Change?
    {
      oldval2 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = currentpreset + 1 ; // Yes, goto next preset station
        //dbgprint ( "Digital button 2 pushed" ) ;
      }
      return ;
    }
#if ( not ( defined ( USETFT ) ) )
    newval = digitalRead ( BUTTON1 ) ;            // Test if below certain level
    if ( newval != oldval1 )                      // Change?
    {
      oldval1 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = 0 ;                 // Yes, goto first preset station
        //dbgprint ( "Digital button 1 pushed" ) ;
      }
      return ;
    }
    // Note that BUTTON3 has inverted input
    newval = digitalRead ( BUTTON3 ) ;            // Test if below certain level
    newval = HIGH + LOW - newval ;                // Reverse polarity
    if ( newval != oldval3 )                      // Change?
    {
      oldval3 = newval ;                          // Yes, remember value
      if ( newval == LOW )                        // Button pushed?
      {
        ini_block.newpreset = currentpreset - 1 ; // Yes, goto previous preset station
        //dbgprint ( "Digital button 3 pushed" ) ;
      }
      return ;
    }
#endif
    v = analogRead ( A0 ) ;                       // Read analog value
    anewval = anagetsw ( v ) ;                    // Check analog value for program switches
    if ( anewval != aoldval )                     // Change?
    {
      aoldval = anewval ;                         // Remember value for change detection
      if ( anewval != 0 )                         // Button pushed?
      {
        //dbgprint ( "Analog button %d pushed, v = %d", anewval, v ) ;
        if ( anewval == 1 )                       // Button 1?
        {
          ini_block.newpreset = 0 ;               // Yes, goto first preset
        }
        else if ( anewval == 2 )                  // Button 2?
        {
          ini_block.newpreset = currentpreset + 1 ; // Yes, goto next preset
        }
        else if ( anewval == 3 )                  // Button 3?
        {
          ini_block.newpreset = currentpreset - 1 ; // Yes, goto previous preset
        }
      }
    }
  }
}


//******************************************************************************************
//                              D I S P L A Y V O L U M E                                  *
//******************************************************************************************
// Show the current volume as an indicator on the screen.                                  *
//******************************************************************************************
void displayvolume()
{
#if defined ( USETFT )
  static uint8_t oldvol = 0 ;                        // Previous volume
  uint8_t pos ;                                      // Positon of volume indicator

  if ( vs1053player.getVolume() != oldvol )
  {
    pos = map ( vs1053player.getVolume(), 0, 100, 0, 160 ) ;
    tft.fillRect ( 0, 126, pos, 2, RED ) ;             // Paint red part
    tft.fillRect ( pos, 126, 160 - pos, 2, GREEN ) ;   // Paint green part
  }
#endif
}


//******************************************************************************************
//                              D I S P L A Y I N F O                                      *
//******************************************************************************************
// Show a string on the LCD at a specified y-position in a specified color                 *
//******************************************************************************************
#if defined ( USETFT )
void displayinfo ( const char* str, uint16_t pos, uint16_t height, uint16_t color )
{
  char buf [ strlen ( str ) + 1 ] ;             // Need some buffer space
  
  strcpy ( buf, str ) ;                         // Make a local copy of the string
  utf8ascii ( buf ) ;                           // Convert possible UTF8
  tft.fillRect ( 0, pos, 160, height, BLACK ) ; // Clear the space for new info
  tft.setTextColor ( color ) ;                  // Set the requested color
  tft.setCursor ( 0, pos ) ;                    // Prepare to show the info
  tft.println ( buf ) ;                         // Show the string
}
#else
#define displayinfo(a,b,c,d)                    // Empty declaration
#endif


//******************************************************************************************
//                        S H O W S T R E A M T I T L E                                    *
//******************************************************************************************
// Show artist and songtitle if present in metadata.                                       *
// Show always if full=true.                                                               *
//******************************************************************************************
void showstreamtitle ( const char *ml, bool full )
{
  char*             p1 ;
  char*             p2 ;
  char              streamtitle[150] ;           // Streamtitle from metadata

  if ( strstr ( ml, "StreamTitle=" ) )
  {
    dbgprint ( "Streamtitle found, %d bytes", strlen ( ml ) ) ;
    dbgprint ( ml ) ;
    p1 = (char*)ml + 12 ;                       // Begin of artist and title
    if ( ( p2 = strstr ( ml, ";" ) ) )          // Search for end of title
    {
      if ( *p1 == '\'' )                        // Surrounded by quotes?
      {
        p1++ ;
        p2-- ;
      }
      *p2 = '\0' ;                              // Strip the rest of the line
    }
    // Save last part of string as streamtitle.  Protect against buffer overflow
    strncpy ( streamtitle, p1, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else if ( full )
  {
    // Info probably from playlist
    strncpy ( streamtitle, ml, sizeof ( streamtitle ) ) ;
    streamtitle[sizeof ( streamtitle ) - 1] = '\0' ;
  }
  else
  {
    icystreamtitle = "" ;                       // Unknown type
    return ;                                    // Do not show
  }
  // Save for status request from browser ;
  icystreamtitle = streamtitle ;
  if ( ( p1 = strstr ( streamtitle, " - " ) ) ) // look for artist/title separator
  {
    *p1++ = '\n' ;                              // Found: replace 3 characters by newline
    p2 = p1 + 2 ;
    if ( *p2 == ' ' )                           // Leading space in title?
    {
      p2++ ;
    }
    strcpy ( p1, p2 ) ;                         // Shift 2nd part of title 2 or 3 places
  }
  displayinfo ( streamtitle, 20, 40, CYAN ) ;   // Show title at position 20
}


//******************************************************************************************
//                            S T O P _ M P 3 C L I E N T                                  *
//******************************************************************************************
// Disconnect from the server.                                                             *
//******************************************************************************************
void stop_mp3client ()
{
  if ( mp3client )
  {
    if ( mp3client->connected() )                    // Need to stop client?
    {
      dbgprint ( "Stopping client" ) ;               // Stop connection to host
      mp3client->flush() ;
      mp3client->stop() ;
      delay ( 500 ) ;
    }
    delete ( mp3client ) ;
    mp3client = NULL ;
  }
}


//******************************************************************************************
//                            C O N N E C T T O H O S T                                    *
//******************************************************************************************
// Connect to the Internet radio server specified by newpreset.                            *
//******************************************************************************************
bool connecttohost()
{
  int         inx ;                                 // Position of ":" in hostname
  char*       pfs ;                                 // Pointer to formatted string
  int         port = 80 ;                           // Port number for host
  String      extension = "/" ;                     // May be like "/mp3" in "skonto.ls.lv:8002/mp3"
  String      hostwoext ;                           // Host without extension and portnumber

  stop_mp3client() ;                                // Disconnect if still connected
  dbgprint ( "Connect to new host %s", host.c_str() ) ;
  displayinfo ( "   ** Internet radio **", 0, 20, WHITE ) ;
  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  if ( host.endsWith ( ".m3u" ) )                   // Is it an m3u playlist?
  {
    playlist = host ;                               // Save copy of playlist URL
    datamode = PLAYLISTINIT ;                       // Yes, start in PLAYLIST mode
    if ( playlist_num == 0 )                        // First entry to play?
    {
      playlist_num = 1 ;                            // Yes, set index
    }
    dbgprint ( "Playlist request, entry %d", playlist_num ) ;
  }
  // In the URL there may be an extension
  inx = host.indexOf ( "/" ) ;                      // Search for begin of extension
  if ( inx > 0 )                                    // Is there an extension?
  {
    extension = host.substring ( inx ) ;            // Yes, change the default
    hostwoext = host.substring ( 0, inx ) ;         // Host without extension
  }
  // In the URL there may be a portnumber
  inx = host.indexOf ( ":" ) ;                      // Search for separator
  if ( inx >= 0 )                                   // Portnumber available?
  {
    port = host.substring ( inx + 1 ).toInt() ;     // Get portnumber as integer
    hostwoext = host.substring ( 0, inx ) ;         // Host without portnumber
  }
  pfs = dbgprint ( "Connect to %s on port %d, extension %s",
                   hostwoext.c_str(), port, extension.c_str() ) ;
  displayinfo ( pfs, 60, 66, YELLOW ) ;             // Show info at position 60..125
  mp3client = new WiFiClient() ;
  if ( mp3client->connect ( hostwoext.c_str(), port ) )
  {
    dbgprint ( "Connected to server" ) ;
    // This will send the request to the server. Request metadata.
    mp3client->print ( String ( "GET " ) +
                       extension +
                      String ( " HTTP/1.1\r\n" ) +
                      String ( "Host: " ) +
                      hostwoext +
                      String ( "\r\n" ) +
                      String ( "Icy-MetaData:1\r\n" ) +
                      String ( "Connection: close\r\n\r\n" ) ) ;
    return true ;
  }
  dbgprint ( "Request %s failed!", host.c_str() ) ;
  return false ;
}


//******************************************************************************************
//                               C O N N E C T T O F I L E                                 *
//******************************************************************************************
// Open the local mp3-file.                                                                *
//******************************************************************************************
bool connecttofile()
{
  String path ;                                           // Full file spec
  char*  p ;                                              // Pointer to filename

  displayinfo ( "   **** MP3 Player ****", 0, 20, WHITE ) ;
  path = host.substring ( 9 ) ;                           // Path, skip the "localhost" part
  mp3file = LittleFS.open ( path, "r" ) ;                 // Open the file
  if ( !mp3file )
  {
    dbgprint ( "Error opening file %s", path.c_str() ) ;  // No luck
    return false ;
  }
  p = (char*)path.c_str() + 1 ;                           // Point to filename
  showstreamtitle ( p, true ) ;                           // Show the filename as title
  displayinfo ( "Playing from local file",
                60, 68, YELLOW ) ;                        // Show Source at position 60
  icyname = "" ;                                          // No icy name yet
  chunked = false ;                                       // File not chunked
  return true ;
}


//******************************************************************************************
//                               C O N N E C T W I F I                                     *
//******************************************************************************************
// Connect to WiFi using passwords available in the SPIFFS.                                *
// If connection fails, an AP is created and the function returns false.                   *
//******************************************************************************************
bool connectwifi()
{
  char*  pfs ;                                         // Pointer to formatted string

  WiFi.disconnect() ;                                  // After restart the router could
  WiFi.softAPdisconnect(true) ;                        // still keep the old connection
  WiFi.begin ( ini_block.ssid.c_str(),
               ini_block.passwd.c_str() ) ;            // Connect to selected SSID
  dbgprint ( "Try WiFi %s", ini_block.ssid.c_str() ) ; // Message to show during WiFi connect
  if (  WiFi.waitForConnectResult() != WL_CONNECTED )  // Try to connect
  {
    dbgprint ( "WiFi Failed!  Trying to setup AP with name %s and password %s.", NAME, NAME ) ;
    WiFi.softAP ( NAME, NAME ) ;                       // This ESP will be an AP
    delay ( 5000 ) ;
    pfs = dbgprint ( "IP = 192.168.4.1" ) ;            // Address if AP
    return false ;
  }
  pfs = dbgprint ( "IP = %d.%d.%d.%d",
                   WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3] ) ;
#if defined ( USETFT )
  tft.println ( pfs ) ;
#endif
  return true ;
}


//******************************************************************************************
//                                   O T A S T A R T                                       *
//******************************************************************************************
// Update via WiFi has been started by Arduino IDE.                                        *
//******************************************************************************************
void otastart()
{
  dbgprint ( "OTA Started" ) ;
}


//******************************************************************************************
//                          R E A D H O S T F R O M I N I F I L E                          *
//******************************************************************************************
// Read the mp3 host from the ini-file specified by the parameter.                         *
// The host will be returned.                                                              *
//******************************************************************************************
String readhostfrominifile ( int8_t preset )
{
  String      path ;                                   // Full file spec as string
  File        inifile ;                                // File containing URL with mp3
  char        tkey[12] ;                               // Key as an array of chars
  String      line ;                                   // Input line from .ini file
  String      linelc ;                                 // Same, but lowercase
  int         inx ;                                    // Position within string
  String      res = "" ;                               // Assume not found

  path = String ( INIFILENAME ) ;                      // Form full path
  inifile = LittleFS.open ( path, "r" ) ;              // Open the file
  if ( inifile )
  {
    sprintf ( tkey, "preset_%02d", preset ) ;           // Form the search key
    while ( inifile.available() )
    {
      line = inifile.readStringUntil ( '\n' ) ;        // Read next line
      linelc = line ;                                  // Copy for lowercase
      linelc.toLowerCase() ;                           // Set to lowercase
      if ( linelc.startsWith ( tkey ) )                // Found the key?
      {
        inx = line.indexOf ( "=" ) ;                   // Get position of "="
        if ( inx > 0 )                                 // Equal sign present?
        {
          line.remove ( 0, inx + 1 ) ;                 // Yes, remove key
          res = chomp ( line ) ;                       // Remove garbage
          break ;                                      // End the while loop
        }
      }
    }
    inifile.close() ;                                  // Close the file
  }
  else
  {
    dbgprint ( "File %s not found, please create one!", INIFILENAME ) ;
  }
  return res ;
}


//******************************************************************************************
//                               R E A D I N I F I L E                                     *
//******************************************************************************************
// Read the .ini file and interpret the commands.                                          *
//******************************************************************************************
void readinifile()
{
  String      path ;                                   // Full file spec as string
  File        inifile ;                                // File containing URL with mp3
  String      line ;                                   // Input line from .ini file

  path = String ( INIFILENAME ) ;                      // Form full path
  inifile = LittleFS.open ( path, "r" ) ;              // Open the file
  if ( inifile )
  {
    while ( inifile.available() )
    {
      line = inifile.readStringUntil ( '\n' ) ;        // Read next line
      analyzeCmd ( line.c_str() ) ;
    }
    inifile.close() ;                                  // Close the file
  }
  else
  {
    dbgprint ( "File %s not found, use save command to create one!", INIFILENAME ) ;
  }
}


//******************************************************************************************
//                            P U B L I S H I P                                            *
//******************************************************************************************
// Publish IP to MQTT broker.                                                              *
//******************************************************************************************
void publishIP()
{
  IPAddress ip ;
  char      ipstr[20] ;                          // Hold IP as string

  if ( ini_block.mqttpubtopic.length() )        // Topic to publish?
  {
    ip = WiFi.localIP() ;
    // Publish IP-adress.  qos=1, retain=true
    sprintf ( ipstr, "%d.%d.%d.%d",
              ip[0], ip[1], ip[2], ip[3] ) ;
    mqttclient.publish ( ini_block.mqttpubtopic.c_str(), 1, true, ipstr ) ;
    dbgprint ( "Publishing IP %s to topic %s",
               ipstr, ini_block.mqttpubtopic.c_str() ) ;
  }
}


//******************************************************************************************
//                            O N M Q T T C O N N E C T                                    *
//******************************************************************************************
// Will be called on connection to the broker.  Subscribe to our topic and publish a topic.*
//******************************************************************************************
void onMqttConnect( bool sessionPresent )
{
  uint16_t    packetIdSub ;
  const char* present = "is" ;                      // Assume Session is present

  if ( !sessionPresent )
  {
    present = "is not" ;                            // Session is NOT present
  }
  dbgprint ( "MQTT Connected to the broker %s, session %s present",
             ini_block.mqttbroker.c_str(), present ) ;
  packetIdSub = mqttclient.subscribe ( ini_block.mqtttopic.c_str(), 2 ) ;
  dbgprint ( "Subscribing to %s at QoS 2, packetId = %d ",
             ini_block.mqtttopic.c_str(),
             packetIdSub ) ;
  publishIP() ;                                     // Topic to publish: IP
}


//******************************************************************************************
//                      O N M Q T T D I S C O N N E C T                                    *
//******************************************************************************************
// Will be called on disconnect.                                                           *
//******************************************************************************************
void onMqttDisconnect ( AsyncMqttClientDisconnectReason reason )
{
  dbgprint ( "MQTT Disconnected from the broker, reason %d, reconnecting...",
             reason ) ;
  if ( mqttcount < MAXMQTTCONNECTS )            // Try again?
  {
    mqttcount++ ;                               // Yes, count number of tries
    mqttclient.connect() ;                      // Reconnect
  }
}


//******************************************************************************************
//                      O N M Q T T S U B S C R I B E                                      *
//******************************************************************************************
// Will be called after a successful subscribe.                                            *
//******************************************************************************************
void onMqttSubscribe ( uint16_t packetId, uint8_t qos )
{
  dbgprint ( "MQTT Subscribe acknowledged, packetId = %d, QoS = %d",
             packetId, qos ) ;
}


//******************************************************************************************
//                              O N M Q T T U N S U B S C R I B E                          *
//******************************************************************************************
// Will be executed if this program unsubscribes from a topic.                             *
// Not used at the moment.                                                                 *
//******************************************************************************************
void onMqttUnsubscribe ( uint16_t packetId )
{
  dbgprint ( "MQTT Unsubscribe acknowledged, packetId = %d",
             packetId ) ;
}


//******************************************************************************************
//                            O N M Q T T M E S S A G E                                    *
//******************************************************************************************
// Executed when a subscribed message is received.                                         *
// Note that message is not delimited by a '\0'.                                           *
//******************************************************************************************
void onMqttMessage ( char* topic, char* payload, AsyncMqttClientMessageProperties properties,
                     size_t len, size_t index, size_t total )
{
  char*  reply ;                                    // Result from analyzeCmd

  // Available properties.qos, properties.dup, properties.retain
  if ( len >= sizeof(cmd) )                         // Message may not be too long
  {
    len = sizeof(cmd) - 1 ;
  }
  strncpy ( cmd, payload, len ) ;                   // Make copy of message
  cmd[len] = '\0' ;                                 // Take care of delimeter
  dbgprint ( "MQTT message arrived [%s], lenght = %d, %s", topic, len, cmd ) ;
  reply = analyzeCmd ( cmd ) ;                      // Analyze command and handle it
  dbgprint ( reply ) ;                              // Result for debugging
}


//******************************************************************************************
//                             O N M Q T T P U B L I S H                                   *
//******************************************************************************************
// Will be executed if a message is published by this program.                             *
// Not used at the moment.                                                                 *
//******************************************************************************************
void onMqttPublish ( uint16_t packetId )
{
  dbgprint ( "MQTT Publish acknowledged, packetId = %d",
             packetId ) ;
}


//******************************************************************************************
//                             S C A N S E R I A L                                         *
//******************************************************************************************
// Listen to commands on the Serial inputline.                                             *
//******************************************************************************************
void scanserial()
{
  static String serialcmd ;                      // Command from Serial input
  char          c ;                              // Input character
  char*         reply ;                          // Reply string froma analyzeCmd
  uint16_t      len ;                            // Length of input string

  while ( Serial.available() )                   // Any input seen?
  {
    c =  (char)Serial.read() ;                   // Yes, read the next input character
    //Serial.write ( c ) ;                       // Echo
    len = serialcmd.length() ;                   // Get the length of the current string
    if ( ( c == '\n' ) || ( c == '\r' ) )
    {
      if ( len )
      {
        strncpy ( cmd, serialcmd.c_str(), sizeof(cmd) ) ;
        reply = analyzeCmd ( cmd) ;              // Analyze command and handle it
        dbgprint ( reply ) ;                     // Result for debugging
        serialcmd = "" ;                         // Prepare for new command
      }
    }
    if ( c >= ' ' )                              // Only accept useful characters
    {
      serialcmd += c ;                           // Add to the command
    }
    if ( len >= ( sizeof(cmd) - 2 )  )           // Check for excessive length
    {
      serialcmd = "" ;                           // Too long, reset
    }
  }
}


//******************************************************************************************
//                                   M K _ L S A N                                         *
//******************************************************************************************
// Make al list of acceptable networks in .ini file.                                       *
// The result will be stored in anetworks like "|SSID1|SSID2|......|SSIDN|".               *
// The number of acceptable networks will be stored in num_an.                             *
//******************************************************************************************
void mk_lsan()
{
  String      path ;                                   // Full file spec as string
  File        inifile ;                                // File containing URL with mp3
  String      line ;                                   // Input line from .ini file
  String      ssid ;                                   // SSID in line
  int         inx ;                                    // Place of "/"

  num_an = 0 ;                                         // Count acceptable networks
  anetworks = "|" ;                                    // Initial value
  path = String ( INIFILENAME ) ;                      // Form full path
  inifile = LittleFS.open ( path, "r" ) ;              // Open the file
  if ( inifile )
  {
    while ( inifile.available() )
    {
      line = inifile.readStringUntil ( '\n' ) ;        // Read next line
      ssid = line ;                                    // Copy holds original upper/lower case
      line.toLowerCase() ;                             // Case insensitive
      if ( line.startsWith ( "wifi" ) )                // Line with WiFi spec?
      {
        inx = line.indexOf ( "/" ) ;                   // Find separator between ssid and password
        if ( inx > 0 )                                 // Separator found?
        {
          ssid = ssid.substring ( 5, inx ) ;           // Line holds SSID now
          dbgprint ( "Added SSID %s to acceptable networks",
                     ssid.c_str() ) ;
          anetworks += ssid ;                          // Add to list
          anetworks += "|" ;                           // Separator
          num_an++ ;                                   // Count number oif acceptable networks
        }
      }
    }
    inifile.close() ;                                  // Close the file
  }
  else
  {
    dbgprint ( "File %s not found!", INIFILENAME ) ;   // No .ini file
  }
}


//******************************************************************************************
//                             G E T P R E S E T S                                         *
//******************************************************************************************
// Make a list of all preset stations.                                                     *
// The result will be stored in the String presetlist (global data).                       *
//******************************************************************************************
void getpresets()
{
  String              path ;                             // Full file spec as string
  File                inifile ;                          // File containing URL with mp3
  String              line ;                             // Input line from .ini file
  int                 inx ;                              // Position of search char in line
  int                 i ;                                // Loop control
  char                vnr[3] ;                           // 2 digit presetnumber as string

  presetlist = String ( "" ) ;                           // No result yet
  path = String ( INIFILENAME ) ;                        // Form full path
  inifile = LittleFS.open ( path, "r" ) ;                // Open the file
  if ( inifile )
  {
    while ( inifile.available() )
    {
      line = inifile.readStringUntil ( '\n' ) ;          // Read next line
      if ( line.startsWith ( "preset_" ) )               // Found the key?
      {
        i = line.substring(7, 9).toInt() ;               // Get index 00..99
        // Show just comment if available.  Otherwise the preset itself.
        inx = line.indexOf ( "#" ) ;                     // Get position of "#"
        if ( inx > 0 )                                   // Hash sign present?
        {
          line.remove ( 0, inx + 1 ) ;                   // Yes, remove non-comment part
        }
        else
        {
          inx = line.indexOf ( "=" ) ;                   // Get position of "="
          if ( inx > 0 )                                 // Equal sign present?
          {
            line.remove ( 0, inx + 1 ) ;                 // Yes, remove first part of line
          }
        }
        line = chomp ( line ) ;                          // Remove garbage from description
        sprintf ( vnr, "%02d", i ) ;                     // Preset number
        presetlist += ( String ( vnr ) + line +          // 2 digits plus description
                        String ( "|" ) ) ;
      }
    }
    inifile.close() ;                                    // Close the file
  }
}


//******************************************************************************************
//                                   S E T U P                                             *
//******************************************************************************************
// Setup for the program.                                                                  *
//******************************************************************************************
void setup()
{
  FSInfo      fs_info ;                                // Info about SPIFFS
  Dir         dir ;                                    // Directory struct for SPIFFS
  File        f ;                                      // Filehandle
  String      filename ;                               // Name of file found in SPIFFS

  Serial.begin ( 115200 ) ;                            // For debug
  Serial.println() ;
  system_update_cpu_freq ( 160 ) ;                     // Set to 80/160 MHz
  #ifdef SPIRAM                                        // Use SPI RAM?
    spiramSetup() ;                                    // Yes, do set-up
    emptyring() ;                                      // Empty the buffer
  #else
    ringbuf = (uint8_t *) malloc ( RINGBFSIZ ) ;       // Create ring buffer
  #endif
  xml.init ( xmlbuffer, sizeof(xmlbuffer),             // Initilize XML stream.
             &XML_callback ) ;
  //memset ( &ini_block, 0, sizeof(ini_block) ) ;      // Init ini_block
  ini_block.mqttbroker = "" ;
  ini_block.mqttport   = 1883 ;                        // Default port for MQTT
  ini_block.mqttuser   = "" ;
  ini_block.mqttpasswd = "" ;
  ini_block.mqtttopic  = "" ;
  ini_block.mqttpubtopic = "" ;
  ini_block.reqvol       = 0 ;
  memset ( ini_block.rtone, 0, 4 )  ;
  ini_block.newpreset    = 0 ;
  ini_block.ssid = "" ;
  ini_block.passwd = "" ;
  LittleFS.begin() ;                                   // Enable file system
  // Show some info about the SPIFFS
  LittleFS.info ( fs_info ) ;
  dbgprint ( "FS Total %d, used %d", fs_info.totalBytes, fs_info.usedBytes ) ;
  if ( fs_info.totalBytes == 0 )
  {
    dbgprint ( "No SPIFFS found!  See documentation." ) ;
  }
  dir = LittleFS.openDir("/") ;                        // Show files in FS
  while ( dir.next() )                                 // All files
  {
    f = dir.openFile ( "r" ) ;
    filename = dir.fileName() ;
    dbgprint ( "%-32s - %7d",                          // Show name and size
               filename.c_str(), f.size() ) ;
  }
  mk_lsan() ;                                          // 在 data\radio.ini 文件中列出可接受的网络。
  listNetworks() ;                                     // 搜索 WiFi 网络
  readinifile() ;                                      // 读取 .ini 文件
  getpresets() ;                                       // 从 .ini 文件中获取预设
  WiFi.setPhyMode ( WIFI_PHY_MODE_11N ) ;              // 强制 802.11N 连接
  WiFi.persistent ( false ) ;                          // 不保存SSID和密码
  WiFi.disconnect() ;                                  // 路由器可能会保持旧连接
  WiFi.mode ( WIFI_STA ) ;                             // 该ESP为站点station模式。
  wifi_station_set_hostname ( (char*)NAME ) ;
  SPI.begin() ;                                        // 初始化SPI总线。
  // 打印一些内存和草图信息
  dbgprint ( "Starting ESP Version %s...  Free memory %d",
             VERSION,
             system_get_free_heap_size() ) ;
  dbgprint ( "Sketch size %d, free size %d",
             ESP.getSketchSize(),
             ESP.getFreeSketchSpace() ) ;
  pinMode ( BUTTON2, INPUT_PULLUP ) ;                  // Input for control button 2
  vs1053player.begin() ;                               // Initialize VS1053 player
#if defined ( USETFT )
  tft.begin() ;                                        // Init TFT interface
  tft.fillRect ( 0, 0, 160, 128, BLACK ) ;             // Clear screen does not work when rotated
  tft.setRotation ( 3 ) ;                              // Use landscape format
  tft.clearScreen() ;                                  // Clear screen
  tft.setTextSize ( 1 ) ;                              // Small character font
  tft.setTextColor ( WHITE ) ;                         // Info in white
  tft.println ( "Starting" ) ;
  tft.println ( "Version:" ) ;
  tft.println ( VERSION ) ;
#else
  pinMode ( BUTTON1, INPUT_PULLUP ) ;                  // Input for control button 1
  pinMode ( BUTTON3, INPUT_PULLUP ) ;                  // Input for control button 3
#endif
  delay(10);
  analogrest = ( analogRead ( A0 ) + asw1 ) / 2  ;     // Assumed inactive analog input
  tckr.attach ( 0.100, timer100 ) ;                    // Every 100 msec
  dbgprint ( "Selected network: %-25s", ini_block.ssid.c_str() ) ;
  NetworkFound = connectwifi() ;                       // Connect to WiFi network
  //NetworkFound = false ;                             // TEST, uncomment for no network test
  dbgprint ( "Start server for commands" ) ;
  cmdserver.on ( "/", handleCmd ) ;                    // Handle startpage
  cmdserver.onNotFound ( handleFS ) ;                  // Handle file from FS
  cmdserver.onFileUpload ( handleFileUpload ) ;        // Handle file uploads
  cmdserver.begin() ;
  if ( NetworkFound )                                  // OTA and MQTT only if Wifi network found
  {
    ArduinoOTA.setHostname ( NAME ) ;                  // Set the hostname
    ArduinoOTA.onStart ( otastart ) ;
    ArduinoOTA.begin() ;                               // Allow update over the air
    if ( ini_block.mqttbroker.length() )               // Broker specified?
    {
      // Initialize the MQTT client
      WiFi.hostByName ( ini_block.mqttbroker.c_str(),
                        mqtt_server_IP ) ;             // Lookup IP of MQTT server
      mqttclient.onConnect ( onMqttConnect ) ;
      mqttclient.onDisconnect ( onMqttDisconnect ) ;
      mqttclient.onSubscribe ( onMqttSubscribe ) ;
      mqttclient.onUnsubscribe ( onMqttUnsubscribe ) ;
      mqttclient.onMessage ( onMqttMessage ) ;
      mqttclient.onPublish ( onMqttPublish ) ;
      mqttclient.setServer ( mqtt_server_IP,           // Specify the broker
                             ini_block.mqttport ) ;    // And the port
      mqttclient.setCredentials ( ini_block.mqttuser.c_str(),
                                  ini_block.mqttpasswd.c_str() ) ;
      mqttclient.setClientId ( NAME ) ;
      dbgprint ( "Connecting to MQTT %s, port %d, user %s, password %s...",
                 ini_block.mqttbroker.c_str(),
                 ini_block.mqttport,
                 ini_block.mqttuser.c_str(),
                 ini_block.mqttpasswd.c_str() ) ;
      mqttclient.connect();
    }
  }
  else
  {
    currentpreset = ini_block.newpreset ;              // No network: do not start radio
  }
  delay ( 1000 ) ;                                     // Show IP for a while
  analogrest = ( analogRead ( A0 ) + asw1 ) / 2  ;     // Assumed inactive analog input
  #ifdef SPIRAM
    dbgprint ( "Testing SPIRAM getring/putring" ) ;
    for ( int i = 0 ; i < 96 ; i++ )                    // Test for 96 bytes, 3 chunks
    {
      putring ( i ) ;                                    // Store in spiram
      dbgprint ( "Test 1: %d, chunks avl is %d",         // Test, expect 0,0 1,0 2,0 .... 95,3
                i, ringavail() ) ;
    }
    for ( int i = 0 ; i < 96 ; i++ )                    // Test for 100 bytes
    {
      uint8_t c = getring() ;                            // Read from spiram
      dbgprint ( "Test 2: %d, data is %d, ch av is %d",  // Test, expect 0,0,2 1,1,2 2,2,2 .... 95,95,0
                i, c, ringavail() ) ;
    }
    dbgprint ( "Chunks avl is %d",                       // Test, expect 0
              ringavail() ) ;
  #endif
}


//******************************************************************************************
//                                  X M L  C A L L B A C K                                 *
//******************************************************************************************
// Process XML tags into variables.                                                        *
//******************************************************************************************
void XML_callback ( uint8_t statusflags, char* tagName, uint16_t tagNameLen,
                    char* data,  uint16_t dataLen )
{
  if ( statusflags & STATUS_START_TAG )
  {
    if ( tagNameLen )
    {
      xmlOpen = String ( tagName ) ;
      //dbgprint ( "Start tag %s",tagName ) ;
    }
  }
  else if ( statusflags & STATUS_END_TAG )
  {
    //dbgprint ( "End tag %s", tagName ) ;
  }
  else if ( statusflags & STATUS_TAG_TEXT )
  {
    xmlTag = String( tagName ) ;
    xmlData = String( data ) ;
    //dbgprint ( Serial.print( "Tag: %s, text: %s", tagName, data ) ;
  }
  else if ( statusflags & STATUS_ATTR_TEXT )
  {
    //dbgprint ( "Attribute: %s, text: %s", tagName, data ) ;
  }
  else if  ( statusflags & STATUS_ERROR )
  {
    //dbgprint ( "XML Parsing error  Tag: %s, text: %s", tagName, data ) ;
  }
}


//******************************************************************************************
//                                  X M L  P A R S E                                       *
//******************************************************************************************
// Parses streams from XML data.                                                           *
//******************************************************************************************
String xmlparse ( String mount )
{
  // Example URL for XML Data Stream:
  // http://playerservices.streamtheworld.com/api/livestream?version=1.5&mount=IHR_TRANAAC&lang=en
  // Clear all variables for use.
  char   tmpstr[200] ;                              // Full GET command, later stream URL
  char   c ;                                        // Next input character from reply
  String urlout ;                                   // Result URL
  bool   urlfound = false ;                         // Result found

  stationServer = "" ;
  stationPort = "" ;
  stationMount = "" ;
  xmlTag = "" ;
  xmlData = "" ;
  stop_mp3client() ; // Stop any current wificlient connections.
  dbgprint ( "Connect to new iHeartRadio host: %s", mount.c_str() ) ;
  datamode = INIT ;                                 // Start default in metamode
  chunked = false ;                                 // Assume not chunked
  // Create a GET commmand for the request.
  sprintf ( tmpstr, xmlget, mount.c_str() ) ;
  dbgprint ( "%s", tmpstr ) ;
  // Connect to XML stream.
  mp3client = new WiFiClient() ;
  if ( mp3client->connect ( xmlhost, xmlport ) ) {
    dbgprint ( "Connected!" ) ;
    mp3client->print ( String ( tmpstr ) + " HTTP/1.1\r\n"
                       "Host: " + xmlhost + "\r\n"
                       "User-Agent: Mozilla/5.0\r\n"
                       "Connection: close\r\n\r\n" ) ;
    // Check for XML Data.
    while ( true )
    {
      if ( mp3client->available() )
      {
        char c = mp3client->read() ;
        if ( c == '<' )
        {
          c = mp3client->read() ;
          if ( c == '?' )
          {
            xml.processChar ( '<' ) ;
            xml.processChar ( '?' ) ;
            break ;
          }
        }
      }
      yield() ;
    }
    dbgprint ( "XML parser processing..." ) ;
    // Process XML Data.
    while (true) 
    {
      if ( mp3client->available() )
      {
        c = mp3client->read() ;
        xml.processChar ( c ) ;
        if ( xmlTag != "" )
        {
          if ( xmlTag.endsWith ( "/status-code" ) )   // Status code seen?
          {
            if ( xmlData != "200" )                   // Good result?
            {
              dbgprint ( "Bad xml status-code %s",    // No, show and stop interpreting
                          xmlData.c_str() ) ;
              break ;
            }
          }
          if ( xmlTag.endsWith ( "/ip" ) )
          {
            stationServer = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/port" ) )
          {
            stationPort = xmlData ;
          }
          else if ( xmlTag.endsWith ( "/mount"  ) )
          {
            stationMount = xmlData ;
          }
        }
      }
      // Check if all the station values are stored.
      urlfound = ( stationServer != "" && stationPort != "" && stationMount != "" ) ;
      if ( urlfound )
      {
        xml.reset() ;
        break ;
      }
      yield() ;
    }
    tmpstr[0] = '\0' ;                            
    if ( urlfound )
    {
      sprintf ( tmpstr, "%s:%s/%s_SC",                   // Build URL for ESP-Radio to stream.
                        stationServer.c_str(),
                        stationPort.c_str(),
                        stationMount.c_str() ) ;
      dbgprint ( "Found: %s", tmpstr ) ;
    }
    dbgprint ( "Closing XML connection." ) ;
    stop_mp3client () ;
  }
  else
  {
    dbgprint ( "Can't connect to XML host!" ) ;
    tmpstr[0] = '\0' ;                            
  }
  return String ( tmpstr ) ;                           // Return final streaming URL.
}


//******************************************************************************************
//                                   L O O P                                               *
//******************************************************************************************
// Main loop of the program.  Minimal time is 20 usec.  Will take about 4 msec if VS1053   *
// needs data.                                                                             *
// Sometimes the loop is called after an interval of more than 100 msec.                   *
// In that case we will not be able to fill the internal VS1053-fifo in time (especially   *
// at high bitrate).                                                                       *
// A connection to an MP3 server is active and we are ready to receive data.               *
// Normally there is about 2 to 4 kB available in the data stream.  This depends on the    *
// sender.                                                                                 *
//******************************************************************************************
void loop()
{
  uint32_t    maxfilechunk  ;                           // Max number of bytes to read from
                                                        // stream or file

  // Try to keep the ringbuffer filled up by adding as much bytes as possible
  if ( datamode & ( INIT | HEADER | DATA |              // Test op playing
                    METADATA | PLAYLISTINIT |
                    PLAYLISTHEADER |
                    PLAYLISTDATA ) )
  {
    if ( localfile )
    {
      maxfilechunk = mp3file.available() ;              // Bytes left in file
      if ( maxfilechunk > 1024 )                        // Reduce byte count for this loop()
      {
        maxfilechunk = 1024 ;
      }
      while ( ringspace() && maxfilechunk-- )
      {
        putring ( mp3file.read() ) ;                    // Yes, store one byte in ringbuffer
        yield() ;
      }
    }
    else
    {
      maxfilechunk = mp3client->available() ;          // Bytes available from mp3 server
      if ( maxfilechunk > 1024 )                       // Reduce byte count for this loop()
      {
        maxfilechunk = 1024 ;
      }
      while ( ringspace() && maxfilechunk-- )
      {
        putring ( mp3client->read() ) ;                // Yes, store one byte in ringbuffer
        yield() ;
      }
    }
    yield() ;
  }
  while ( vs1053player.data_request() && ringavail() ) // Try to keep VS1053 filled
  {
    #ifdef SPIRAM
      if ( spiramdelay != 0 )                          // Delay before reading SPIRAM?
      {
        spiramdelay-- ;                                // Yes, count down
        break ;                                        // and skip handling of data
      }
    #endif
    handlebyte_ch ( getring() ) ;                      // Yes, handle it
  }
  yield() ;
  if ( datamode == STOPREQD )                          // STOP requested?
  {
    dbgprint ( "STOP requested" ) ;
    if ( localfile )
    {
      mp3file.close() ;
    }
    else
    {
      stop_mp3client() ;                               // Disconnect if still connected
    }
    handlebyte_ch ( 0, true ) ;                        // Force flush of buffer
    vs1053player.setVolume ( 0 ) ;                     // Mute
    vs1053player.stopSong() ;                          // Stop playing
    emptyring() ;                                      // Empty the ringbuffer
    datamode = STOPPED ;                               // Yes, state becomes STOPPED
#if defined ( USETFT )
    tft.fillRect ( 0, 0, 160, 128, BLACK ) ;           // Clear screen does not work when rotated
#endif
    delay ( 500 ) ;
  }
  if ( localfile )
  {
    if ( datamode & ( INIT | HEADER | DATA |           // Test op playing
                      METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER |
                      PLAYLISTDATA ) )
    {
      if ( ( mp3file.available() == 0 ) && ( ringavail() == 0 ) )
      {
        datamode = STOPREQD ;                          // End of local mp3-file detected
      }
    }
  }
  if ( ini_block.newpreset != currentpreset )          // New station or next from playlist requested?
  {
    if ( datamode != STOPPED )                         // Yes, still busy?
    {
      datamode = STOPREQD ;                            // Yes, request STOP
    }
    else
    {
      if ( playlist_num )                               // Playing from playlist?
      { // Yes, retrieve URL of playlist
        playlist_num += ini_block.newpreset -
                        currentpreset ;                 // Next entry in playlist
        ini_block.newpreset = currentpreset ;           // Stay at current preset
      }
      else
      {
        host = readhostfrominifile(ini_block.newpreset) ; // Lookup preset in ini-file
      }
      dbgprint ( "New preset/file requested (%d/%d) from %s",
                 currentpreset, playlist_num, host.c_str() ) ;
      if ( host != ""  )                                // Preset in ini-file?
      {
        hostreq = true ;                                // Force this station as new preset
      }
      else
      {
        // This preset is not available, return to preset 0, will be handled in next loop()
        ini_block.newpreset = 0 ;                       // Wrap to first station
      }
    }
  }
  if ( hostreq )                                        // New preset or station?
  {
    hostreq = false ;
    currentpreset = ini_block.newpreset ;               // Remember current preset
    
    localfile = host.startsWith ( "localhost/" ) ;      // Find out if this URL is on localhost
    if ( localfile )                                    // Play file from localhost?
    {
      if ( connecttofile() )                            // Yes, open mp3-file
      {
        datamode = DATA ;                               // Start in DATA mode
      }
    }
    else
    {
      if ( host.startsWith ( "ihr/" ) )                 // iHeartRadio station requested?
      {
        host = host.substring ( 4 ) ;                   // Yes, remove "ihr/"
        host = xmlparse ( host ) ;                      // Parse the xml to get the host
      }
      connecttohost() ;                                 // Switch to new host
    }
  }
  if ( xmlreq )                                         // Directly xml requested?
  {
    xmlreq = false ;                                    // Yes, clear request flag
    host = xmlparse ( host ) ;                          // Parse the xml to get the host
    connecttohost() ;                                   // and connect to this host
  }
  if ( reqtone )                                        // Request to change tone?
  {
    reqtone = false ;
    vs1053player.setTone ( ini_block.rtone ) ;          // Set SCI_BASS to requested value
  }
  if ( resetreq )                                       // Reset requested?
  {
    delay ( 1000 ) ;                                    // Yes, wait some time
    ESP.restart() ;                                     // Reboot
  }
  if ( muteflag )
  {
    vs1053player.setVolume ( 0 ) ;                      // Mute
  }
  else
  {
    vs1053player.setVolume ( ini_block.reqvol ) ;       // Unmute
  }
  displayvolume() ;                                     // Show volume on display
  if ( testfilename.length() )                          // File to test?
  {
    testfile ( testfilename ) ;                         // Yes, do the test
    testfilename = "" ;                                 // Clear test request
  }
  scanserial() ;                                        // Handle serial input
  ArduinoOTA.handle() ;                                 // Check for OTA
}


//******************************************************************************************
//                            C H K H D R L I N E                                          *
//******************************************************************************************
// Check if a line in the header is a reasonable headerline.                               *
// Normally it should contain something like "icy-xxxx:abcdef".                            *
//******************************************************************************************
bool chkhdrline ( const char* str )
{
  char    b ;                                         // Byte examined
  int     len = 0 ;                                   // Lengte van de string

  while ( ( b = *str++ ) )                            // Search to end of string
  {
    len++ ;                                           // Update string length
    if ( ! isalpha ( b ) )                            // Alpha (a-z, A-Z)
    {
      if ( b != '-' )                                 // Minus sign is allowed
      {
        if ( b == ':' )                               // Found a colon?
        {
          return ( ( len > 5 ) && ( len < 50 ) ) ;    // Yes, okay if length is okay
        }
        else
        {
          return false ;                              // Not a legal character
        }
      }
    }
  }
  return false ;                                      // End of string without colon
}


//******************************************************************************************
//                           H A N D L E B Y T E _ C H                                     *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// Chunked transfer encoding aware. Chunk extensions are not supported.                    *
//******************************************************************************************
void handlebyte_ch ( uint8_t b, bool force )
{
  static int  chunksize = 0 ;                         // Chunkcount read from stream

  if ( chunked && !force && 
       ( datamode & ( DATA |                          // Test op DATA handling
                      METADATA |
                      PLAYLISTDATA ) ) )
  {
    if ( chunkcount == 0 )                            // Expecting a new chunkcount?
    {
       if ( b == '\r' )                               // Skip CR
       {
         return ;      
       }
       else if ( b == '\n' )                          // LF ?
       {
         chunkcount = chunksize ;                     // Yes, set new count
         chunksize = 0 ;                              // For next decode
         return ;
       }
       // We have received a hexadecimal character.  Decode it and add to the result.
       b = toupper ( b ) - '0' ;                      // Be sure we have uppercase
       if ( b > 9 )
       {
         b = b - 7 ;                                  // Translate A..F to 10..15
       }
       chunksize = ( chunksize << 4 ) + b ;
    }
    else
    {
      handlebyte ( b, force ) ;                       // Normal data byte
      chunkcount-- ;                                  // Update count to next chunksize block
    }
  }
  else
  {
    handlebyte ( b, force ) ;                         // Normal handling of this byte
  }
}


//******************************************************************************************
//                           H A N D L E B Y T E                                           *
//******************************************************************************************
// Handle the next byte of data from server.                                               *
// This byte will be send to the VS1053 most of the time.                                  *
// Note that the buffer the data chunk must start at an address that is a muttiple of 4.   *
// Set force to true if chunkbuffer must be flushed.                                       *
//******************************************************************************************
void handlebyte ( uint8_t b, bool force )
{
  static uint16_t  playlistcnt ;                       // Counter to find right entry in playlist
  static bool      firstmetabyte ;                     // True if first metabyte (counter)
  static int       LFcount ;                           // Detection of end of header
  static __attribute__((aligned(4))) uint8_t buf[32] ; // Buffer for chunk
  static int       bufcnt = 0 ;                        // Data in chunk
  static bool      firstchunk = true ;                 // First chunk as input
  String           lcml ;                              // Lower case metaline
  String           ct ;                                // Contents type
  static bool      ctseen = false ;                    // First line of header seen or not
  static bool      redirection = false ;               // Redirection or not
  int              inx ;                               // Pointer in metaline
  int              i ;                                 // Loop control

  if ( datamode == INIT )                              // Initialize for header receive
  {
    ctseen = false ;                                   // Contents type not seen yet
    redirection = false ;                              // No redirect yet
    metaint = 0 ;                                      // No metaint found
    LFcount = 0 ;                                      // For detection end of header
    bitrate = 0 ;                                      // Bitrate still unknown
    dbgprint ( "Switch to HEADER" ) ;
    datamode = HEADER ;                                // Handle header
    totalcount = 0 ;                                   // Reset totalcount
    metaline = "" ;                                    // No metadata yet
    firstchunk = true ;                                // First chunk expected
  }
  if ( datamode == DATA )                              // Handle next byte of MP3/Ogg data
  {
    buf[bufcnt++] = b ;                                // Save byte in chunkbuffer
    if ( bufcnt == sizeof(buf) || force )              // Buffer full?
    {
      if ( firstchunk )
      {
        firstchunk = false ;
        dbgprint ( "First chunk:" ) ;                  // Header for printout of first chunk
        for ( i = 0 ; i < 32 ; i += 8 )                // Print 4 lines
        {
          dbgprint ( "%02X %02X %02X %02X %02X %02X %02X %02X",
                     buf[i],   buf[i + 1], buf[i + 2], buf[i + 3],
                     buf[i + 4], buf[i + 5], buf[i + 6], buf[i + 7] ) ;
        }
      }
      vs1053player.playChunk ( buf, bufcnt ) ;         // Yes, send to player
      bufcnt = 0 ;                                     // Reset count
    }
    totalcount++ ;                                     // Count number of bytes, ignore overflow
    if ( metaint != 0 )                                // No METADATA on Ogg streams or mp3 files
    {
      if ( --datacount == 0 )                          // End of datablock?
      {
        if ( bufcnt )                                  // Yes, still data in buffer?
        {
          vs1053player.playChunk ( buf, bufcnt ) ;     // Yes, send to player
          bufcnt = 0 ;                                 // Reset count
        }
        datamode = METADATA ;
        firstmetabyte = true ;                         // Expecting first metabyte (counter)
      }
    }
    return ;
  }
  if ( datamode == HEADER )                            // Handle next byte of MP3 header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      if ( chkhdrline ( metaline.c_str() ) )           // Reasonable input?
      {
        lcml = metaline ;                              // Use lower case for compare
        lcml.toLowerCase() ;
        dbgprint ( metaline.c_str() ) ;                // Yes, Show it
        if ( lcml.startsWith ( "location: " ) )        // Redirection?
        {
          redirection = true ;
          if ( lcml.indexOf ( "http://" ) > 8 )        // Redirection with http://?
          {
            host = metaline.substring ( 17 ) ;         // Yes, get new URL
          }
          else if ( lcml.indexOf ( "https://" ) )      // Redirection with ttps://?
          {
            host = metaline.substring ( 18 ) ;         // Yes, get new URL
          }
          hostreq = true ;
        }
        if ( lcml.indexOf ( "content-type" ) >= 0 )    // Line with "Content-Type: xxxx/yyy"
        {
          ctseen = true ;                              // Yes, remember seeing this
          ct = metaline.substring ( 14 ) ;             // Set contentstype. Not used yet
          dbgprint ( "%s seen.", ct.c_str() ) ;
        }
        if ( lcml.startsWith ( "icy-br:" ) )
        {
          bitrate = metaline.substring(7).toInt() ;    // Found bitrate tag, read the bitrate
          if ( bitrate == 0 )                          // For Ogg br is like "Quality 2"
          {
            bitrate = 87 ;                             // Dummy bitrate
          }
        }
        else if ( lcml.startsWith ( "icy-metaint:" ) )
        {
          metaint = metaline.substring(12).toInt() ;   // Found metaint tag, read the value
        }
        else if ( lcml.startsWith ( "icy-name:" ) )
        {
          icyname = metaline.substring(9) ;            // Get station name
          icyname.trim() ;                             // Remove leading and trailing spaces
          displayinfo ( icyname.c_str(), 60, 68,
                        YELLOW ) ;                     // Show station name at position 60
        }
        else if ( lcml.startsWith ( "transfer-encoding:" ) )
        {
          // Station provides chunked transfer
          if ( lcml.endsWith ( "chunked" ) )
          {
            chunked = true ;                           // Remember chunked transfer mode
            chunkcount = 0 ;                           // Expect chunkcount in DATA
          }
        }
      }
      metaline = "" ;                                  // Reset this line
      if ( LFcount == 2 )                              // Double linfeed ends header
      {
        bufcnt = 0 ;                                   // Reset buffer count
        if ( ctseen )                                  // Some data seen and a double LF?
        {
          dbgprint ( "Switch to DATA, bitrate is %d"   // Show bitrate
                    ", metaint is %d",                 // and metaint
                    bitrate, metaint ) ;
          datamode = DATA ;                            // Expecting data now
          #ifdef SPIRAM
            spiramdelay = SPIRAMDELAY ;                // Start delay
          #endif
          //emptyring() ;                              // Empty SPIRAM buffer (not sure about this)
          datacount = metaint ;                        // Number of bytes before first metadata
          bufcnt = 0 ;                                 // Reset buffer count
          vs1053player.startSong() ;                   // Start a new song
        }
        if ( redirection )                             // Redirect seen?
        {
          datamode = INIT ;
        }
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
    return ;
  }
  if ( datamode == METADATA )                          // Handle next byte of metadata
  {
    if ( firstmetabyte )                               // First byte of metadata?
    {
      firstmetabyte = false ;                          // Not the first anymore
      metacount = b * 16 + 1 ;                         // New count for metadata including length byte
      if ( metacount > 1 )
      {
        dbgprint ( "Metadata block %d bytes",
                   metacount - 1 ) ;                   // Most of the time there are zero bytes of metadata
      }
      metaline = "" ;                                  // Set to empty
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
    }
    if ( --metacount == 0 )
    {
      if ( metaline.length() )                         // Any info present?
      {
        // metaline contains artist and song name.  For example:
        // "StreamTitle='Don McLean - American Pie';StreamUrl='';"
        // Sometimes it is just other info like:
        // "StreamTitle='60s 03 05 Magic60s';StreamUrl='';"
        // Isolate the StreamTitle, remove leading and trailing quotes if present.
        showstreamtitle ( metaline.c_str() ) ;         // Show artist and title if present in metadata
      }
      if ( metaline.length() > 1500 )                  // Unlikely metaline length?
      {
        dbgprint ( "Metadata block too long! Skipping all Metadata from now on." ) ;
        metaint = 0 ;                                  // Probably no metadata
        metaline = "" ;                                // Do not waste memory on this
      }
      datacount = metaint ;                            // Reset data count
      bufcnt = 0 ;                                     // Reset buffer count
      datamode = DATA ;                                // Expecting data
    }
  }
  if ( datamode == PLAYLISTINIT )                      // Initialize for receive .m3u file
  {
    // We are going to use metadata to read the lines from the .m3u file
    metaline = "" ;                                    // Prepare for new line
    LFcount = 0 ;                                      // For detection end of header
    datamode = PLAYLISTHEADER ;                        // Handle playlist data
    playlistcnt = 1 ;                                  // Reset for compare
    totalcount = 0 ;                                   // Reset totalcount
    dbgprint ( "Read from playlist" ) ;
  }
  if ( datamode == PLAYLISTHEADER )                    // Read header
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      LFcount++ ;                                      // Count linefeeds
      dbgprint ( "Playlistheader: %s",                 // Show playlistheader
                 metaline.c_str() ) ;
      metaline = "" ;                                  // Ready for next line
      if ( LFcount == 2 )
      {
        dbgprint ( "Switch to PLAYLISTDATA" ) ;
        datamode = PLAYLISTDATA ;                      // Expecting data now
        return ;
      }
    }
    else
    {
      metaline += (char)b ;                            // Normal character, put new char in metaline
      LFcount = 0 ;                                    // Reset double CRLF detection
    }
  }
  if ( datamode == PLAYLISTDATA )                      // Read next byte of .m3u file data
  {
    if ( ( b > 0x7F ) ||                               // Ignore unprintable characters
         ( b == '\r' ) ||                              // Ignore CR
         ( b == '\0' ) )                               // Ignore NULL
    {
      // Yes, ignore
    }
    else if ( b == '\n' )                              // Linefeed ?
    {
      dbgprint ( "Playlistdata: %s",                   // Show playlistheader
                 metaline.c_str() ) ;
      if ( metaline.length() < 5 )                     // Skip short lines
      {
        return ;
      }
      if ( metaline.indexOf ( "#EXTINF:" ) >= 0 )      // Info?
      {
        if ( playlist_num == playlistcnt )             // Info for this entry?
        {
          inx = metaline.indexOf ( "," ) ;             // Comma in this line?
          if ( inx > 0 )
          {
            // Show artist and title if present in metadata
            showstreamtitle ( metaline.substring ( inx + 1 ).c_str(), true ) ;
          }
        }
      }
      if ( metaline.startsWith ( "#" ) )               // Commentline?
      {
        metaline = "" ;
        return ;                                       // Ignore commentlines
      }
      // Now we have an URL for a .mp3 file or stream.  Is it the rigth one?
      dbgprint ( "Entry %d in playlist found: %s", playlistcnt, metaline.c_str() ) ;
      if ( playlist_num == playlistcnt  )
      {
        inx = metaline.indexOf ( "http://" ) ;         // Search for "http://"
        if ( inx >= 0 )                                // Does URL contain "http://"?
        {
          host = metaline.substring ( inx + 7 ) ;      // Yes, remove it and set host
        }
        else
        {
          host = metaline ;                            // Yes, set new host
        }
        connecttohost() ;                              // Connect to it
      }
      metaline = "" ;
      host = playlist ;                                // Back to the .m3u host
      playlistcnt++ ;                                  // Next entry in playlist
    }
    else
    {
      metaline += (char)b ;                            // Normal character, add it to metaline
    }
    return ;
  }
}


//******************************************************************************************
//                             G E T C O N T E N T T Y P E                                 *
//******************************************************************************************
// Returns the contenttype of a file to send.                                              *
//******************************************************************************************
String getContentType ( String filename )
{
  if      ( filename.endsWith ( ".html" ) ) return "text/html" ;
  else if ( filename.endsWith ( ".png"  ) ) return "image/png" ;
  else if ( filename.endsWith ( ".gif"  ) ) return "image/gif" ;
  else if ( filename.endsWith ( ".jpg"  ) ) return "image/jpeg" ;
  else if ( filename.endsWith ( ".ico"  ) ) return "image/x-icon" ;
  else if ( filename.endsWith ( ".css"  ) ) return "text/css" ;
  else if ( filename.endsWith ( ".zip"  ) ) return "application/x-zip" ;
  else if ( filename.endsWith ( ".gz"   ) ) return "application/x-gzip" ;
  else if ( filename.endsWith ( ".mp3"  ) ) return "audio/mpeg" ;
  else if ( filename.endsWith ( ".pw"   ) ) return "" ;              // Passwords are secret
  return "text/plain" ;
}


//******************************************************************************************
//                         H A N D L E F I L E U P L O A D                                 *
//******************************************************************************************
// Handling of upload request.  Write file to SPIFFS.                                      *
//******************************************************************************************
void handleFileUpload ( AsyncWebServerRequest *request, String filename,
                        size_t index, uint8_t *data, size_t len, bool final )
{
  String          path ;                              // Filename including "/"
  static File     f ;                                 // File handle output file
  char*           reply ;                             // Reply for webserver
  static uint32_t t ;                                 // Timer for progress messages
  uint32_t        t1 ;                                // For compare
  static uint32_t totallength ;                       // Total file length
  static size_t   lastindex ;                         // To test same index

  if ( index == 0 )
  {
    path = String ( "/" ) + filename ;                // Form SPIFFS filename
    LittleFS.remove ( path ) ;                        // Remove old file
    f = LittleFS.open ( path, "w" ) ;                 // Create new file
    t = millis() ;                                    // Start time
    totallength = 0 ;                                 // Total file lengt still zero
    lastindex = 0 ;                                   // Prepare test
  }
  t1 = millis() ;                                     // Current timestamp
  // Yes, print progress
  dbgprint ( "File upload %s, t = %d msec, len %d, index %d",
               filename.c_str(), t1 - t, len, index ) ;
  if ( len )                                          // Something to write?
  {
    if ( ( index != lastindex ) || ( index == 0 ) )   // New chunk?
    {
      f.write ( data, len ) ;                         // Yes, transfer to SPIFFS
      totallength += len ;                            // Update stored length
      lastindex = index ;                             // Remenber this part
    }
  }
  if ( final )                                        // Was this last chunk?
  {
    f.close() ;                                       // Yes, clode the file
    reply = dbgprint ( "File upload %s, %d bytes finished",
                       filename.c_str(), totallength ) ;
    request->send ( 200, "", reply ) ;
  }
}


//******************************************************************************************
//                                H A N D L E F S F                                        *
//******************************************************************************************
// Handling of requesting files from the SPIFFS/PROGMEM. Example: /favicon.ico             *
//******************************************************************************************
void handleFSf ( AsyncWebServerRequest* request, const String& filename )
{
  static String          ct ;                           // Content type
  AsyncWebServerResponse *response ;                    // For extra headers

  dbgprint ( "FileRequest received %s", filename.c_str() ) ;
  ct = getContentType ( filename ) ;                    // Get content type
  if ( ( ct == "" ) || ( filename == "" ) )             // Empty is illegal
  {
    request->send ( 404, "text/plain", "File not found" ) ;
  }
  else
  {
    if ( filename.indexOf ( "index.html" ) >= 0 )       // Index page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, index_html ) ;
    }
    else if ( filename.indexOf ( "radio.css" ) >= 0 )   // CSS file is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, radio_css ) ;
    }
    else if ( filename.indexOf ( "config.html" ) >= 0 ) // Config page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, config_html ) ;
    }
    else if ( filename.indexOf ( "about.html" ) >= 0 )  // About page is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, about_html ) ;
    }
    else if ( filename.indexOf ( "favicon.ico" ) >= 0 ) // Favicon icon is in PROGMEM
    {
      response = request->beginResponse_P ( 200, ct, favicon_ico, sizeof ( favicon_ico ) ) ;
    }
    else
    {
      response = request->beginResponse ( LittleFS, filename, ct ) ;
    }
    // Add extra headers
    response->addHeader ( "Server", NAME ) ;
    response->addHeader ( "Cache-Control", "max-age=3600" ) ;
    response->addHeader ( "Last-Modified", VERSION ) ;
    request->send ( response ) ;
  }
  dbgprint ( "Response sent" ) ;
}


//******************************************************************************************
//                                H A N D L E F S                                          *
//******************************************************************************************
// Handling of requesting files from the SPIFFS. Example: /favicon.ico                     *
//******************************************************************************************
void handleFS ( AsyncWebServerRequest* request )
{
  handleFSf ( request, request->url() ) ;               // Rest of handling
}


//******************************************************************************************
//                             A N A L Y Z E C M D                                         *
//******************************************************************************************
// Handling of the various commands from remote webclient, Serial or MQTT.                 *
// Version for handling string with: <parameter>=<value>                                   *
//******************************************************************************************
char* analyzeCmd ( const char* str )
{
  char*  value ;                                 // Points to value after equalsign in command

  value = strstr ( str, "=" ) ;                  // See if command contains a "="
  if ( value )
  {
    *value = '\0' ;                              // Separate command from value
    value++ ;                                    // Points to value after "="
  }
  else
  {
    value = (char*) "0" ;                        // No value, assume zero
  }
  return  analyzeCmd ( str, value ) ;            // Analyze command and handle it
}


//******************************************************************************************
//                                 C H O M P                                               *
//******************************************************************************************
// Do some filtering on de inputstring:                                                    *
//  - String comment part (starting with "#").                                             *
//  - Strip trailing CR.                                                                   *
//  - Strip leading spaces.                                                                *
//  - Strip trailing spaces.                                                               *
//******************************************************************************************
String chomp ( String str )
{
  int   inx ;                                         // Index in de input string

  if ( ( inx = str.indexOf ( "#" ) ) >= 0 )           // Comment line or partial comment?
  {
    str.remove ( inx ) ;                              // Yes, remove
  }
  str.trim() ;                                        // Remove spaces and CR
  return str ;                                        // Return the result
}


//******************************************************************************************
//                             分析 CMD                                         *
//******************************************************************************************
// 处理来自远程webclient，串行或MQTT的各种命令。                 *
// par保存参数名，val保存值。                                    *
// "wifi_00" 和 "preset_00" 可以出现多次，像wifi_01, wifi_02等        *
// 带有参数的示例:                                                     *
//   preset     = 12                        // 选择要连接的起始预设           *
//   preset_00  = <mp3 stream>              // 为预设 00-99 指定电台 *)       *
//   volume     = 95                        // 音量0 到 100 之间的百分比                *
//   upvolume   = 2                         // 添加百分比到当前音量            *
//   downvolume = 2                         // 从当前音量中减去百分比     *
//   toneha     = <0..15>                   // 设置高音增益                         *
//   tonehf     = <0..15>                   // 设置高音频率                    *
//   tonela     = <0..15>                   // 设置低音增益                           *
//   tonelf     = <0..15>                   // 设置高音频率                    *
//   station    = <mp3 stream>              // 选择新电台（不会被保存）      *
//   station    = <URL>.mp3                 // 播放独立的 .mp3 文件（未保存）       *
//   station    = <URL>.m3u                 // 选择播放列表（不会被保存）         *
//   stop                                   // 停止播放                                *
//   resume                                 // 继续播放                              *
//   mute                                   // 将音乐静音                              *
//   unmute                                 // 取消音乐静音                            *
//   wifi_00    = mySSID/mypassword         // 设置 WiFi SSID 和密码 *)               *
//   mqttbroker = mybroker.com              // 设置要使用的 MQTT 代理 *)                   *
//   mqttport   = 1883                      // 设置要使用的 MQTT 端口，默认 1883 *)       *
//   mqttuser   = myuser                    // 设置 MQTT 用户进行认证 *)         *
//   mqttpasswd = mypassword                // 设置 MQTT 验证密码 *)     *
//   mqtttopic  = mytopic                   // 设置 MQTT 主题订阅 *)           *
//   mqttpubtopic = mypubtopic              // 设置 MQTT 主题发布为 *)             *
//   status                                 // 显示当前播放的 URL                    *
//   testfile   = <file on SPIFFS>          // 测试 SPIFFS 读取以进行调试     *
//   test                                   // 用于测试目的                           *
//   debug      = 0 or 1                    // 打开或关闭调试                  *
//   reset                                  // 重启 ESP8266                         *
//   analog                                 // 显示当前模拟输入                   *
// 标有“*)”的命令仅在ini文件中有效*。                                 *
// 注意，建议避免使用表达式作为abs函数的参数。      *
//******************************************************************************************
char* analyzeCmd ( const char* par, const char* val )
{
  String             argument ;                       // Argument as string
  String             value ;                          // Value of an argument as a string
  int                ivalue ;                         // Value of argument as an integer
  static char        reply[250] ;                     // Reply to client, will be returned
  uint8_t            oldvol ;                         // Current volume
  bool               relative ;                       // Relative argument (+ or -)
  int                inx ;                            // Index in string

  strcpy ( reply, "Command accepted" ) ;              // Default reply
  argument = chomp ( par ) ;                          // Get the argument
  if ( argument.length() == 0 )                       // Empty commandline (comment)?
  {
    return reply ;                                    // Ignore
  }
  argument.toLowerCase() ;                            // Force to lower case
  value = chomp ( val ) ;                             // Get the specified value
  ivalue = value.toInt() ;                            // Also as an integer
  ivalue = abs ( ivalue ) ;                           // Make it absolute
  relative = argument.indexOf ( "up" ) == 0 ;         // + relative setting?
  if ( argument.indexOf ( "down" ) == 0 )             // - relative setting?
  {
    relative = true ;                                 // It's relative
    ivalue = - ivalue ;                               // But with negative value
  }
  if ( value.startsWith ( "http://" ) )               // Does (possible) URL contain "http://"?
  {
    value.remove ( 0, 7 ) ;                           // Yes, remove it
  }
  if ( value.length() )
  {
    dbgprint ( "Command: %s with parameter %s",
               argument.c_str(), value.c_str() ) ;
  }
  else
  {
    dbgprint ( "Command: %s (without parameter)",
               argument.c_str() ) ;
  }
  if ( argument.indexOf ( "volume" ) >= 0 )           // Volume setting?
  {
    // Volume may be of the form "upvolume", "downvolume" or "volume" for relative or absolute setting
    oldvol = vs1053player.getVolume() ;               // Get current volume
    if ( relative )                                   // + relative setting?
    {
      ini_block.reqvol = oldvol + ivalue ;            // Up by 0.5 or more dB
    }
    else
    {
      ini_block.reqvol = ivalue ;                     // Absolue setting
    }
    if ( ini_block.reqvol > 100 )
    {
      ini_block.reqvol = 100 ;                        // Limit to normal values
    }
    sprintf ( reply, "Volume is now %d",              // Reply new volume
              ini_block.reqvol ) ;
  }
  else if ( argument == "mute" )                      // Mute request
  {
    muteflag = true ;                                 // Request volume to zero
  }
  else if ( argument == "unmute" )                    // Unmute request?
  {
    muteflag = false ;                                // Request normal volume
  }
  else if ( argument.indexOf ( "preset" ) >= 0 )      // Preset station?
  {
    if ( !argument.startsWith ( "preset_" ) )         // But not a station URL
    {
      if ( relative )                                 // Relative argument?
      {
        ini_block.newpreset += ivalue ;               // Yes, adjust currentpreset
      }
      else
      {
        ini_block.newpreset = ivalue ;                // Otherwise set preset station
      }
      sprintf ( reply, "Preset is now %d",            // Reply new preset
                ini_block.newpreset ) ;
      playlist_num = 0 ;
    }
  }
  else if ( argument == "stop" )                      // Stop requested?
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )

    {
      datamode = STOPREQD ;                           // Request STOP
    }
    else
    {
      strcpy ( reply, "Command not accepted!" ) ;     // Error reply
    }
  }
  else if ( argument == "resume" )                    // Request to resume?
  {
    if ( datamode == STOPPED )                        // Yes, are we stopped?
    {
      hostreq = true ;                                // Yes, request restart
    }
  }
  else if ( argument == "station" )                   // Station in the form address:port
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    hostreq = true ;                                  // Force this station as new preset
    sprintf ( reply,
              "New preset station %s accepted",       // Format reply
              host.c_str() ) ;
  }
  else if ( argument == "xml" )
  {
    if ( datamode & ( HEADER | DATA | METADATA | PLAYLISTINIT |
                      PLAYLISTHEADER | PLAYLISTDATA ) )
    {
      datamode = STOPREQD ;                           // Request STOP
    }
    host = value ;                                    // Save it for storage and selection later
    xmlreq = true ;                                   // Run XML parsing process.
    sprintf ( reply,
              "New xml preset station %s accepted",   // Format reply
              host.c_str() ) ;
  }
  else if ( argument == "status" )                    // Status request
  {
    if ( datamode == STOPPED )
    {
      sprintf ( reply, "Player stopped" ) ;           // Format reply
    }
    else
    {
      sprintf ( reply, "%s - %s", icyname.c_str(),
                icystreamtitle.c_str() ) ;            // Streamtitle from metadata
    }
  }
  else if ( argument.startsWith ( "reset" ) )         // Reset request
  {
    resetreq = true ;                                 // Reset all
  }
  else if ( argument == "testfile" )                  // Testfile command?
  {
    testfilename = value ;                            // Yes, set file to test accordingly
  }
  else if ( argument == "test" )                      // Test command
  {
    #ifdef SPIRAM
      rcount = dataAvailable() ;                      // Yes, get free space
      sprintf ( reply, "Free memory is %d, ringbuf %d, stream %d",
                system_get_free_heap_size(), rcount, mp3client->available() ) ;
    #endif
  }
  // Commands for bass/treble control
  else if ( argument.startsWith ( "tone" ) )          // Tone command
  {
    if ( argument.indexOf ( "ha" ) > 0 )              // High amplitue? (for treble)
    {
      ini_block.rtone[0] = ivalue ;                   // Yes, prepare to set ST_AMPLITUDE
    }
    if ( argument.indexOf ( "hf" ) > 0 )              // High frequency? (for treble)
    {
      ini_block.rtone[1] = ivalue ;                   // Yes, prepare to set ST_FREQLIMIT
    }
    if ( argument.indexOf ( "la" ) > 0 )              // Low amplitue? (for bass)
    {
      ini_block.rtone[2] = ivalue ;                   // Yes, prepare to set SB_AMPLITUDE
    }
    if ( argument.indexOf ( "lf" ) > 0 )              // High frequency? (for bass)
    {
      ini_block.rtone[3] = ivalue ;                   // Yes, prepare to set SB_FREQLIMIT
    }
    reqtone = true ;                                  // Set change request
    sprintf ( reply, "Parameter for bass/treble %s set to %d",
              argument.c_str(), ivalue ) ;
  }
  else if ( argument == "rate" )                      // Rate command?
  {
    vs1053player.AdjustRate ( ivalue ) ;              // Yes, adjust
  }
  else if ( argument.startsWith ( "mqtt" ) )          // Parameter fo MQTT?
  {
    strcpy ( reply, "MQTT broker parameter changed. Save and restart to have effect" ) ;
    if ( argument.indexOf ( "broker" ) > 0 )          // Broker specified?
    {
      ini_block.mqttbroker = value.c_str() ;          // Yes, set broker accordingly
    }
    else if ( argument.indexOf ( "port" ) > 0 )       // Port specified?
    {
      ini_block.mqttport = ivalue ;                   // Yes, set port user accordingly
    }
    else if ( argument.indexOf ( "user" ) > 0 )       // User specified?
    {
      ini_block.mqttuser = value ;                    // Yes, set user accordingly
    }
    else if ( argument.indexOf ( "passwd" ) > 0 )     // Password specified?
    {
      ini_block.mqttpasswd = value.c_str() ;          // Yes, set broker password accordingly
    }
    else if ( argument.indexOf ( "pubtopic" ) > 0 )   // Publish topic specified?
    {
      ini_block.mqttpubtopic = value.c_str() ;        // Yes, set broker password accordingly
    }
    else if ( argument.indexOf ( "topic" ) > 0 )      // Topic specified?
    {
      ini_block.mqtttopic = value.c_str() ;           // Yes, set broker topic accordingly
    }
  }
  else if ( argument == "debug" )                     // debug on/off request?
  {
    DEBUG = ivalue ;                                  // Yes, set flag accordingly
  }
  else if ( argument == "analog" )                    // Show analog request?
  {
    sprintf ( reply, "Analog input = %d units",       // Read the analog input for test
              analogRead ( A0 ) ) ;
  }
  else if ( argument.startsWith ( "wifi" ) )          // WiFi SSID and passwd?
  {
    inx = value.indexOf ( "/" ) ;                     // Find separator between ssid and password
    // Was this the strongest SSID or the only acceptable?
    if ( num_an == 1 )
    {
      ini_block.ssid = value.substring ( 0, inx ) ;   // Only one.  Set as the strongest
    }
    if ( value.substring ( 0, inx ) == ini_block.ssid )
    {
      ini_block.passwd = value.substring ( inx + 1 ) ; // Yes, set password
    }
  }
  else if ( argument == "getnetworks" )               // List all WiFi networks?
  {
    sprintf ( reply, networks.c_str() ) ;             // Reply is SSIDs
  }
  else
  {
    sprintf ( reply, "%s called with illegal parameter: %s",
              NAME, argument.c_str() ) ;
  }
  return reply ;                                      // Return reply to the caller
}


//******************************************************************************************
//                             处理 CMD                                           *
//******************************************************************************************
// 远程处理各种命令(区分大小写)。    *
// 所有命令的格式都是"/?parameter[=value]".  示例：音量=50 "/?volume=50".                                    *
// 如果没有给出参数，则返回起始页。                               *
// 忽略多个参数。一个额外的参数可以是"version=<random number>"   *
// 以防止Edge和IE等浏览器使用它们的缓存。    *
// 此“版本”将被忽略。                                                                                *
// 示例： "/?upvolume=5&version=0.9775479450590543"                                      *
// 保存和列表命令是特殊处理的。                                 *
//******************************************************************************************
void handleCmd ( AsyncWebServerRequest* request )
{
  AsyncWebParameter* p ;                                // Points to parameter structure
  static String      argument ;                         // Next argument in command
  static String      value ;                            // Value of an argument
  const char*        reply ;                            // Reply to client
  //uint32_t         t ;                                // For time test
  int                params ;                           // Number of params
  static File        f ;                                // Handle for writing /radio.ini to SPIFFS

  //t = millis() ;                                      // Timestamp at start
  params = request->params() ;                          // Get number of arguments
  if ( params == 0 )                                    // Any arguments
  {
    if ( NetworkFound )
    {
      handleFSf ( request, String( "/index.html") ) ;   // No parameters, send the startpage
    }
    else
    {
      handleFSf ( request, String( "/config.html") ) ;  // Or the configuration page if in AP mode
    }
    return ;
  }
  p = request->getParam ( 0 ) ;                         // Get pointer to parameter structure
  argument = p->name() ;                                // Get the argument
  argument.toLowerCase() ;                              // Force to lower case
  value = p->value() ;                                  // Get the specified value
  // For the "save" command, the contents is the value of the next parameter
  if ( argument.startsWith ( "save" ) && ( params > 1 ) )
  {
    reply = "Error saving " INIFILENAME ;               // Default reply
    p = request->getParam ( 1 ) ;                       // Get pointer to next parameter structure
    if ( p->isPost() )                                  // Does it have a POST?
    {
      f = LittleFS.open ( INIFILENAME, "w" ) ;          // Save to inifile
      if ( f )
      {
        f.print ( p->value() ) ;
        f.close() ;
        reply = dbgprint ( "%s saved", INIFILENAME ) ;
      }
    }
  }
  else if ( argument.startsWith ( "list" ) )            // List all presets?
  {
    dbgprint ( "list request from browser" ) ;
    request->send ( 200, "text/plain", presetlist ) ;   // Send the reply
    return ;
  }
  else
  {
    reply = analyzeCmd ( argument.c_str(),              // Analyze it
                         value.c_str() ) ;
  }
  request->send ( 200, "text/plain", reply ) ;          // Send the reply
  //t = millis() - t ;
  // If it takes too long to send a reply, we run into the "LmacRxBlk:1"-problem.
  // Reset the ESP8266.....
  //if ( t > 8000 )
  //{
  //  ESP.restart() ;                                   // Last resource
  //}
}
