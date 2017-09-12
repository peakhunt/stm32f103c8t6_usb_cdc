# stm32f103c8t6_usb_cdc

## Introduction
This is a USB CDC composite device demo with STM32F103C8T6 micro controller.
Original STM32 CubeMX USB CDC middleware was massively changed to implement USB CDC composite device.
It suports two USB CDC devices. You might ask why not three? Well my original plan was to implement a composite device
with three USB CDC devices but along the road, I found out that STM32F103C8T6 supports only 8 USB endpoints (as far as I remember). That is why.

Basically it works but it has not been throughly tested. But still this might be a good starting point for you.

Good Luck!
