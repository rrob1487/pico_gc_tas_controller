# MKWii TAS Playback via Raspberry Pi Pico

This software allows for the playback of Mario Kart Wii time trial ghost data on console using a Raspberry Pi Pico and an exposed Gamecube cable dataline connected to a Wii. Software is kept simple thanks to framecount determinism introduced by starting a time trial race with the controller unplugged. Simply plugging in the microcontroller synchronizes the frame timings for the race. This project is forked from [pico-rectangle](https://github.com/JulienBernard3383279/pico-rectangle) as data encoding and transmission timings are already handled. Note that not all ghost data will play back currently (see the [Input Range](#inputrange) section).

Please consult the [Legal information and license](https://github.com/JulienBernard3383279/pico-rectangle#legalInformationAndLicense) section in the upstream repository before using this firmware.

## Hardware Required

In order to achieve playback sync, we need the following:
- [Raspberry Pi Pico](https://a.co/d/3ektdH2)
- [2x 20-pin headers](https://a.co/d/4R5S7OQ)
- [Breadboard kit](https://a.co/d/5nFPmrP)
- [Soldering iron + solder](https://a.co/d/6OQDzBK)
- [Gamecube cable](https://a.co/d/5JQs53b)*
- [28 gauge wire stripper](https://a.co/d/e9WKf9y)
- [2.54mm Dupont connector kit](https://a.co/d/8zIctFd)
- [SN-28B Crimper](https://a.co/d/5cAGSTc)

*If you have an old Gamecube controller you could use that wire, but I didn't want to sacrifice a Gamecube controller for this project.

## Steps to Assemble

1. Solder the Pico to header pins. Unlike other Raspberry Pis, the Pico does not come with headers pre-soldered. Connect Pico headers to breadboard kit.
2. Cut off the end of the Gamecube cable that doesn't plug into the console. Strip the housing off with something like an exacto knife. Wire strippers didn't work for me as they damaged the insulation of the inside wires.
3. Locate the data wire. On OEM cables this is the red one, but if using a third-party controller, you should determine the cable by opening the housing.
4. Crimp the data wire to a female Dupont connector. Plug the data wire into the GP28 pin on the breadboard kit.
5. (Optional) Locate the 3.3V wire. Crimp that as well and plug into VSYS. Alternatively, you can skip this step if you plan to power the Pico via USB. NOTE: If you do this step, don't have this board plugged via USB and via its Gamecube port at the same time. This would feed the USB 5v to the 3v line of the console and likely damage it.

## How to Run

1. Clone repo.
2. In src/file.S, replace file path with a RKG file of your choosing. Be sure to verify that its input set does not contain any tuples outside the valid in-game controller [input range](#inputrange).
3. Build using CMake.
4. With the Pico USB disconnected, hold down the BOOTSEL button on-board the Pico, and plug the Pico into your computer.
5. Copy the .uf2 over to the Pico which should appear as a new drive.
6. Disconnect the Pico from your computer.
7. Using a real Gamecube controller, choose the character, vehicle, transmission, and course that matches the RKG file provided, and start a time trial. Immediately disconnect the controller.
8. Wait until you see the "Controller disconnected" pop-up screen. Wait a few seconds.
9. Plug the Pico's Gamecube wire into the console. If you skipped step 5, plug the Pico back into your computer for power.
10. The Pico will now close the disconnect screen and play the inputs from the provided RKG.

<a name="inputrange"/>

## Regarding Input Ranges

The Wii Wheel, WiiMote+Nunchuk, and Gamecube and Classic Controllers have different ranges of possible stick inputs due to code in Mario Kart Wii which clamps inputs to a unit circle (or doesn't, in the case of Wii Wheel). Modern Tool-Assisted Speedruns take use of a Gecko code, which removes the clamp when emulating the game. Since RKG files store inputs the same regardless of the controller used, the in-game replay system will still sync without the use of the unclamp Gecko code. Since I am trying to play back a TAS live rather than through the replay system, this means that the clamp restricts what inputs will work on the Gamecube controller.

Horizontal and vertical stick inputs are each represented in-game as a value between 0 and 14, where 0 represents full left/down, 7 represents neutral, and 14 represents full right/up. The list of illegal inputs, in the format `{Horizontal, Vertical}`, for a Gamecube controller are as follows:

|||||||
|---|---|---|---|---|---|
|{0,0}|{1,0}|{2,0}|{12,0}|{13,0}|{14,0}|
|{0,1}|{1,1}|||{13,1}|{14,1}|
|{0,2}|||||{14,2}|
|{0,12}|||||{14,12}|
|{0,13}|{1,13}|||{13,13}|{14,13}|
|{0,14}|{1,14}|{2,14}|{12,14}|{13,14}|{14,14}|
