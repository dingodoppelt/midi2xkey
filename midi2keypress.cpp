#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstdint>
#include <bitset>
#include <vector>

jack_port_t *midi_input_port;

// Struktur für MIDI-Nachrichten
struct MidiMessage {
    jack_midi_event_t rawEvent;
    uint8_t status;  // MIDI-Statusbyte
    uint8_t channel; // MIDI-Kanal (1-16)
    uint8_t data1;    // Erstes Datenbyte
    uint8_t data2;    // Zweites Datenbyte (nur für bestimmte Nachrichten)
    std::string name = "unknown";

    // Konstruktor
    MidiMessage(const jack_midi_event_t& event) {
        rawEvent = event;
        status = (event.buffer[0] & 0xF0) >> 4;
        channel = (event.buffer[0] & 0x0F) + 1;
        data1 = event.buffer[1];
        data2 = event.buffer[2];
        switch (status) {
            case 0x8: // Note Off
                name = "Note Off";
                break;
            case 0x9: // Note On
                name = "Note On";
                break;
            case 0xA: // Polyphonic Aftertouch
                name = "Polyphonic Aftertouch";
                break;
            case 0xB: // Control Change
                name = "Control Change";
                break;
            case 0xE: // Pitch Bend
                name = "Pitch Bend";
                break;
            case 0xC: // Program Change
                name = "Program Change";
                break;
            case 0xD: // Channel Aftertouch
                name = "Channel Aftertouch";
                break;
            // Weitere MIDI-Statusbyte-Fälle können hier hinzugefügt werden
        }
    }

    // Funktion zum Ausgeben der MIDI-Nachricht
    void print() const {
        printBinaryData();
        std::cout << "Status: " << static_cast<int>(status)
                  << ", Channel: " << static_cast<int>(channel)
                  << ", Data1: " << static_cast<int>(data1)
                  << ", Data2: " << static_cast<int>(data2)
                  << ", Name: " << name << std::endl;
    }

private:
    void printBinaryData() const {
        std::cout << "Binärdaten: ";
        for (uint8_t i = 0; i < rawEvent.size; i++) {
            for (int j = 7; j >= 0; --j) {
                std::cout << ((rawEvent.buffer[i] >> j) & 1);
            }
            std::cout << " ";
        }
        std::cout << std::endl;
    }
};

int process(jack_nframes_t nframes, void *arg) {
    jack_midi_event_t event;
    void *midi_input_buffer = jack_port_get_buffer(midi_input_port, nframes);
    jack_nframes_t event_count = jack_midi_get_event_count(midi_input_buffer);

    for (jack_nframes_t i = 0; i < event_count; ++i) {
        jack_midi_event_get(&event, midi_input_buffer, i);

        // Parsen und Ausgeben der MIDI-Nachricht
        MidiMessage midiMessage(event);
        midiMessage.print();
    }

    return 0;
}

int main() {
    jack_client_t *client;
    const char *client_name = "midi2key";

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
