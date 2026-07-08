ROBOT_TX16S EdgeTX profile package
===================================

Contents:
- MODELS/model32.yml: EdgeTX model file. Rename to an unused model number if model32.yml already exists.
- MODELS/ROBOT_TX16S.txt: model notes/checklist viewable from the radio.
- ROBOT_TX16S.yml: same model file with a human-friendly name for EdgeTX Companion import/open.
- ROBOT_TX16S_notes.txt: copy of the channel map.

Recommended import with EdgeTX Companion:
1. Open EdgeTX Companion that matches your radio firmware as closely as possible.
2. Read models/settings from the radio or open a backup .etx file.
3. Open ROBOT_TX16S.yml in Companion.
4. If Companion opens it in a separate model window, drag/copy the ROBOT_TX16S model into your radio model list.
5. Write models/settings back to the radio.
6. On the radio, open the model and verify the Channel Monitor before powering the robot.

Direct SD-card import:
1. Back up the SD card first.
2. Copy MODELS/model32.yml to the radio SD card /MODELS folder.
3. If model32.yml already exists, rename this file to an unused model number such as model33.yml.
4. Copy MODELS/ROBOT_TX16S.txt to /MODELS as well.
5. Reboot the radio and select ROBOT_TX16S.

After import, still check these on the radio:
- Model name: ROBOT_TX16S
- Internal RF: CRSF if using built-in ELRS
- External RF: Off
- Channel range: CH1-CH16
- ELRS Lua: 100Hz Full or 333Hz Full, Switch Mode 16ch Rate/2 Full Res

Caveats:
- This file is generated from EdgeTX YAML conventions and examples. I cannot run EdgeTX Companion in this environment, so verify every channel on the radio Channel Monitor.
- If Companion says S1/S2 are invalid sources on your firmware, replace CH5 source S1 with POT1 and CH6 source S2 with POT2.
- If SH2 does not trigger CH14 on your radio, edit CH14's fire mix switch to the SH down/momentary position shown by Companion.
