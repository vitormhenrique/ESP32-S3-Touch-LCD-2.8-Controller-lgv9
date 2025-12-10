#include "Display_ST7789.h"

/* Current rotation setting */
lcd_rotation_t LCD_Rotation = LCD_ROTATION_0;

/* MADCTL register bit definitions for ST7789 */
#define MADCTL_MY  0x80  // Row Address Order
#define MADCTL_MX  0x40  // Column Address Order  
#define MADCTL_MV  0x20  // Row/Column Exchange
#define MADCTL_ML  0x10  // Vertical Refresh Order
#define MADCTL_RGB 0x00  // RGB color order
#define MADCTL_BGR 0x08  // BGR color order
   
SPIClass LCDspi(FSPI);
void SPI_Init()
{
  LCDspi.begin(EXAMPLE_PIN_NUM_SCLK,EXAMPLE_PIN_NUM_MISO,EXAMPLE_PIN_NUM_MOSI); 
}

void LCD_WriteCommand(uint8_t Cmd)  
{ 
  LCDspi.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, LOW); 
  LCDspi.transfer(Cmd);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  LCDspi.endTransaction();
}
void LCD_WriteData(uint8_t Data) 
{ 
  LCDspi.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  LCDspi.transfer(Data);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  LCDspi.endTransaction();
}    
void LCD_WriteData_Word(uint16_t Data)
{
  LCDspi.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH); 
  LCDspi.transfer16(Data);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  LCDspi.endTransaction();
}   
void LCD_WriteData_nbyte(uint8_t* SetData,uint8_t* ReadData,uint32_t Size) 
{ 
  LCDspi.beginTransaction(SPISettings(SPIFreq, MSBFIRST, SPI_MODE0));
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);  
  digitalWrite(EXAMPLE_PIN_NUM_LCD_DC, HIGH);  
  LCDspi.transferBytes(SetData, ReadData, Size);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, HIGH);  
  LCDspi.endTransaction();
} 

void LCD_Reset(void)
{
  digitalWrite(EXAMPLE_PIN_NUM_LCD_CS, LOW);       
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, LOW); 
  delay(50);
  digitalWrite(EXAMPLE_PIN_NUM_LCD_RST, HIGH); 
  delay(50);
}
void LCD_Init(void)
{
  pinMode(EXAMPLE_PIN_NUM_LCD_CS, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_DC, OUTPUT);
  pinMode(EXAMPLE_PIN_NUM_LCD_RST, OUTPUT); 
  SPI_Init();

  LCD_Reset();
  //************* Start Initial Sequence **********// 

  delay(120);
  LCD_WriteCommand(0x29);    //Display on
  delay(120);         
  LCD_WriteCommand(0x11);     
  delay(120);                //ms            
  
  /* Set default rotation */
  LCD_SetRotation(LCD_Rotation);

  LCD_WriteCommand(0x3A);     
  LCD_WriteData(0x05);   

  LCD_WriteCommand(0xB0);
  LCD_WriteData(0x00);
  LCD_WriteData(0xE8); // 5 to 6-bit conversion: r0 = r5, b0 = b5

  LCD_WriteCommand(0xB2);     
  LCD_WriteData(0x0C);   
  LCD_WriteData(0x0C);   
  LCD_WriteData(0x00);   
  LCD_WriteData(0x33);   
  LCD_WriteData(0x33);   

  LCD_WriteCommand(0xB7);     
  LCD_WriteData(0x75);   //VGH=14.97V,VGL=-7.67V

  LCD_WriteCommand(0xBB);     
  LCD_WriteData(0x1A);   

  LCD_WriteCommand(0xC0);     
  LCD_WriteData(0x2C);   

  LCD_WriteCommand(0xC2);     
  LCD_WriteData(0x01);    
  LCD_WriteData(0xFF);     

  LCD_WriteCommand(0xC3);     
  LCD_WriteData(0x13);   

  LCD_WriteCommand(0xC4);     
  LCD_WriteData(0x20);   

  LCD_WriteCommand(0xC6);     
  LCD_WriteData(0x0F);   

  LCD_WriteCommand(0xD0);     
  LCD_WriteData(0xA4);   
  LCD_WriteData(0xA1);   

  LCD_WriteCommand(0xD6);     
  LCD_WriteData(0xA1);   

  LCD_WriteCommand(0xE0);     
  LCD_WriteData(0xD0);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x14);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x0D);
  LCD_WriteData(0x09);
  LCD_WriteData(0x38);
  LCD_WriteData(0x44);
  LCD_WriteData(0x4E);
  LCD_WriteData(0x3A);
  LCD_WriteData(0x17);
  LCD_WriteData(0x18);
  LCD_WriteData(0x2F);
  LCD_WriteData(0x30);

  LCD_WriteCommand(0xE1);     
  LCD_WriteData(0xD0);
  LCD_WriteData(0x09);
  LCD_WriteData(0x0F);
  LCD_WriteData(0x08);
  LCD_WriteData(0x07);
  LCD_WriteData(0x14);
  LCD_WriteData(0x37);
  LCD_WriteData(0x44);
  LCD_WriteData(0x4D);
  LCD_WriteData(0x38);
  LCD_WriteData(0x15);
  LCD_WriteData(0x16);
  LCD_WriteData(0x2C);
  LCD_WriteData(0x2E);

  LCD_WriteCommand(0x21);     

  LCD_WriteCommand(0x29); 

  LCD_WriteCommand(0x2C);     

  Touch_Init();
}
/******************************************************************************
function: Set the cursor position
parameter :
    Xstart:   Start uint16_t x coordinate
    Ystart:   Start uint16_t y coordinate
    Xend  :   End uint16_t coordinates
    Yend  :   End uint16_t coordinatesen
******************************************************************************/
void LCD_SetCursor(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t  Yend)
{ 
  // set the X coordinates
  LCD_WriteCommand(0x2A);
  LCD_WriteData((Xstart + Offset_X) >> 8);
  LCD_WriteData((Xstart + Offset_X) & 0xFF);
  LCD_WriteData((Xend + Offset_X) >> 8);
  LCD_WriteData((Xend + Offset_X) & 0xFF);
  
  // set the Y coordinates
  LCD_WriteCommand(0x2B);
  LCD_WriteData((Ystart + Offset_Y) >> 8);
  LCD_WriteData((Ystart + Offset_Y) & 0xFF);
  LCD_WriteData((Yend + Offset_Y) >> 8);
  LCD_WriteData((Yend + Offset_Y) & 0xFF);
  
  LCD_WriteCommand(0x2C);
}
/******************************************************************************
function: Refresh the image in an area
parameter :
    Xstart:   Start uint16_t x coordinate
    Ystart:   Start uint16_t y coordinate
    Xend  :   End uint16_t coordinates
    Yend  :   End uint16_t coordinates
    color :   Set the color
******************************************************************************/
void LCD_addWindow(uint16_t Xstart, uint16_t Ystart, uint16_t Xend, uint16_t Yend,uint16_t* color)
{             
  uint16_t Show_Width = Xend - Xstart + 1;
  uint16_t Show_Height = Yend - Ystart + 1;
  uint32_t numBytes = Show_Width * Show_Height * sizeof(uint16_t);
  LCD_SetCursor(Xstart, Ystart, Xend, Yend);
  LCD_WriteData_nbyte((uint8_t*)color, NULL, numBytes);        
}


