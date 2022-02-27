//******************************************************************************************
// 例程的头文件。                                                       *
//******************************************************************************************

#ifndef _SPIRAM_HPP
  #define SRAM_CS        10                   // GPIO1O CS 引脚
  #define SRAM_FREQ    13e6                   // 23LC1024 理论上支持最高20MHz

  bool spaceAvailable() ;
  uint16_t dataAvailable() ;
  uint16_t getFreeBufferSpace() ;
  void bufferWrite ( uint8_t *b ) ;
  void bufferRead ( uint8_t *b ) ;
  void bufferReset() ;
  void spiramSetup() ;
  #define _SPIRAM_HPP
#endif
