#!/bin/bash

/home/forrest/install/bin/arduino-cli compile -v --fqbn STMicroelectronics:stm32:GenH7:pnum=DAISY_SEED,upload_method=dfuMethod,usb=CDCgen /home/forrest/programming_2026/daisyduino_projects/guitar_thru && /home/forrest/install/bin/arduino-cli upload -v --fqbn STMicroelectronics:stm32:GenH7:pnum=DAISY_SEED,upload_method=dfuMethod,usb=CDCgen /home/forrest/programming_2026/daisyduino_projects/guitar_thru
