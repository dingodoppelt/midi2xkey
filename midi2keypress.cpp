#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <bitset>
#include <cstdint>

jack_port_t *midi_input_port;

// Struktur für MIDI-Nachrichten
struct MidiMessage {
    uint8_t status;  // MIDI-Statusbyte
    uint8_t channel; // MIDI-Kanal (1-16)
    uint8_t data1;    // Erstes Datenbyte
    uint8_t data2;    // Zweites Datenbyte (nur für bestimmte Nachrichten)

    // Konstruktor für einfache Initialisierung
    MidiMessage(uint8_t s, uint8_t c, uint8_t d1, uint8_t d2 = 0)
        : status(s), channel(c), data1(d1), data2(d2) {}

    // Funktion zum Ausgeben der MIDI-Nachricht
    void print() const {
        std::cout << "Status: " << std::hex << static_cast<int>(status)
                  << ", Channel: " << static_cast<int>(channel)
                  << ", Data1: " << static_cast<int>(data1)
                  << ", Data2: " << static_cast<int>(data2) << std::dec << std::endl;
    }
};

// Funktion zum Parsen von MIDI-Ereignissen
MidiMessage parseMidiEvent(const jack_midi_event_t& event) {
    MidiMessage midiMessage(0, 0, 0, 0);

    // Extrahiere Statusbyte und Kanal
    midiMessage.status = event.buffer[0];
    midiMessage.channel = (midiMessage.status & 0x0F) + 1;

    // Extrahiere Datenbytes (abhängig vom MIDI-Statusbyte)
    switch ((midiMessage.status & 0xF0) >> 4) {
        case 8: // Note Off
        case 9: // Note On
        case 0xA: // Polyphonic Aftertouch
        case 0xB: // Control Change
        case 0xE: // Pitch Bend
            midiMessage.data1 = event.buffer[1];
            midiMessage.data2 = event.buffer[2];
            break;
        case 0xC: // Program Change
        case 0xD: // Channel Aftertouch
            midiMessage.data1 = event.buffer[1];
            break;
        // Weitere MIDI-Statusbyte-Fälle können hier hinzugefügt werden
    }

    return midiMessage;
}

int process(jack_nframes_t nframes, void *arg) {
    jack_midi_event_t event;
    void *midi_input_buffer = jack_port_get_buffer(midi_input_port, nframes);
    jack_nframes_t event_count = jack_midi_get_event_count(midi_input_buffer);

    for (jack_nframes_t i = 0; i < event_count; ++i) {
        jack_midi_event_get(&event, midi_input_buffer, i);

        // Parsen und Ausgeben der MIDI-Nachricht
        MidiMessage midiMessage = parseMidiEvent(event);
        midiMessage.print();
    }

    return 0;
}

int main() {
    jack_client_t *client;
    const char *client_name = "mein-midi-client";

    client = jack_client_open(client_name, JackNullOption, NULL);
    if (client == NULL) {
        std::cerr << "Fehler beim Öffnen des JACK-Clients" << std::endl;
        return 1;
    }

    midi_input_port = jack_port_register(client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0);

    jack_set_process_callback(client, process, 0);

    if (jack_activate(client)) {
        std::cerr << "Fehler beim Aktivieren des JACK-Clients" << std::endl;
        return 1;
    }

    std::cout << "MIDI-Client gestartet. Drücke Enter zum Beenden." << std::endl;
    char input;
    std::cin.get(input);

    jack_client_close(client);

    return 0;
}
