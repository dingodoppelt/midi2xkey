# MIDI2xKey
A JACK (pipewire) MIDI client converting MIDI messages to keystrokes using libxdo.

## Build instructions
### Dependencies
- JACK (or pipewire)
- [libxdo](https://github.com/jordansissel/xdotool)
- [nlohmann/json](https://github.com/nlohmann/json)

### Building
```
git clone https://github.com/dingodoppelt/midi2xkey.git
cd midi2xkey/
make
```

## Configuring
A json file is provided for mapping MIDI events to keystrokes or key sequences.
The MIDI events consist of either 3 or 2 bytes of data depending on the type of the event.
This tool handles the following MIDI messages:
- **note on** (sends a keydown event so you can hold the key for velocity > 0)
- **note off** (sends a keyup event so you can release a key)
- **control change** (sends keyup/keydown events depending on the value stored in data byte 3, 127 = keydown, 0 = keyup)
- **program change** (sends a single keypress on every program change event)

Say you wanted to map a **Note On/Note Off event** for **note number 60** on **MIDI channel 2** to the **keys "V" and "K"** you'd add this to the `message_map`:
```
[[145, 60], ["V", "K"]],
[[129, 60], ["V", "K"]
```
**Don't forget to add the second line to release the keys on note off!**

explanation:
`145` = Decimal value representing a MIDI note on event (1001) on channel 2 (0001). (Binary: 1001 0001)
`60`  = Decimal value of the MIDI note number you want to map to a key
`["V", "K"]` = the key(s), key combination(s) or key sequence(s) to be sent.
See a [list of keycodes](https://gitlab.com/nokun/gestures/-/wikis/xdotool-list-of-key-codes)
See [summary of MIDI messages](https://www.midi.org/specifications-old/item/table-1-summary-of-midi-message) for detailed information on how to construct your MIDI message.


If you want to send a keystroke on a program change event with value 0 on channel 2:
```
[[193, 0], ["L"]]
```
You don't have to release the key because program change triggers a single keytroke (down and up)

## Usage
1. Start a JACK server (not needed if using pipewire)
2. Run `midi2xkey`
3. Connect any MIDI controller to the client using your favourite JACK tool (carla, catia, qjackctl, jack_connect, jack-matchmaker, helvum, qpwgraph, ...)
4. Change to the window you want the events to be sent to and start working your MIDI controllers
