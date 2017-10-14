//#define PRINT_DEBUG

#define ICM_VERSION "ICM-Teensy v1.0 2004"

#ifdef PRINT_DEBUG
#include "referenceBmp.h"
#endif

#define PinInt 12

#define data0 2
#define data1 14
#define data2 7
#define data3 8
#define data4 6
#define data5 20
#define data6 21
#define data7 5

void setup(){

Serial.begin(115200);

pinMode(PinInt, INPUT); // sets the digital pin as input : CLK

pinMode(data0, INPUT); // sets the digital pin as input : D0
pinMode(data1, INPUT); // sets the digital pin as input : D1
pinMode(data2, INPUT); // sets the digital pin as input : D2
pinMode(data3, INPUT); // sets the digital pin as input : D3
pinMode(data4, INPUT); // sets the digital pin as input : D4
pinMode(data5, INPUT); // sets the digital pin as input : D5
pinMode(data6, INPUT); // sets the digital pin as input : D6
pinMode(data7, INPUT); // sets the digital pin as input : D7

attachInterrupt(PinInt, isrService, FALLING); // interrrupt 1 is data ready
//highest priority
NVIC_SET_PRIORITY(IRQ_PORTC, 0);

//wait opening com port on pc side
while(!Serial);
Serial.println(ICM_VERSION);
Serial.println(__DATE__);
Serial.println(__TIME__);
}

#define DATABUFFER_SIZE_2048  (2048)
#define DATABUFFER_SIZE_2096  (2096)
extern volatile unsigned char newDataIsr[DATABUFFER_SIZE_2096];
extern volatile unsigned char dataReadyIsr;
extern const unsigned char protocolFound;
#ifdef PRINT_DEBUG
extern volatile unsigned int startFound;
#endif

#define NB_COLUMNS_32 32
#define NB_ROWS_64 64
#define PIXEL_DATA_SIZE_2048 (NB_ROWS_64*NB_COLUMNS_32)
unsigned char bmpData[PIXEL_DATA_SIZE_2048];

#define COLUMN_SIZE_8 8

char bmpDataReady = 0;

void bmpDataBuild(void)
{
  unsigned int i = 0;
  unsigned int j = 0;
  unsigned int k = 0;
  unsigned int pixelStart = 0;
  unsigned int pixelXOffset = 0;
  unsigned int pixelYOffset = 0;
  unsigned char tmp = 0;

  bmpDataReady = 0;

  for(i = 0; i < PIXEL_DATA_SIZE_2048; i++)
  {
    bmpData[i] = 0;
  }

  //remove 3 bytes every 128 bytes
  if(protocolFound)
  {
    for(i = 0; i < DATABUFFER_SIZE_2096; i++)
    {
      if(!(i%128))
      {
        for(j = 0; j < (DATABUFFER_SIZE_2096 - i); j++)
        {
          newDataIsr[i+j] = newDataIsr[i+j+3];
        }
      }
    }
  }

  //start at bottom end
  pixelStart += (PIXEL_DATA_SIZE_2048 - (NB_COLUMNS_32 * COLUMN_SIZE_8));
    
  for(i = 0; i < PIXEL_DATA_SIZE_2048; i++)
  {
    for(j = 0; j < COLUMN_SIZE_8; j++)
    {
      cli();
      bmpData[i] |= (((newDataIsr[pixelStart + pixelXOffset] & (0x80 >> pixelYOffset)) >> ((COLUMN_SIZE_8 - 1) - pixelYOffset)) << ((COLUMN_SIZE_8 - 1) - j));
      sei();
      pixelXOffset++;
      if(pixelXOffset >= (NB_COLUMNS_32 * COLUMN_SIZE_8))
      {
        pixelXOffset = 0;
        pixelYOffset++;
        if(pixelYOffset >= COLUMN_SIZE_8)
        {
          pixelYOffset = 0;
          pixelStart -=(NB_COLUMNS_32 * COLUMN_SIZE_8);
        }
      }
    }
  }

  
  if(protocolFound)
  {
#if 1
    //1st pass
    j = ((32*32)+16);
    for(k = 0; k < 32; k++)
    {
      for(i = (32*k); i < (32*k)+16; i++)
      {
        tmp = bmpData[i+j];
        bmpData[i+j] = bmpData[i];
        bmpData[i] = tmp;
      }
    }
#endif
#if 1
    //2nd pass
    j = 32*32;
    for(k = 0; k < 8; k++)
    {
      for(i = (32*k); i < (32*k)+32; i++)
      {
        tmp = bmpData[i+j];
        bmpData[i+j] = bmpData[i+(32*8)];
        bmpData[i+(32*8)] = tmp;
      }
    }
#endif
#if 1
    //3rd pass
    j = 32*32;
    for(k = 0; k < 8; k++)
    {
      for(i = (32*k); i < (32*k)+32; i++)
      {
        tmp = bmpData[i+j];
        bmpData[i+j] = bmpData[i+(32*16)];
        bmpData[i+(32*16)] = tmp;
      }
    }
#endif
#if 1
    //4th pass
    j = 32*40;
    for(k = 0; k < 8; k++)
    {
      for(i = (32*k); i < (32*k)+32; i++)
      {
        tmp = bmpData[i+j];
        bmpData[i+j] = bmpData[i+(32*24)];
        bmpData[i+(32*24)] = tmp;
      }
    }
#endif
#if 1
    //5th pass
    j = 32*48;
    for(k = 0; k < 8; k++)
    {
      for(i = (32*k); i < (32*k)+32; i++)
      {
        tmp = bmpData[i+j];
        bmpData[i+j] = bmpData[i+(32*40)];
        bmpData[i+(32*40)] = tmp;
      }
    }
#endif
  }

  bmpDataReady = 1;
}