// backlight
uint8_t LCD_Backlight = 50;
void Backlight_Init()
{
  ledcAttach(LCD_Backlight_PIN, Frequency, Resolution);   
  ledcWrite(LCD_Backlight_PIN, Dutyfactor);  
  Set_Backlight(LCD_Backlight);      //0~100                 
}

void Set_Backlight(uint8_t Light)                     
{
  if(Light > Backlight_MAX || Light < 0)
    printf("Set Backlight parameters in the range of 0 to 100 \r\n");
  else{
    uint32_t Backlight = Light*10;
    if(Backlight == 1000)
      Backlight = 1024;
    ledcWrite(LCD_Backlight_PIN, Backlight);
  }
}

/******************************************************************************
function: Get current display width based on rotation
******************************************************************************/
uint16_t LCD_GetWidth(void)
{
  if (LCD_Rotation == LCD_ROTATION_90 || LCD_Rotation == LCD_ROTATION_270) {
    return LCD_HEIGHT;  // Landscape: width = 320
  }
  return LCD_WIDTH;     // Portrait: width = 240
}

/******************************************************************************
function: Get current display height based on rotation
******************************************************************************/
uint16_t LCD_GetHeight(void)
{
  if (LCD_Rotation == LCD_ROTATION_90 || LCD_Rotation == LCD_ROTATION_270) {
    return LCD_WIDTH;   // Landscape: height = 240
  }
  return LCD_HEIGHT;    // Portrait: height = 320
}

/******************************************************************************
function: Set the display rotation
parameter:
    rotation: LCD_ROTATION_0, LCD_ROTATION_90, LCD_ROTATION_180, LCD_ROTATION_270
    
MADCTL (Memory Data Access Control) Register 0x36:
    Bit 7 (MY):  Row Address Order
    Bit 6 (MX):  Column Address Order
    Bit 5 (MV):  Row/Column Exchange
    Bit 3 (RGB): RGB-BGR Order
******************************************************************************/
void LCD_SetRotation(lcd_rotation_t rotation)
{
  LCD_Rotation = rotation;
  LCD_WriteCommand(0x36);  // MADCTL
  
  switch (rotation) {
    case LCD_ROTATION_0:   // Portrait 0°: 240x320
      LCD_WriteData(MADCTL_RGB);
      break;
    case LCD_ROTATION_90:  // Landscape 90°: 320x240
      LCD_WriteData(MADCTL_MV | MADCTL_MY | MADCTL_RGB);
      break;
    case LCD_ROTATION_180: // Portrait 180°: 240x320
      LCD_WriteData(MADCTL_MX | MADCTL_MY | MADCTL_RGB);
      break;
    case LCD_ROTATION_270: // Landscape 270°: 320x240
      LCD_WriteData(MADCTL_MV | MADCTL_MX | MADCTL_RGB);
      break;
  }
}