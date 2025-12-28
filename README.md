# sentinel-esp
IIC Bus:

SDA:GPIO8
SCL:GPIO18

PCF8575: i2c address:0x22

PCF8575->P0 (relay1)
PCF8575->P1 (relay2)
PCF8575->P2 (relay3)
PCF8575->P3 (relay4)
PCF8575->P4 (relay5)
PCF8575->P5 (relay6)
PCF8575->P6 (relay7)
PCF8575->P7 (relay8)

PCF8575->P8 (DI1)
PCF8575->P9 (DI2)
PCF8575->P10 (DI3)
PCF8575->P11 (DI4)
PCF8575->P12 (DI5)
PCF8575->P13 (DI6)
PCF8575->P14 (DI7)
PCF8575->P15 (DI8)

24C02 EPROM i2c address: 0x50
DS3231 RTC i2c address: 0x68
SSD1306 display: i2c address:0x3c


Analog input (A1: DC 0-5v): GPIO7
Analog input (A2: DC 0-5v): GPIO6

-----------------
1-wire GPIOs (with pull-up resistance on PCB):
S1:GPIO40
S2:GPIO13
S3:GPIO48
S4:GPIO14


-----------------

Ethernet (W5500) I/O define:

clk_pin: GPIO1
mosi_pin: GPIO2
miso_pin: GPIO41
cs_pin: GPIO42

interrupt_pin: GPIO43
reset_pin: GPIO44

--------------------
RS485:
RXD:GPIO38
TXD:GPIO39

Tuya module:
RXD:GPIO17
TXD:GPIO16

Tuya network button: Tuya module's P28
Tuya network LED: Tuya module's P16
--------------------
SD Card:
SPI-MOSI:GPIO10
SPI-SCK:GPIO11
SPI-MISO:GPIO12
SPI-CS:GPIO9
SPI-CD:GPIO47
--------------------
RF433M sender:GPIO4
RF433M receiver:GPIO5