//1Bit 256*64 BMP file header
#define BMP_HEADER_SIZE_62 62
const unsigned char bmpHeader[BMP_HEADER_SIZE_62]={
  0x42,0x4D,0x3E,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x28,0x00,
  0x00,0x00,0x00,0x01,0x00,0x00,0x40,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,
  0x00,0x00,0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00
};
#define BMP_FILE_SIZE_2110 (BMP_HEADER_SIZE_62 + PIXEL_DATA_SIZE_2048)
unsigned char bmpFile[BMP_FILE_SIZE_2110];

char bmpFileReady = 0;

void bmpFileBuild(void)
{
  unsigned int i = 0;

  bmpFileReady = 0;

  if(bmpDataReady)
  {
    //1 - Header first
    for(i = 0; i < BMP_HEADER_SIZE_62; i++)
    {
      bmpFile[i] = bmpHeader[i];
    }
    //2 - Then pixel data, bottom to top aligned
    for(i = BMP_HEADER_SIZE_62; i < BMP_FILE_SIZE_2110; i++)
    {
      bmpFile[i] = bmpData[i-BMP_HEADER_SIZE_62];
    }
    bmpFileReady = 1;
  }
}

void loop()
{
// int incomingByte = 0;   // for incoming serial data
  unsigned int i = 0;
#ifdef PRINT_DEBUG
  unsigned char minRef = 100;
  unsigned char maxRef = 0;
  unsigned char currentRef = 0;
  unsigned int errors = 0;
  unsigned int nbNoError = 0;
#endif

  while(1)
  {
//    if (Serial.available() > 0)
//    {
//      // read the incoming byte:
//      incomingByte = Serial.read();
//
//      if(incomingByte == 'b')
//      {
//        bmpDataReady = 0;
//        bmpFileReady = 0;
//        dataReadyIsr = 0;
//      }
//    }

    delay(100);

    if(bmpFileReady == 0)
    {
      if(dataReadyIsr)
      {
        bmpDataBuild();
        bmpFileBuild();
      }
    }
    if(bmpFileReady == 1)
    {
      bmpFileReady = 2;

      //wait opening com port on pc side
      while(!Serial);
      
#ifndef PRINT_DEBUG
      for(i = 0; i < BMP_FILE_SIZE_2110; i++)
      {
        //print extra zero if less than two chars
        if(bmpFile[i] < 0x10) Serial.print("0");
        Serial.print(bmpFile[i], HEX);
        //compare with a reference file
        //if(bmpFile[i] != referenceBmp[i]) errors++;
      }
      Serial.println("");
#endif

#ifdef PRINT_DEBUG
/*      for(i = 0; i < 2048; i++)
      {
        if(newDataIsr[i] < 0x10) Serial.print("0");
        Serial.print(newDataIsr[i], HEX);
      }
      Serial.println("");
*/
/*
      //stats about errors between current file and reference file
      currentRef = 100 - ((errors * 100) / BMP_FILE_SIZE_2110);
      if(currentRef < minRef) minRef = currentRef;
      if(currentRef > maxRef) maxRef = currentRef;
      if(currentRef == 100) nbNoError++;

      Serial.println("MIN<CUR<MAX NBNOERR NBSTART");
      Serial.print(minRef, DEC);
      Serial.print(" ");
      Serial.print(currentRef, DEC);
      Serial.print(" ");
      Serial.print(maxRef, DEC);
      Serial.print("  ");
      Serial.print(nbNoError, DEC);
      Serial.print("    ");
*/
      Serial.println("NBSTART startFound");
      Serial.print(startFound, DEC);
      Serial.print(" ");
      Serial.print(startFound, DEC);
      Serial.println("");

      errors = 0;
#endif

      bmpDataReady = 0;
      bmpFileReady = 0;
      dataReadyIsr = 0;
    }
  }
}

// watermark generates this interrupt
FASTRUN inline void isrService()
{
}
