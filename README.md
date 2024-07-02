# Inkplate Motion Arduino Library

<p align="center">
  <img src="https://raw.githubusercontent.com/SolderedElectronics/Inkplate-Motion-Arduino-Library/main/extras/images/Inkplate6Motion.jpg">
</p>

All in one and ready to use Inkplate Motion Arduino Library. Use e-paper with one of the quickest refresh updates with user-frendly Arduino IDE. Draw text, shapes or image with just few lines on code!

# ⚠️This library is still in the development!⚠️

## Getting started
1. Install Inkplate Motion Board Definition though [Dasduino Board Definition](https://github.com/SolderedElectronics/Dasduino-Board-Definitions-for-Arduino-IDE). Select Inklplate Motion board.
2. If you aren't using macOS install CH340 drivers (in case you don't have them yet) - instructions [here](https://soldered.com/learn/ch340-driver-installation-croduino-basic3-nova2/)
3. Get the STM32CubeProgrammer from [STMicroelectronics](https://www.st.com/en/development-tools/stm32cubeprog.html). It's needed to for programming Inkplate Motion STM32 MCU.
4. Get the library by downloading it from here or by using ~Arduino Library Manager~
5. Open Inkplate Motion example and run them on Inkplate Motion!

ℹ**NOTE**ℹ
In order to be able to upload the program to the Inkplate Motion with the USB, put Inkplate Motion in programming mode by pressing PROGRAMMING BUTTON. If you have Inkplate with the case, open the case, you will find the tool for pressing programming button with case closed.

## Code examples
There are many examples in the library that you demonstrate how to use any of the Inkplate functionality. Use Basic folder to get familiar with principles of using the Inkplate, such as modes (BW and Gray), how to write graphics and text. In Advanced folder, you'll learn how to make HTTP/HTTPS requests, utilise low power, use SD card, read RTC, etc. Finally, get easily started on some projects. Examples and projects are added regularly by us or from community contributions. There is also a diagnostics folder with all tools for more advanced users (such as VCOM programming, waveform selection, etc).

## Battery power

Inkplate boards has two options for powering it. First one is obvious - USB port at side of the board. Just plug any micro USB cable and you are good to go. Second option is battery. Supported batteries are standard Li-Ion/Li-Poly batteries with 3.7V nominal voltage. Connector for the battery is standard 2.00mm pitch JST connector (except on Inkplate 2, it uses SMD solder pads for battery terminals). The onboard charger will charge the battery with 500mA when USB is plugged at the same time. You can use battery of any size or capacity if you don't have a enclosure. If you are using our enclosure, battery size shouldn't exceed 90mm x 40mm (3.5 x 1.57 inch) and 5mm (0.19 inch) in height (excluding Inkplate 2, it uses [this battery](https://soldered.com/product/li-ion-baterija-600mah-3-7v/). [This battery](https://soldered.com/product/li-ion-battery-1200mah-3-7v/) is good fit for the Inkplate. Also, Inkplate's hardware is specially optimized for low power consumption in deep sleep mode, making it extremely suitable for battery applications.

### ⚠️ WARNING
Please check the polarity on the battery JST connector! Some batteries that can be purchased from the web have reversed polarity that can damage Inkplate board! You are safe if you are using the pouch battery from [soldered.com](https://soldered.com/categories/power-sources-batteries/batteries/lithium-batteries/) or Inkplate with the built-in battery . 

### ℹ NOTE
CR2032 battery is only for RTC backup. Inkplate cannot be powered with it.

## License
This repo uses the source code from another repositories. All their license files are located in "licences" folder.

## Where to get the Inkplate Motion?
You can get it by supporting by backing this project on the [Crowdsupply](https://www.crowdsupply.com/soldered/inkplate-6-motion).


For all questions and issues please reach us via [e-mail](mailto:hello@soldered.com) or our [contact form](https://soldered.com/contact/).
