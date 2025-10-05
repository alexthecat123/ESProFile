# ESProFile
A powerful ESP32-based emulator and diagnostic tool for ProFile and Widget hard drives.

![IMG_1486](https://github.com/user-attachments/assets/cca3458e-213d-405e-afff-d3c4edb18b24)
# Introduction
A couple years ago, I designed [ArduinoFile](https://github.com/alexthecat123/arduinoFile/), a ProFile emulator based around the Arduino Mega. You could also connect it to a real ProFile and use it as a diagnostic tool, but most people used it in its emulator mode. It worked well for a lot of people, but it also had some major problems that were pretty frustrating. The biggest of these issues was that it would work fine on a Lisa 1 or 2/5, but would behave unreliably (or just not work at all) on a 2/10, when connected to a parallel card, or when connected to an XLerated Lisa. Plus, there were several other enhancements that I wanted to make, and they have all come together into the ESProFile project!

# Key Features
Given that ESProFile is essentially an evolution of ArduinoFile, it's easiest to talk about its features by comparing its capabilities to those of ArduinoFile.

- ArduinoFile is based around an Arduino Mega, while ESProFile is based around an ESP32 (as you might be able to guess from the name). This makes it significantly more powerful, allowing me to add more features and fix pretty much all of the issues that ArduinoFile had.
- ArduinoFile would often fail to work when connected to a Lisa 2/10, parallel card, or XLerated Lisa, but ESProFile can handle all these configurations with ease!
- Being based around an Arduino Mega, ArduinoFile was more expensive to build than ESProFile. Looking on Amazon, you can get about 5 ESP32s for the same price as a single off-brand Arduino Mega, so it's a pretty significant cost savings!
- ESProFile's diagnostic mode is MUCH improved over ArduinoFile's. With ESProFile, you can send pretty much any command in the ProFile command set (including all the 5MB diagnostic Z8 commands, 10MB diagnostic Z8 commands, and Widget-specific commands) in a nice and user-friendly way, while ArduinoFile only supported a select few and couldn't help you a whole lot if you had a 10MB ProFile or Widget. You should be able to troubleshoot pretty much any drive-related issue with ESProFile, as well as performing tasks like backing up your drive and restoring from a backup.
- ESProFile is capable of low-level formatting both 5MB and 10MB ProFiles, as well as Widgets, while ArduinoFile could only do 5MB ProFiles and only gave limited error information. By contrast, if something goes wrong during your format on ESProFile, it'll tell you exactly what went wrong, which is really helpful. This kind of goes with the previous bullet, but since low-level formatting will probably be the most-used command in ESProFile's diagnostic mode, I wanted to list it separately. 
- ArduinoFile was only available in an external version, while ESProFile is available in both an external version and an internal Widget replacement version that mounts right into the Widget drive bay!
- I've selected the parts for ESProFile so that, if you order from JLCPCB, they can assemble the entire board for you for a really reasonable price. So once you get them in the mail, just pop your ESP32 into its socket, add a ribbon cable and SD card, and you'll be up and running!
- ArduinoFile required that you upload different sets of firmware to the device in order to switch between emulator mode and diagnostic mode. But on ESProFile, thanks to the ESP32's larger flash memory, both sets of code are stored on the device simultaneously and you can swap between them at the flip of a switch!

# Frequently-Asked Questions
As people begin to ask questions about ESProFile, I'll start throwing the answers to the most noteworthy ones in here!

## Q: I don't want to (or don't have the tools, etc) to install an OS onto my ESProFile from scratch. Can you provide a ready-to-go disk image?
Of course! I've uploaded preinstalled ESProFile images for every major Lisa OS [right here](https://mega.nz/folder/hxBXXKyA#KQHomG_6NikZonALWmdTCA). The LOS images are patched so that the serial number check is disabled, meaning that the apps will run on any Lisa, regardless of its serial number. And note that the Xenix 5MB image will only work on a Lisa 2/5 and the Xenix 10MB image will only work on a 2/10.

## Q: I've heard that the ESP32 isn't 5V-tolerant. So why aren't you using level shifters?
A very good question, and one that has been raised quite a few times with regards to my ESP32 projects over the years! So let's finally answer it. When picking a microcontroller to use when developing the successor to ArduinoFile, I had a couple of criteria, but one of the biggest was that the microcontroller be 5V tolerant so that I wouldn't have to use level shifters. My goal with this project was to make the board as simple and inexpensive as possible, and level shifters add both cost and complexity. Finding a microcontroller that was both fast and 5V-tolerant was actually really tough, but I eventually settled on the ESP32, even though they don't officially state its 5V-tolerance anywhere. I've got a couple reasons for this.

I don't have a source that I can quickly link to for this, but I read that Espressif (the makers of the ESP32) used to state that the ESP32 was 5V-tolerant, but later removed this claim from their website, and this is what led to a lot of the confusion on the topic. Apparently a lot of people took this claim to mean that they could _power_ their ESP32 off 5V (which definitely _would_ fry it) and would get mad when it blew up, so Espressif just stopped claiming 5V-tolerance to avoid all the drama.

I've also done some pretty extensive testing with ESP32s in a variety of 5V applications, and have never had a single problem. Plus, several other people have built my other ESP32 designs (although these designs aren't on GitHub) and they have used them extensively without any issues either. Just to be sure that I didn't get lucky with my ESP32s or anything, I've also been sure to buy from several different vendors at several different points in time, and every single one I've received has been rock-solid.

Perhaps most convincingly, the CEO of Espressif himself actually [verified this on Facebook](https://www.facebook.com/groups/1499045113679103/permalink/1731855033731442) a few years ago! Scroll down and find the comment with over 100 likes where he refers to the ESP8266, and then read the replies to that comment to find his answer that's specific to the ESP32.

One thing you'll notice when you search for info about ESP32 5V-tolerance is that some people say it works and others say it doesn't, but that not a single one of the people who says it won't work has actually tried it. They're all just saying that because they heard it from somewhere else or made some sort of assumption.
## Q: Clearly ESProFile works with the Lisa, but does it work with the Apple ][ and Apple ///?
The answer in the case of the Apple ][ is "I have no idea" because I don't actually have an Apple ][ ProFile interface card. But I would be pretty surprised if it didn't, and someone I know is working on testing this in the near future. If you've tried it, let me know so I can update this!

Update: Someone has now tested things with an Apple ][ and unfortunately it doesn't work. But I'm going to try and fix this as soon as I can!

Unfortunately, ESProFile does _not_ currently work with the Apple ///. I don't have my Apple /// with me right now, so I can't test it for myself, but I've had two people confirm this for me. Neither of them have the debug version of the board, so I don't have any data on why exactly it doesn't work, but I'm planning on looking into it once I'm reunited with my Apple /// again. Thanks to using DMA for ProFile data transfers, the Apple /// boasts the fastest transfer speeds of any ProFile-capable computer, so it's possible that it's just too fast and ESProFile can't keep up. Which would suck because the code is already pretty optimized and it would likely be impossible to fix. But it could also just be some weird timing thing in the bus handshaking, which would be completely fixable. We'll just have to wait and see. If you ordered the debug version of the board and want to hook it up to your logic analyzer and send me some traces of you trying to communicate with it from your Apple ///, then I'd love to take a look!

# Building One
With all that out of the way, let's talk about how to actually build an ESProFile!
## Hardware

### Variants
There are two different versions of the ESProFile hardware, and each of these has two subvariants. Note that the pictures of the boards shown below were taken of development versions of the PCBs, and the final boards that you'll be fabricating might have a couple visual changes and quality of life improvements.

- External PCB: This board is designed for use outside your Lisa, like how a ProFile is used. Given its small size and its form factor, this is also the best choice when using the board in diagnostic mode. Since it doesn't receive any power from the Lisa, the external ESProFile must be powered via USB whenever it's being used.

![IMG_1487](https://github.com/user-attachments/assets/0f368ebf-1d03-4b90-80c0-f3da04b81240)

- Internal PCB: This board is designed to be mounted in your Lisa's drive bay, replacing the Widget drive in a 2/10 (or the Sun20 drive in a 2/5). It has mounting holes that line up with the Widget's mounting holes, and the status LED is aligned with the Widget's LED cutout in the Lisa's front panel. The board is powered by the Widget power connector instead of USB, and also supports power from the spare Twiggy connector in a Lisa 2/5 if you want to mount it in a 2/5 like a Sun20. You can still use the board in diagnostic mode like the external model, but the form factor is less ideal for this and it's designed more with emulation in mind.
  
![IMG_1488](https://github.com/user-attachments/assets/eaf0bb2c-198f-4a7c-aecf-cbbfd7d85f83)

On ArduinoFile, I used an SD card breakout board so that the ArduinoFile would be entirely through-hole and people wouldn't have to solder a potentially-challenging SMD SD card slot directly. But for ESProFile, I opted to use an SMD SD card slot to facilitate fully-automated assembly by JLCPCB. This way, people who don't have any soldering experience can still easily build an ESProFile! This also makes the board look a lot better; I'm sure most ArduinoFile users agree that the breakout was really ugly!

Both the internal and external board variants come in debug and non-debug versions, with the only difference being that the debug version includes a pin header that exposes the ProFile bus for easy troubleshooting with a logic analyzer, while the non-debug version omits this, saving space and making things look nicer. All of the pictures in this document show the non-debug versions.

Unless you just really, really, really love soldering for some reason, I would recommend getting JLCPCB to assemble your boards for you. Not only is it easier, but it's also cheaper than ordering the parts separately, even after you factor in the labor costs. 

Picking between the debug and non-debug versions is entirely up to you; I've used the debug header on ArduinoFile to help several people troubleshoot issues remotely, but it's also rather ugly and unnecessary for a lot of people. And it's also a lot less necessary on ESProFile compared to ArduinoFile, given ESProFile's increased reliability, the large amount of testing that I've done, and the fact that assembly mistakes will be a lot less common since the majority of people won't be assembling the boards themselves.

The Gerber files for all four PCB variants can be found in the hw/boards folder. In general, I would recommend the external_noDebug board for 2/5 owners and people who primarily want to use ESProFile to troubleshoot their actual ProFiles and Widgets, and I would recommend the internal_noDebug board for 2/10 owners who want to mount an emulator internally. Or heck, they're so cheap that you could just get both!

### Ordering PCBs

I use [JLCPCB](https://jlcpcb.com/) for fabricating all my boards since it's nice and cheap and the quality is really good too. If you're just ordering boards that you're going to assemble by hand, then you can order them from wherever you want, but if you're planning on having the PCB fabricator assemble them for you, then it's easiest to use JLCPCB since I picked parts that are stocked in their parts library.

Given that some people reading this might not have any experience fabricating PCBs, I'm going to go into some detail here, so maybe just skip ahead a bit if you already know how to get a PCB fabricated and assembled through JLCPCB!

First, just go to JLCPCB's site and upload the Gerber zip file for the PCB that you want to get fabricated (found in one of the subfolders under hw/boards). This should present you with a screen like the one seen below. You can leave all the options at their default values, although I sometimes like to change the color to black because it looks cool and I also like to ask them to remove the order number from the board so that the silkscreen is nice and clean. This image shows both of those changes.

<img width="1920" alt="JLC_Board_Order" src="https://github.com/user-attachments/assets/ced7139b-33da-4036-845e-4c6b39928528" />

If you only want to fabricate the board and don't want them to assemble it for you, then just add the boards to your cart, check out, and you're done! You'll be able to find all the parts you need to assemble things a little later on at the end of this section.

If you want them to assemble the board for you, then scroll down and turn on the toggle for PCB Assembly. This presents you with the options shown below. Just leave all this at the default values and hit Next.

<img width="1920" alt="JLC_Assembly_Order" src="https://github.com/user-attachments/assets/a618cfe8-52cf-4363-bd4c-f6e8e9fa9930" />

The next screen that comes up is just a large preview of the front side of the board, so just hit Next again to skip this. There aren't any options to configure on that screen. After you do that, another screen will appear that asks you to upload a BOM file and a CPL file. The BOM file is the bill of materials that lists the part numbers of the parts that should be populated into the board, and the CPL (pick-and-place) file gives the coordinates of each part to the pick-and-place machines that insert components into the board. Just upload the BOM and PickAndPlace csv files from the same directory as your Gerber files, as shown in the screenshot below, and then hit Process BOM & CPL.

<img width="1920" alt="JLC_Assembly_BOM_CPL" src="https://github.com/user-attachments/assets/619076df-6bc5-4585-8fc9-09293be376af" />

Now you'll be presented with a list of all the parts that will be installed into the board. Assuming none of the parts are out of stock (or the stock is too low), you can just hit Next. But, as seen in the screenshot below, sometimes you get unlucky and something is out of stock. In my example, they don't have enough 15 Ohm resistors to populate R1 on all the boards.

<img width="1286" alt="JLC_Assembly_PartsList" src="https://github.com/user-attachments/assets/ed235a0c-6a73-4e22-a3cc-b5e0de0ef612" />

To fix this, click on the problematic part and read over the part info that pops up to see the dimensions and specs of the part. Then go to the Search Part tab and try to find a substitute. Once you find one, just hit Select, and then you can hit Next to proceed to the next page. You can see this part selection process below.

<img width="1294" alt="JLC_Assembly_PickSubPart" src="https://github.com/user-attachments/assets/0355b93b-eee5-4023-b39a-ebda81869d22" />

Now it's time for the fun part: on this next screen, we get to see a 3D model of the board with all the components populated! Just look over this to make sure that everything looks right, and then hit Next when you're done. Everything should be fine because I double-checked the files for every board, but if you see anything weird, you can rotate parts by clicking them and hitting the spacebar, and you can move parts around using the arrow keys to get them positioned just right. You can see this window below.

<img width="1297" alt="JLC_Assembly_Preview" src="https://github.com/user-attachments/assets/ee4b1978-6b2b-4c81-9aed-af19d48a17b9" />

The next screen gives you a summary of your order, which should be between $30-40 depending on which board you chose to order. But remember, that's for 5 fully-assembled ESProFiles (minus the ESP32), so only $6-8 apiece! You have to choose a product description on this screen for export reasons (or something) before adding it to your cart, so just pick something under the DIY category and proceed.

Now you can just place your order and wait for the assembled boards to arrive!

When I ordered my boards, they emailed me and asked me to confirm the placement of the Widget power connector on the internal version of the board since it only takes up half the footprint and looks a little confusing. So if you get this same email, just tell them that yes, it's correct as-is. If they email you asking to confirm anything else, and you're not sure what they're talking about, then feel free to contact me!

If you've elected to assemble the board yourself, you can get all the parts you need from [this DigiKey list](https://www.digikey.com/en/mylists/list/T5290A3F90). This contains the parts for the internal version of the board with the debug header, and you can just omit parts depending on which version of the board you're building. If you're building an ESProFile that doesn't include the debug header, omit the 15-pin male header. If you're building the external version of ESProFile, omit the 8-pin 3.96mm pitch male header, the 26-pin (2x13) male header, the 2-pin male header, the 1N4001 diode, and the 10uF capacitor. You can follow the schematics in the hw/schematics folder to better understand how everything goes together.

Regardless of whether you're getting the board pre-assembled or building it yourself, you'll need [an ESP32](https://www.amazon.com/dp/B07WCG1PLV) and [a microSD card](https://www.amazon.com/dp/B07R8GVGN9) (Lisa disk images are small, so any size is fine). Depending on your needs, you might also need to make an interface cable or two. If you're just planning on using ESProFile as a Widget replacement, then you won't need any cables. But you'll need to make [a ProFile cable](https://www.digikey.com/en/mylists/list/XJCXVWD9R8) if you're planning on using ESProFile as a ProFile replacement on a 2/5 (or if you're planning on connecting it to a ProFile in diagnostic mode). If you're also planning on connecting it to Widgets in diagnostic mode, you'll need to make [a Widget cable](https://www.digikey.com/en/mylists/list/S48Q9YR4S0) as well. The ProFile and Widget cables should look like the cables in the image below once you've put them together. Note the pin that you have to remove from the DB25 connector on the ProFile cable; on most Lisas, this pin is blocked off on the female connector.

Don't put the ESP32 into its socket yet; there's one thing we need to do down in the Software section before we can do that!

![IMG_1489](https://github.com/user-attachments/assets/e6ddb9e4-1dec-4146-b18f-54908e1c803c)

You can 3D-print a case for your external ESProFile if you'd like! There are 2 designs out there right now: a nice and compact case by Wottle on TinkerDifferent, and a larger case that actually looks like a real ProFile by Michael Schaffer. Here's how to print and use each of them:

If you want to use Wottle's design, then download the files in the hw/cad/Compact_External_Case folder and open them in your slicer to see what they look like. You'll be looking at two different versions of the case: one that has vents and another that doesn't. Just pick whichever one you prefer, and print the Case_Top and Case_Bottom for that version. Any material and print settings should be fine, although you might want to use supports if your printer is utterly terrible at bridging. Then your ESProFile should just slide right in, and you can secure it with three M2 x 5mm screws. Note that I added screwholes to the external PCB in revision 1.1, so you'll need to glue it in place or let sit free in the case if you have a v1.0 PCB. The top of the case should press-fit in place, and the finished product should look like one of the two cases in the following picture, depending on whether you picked the vented or unvented version:

![IMG_1552](https://github.com/user-attachments/assets/1ffd45e4-fff1-4eb2-906a-0efe998f9b41)

If you want to use Michael Schaffer's design, then download the files in the hw/cad/ProFile_External_Case directory, and import them into your slicer. It's just 2 files: the main ProFile body and an end cap to close things up once you insert your ESProFile. I'd recommend printing the main body flipped up on its side so that you don't have to use supports to hold up the entire top of the ProFile. You'll still probably want supports for the connector hole and the vents on the rear edge of the ProFile though! Print the end cap on its back to eliminate the need for supports too. Once everything's printed out, just slide your ESProFile down into the case, SD card end first, until it's gripped by the little plastic tabs. Make sure that the USB port, SD card slot, and interface connector line up with their openings on the case, and then secure your board with a drop of hot glue. Then pop the end cap on and you should be good to go!

I printed my case in white, but it would be really cool to have one in beige to match the original ProFile. If you know of a filament color that's close to the original ProFile, please let me know so I can try it out and link to it here!

Here are a couple pictures of the ProFile case once it's all put together:

![IMG_1826](https://github.com/user-attachments/assets/bb867493-5cc0-4c6e-a88d-d32b04a657ed)

![IMG_1828](https://github.com/user-attachments/assets/f3e075bb-9230-4c14-b7f3-0c5b22206a6a)

If you made the internal version, then you'll probably want to mount it inside your Lisa's drive cage! There are three main ways to do this: my (somewhat questionable) original way, and two 3D-printed mounting systems designed by JustDaveIII on LisaList2 and wottle on TinkerDifferent. The one designed by wottle mounts the board vertically just like my original mounting method, ensuring that the status LED lines up with the little lens thing on the Lisa's front panel. It's much cleaner than my mounting method though, and prevents you from having to use nuts as standoffs. JustDaveIII's solution mounts the board horizontally, with the downside being that the drive status LED doesn't line up with the Lisa's front panel, so it's much harder to see drive activity. JustDaveIII's design also lets you mount your external ESProFile internally, if you ever wanted to do that for some reason.

If you want to do things my original way, then remove any hard disk that's already in the drive bay, and then use two M3 x 10mm bolts and six M3 nuts to attach ESProFile to the side of your drive cage, using two of the original mounting holes for the Widget and the two large holes in the bottom of the ESProFile board. I got my nuts and bolts from [this assortment on Amazon](https://www.amazon.com/dp/B0BN297MP5). You'll want to mount ESProFile using the two frontmost holes on the right side of the drive cage, so that the status LED shines through the little transparent lens on your Lisa's front panel that the original Widget status LED also lined up with. Put the bolts through from the outside of the cage, and then screw a stack of two nuts onto the other side of each bolt. These will serve as standoffs to keep ESProFile from touching the side of the cage. Then slide ESProFile over the portion of the bolts that are still sticking out, and secure it in place with your final two nuts. I know this probably isn't the best mounting solution out there, but I've never claimed to be great with mounting hardware, so feel free to do something else if you have a better idea! Once you're done, it should look like the following pictures:

![IMG_1490](https://github.com/user-attachments/assets/937983e4-920b-4883-bd9f-0778829440d0)

![IMG_1475](https://github.com/user-attachments/assets/bc143815-9c61-4cf8-9d1e-009c586f10ed)

To use wottle's 3D-printed mounting system, download the ESProFile_Vertical_Mount.obj file from the hw/cad/wottle_Drive_Bay_Mount folder and open it in your slicer of choice. Slice it with whatever settings you want (it's so simple that anything should work) and print it out. Then mount things the same way that you would with my original mounting method, except use the 3D-printed bracket as a spacer instead of the two stacked nuts. You'll notice that the bracket has a third hole at the top. We can't screw into this from the outside because it would make it impossible to slide the drive cage into the Lisa properly, so instead screw an M3 x 6mm screw through the board and into the 3D-printed bracket from the inside of the drive cage. Once you're done, things should look like the following pictures:

![IMG_1607](https://github.com/user-attachments/assets/9f14f217-df46-40b7-a264-9404ce6283d7)
![IMG_1608](https://github.com/user-attachments/assets/57e0b5ab-ebb3-4dbe-a197-21c02d636ff2)
![IMG_1609](https://github.com/user-attachments/assets/41ee7b5f-839c-4bad-bb7d-089db9e5f0e2)

If you want to use JustDaveIII's 3D-printed bracket, then grab the files from the hw/cad/JustDaveIII_Drive_Bay_Mounts folder and open them in your slicer of choice. If you're planning on mounting an internal ESProFile (the most likely case), then just slice ESProFile_Widget_Bracket.stl. But if you're planning on mounting an external ESProFile board internally, then slice ESProFile_Widget_Bracket.stl as well as two copies of ESProFile_External_Drive_Bay_Mount.stl. Then go ahead and print them out (the files are so simple that pretty much any material and settings will work). Then, in the case of the internal ESProFile, you can mount it straight to the Widget bracket using three M3 bolts and nuts. If you've got an external ProFile, you'll first want to slide it into the grooves on the two small brackets that you printed, and then bolt the small brackets to the Widget bracket using four M3 nuts and bolts. Then use three M2 nuts and bolts to secure the Widget bracket to the right side (when looking at it from the front) of your drive cage. When mounted in this configuration, things will look like the following two pictures, depending on whether you followed this procedure with the internal or external version of ESProFile:

![IMG_1547](https://github.com/user-attachments/assets/18d3e47d-c0cc-44ee-a339-1211fcf1ad08)

![IMG_1549](https://github.com/user-attachments/assets/31d845bc-d1f0-4253-8c1b-1387e27bee1f)

Once it's installed, just connect the Widget data cable to the connector on the board labeled Widget Data, and connect the Widget power cable to the connector labeled Widget Power. Note that the top four pins on the Widget power cable are unused, so align the bottom of the Widget power cable with the bottom of the connector. If you instead have a Lisa 2/5 that's set up for use with a Sun20, you'll still connect your data cable to the Widget Data connector, but for power, you'll connect your spare Twiggy cable to the header labeled Twiggy Power, just like how your Sun20 was wired up.

If your drive cage has a fan and you want to power it (completely optional), then plug the fan into the Fan Power header on the bottom edge of the PCB. It should only fit one way!

That's about it for the hardware side of things, so let's get into the software now!

## Software

### ESProFile Firmware

We'll be uploading the ESProFile firmware to the board using the Arduino IDE, so go ahead and install that from [here](https://www.arduino.cc/en/software) if you don't already have it.

Once you've got it installed, use the IDE to open the ESProFile.ino file from the sw/ESProFile folder of this repo. This should automatically pull in ESProFile_Diagnostic.ino and ESProFile_Emulator.ino as well, since they're in the same directory.

The Arduino IDE doesn't have support for the ESP32 included out of the box, so add it by clicking the little circuit board icon on the ribbon at the left of the screen, searching for "esp32" in the pane that appears, and then clicking Install next to the "esp32" result. Make sure you're installing the package from Espressif Systems, NOT the one from Arduino!

<img width="1728" alt="Boards_Manager" src="https://github.com/user-attachments/assets/bdf66031-0194-46eb-9e47-aa62ada1f26b" />

Now we need to install the SDFat library that the ESP32 uses to communicate with the SD card. To do this, click the little book icon that's right below the board icon, and then search for "SDFat". There should only be one result called SDFat, so install it (just to make sure you're installing the right one, the author is Bill Greiman).

<img width="1728" alt="Libraries_Manager" src="https://github.com/user-attachments/assets/c5864be5-15bd-47c1-a623-cb56942bb92e" />

And that's it for the setup! 

We can now connect to the board using the Arduino IDE. Connect a Micro-USB cable between the ESP32 and your computer, and then select Tools -> Board -> esp32 -> ESP32 Dev Module to tell the IDE what type of board we're using. Then go to Tools -> Port and select whichever port your board is connected to. If you're not sure which one it is, just unplug the board, check this menu, plug the board back in, and check the menu again. The port that appears when you plug the board back in is the one you want!

<img width="752" alt="Board_Port" src="https://github.com/user-attachments/assets/2ccaa2ca-b6de-436e-8522-586a9cfe8763" />

Before we actually upload the code, we need to configure an efuse within the ESP32. The ESProFile board holds the ESP32's pin 12 high at boot thanks to some pullup resistors, which has the unwanted side effect of telling the ESP32 to power its flash from 1.8V instead of 3.3V, preventing it from booting properly. By using a tool to set an efuse, we can tell it to ignore that pin and always power the flash from 3.3V, solving our problem. Installing the proper tool, esptool, is slightly different depending on your OS.

#### Windows
Download the win64 version of esptool from [here](https://github.com/espressif/esptool/releases/) and extract it to a folder on your computer. Then open a Command Prompt and cd into whatever folder you just extracted everything to. Then type:
```
espefuse.exe --port myPort set_flash_voltage 3.3V
```
Replace myPort with whatever port you just selected in the Arduino IDE.

#### macOS
If you don't already have it, install Homebrew by opening a terminal and pasting in this command:
```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
Once it's installed, type:
```
brew install esptool
```
And then you can run the command:
```
espefuse.py --port myPort set_flash_voltage 3.3V
```
Replace myPort with whatever port you just selected in the Arduino IDE.

#### Linux
If you're a Linux user, you're probably technical enough that I don't really have to explain the installation process. Just install esptool using your package manager, and then type:
```
espefuse --port myPort set_flash_voltage 3.3V
```
Replace myPort with whatever port you just selected in the Arduino IDE.

#### Finishing Up
Note that, regardless of which OS you're using, espefuse may ask you to type "BURN" in order to confirm that you'd like to burn the efuse.

Now that the efuse programming is complete, you can install the ESP32 into its socket on the ESProFile board. And burning an efuse is permanent, so you'll never need to mess with it again and you can do all future firmware updates without removing the ESP32 from your PCB.

Now go back to the Arduino IDE, click the Upload button (the arrow in the upper-left corner of the screen), and wait for the firmware to be uploaded to your board. And that's it; your ESProFile is now flashed with its firmware!

### SD Card
Before using ESProFile in emulator mode, you'll probably want to put some files onto the SD card to make your life easier. ESProFile supports the Selector (discussed later), which allows you to manage disk images right from your Lisa, so we need to put the Selector's files on the SD card if you want to use it (highly recommended). And regardless of whether or not you want to use the Selector, we need to format the SD card. So go ahead and format the card as FAT32, and then copy the contents of the SDTemplate folder over to the card if you want to use the Selector. There's a PDF file in that SDTemplate directory too written by @bmwcyclist that gives instructions on how to create your own blank ESProFile images, in case you're not sure!

Note that you may encounter issues if you use a really cheap and/or slow SD card, especially after the v1.2 emulator update that improves access speeds. The most likely symptom of this is your status LED staying red, and an "SD card initialization failed!" error message printing out over the serial console. If you get this message, just try a faster SD card and everything should work fine. Pretty much anything from the big storage brands should be fine, including the one from PNY that I linked to earlier in the Hardware section. If the problem persists, then double-check that you formatted your card as FAT32 and not something else!

If you don't care about speed and just really, really want to use the slow SD card that you have laying around, then don't worry, it's still absolutely possible with a simple change to the code! Just open the ESProFile_Emulator.ino file in the sw/ESProFile folder, and change the line that says:
```
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(20), &SD_SPI)
```
To:
```
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(10), &SD_SPI)
```
This tells ESProFile to clock the card at 10MHz instead of 20MHz, which should allow pretty much any SD card in existence to work, at the expense of a pretty significant amount of speed. If you make this change and it still doesn't work, then I would highly recommend that you get a new SD card!

Now we can talk about how to actually use this thing!

# Using It!

Switching between emulator mode and diagnostic mode is really easy; just flip the switch on the ESProFile board to the desired position, and then either hit the reset button on the board or power-cycle it. The switch only takes effect after ESProFile has been reset, so don't worry about accidentally changing modes in the middle of operation if you accidentally hit the switch!

## Emulator Mode

In emulator mode, ESProFile can be connected to a Lisa and will serve as a replacement for an original ProFile or Widget hard drive. There isn't a lot to say here; plug it in and it should just work.

At power-on, ESProFile looks for an image on the SD card called profile.image (case-sensitive) and if this image is not found, then it will halt. If you're using the Selector, then the Selector image is already named this, and you can name your actual disk images whatever you want. If you're not using the Selector, then you must change the name of whichever image you want ESProFile to use to profile.image.

### Selector Info

[The Selector](https://github.com/stepleton/cameo/tree/master/aphid/selector) is an incredibly useful program by Tom Stepleton that allows you to manage your collection of disk images right from your Lisa, and I would highly recommend using it. Just like Tom's own Cameo/Aphid emulator, ESProFile is fully compatible with the Selector's set of commands. For more info about the Selector, and how to use it, read [its user's manual](https://github.com/stepleton/cameo/blob/master/aphid/selector/MANUAL.md), but the gist of it is that you can create a bunch of disk images with different OS's on them, and select the one that you'd like to boot from whenever you start up your Lisa. Pretty awesome! To get back to the Selector interface after you've already used it to select another image, simply turn off your Lisa, hit ESProFile's reset button or power-cycle the board, and then turn your Lisa back on.

Two things to note about the Selector on ESProFile.
- Given that ESProFile's SD card accesses aren't as fast as the accesses on Tom's PocketBeagle-based Cameo/Aphid emulator, it can take several seconds to execute a Copy operation in the Selector. This causes the Selector to time out, and it'll even tell you that this could be caused by a slow SD card. Don't worry; just wait until ESProFile's status LED turns green again, and then you should be good to go! Note that your newly-copied image probably won't show up in the image list because of the timeout, so either execute another Selector command that refreshes the image list or exit and reenter the Selector to get it to show up. Copying a 5MB ProFile will take about 7-10 seconds, and the times will go up from there as the drive size increases. Luckily, copying images isn't something that you do super often, so this isn't too big of a problem.
- It takes quite a bit of time (up to 10 seconds) for ESProFile to count up the free space on larger SD cards, once again thanks to slow SD card accesses. The Selector requests this information frequently (once every second or two, although I've written ESProFile's code to only update it when a file operation has occurred), which leads to long delays where the Selector just freezes while it waits for ESProFile to send this information. This is so annoying that I've just disabled the free space reporting, just like on ArduinoFile, and ESProFile reports "????????" for the amount of space available on the SD card. This isn't a huge deal though because Lisa disk images are really small and modern SD cards are large enough that you never need to worry about running out of space!

### Serial Interface

If you're curious about what commands your Lisa is sending over to your emulated drive, just open up a serial connection with your ESProFile at 115200 baud, and you'll be able to see the commands scroll by in real time as the Lisa sends them. On ArduinoFile, it just printed the raw command bytes, but I've updated ESProFile so that it actually interprets the commands for you, allowing you to see exactly what your Lisa is doing!

## Diagnostic Mode

Unlike emulator mode, there's a whole lot to say here, just because of how many commands there are!

When you switch into diagnostic mode, ESProFile will allow you to send commands to an actual ProFile or Widget that you connect to the board in order to troubleshoot it, perform backups/restores, low-level formats, and many other utility functions.

To access the ESProFile diagnostic interface, connect to ESProFile over serial at 115200 baud. Note that ESProFile sends ANSI escape codes (for things like clearing the screen and moving the cursor), so make sure that you're using a terminal program that supports this. Otherwise it'll look pretty ugly. Note that the Arduino IDE's built-in serial monitor does NOT support escape codes, so you'll need to use something else. I can't make a recommendation for Windows or Linux, but on Mac, I use [Serial](https://www.decisivetactics.com/products/serial/). It's paid software, and I know it might seem really dumb to pay for a terminal program (that's certainly what I thought at first), but it's really nice and worth the money in my opinion!

Once you're connected to ESProFile, you should see a main menu with a variety of options that looks like this:

<img width="517" alt="Main_Menu" src="https://github.com/user-attachments/assets/5bec0009-8317-46a5-b9ba-7f466b327dc5" />

This main menu also contains several submenus, including menus for the 5MB and 10MB ProFile diagnostic command sets, as well as Widget-specific commands. There is also a submenu for drive tests, allowing you to select various options for exercising/burning in your drive.

A couple of notes before we get into all of the menu options.
- All numbers that ESProFile displays and requests from you should be in hex, unless specified otherwise.
- Unlike ArduinoFile, ESProFile does NOT require you to type out leading zeros whenever it requests user input that must be a certain length. For instance, if you wanted to access block 0 on ArduinoFile, you'd have to type out 000000, while you can just type 0 on ESProFile.
- When performing particular operations, ESProFile will ask you if you want to use default values or specify your own values for certain parameters. In the VAST majority of cases, you can just leave these at the default, so don't worry if you don't know what they mean. In fact, I don't think that I've ever had to override any of these default values when using ESProFile in practical applications.
- Any command that can destroy all the data on your disk will warn you before proceeding, so don't worry about accidentally wiping your drive!
- Pretty much any command that iterates over multiple blocks and takes a while to execute (like a sequential read test or a search of the drive for a string) can be interrupted by pressing return. The only exception to this that I can think of at the moment is a low-level format; that's something that you really don't want to interrupt unless you absolutely have to!

Now let's get into all the menu options. There are so many of them that it would be impractical to show pictures for each one, so I'll just show pictures of each major submenu and provide descriptions of the other menu options. When you select any option, ESProFile generally makes it pretty easy to understand how to provide any info needed to execute it, so you shouldn't have any problems there.

### Main Menu
#### 1) Reset Drive
Sends a soft reset command to your drive. You shouldn't really ever need to use this on ProFiles, but it's useful on Widgets when they get stuck. This command can cause your drive to start doing stuff; wait for drive activity to stop before executing anything else.
#### 2) Get Drive Info
Reads block FFFFFF (the spare table, basically your drive's info block) from your drive to get some general information about it. You'll be shown both the raw hex data, and a human-readable interpretation of it. Most of this info is self-explanatory; the only two things that some people may not understand are the "Total Spares" and "Spares Allocated" entries. A "spare" is just a spare block on the disk that the ProFile can move data to if any block on the drive goes bad, so "Total Spares" tells you the total number of spare blocks that your drive contains, and "Spares Allocated" tells you how many of these are actually in use. A high number here means that your drive may not be in great shape, or could need a low-level format.
#### 3) Read Block Into Data Buffer
Reads the 532-byte contents of a user-specified block number into ESProFile's data buffer. The maximum block number that you can access on a 5MB ProFile is 25FF, and the maximum on a 10MB ProFile or Widget is 4BFF. And of course, you can also read the "magic" block of the spare table (block FFFFFF) as well.
#### 4) Modify Data Buffer
Allows you to manually modify the contents of the data buffer; useful if you want to write specific data to a block. This command will show you the buffer's current contents, ask you for the address (from 0 to 213 hex) that you want to modify, and then prompt you to enter the hex data that you'd like to insert at that address. If your data runs over the end of the buffer, it'll simply prompt you to enter it again. And once you've entered your data, it'll show you the modified buffer contents.
#### 5) Fill Buffer With Pattern
Allows you to fill the data buffer with a repeating pattern of any length. Shows you the current contents of the buffer, prompts you for your hex pattern that you would like to fill with, and then shows you the buffer once you're done. For instance, if you enter 789ABC, the buffer will be 789ABC789ABC789ABC789ABC789ABC and so on, with the pattern being cut off once we reach the 532nd byte.
#### 6) Write Buffer to Block
Writes the 532-byte contents of the ESProFile data buffer into a user-specified block on the drive. The maximum block number that you can write on a 5MB ProFile is 25FF, and the maximum on a 10MB ProFile or Widget is 4BFF. Note that you can't overwrite the spare table (or at least not this way).
#### 7) Write-Verify Buffer to Block
Exactly the same as the above, except we ask the drive to perform a write-verify operation instead of just a standard write.
#### 8) Write Buffer to Entire Drive
Writes the contents of the ESProFile data buffer to every single block on the drive. Useful for erasing the drive, or filling it with a particular pattern if you want to do that for whatever reason.
#### 9) Write Zeros to Drive
Writes zeros to every single block on the drive; basically a subset of the above. This is just really useful for erasing data, so I made it a separate easy-to-execute command, but you could accomplish the same thing by using 5) to fill the buffer with a pattern of zeros and then executing 8) after that.
#### A) Compare Every Block With Buffer
Reads every block on your drive and compares the contents of each block with the contents of the data buffer. Will display errors telling you which blocks are mismatches if there are any. This is useful for double-checking that every block on your drive can be written properly; first, execute 8) to fill every block on the drive with the same data, and then execute this command to make sure that the write "sticks" for every block on the drive.
#### B) Search Entire Drive For String
Allows you to search the entirety of your drive for a string of hex bytes, a string of text, or the full contents of the data buffer. It will ask you which of these you want to search for, and then ask you for your desired string if you choose one of the first two. Then sit back and wait while it searches the drive. It will print the block numbers where it found any matches as it goes.
#### C) Backup Entire Drive
Backs up the entire contents of your drive to your modern computer over serial. The backup occurs over XMODEM to add error checking, so make sure that your terminal program supports it! When ESProFile asks you to start your XMODEM receiver, do as it says, and make sure that you've selected CRC as the form of error checking that it uses. Now just wait (for quite a long time) for the transfer to complete. The image that it creates is compatible with ESProFile in emulator mode, so you can just copy it right over to your SD card and immediately start emulating the drive!
#### D) Restore Drive From Backup
Restores a drive from a backup that was previously created using C). Just like backups, this uses XMODEM, so start your terminal software's XMODEM sender when prompted and select your desired image file. Make sure that your XMODEM sender is configured to send using a 1K block size, and hit start! Now wait for a while, and your drive should be good to go!
#### E) Show Command, Status, and Data Buffers
As the name implies, this option shows the contents of ESProFile's command, status, and data buffers. So this will tell you what the last command was that was sent to the drive, and the last status that the drive reported back, complete with human-readable interpretations for each. And it also shows what's currently in the data buffer, but you're already very familiar with how the data buffer works by now!
#### F) Send Custom Command
This one's only for more advanced users. It allows you to send a custom 6-byte command to your drive. Since ESProFile already supports sending all the standard drive commands, this is only useful if you have some sort of weird prototype drive that we've never seen before, or you have some other special use case that I'm not thinking of right now. When it asks for your command, you MUST enter a full 6 bytes of hex, so this is one of the few cases where you actually have to pad with zeros.

That's everything in terms of actual commands; these next four options are submenus, and their contents will be enumerated below.

### G) Drive Tests Submenu
This submenu contains several burn-in tests for exercising your drive, as shown in the picture below.

<img width="518" alt="Drive_Tests" src="https://github.com/user-attachments/assets/600343e5-1622-46cb-9600-a000dbbfd980" />

Here's a description of each of them. All tests will prompt you with the option to loop them forever, if you wanted to do something like burning in your drive overnight, in which case the output will display the pass number that the test is currently on. You can always interrupt a test by pressing return on your keyboard.
#### 1) Sequential Read
This test goes through and reads every block sequentially from block 0 up to the highest block that your drive supports. If any read errors are encountered, it'll let you know.
#### 2) Sequential Write
Just like sequential read, except this time it writes data to each block sequentially instead. And once again, it'll print any errors.
#### 3) Repeatedly Read Block
If you have a single block on your drive that's a bit flaky, this test lets you stress test it. Simply specify the block number, and ESProFile will read it over and over again as fast as possible until you interrupt it by pressing return. Any read errors will be displayed.
#### 4) Repeatedly Write Block
Just like the repeated read test, except this one writes the block instead.
#### 5) Random Read
Reads random block numbers from your drive, printing any read errors along the way. This is really useful for exercising your drive's ability to seek reliably, since the next block that's going to be accessed could be anywhere on the disk. Note that this test will perform a number of reads that is equal to the number of blocks that your drive contains, which does not guarantee that every block on the drive will be accessed due to the randomness. Some may not be accessed at all, and others may be accessed multiple times, but things should be evenly distributed for the most part.
#### 6) Random Write
Just like the random read test, except this one does writes instead of reads.
#### 7) Butterfly Read
This test goes through and reads the highest block on the disk, followed by the lowest, adds one to the lower bound and subtracts one from the upper bound, and then repeats until every block on the disk has been accessed, printing errors along the way. So on a 5MB ProFile, this would look like 25FF -> 0 -> 25FE -> 1 -> 25FC -> 2, and so on. This leads to the heads taking long strokes across the disk, gradually becoming shorter and shorter as we reach the middle of the disk, and then gradually becoming longer and longer again until the numbers have fully reversed and we reach the ends of the disk again. Due to the long strokes of the heads, this is good for stressing the positioning mechanism and testing seek reliability, but maybe not quite as good as the unpredictability of the random read test. However, unlike the random read test, this one guarantees that every block on the disk will be visited.
#### 8) Butterfly Write
Just like butterfly read, except we're writing to the disk instead of reading.
#### 9) Read-Write-Read
Tests your disk's data retention by first reading the contents of the disk, writing each block with a test pattern, and then rereading everything to compare it with the data that was written. Errors will be reported along the way in each phase.
#### A) Return to Main Menu
I would be pretty concerned about you if you couldn't figure out what this one did.   
<br />
<br />
The next submenu on the main menu is:
### H) 5MB ProFile Diagnostic Z8 Commands Submenu
In order to execute any of the commands in this menu, you must have a 5MB ProFile with a [2K (or 4K) ROMless Z8](https://www.ebay.com/itm/394267646989) that's fitted with the LLF version of the 5MB ProFile ROM. A standard ProFile Z8 processor with its built-in mask ROM can't execute these commands. When attempting to enter this menu, ESProFile will try to detect if your ProFile has the proper ROM fitted, and will warn you (but still let you proceed) if not. You can find the firmware (both standard and LLF) for both the 5MB and 10MB ProFiles [here](http://www.bitsavers.org/pdf/apple/disk/profile/firmware/). For the 5MB ProFile, you can burn this into either a 2716 or (if you copy-paste the image twice) a 2732 EPROM.

The 5MB diagnostic menu looks like this:

<img width="519" alt="5MB_Diag" src="https://github.com/user-attachments/assets/d07ebb16-e919-4d56-9cd6-084bedd8ab3b" />

The options in this menu are as follows.
#### 1) Read CHS Into Data Buffer
Just like the read command on the main menu, except now you get to specify the block in terms of physical cylinder, head, and sector instead of logical block.
#### 2) Write Buffer to CHS
Same as 1), except this time we're writing to a CHS instead of reading.
#### 3) Repeatedly Read CHS
Reads a particular cylinder, head, and sector over and over again until you stop it. Very similar to Repeatedly Read Block in the Drive Tests submenu, except this one is done by CHS instead of block number.
#### 4) Repeatedly Write CHS
Same as 3), but this time we're writing instead of reading.
#### 5) Write-Verify Buffer to CHS
Just like the write-verify command on the main menu, except the block is specified in terms of physical cylinder, head, and sector instead of logical block.
#### 6) Low-Level Format
This one's probably going to be the most-used command out of everything in the entirety of ESProFile! It does exactly what the name says: it low-level formats your 5MB ProFile, which often fixes drives that produce a lot of read/write errors or even drives where you can't access the disk at all. It consists of three steps. First is the Format step, which does the actual low-level format itself. Then we perform a Scan operation, which scans over the surface of the disk to check for any defects that are still there after formatting. This scan is identical to the one that takes a minute or two when you first turn on your ProFile. Finally, it executes the Init Spare Table step, which creates a new spare table on the disk, updating it with any defects that were found during the Scan step. In order to execute the Format step, a jumper must be installed on the ProFile digital board bridging the two pins on the header at P7, so install and remove the jumper when prompted. This format operation is easy to use, but if it fails and you want verbose error information, then I would recommend running the 9) Format, A) Scan, and B) Init Spare Table commands individually (in that order) for more error info.
#### 7) Read Drive RAM Into Buffer
This one's going to be for advanced users only. It does exactly what it says; it reads the contents of the ProFile's RAM into ESProFile's data buffer. Since the RAM is larger than the 532-byte data buffer, you get to specify the starting address that you want to read from. The contents of RAM are very difficult to decipher and are specific to the current state of the ProFile, so this command is only going to get used in very niche circumstances.
#### 8) Write Buffer Contents to Drive RAM
Just like 7), except for writing to the drive's RAM. Be careful doing this, because it could really confuse your drive and require you to power cycle it if you mess up.
#### 9) Format
Performs the Format step (step 1/3) of a low-level format operation, without performing the following steps. This one's useful if you want more control over the formatting process, or if the all-in-one low-level format command fails and you want more error info from the drive. In order to execute this command, a jumper must be installed on the ProFile digital board bridging the two pins on the header at P7, so install and remove the jumper when prompted.
#### A) Scan
Performs the Scan step (step 2/3) of a low-level format operation, without performing the other two steps of the format. You can also use it outside the context of a low-level format to scan your drive for defects. If the all-in-one low-level format command fails, you can run the three steps, including this one, individually to get more error info. This is identical to the surface scan that your ProFile takes a minute or two to complete when you first power it on.
#### B) Init Spare Table
Performs the Init Spare Table step (step 3/3) of a low-level format operation, without performing the other two steps of the format. You can also just run it on its own to recreate your spare table if you ever want to do that for some reason, without doing a low-level format. If the all-in-one low-level format command fails, you can run the three steps, including this one, individually to get more error info.
#### C) Disable Head Stepper
Normally, the stepper motor that moves the heads is energized so that it's very difficult to move by hand. It's basically locked in place unless the ProFile commands it to move. But if you run this command, then the stepper will be disabled, and you can easily rotate it by hand until the next command gets sent, at which point it'll turn back on again.
#### D) Modify Data Buffer
Exactly the same as Modify Data Buffer on the main menu, just placed here as well for convenience.
#### E) Fill Buffer With Pattern
Exactly the same as Fill Buffer With Pattern on the main menu, just placed here as well for convenience.
#### F) Show Command, Status, and Data Buffers
Exactly the same as Show Command, Status, and Data Buffers on the main menu, just placed here as well for convenience.
#### G) Return to Main Menu
As I said with the Drive Tests submenu, I'd be pretty worried about you if you couldn't figure this one out!   
<br />
<br />
Our next submenu on the main menu is:
### I) 10MB ProFile Diagnostic Z8 Commands Submenu
In order to execute any of the commands in this menu, you must have a 10MB ProFile with a [4K ROMless Z8](https://www.ebay.com/itm/394267646989) that's fitted with the LLF version of the 10MB ProFile ROM. A standard ProFile Z8 processor with its built-in mask ROM can't execute these commands. When attempting to enter this menu, ESProFile will try to detect if your ProFile has the proper ROM fitted, and will warn you (but still let you proceed) if not. You can find the firmware (both standard and LLF) for both the 5MB and 10MB ProFiles [here](http://www.bitsavers.org/pdf/apple/disk/profile/firmware/). For the 10MB ProFile, you should burn this into a 2732 EPROM.

The 10MB diagnostic menu looks like this:

<img width="516" alt="10MB_Diag" src="https://github.com/user-attachments/assets/bb599b76-acf8-4dad-a01c-bb15cf8c1064" />

And here are all the commands within it!
#### 1) Read CHS Into Data Buffer
Just like the read command on the main menu, except now you get to specify the block in terms of physical cylinder, head, and sector instead of logical block.
#### 2) Write Buffer to CHS
Same as 1), except this time we're writing to a CHS instead of reading.
#### 3) Repeatedly Read CHS
Reads a particular cylinder, head, and sector over and over again until you stop it. Very similar to Repeatedly Read Block in the Drive Tests submenu, except this one is done by CHS instead of block number.
#### 4) Repeatedly Write CHS
Same as 3), but this time we're writing instead of reading.
#### 5) Seek Heads
Seeks the drive's heads to the user-specified cylinder, head, and sector, but doesn't read or write the data there. This is useful in conjunction with the Format Tracks(s) and Erase Tracks(s) commands, as discussed later on.
#### 6) Low-Level Format
Probably the third-most used command, after the 5MB ProFile LLF and the Widget LLF. It does exactly what the name says: it low-level formats your 10MB ProFile, which often fixes drives that produce a lot of read/write errors or even drives where you can't access the disk at all. The 10MB ProFile's formatting commands are a good bit more flexible and verbose than the 5MB ProFile's commands, and the format consists of four steps. First is the Format step, which does the actual low-level format itself. Then we verify the integrity of the sectors that we'll be writing the spare table to by writing some test patterns to them and reading them back. Next, we perform an Init Spare Table operation to actually create the spare table within those sectors. And finally, we perform a Scan operation, which scans over the surface of the disk to check for any defects that are still there after formatting, and they are automatically entered into the spare table. This scan is identical to the one that takes a minute or two when you first turn on your ProFile. In order to execute the Format step, a jumper must be installed on the ProFile digital board bridging the two pins on the header at P7, so install and remove the jumper when prompted. This format operation is easy to use and gives a lot more error info than the 5MB LLF routine, but you can get slightly more error info by running the Format Track(s), Init Spare Table, and Scan commands, in that order.
#### 7) Format Track(s)
Performs the Format step (step 1/4) of a low-level format operation without performing the following steps. The user can also choose to only format the track that the heads are currently over, and the heads can be moved to the desired track before running this command by using the Seek command. This one's useful if you want more control over the formatting process, or if the all-in-one low-level format command fails and you want more error info from the drive. In order to execute this command, a jumper must be installed on the ProFile digital board bridging the two pins on the header at P7, so install and remove the jumper when prompted.
#### 8) Scan
Performs the Scan step (step 4/4) of a low-level format operation, without performing the other three steps of the format. You can also use it outside the context of a low-level format to scan your drive for defects. If the all-in-one low-level format command fails, you can run the individual steps, including this one, individually to get more error info. This is identical to the surface scan that your ProFile takes a minute or two to complete when you first power it on.
#### 9) Init Spare Table
Performs the Verify Spare Sectors Integrity and Init Spare Table steps (steps 2/4 and 3/4) of a low-level format operation, without performing the other three steps of the format. You can also just run it on its own to recreate your spare table if you ever want to do that for some reason, without doing a low-level format. If the all-in-one low-level format command fails, you can run the individual steps, including this one, individually to get more error info. Note that, unlike with the all-in-one formatter, you can elect not to verify the integrity of the spare table sectors if you run this command separately.
#### A) Test ProFile RAM
Commands the drive to perform a self-test of its RAM. If any errors are found, the address at which the error occurred will be displayed, as well as the data that was expected to be read and the actual incorrect data that was read out.
#### B) Read Header
Reads the header field from the user-specified cylinder, head, and sector. The header is essentially the behind-the-scenes information on the disk that the drive uses to determine what cylinder and sector it's over and what head it's using, and without this info, it will fail to seek properly and fail to access any data on the disk. The headers are only written during low-level formatting and can fade over the years, which is why a LLF that rewrites the headers can often revive a marginal drive. This command will read the header and give you a human-readable interpretation of it. It'll also try to check and see if all the header fields are valid, and alert you if the integrity of the header seems to be compromised.
#### C) Erase Track(s)
Erases either all tracks on the disk or only the track that the heads are currently positioned over, depending on what the user wants. Similar to the Format Track(s) command, except this one only erases the tracks without formatting them. Like Format, this one also requires you to install a jumper on the header at P7 on the ProFile digital board, so install and remove the jumper when prompted. If you choose the "only erase one track" option, then you can position the heads over the desired track by executing the Seek command before running this one.
#### D) Get Result Table
Several of the commands in this menu (such as the formatting commands and the RAM test) produce a result table containing info about the success or failure of the command. Executing this command retrieves the 532-byte result table from the drive, puts it into ESProFile's data buffer, and shows it to you. Most of the commands that produce a result table already display and interpret it for you as part of the command, so you shouldn't need to use this much (if ever), but it's there if you ever need to retrieve that data again for whatever reason. Although it won't be interpreted for you in this case because ESProFile has no way of knowing what command produced it.
#### E) Disable Head Stepper
Normally, the stepper motor that moves the heads is energized so that it's very difficult to move by hand. It's basically locked in place unless the ProFile commands it to move. But if you run this command, then the stepper will be disabled, and you can easily rotate it by hand until the next command gets sent, at which point it'll turn back on again.
#### F) Park Heads (Do This Before Exiting!)
Moves the heads to their parked position, off the data-holding portion of the hard disk platters. When the ProFile is fitted with its standard ROM, this is done automatically, but it does NOT auto-park when fitted with the LLF ROM, so you have to do this manually. It's highly recommended to park the heads before powering off your drive to avoid damage to the disk surface!
#### G) Send Custom 10MB Diagnostic Command
This option is going to be for advanced users only. Similar to F) on the main menu, except it allows you to send a diagnostic command instead of a standard command. Unlike the 5MB ProFile diagnostic command format, the 10MB ProFile diagnostic command format is different from the standard ProFile command format, so we have a separate menu option here for the 10MB drive. Just enter your command, answer a few questions about it, and you're good to go!
#### H) Modify Data Buffer
Exactly the same as Modify Data Buffer on the main menu, just placed here as well for convenience.
#### I) Fill Buffer With Pattern
Exactly the same as Fill Buffer With Pattern on the main menu, just placed here as well for convenience.
#### J) Show Command, Status, and Data Buffers
Exactly the same as Show Command, Status, and Data Buffers on the main menu, just placed here as well for convenience.
#### K) Return to Main Menu
It does exactly what it says.   
<br />
<br />
The final submenu is:
### J) Widget-Specific Commands Submenu
Boy, is this one long! And it's got yet another submenu under it too! But the good news is that the Widget's abundance of commands sure comes in handy when you're troubleshooting one! Although some of these commands will execute on any Widget, others require a Widget running firmware version 1A45. If you try to enter the menu with a non-1A45 Widget, you'll be warned that certain commands may not work, but you can absolutely still try as many commands as you want at your own risk. If you don't have the 1A45 firmware, you can download it from [here](http://www.bitsavers.org/pdf/apple/disk/widget/firmware/). Just burn it into a 2764 EPROM and replace the Widget's existing 2764 with your new one. And there's no need to swap back to your Widget's original ROM when you're done; you can just keep the new one in there forever! Unlike the ProFile, the Widget doesn't have separate LLF and standard ROMs, so all the commands, including low-level format, are available from the same stock Widget ROM!

The Widget is a really complex drive, and it would be really hard for me to do all these commands justice here, so I'd suggest reading the Widget ERS document starting on page 81 and ending on page 133 (of the PDF, not the page numbers written in the document itself) to truly become a Widget pro.

The Widget menu looks like this:

<img width="516" alt="Widget_Diag" src="https://github.com/user-attachments/assets/16328dd9-30e7-4549-899e-d440dea6bc9a" />

And now onto the commands!
#### 1) Soft Reset
This command is very similar to the Reset Drive command on the main menu; it's just a Widget-specific version of it. The controller will perform a quick self-test and you'll see some head activity for a few seconds. If the drive ever starts behaving weirdly, execute this command and it'll probably fix things up!
#### 2) Reset Servo
This command resets the servo board on the Widget, which is really useful if the servo has encountered some kind of unrecoverable error and is refusing to respond. The light on the front of the Widget will probably go out after you've executed this command, but it should turn back on once you execute another command.
#### 3) Get Drive ID
This command is identical to the Get Drive Info command on the main menu; it's just that the Widget doesn't store this in the spare table, so it's called something a little different.
#### 4) Read Spare Table
Reads the Widget's spare table into ESProFile's data buffer. Unlike the ProFiles, where the spare table contains drive ID info as well as the spare block information, the Widget's spare table only contains the spare block information. To get the drive ID, run 3) instead. Given that the Widget stores these separately, things are a lot more detailed here than in the ProFile's spare table, and ESProFile interprets all the information in the spare table for you.
#### 5) Get Controller Status
Returns detailed status info from the Widget's controller board, and interprets all of it for you. There are a lot of status fields here, but they can be super useful. One that you might find particularly helpful is Byte 1 of the State Registers field, which contains some overall error information. Any field that has a "/" before it (like "/CRC error") means that it's active low, so if you see "/CRC error: 1", then things are actually fine because it's active low and it would need to be a 0 for there to actually be a CRC error.
#### 6) Get Servo Status
Returns detailed status info from the Widget's servo board. The stuff at the end that interprets the most recently-recieved and most recently-processed servo commands is particularly useful to determine if the servo is working properly.
#### 7) Get Abort Status
Returns status about a command that failed to execute and was aborted for whatever reason (such as a read failure, an invalid parameter, and so on). Note that the info shown here is only valid if the most recent command failed to execute; if the command was successful, then this information is all meaningless. This command not only prints out the raw abort status, but also prints out the address of the routine that caused the abort, and uses this address to print a human-readable explanation of what actually caused the abort.
#### 8) Diagnostic Read
Reads data from a user-specified block on the disk into ESProFile's data buffer, but the block is specified in terms of a physical cylinder, head, and sector instead of a logical block number. You can also leave the CHS values blank to have it read whatever block the previous seek command positioned the heads over.
#### 9) Diagnostic Write
Same as Diagnostic Read, except this one writes the contents of ESProFile's data buffer to the disk instead.
#### A) Read Header
Reads the header field from the user-specified cylinder, head, and sector. You can also choose to read from whatever cylinder and head were specified by the previous seek, and only specify the sector. The header is essentially the behind-the-scenes information on the disk that the drive uses to determine what cylinder and sector it's over and what head it's using, and without this info, it will fail to seek properly and fail to access any data on the disk. The headers are only written during low-level formatting and can fade over the years, which is why a LLF that rewrites the headers can often revive a marginal drive. This command will read the header and give you a human-readable interpretation of it. It'll also try to check and see if all the header fields are valid, and alert you if the integrity of the header seems to be compromised.
#### B) Write Buffer to Spare Table
This command simply writes the contents of ESProFile's data buffer to the spare table. Which is really dangerous because if the contents of the data buffer aren't a properly-formatted spare table, then you'll mess things up and your drive won't start up properly anymore. You'll have to run Init Spare Table to fix things if you end up doing that. But if you write a properly-formatted spare table structure to your drive, then this could be useful for manually adding/removing entries in the spare table.
#### C) Seek
Seeks the drive's heads to the user-specified cylinder, head, and sector, but doesn't read or write the data there. This is useful in conjunction with a variety of other Widget commands since they can operate on whatever CHS the heads are currently positioned over, without you actually having to specify again.
#### D) Send Servo Command
This is actually a submenu, which we'll discuss below once we get through the rest of the commands on this main menu screen.
#### E) Send Restore
This command initializes the servo board and moves the heads to a known location, updating the Widget controller's internal state to account for the new head location. The data/format recal commands (discussed later) do the same thing, except they don't update the controller's state, so it doesn't realize that the heads have moved. You can perform either a data restore or a format restore; the data restore puts the heads over the innermost data track on the disk (I think it's like track 176 or something), while the format restore moves the heads even further inwards and off the data region entirely (track 220). If your servo starts acting weird, send a data restore to revive it; the format restore is really only used when you're about to do a low-level format, and it's handled automatically by ESProFile's LLF routine.
#### F) Set Recovery
This option allows you to toggle the Widget's recovery mode on and off; running the command will tell you which state the Widget is currently in and ask if you want to switch into the other state. Recovery is on by default when the Widget powers up, and it's basically the Widget's error-handling subsystem. When you turn it off, the Widget controller will always fail in an operation when it encounters an exception instead of trying to recover, leaving it up to ESProFile to process and respond to the exception. Certain commands, such as low-level formatting, require Recovery to be turned off, and ESProFile's low-level formatter routine will turn it off automatically.
#### G) Set Auto-Offset
This command will determine if the Widget's auto-offset functionality is enabled, then enable it if it wasn't, and finally try to auto-offset the heads over the current track. Auto-offset is the servo board's ability to automatically try and center the heads over a track after a seek has completed in an attempt to get the strongest signal off the disk that it can. When it's disabled, you have to manually send commands to the servo to modify the offset, but when it's on, the offset will be automatically determined after every seek.
#### H) View Track Offsets
Remember that offset thing we just talked about? Well, this command lets you view the auto-offsets that the servo board uses on your Widget. You can either specify a specific cylinder and head, or you can request the offsets for every track on the disk. These offsets can be either positive or negative, depending on whether the servo fine-tuned the head to be slightly to the left or slightly to the right of the original seek position, and the offsets being too high can be a sign of alignment or formatting issues in your drive. ESProFile will tell you if any of your offsets look concerningly high.
#### I) Low-Level Format
This will probably be the second-most used command on ESProFile, with only the 5MB ProFile low-level formatter topping it out! It does exactly what the name says: it low-level formats your Widget, which often fixes drives that produce a lot of read/write errors or even drives where you can't access the disk at all. The Widget provides some pretty verbose status info during the formatting process, so you'll get a lot of information about what's happening and if anything goes wrong. Low-level formatting a Widget is pretty darn complex and consists of over 10 steps, so I'm not going to go over them all here like I did for the ProFiles, but ESProFile will tell you about each step as it completes it. Unlike ProFiles, the Widget does not require you to install a jumper in order to format the disk. Given that the procedure is a little more complicated than with the ProFiles (and given how much error info the formatter gives you), I would not recommend trying to execute all the formatting steps manually, but of course you can try if you want!
#### J) Format Track(s)
Performs the Format step of a low-level format operation without performing any of the other steps. The user can also choose to only format the track that the heads are currently over, and the heads can be moved to the desired track before running this command by using the Seek command. Only use this if you're a pretty advanced user who knows how to LLF a Widget without the help of ESProFile's low-level formatter utility!
#### K) Init Spare Table
Performs the Init Spare Table step of a low-level format operation, without performing the other steps of the format. You can also just run it on its own to recreate your spare table if you ever want to do that for some reason, without doing a low-level format. If you mess up your spare table with B), then this is the command to run to fix things!
#### L) Scan
Performs the Scan step of a low-level format operation, without performing any of the other steps of the format. You can also use it outside the context of a low-level format to scan your drive for defects. This is identical to the surface scan that your Widget takes a minute or two to complete when you first power it on.
#### M) Park Heads (Do This Before Exiting!)
Moves the heads to their parked position, off the data-holding portion of the hard disk platters. The Widget normally does this automatically, but if you've been running particular diagnostic commands, then it'll stop, so it's highly recommended to run this before powering off your drive if you've executed any commands in this menu, just to make sure that you don't damage your disk surface!
#### N) Send Custom Widget Command
This is only for advanced users. Similar to F) on the main menu, except it allows you to send a Widget-specific command since many of the Widget commands use a completely different format than the standard ProFile commands. Just enter your command, answer a few questions about it, and you're good to go!
#### O) Modify Data Buffer
Exactly the same as Modify Data Buffer on the main menu, just placed here as well for convenience.
#### P) Fill Buffer With Pattern
Exactly the same as Fill Buffer With Pattern on the main menu, just placed here as well for convenience.
#### Q) Show Command, Status, and Data Buffers
Exactly the same as Show Command, Status, and Data Buffers on the main menu, just placed here as well for convenience.
#### R) Return to Main Menu
As I've said multiple times before, I would be pretty concerned if you couldn't figure this one out.   
<br />
<br />
The one submenu on the Widget-Specific Commands menu is:
### D) Send Servo Command Submenu
As the name implies, this menu is dedicated to sending commands that directly communicate with the Widget servo board.
Note that, given that all of these servo commands are sent directly to the servo board, they are completely transparent to the controller. This means that if we ask the servo to move the heads to a new track, the controller will still think they're at the old location. In order to bring the controller up to speed, we have to use a command like Seek or Send Restore that is actually processed by the controller.

The servo menu looks like this:

<img width="541" alt="Servo_Menu" src="https://github.com/user-attachments/assets/9a14133b-6133-4f04-84df-79782ae8de66" />

And here are the commands!
#### 1) Soft Reset
This is identical to Soft Reset on the Widget-Specific Commands menu; it's just included here for convenience since messing with the servo can break things and require a reset.
#### 2) Reset Servo
Identical to Reset Servo on the Widget-Specific Commands menu.
#### 3) Get Controller Status
Once again, it's identical to the equivalent option on the Widget-Specific Commands menu.
#### 4) Get Servo Status
Identical yet again.
#### 5) Get Abort Status
Yet another command that's identical.
#### 6) Read Header
And a final one for good measure. Since the servo deals with positioning the heads over tracks, having the option to read the header of the current CHS in this menu is really convenient.
#### 7) Recal
Just like the Send Restore option in the Widget-Specific Commands menu, except this one doesn't update the Widget controller's state to let it know the new position of the heads. For all it knows, the heads are still in the old location. As with Send Restore, this command has both Format and Data variants (see Send Restore for details).
#### 8) Access
Replace "Access" with "Seek", and you'll get a good idea of what this one does. It just moves the heads to a user-specified track. But, interestingly enough, the servo has no idea what track the heads are over at any given time; that state info is kept by the controller, but we're bypassing the controller here. So we specify our access value in terms of a relative distance from the current track. So saying "5" will move us 5 tracks forward from where we are right now, and saying "-3" will move us 3 tracks backward. The only way to do absolute positioning (without bringing the controller into things) is to send a data or format recal to bring us back to a known track, and then do mental math from there with the distances. Note that this command does not try to perform an auto-offset (discussed in the Set Auto-Offset option of the previous menu); it just puts the heads where it thinks the center of the track is, and calls it a day.
#### 9) Access With Offset
This is identical to the above, except once it seeks to the desired track, the servo performs an offset operation over the track. You can elect to either manually specify the offset or have it do an auto-offset to try and fine-tune the positioning of the heads and get them precisely positioned over the center of the track to maximize signal strength.
#### A) Offset
This command performs only an offset operation, without doing a seek first. This offset can be positive or negative (positive moves the heads off-center in one direction; negative in the other). I'm not actually sure what the bounds for the offset are, but I'd try to keep it small-ish; no more than +/- 20 hex sounds like a good limit to me.
#### B) Home
This command tells the servo to move the heads off the data surface of the disk and hold them really close to the spindle. It's basically the "Park Heads" command, but without letting the controller in on the fun since we're talking to the servo directly. Counterintuitively, this is NOT the command that you want to issue in order to get the heads into a known position; you want the Recal command for that. And note that the drive will start acting nonresponsive after sending this command, so you have to send it a Soft Reset afterwards in order to wake the drive back up again.
#### C) Show Command, Status, and Data Buffers
Exactly the same as Show Command, Status, and Data Buffers on the main menu and the Widget-Specific Commands menu, just placed here as well for convenience.
#### D) Return to Widget Menu
This is the very last menu option. I've been typing for about 9 hours straight now, so I'm really glad we're done!

# Desirable Features
- Get the free space indicator working in the Selector. It's currently disabled and returns "????????" because it just takes way too long to count up the free space on larger SD cards (on the order of several seconds), which causes the Selector to freeze for that period of time whenever it requests the free space.
- Add support for BLU 5:1 interleave disk images without requiring conversion.
- Allow backing up and restoring images directly to the SD card instead of over XMODEM when in diagnostic mode. This way, backups and restores will be quicker and you can also immediately boot from your backup by switching to emulator mode!
- Make a parallel card version of ESProFile. This design would take the 2-port parallel card and replace one (or both) of the ports with one (or two) ESProFile(s) built directly into the card. This would make for a really easy plug-in hard drive replacement for a lot of Lisa users, without any messy cables or mounting headaches. Due to being card-based, this version of ESProFile would not support diagnostic mode; the mode pin on the ESP32 that normally connects to the switch would be hard-wired for emulator mode only.

Let me know if you can think of anything else!

# Contact Me!

Feel free to email me at [alexelectronicsguy@gmail.com](mailto:alexelectronicsguy@gmail.com) if you need help, find any bugs, or have any questions/comments!

# Changelog
1/20/2025 - Initial 1.0 Release

1/21/2025 - Added an FAQ section to the readme, and used it to answer a question about whether or not the ESP32 is 5V-tolerant.

1/24/2025 - Replaced an out-of-stock resistor in the BOM, fixed an issue with the external PCB pick-and-place file that may have led to JLCPCB sending you a confirmation email, and rewrote the efuse section of the readme to make it more accurate and easier to follow.

2/20/2025 - Emulator firmware release 1.1; fixed a bug where ESProFile would sometimes fail to respond when connected to a parallel card while running LOS 3. Also optimized the serial debug output for a very slight performance boost.

2/21/2025 - Internal PCB version 1.1; added a fan header so that users have the option to power the Widget drive cage fan.

2/22/2025 - Emulator firmware release 1.2; improved general emulator performance by about 30% by making some tweaks to the SD card code, among other things. Selector copy operations are about 45% faster now, but still pretty painful for large disks!

2/23/2025 - Fixed a typo in the readme where the Mac version of the espefuse command should've said "espefuse.py" instead of "espefuse".

2/29/2025 - Added STL files for a 3D-printed drive bay mount by JustDaveIII and an external ESProFile case by wottle. Also added a note about potential problems with really cheap/slow SD cards and an FAQ question about Apple ][ and Apple /// compatibility.

3/2/2025 - External PCB version 1.1; added screwholes at the corners of the PCB to facilitate mounting in future case designs.

3/19/2025 - Updated external case with screwholes to accomodate the v1.1 PCB, and added a new 3D-printed internal ESProFile mount by wottle on TinkerDifferent.

4/9/2025 - Added STL files for a ProFile-shaped ESProFile case by Michael Schaffer. Also updated the note about Apple ][ compatibility to reflect a recent report that I got from someone.

6/4/2025 - Fixed a typo in the readme where I accidentally said that you should use checksum error checking when performing XMODEM disk backups with ESProFile, when you should actually use CRC instead.

7/20/2025 - Added an FAQ entry that links to some ready-to-go ESProFile images, in case you don't want to install an OS yourself.

10/5/2025 - Added a guide to the SDTemplate directory written by @bmwcyclist on LisaList2 explaining how to create blank ESProFile disk images.
