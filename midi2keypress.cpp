#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstdint>
#include <bitset>
#include <vector>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>

jack_port_t *midi_input_port;

// Struktur für MIDI-Nachrichten
struct MidiMessage {
    jack_midi_event_t rawEvent;

    // Konstruktor
    MidiMessage(const jack_midi_event_t& event) {
        rawEvent = event;
    }
};

// Global map to associate MIDI events with a vector of strings
std::map<std::vector<jack_midi_data_t>, std::vector<std::string>> midiEventMap;

// Function to load MIDI mappings from a JSON file
void loadMidiMappings(const std::string& jsonFile) {
    std::ifstream conf(jsonFile);
    if (!conf.is_open()) {
        std::cerr << "Error opening JSON file: " << jsonFile << std::endl;
        return;
    }

    nlohmann::json data;
    conf >> data;

    auto messageMapJson = data.at("message_map");
    for (const auto& entry : messageMapJson) {
        const auto& midiEvent = entry[0].get<std::vector<int>>();
        const auto& names = entry[1].get<std::vector<std::string>>();

        std::vector<jack_midi_data_t> midiEventBytes;
        for (int byte : midiEvent) {
            midiEventBytes.push_back(static_cast<jack_midi_data_t>(byte));
        }

        midiEventMap[midiEventBytes] = names;
    }
}

int process(jack_nframes_t nframes, void *arg) {
    jack_midi_event_t event;
    void *midi_input_buffer = jack_port_get_buffer(midi_input_port, nframes);
    jack_nframes_t event_count = jack_midi_get_event_count(midi_input_buffer);

    for (jack_nframes_t i = 0; i < event_count; ++i) {
        jack_midi_event_get(&event, midi_input_buffer, i);

        // Use the entire raw MIDI message buffer as the key for the map
        std::vector<jack_midi_data_t> key(event.buffer, event.buffer + event.size);

        // Look up MIDI event in the map
        auto mapIter = midiEventMap.find(key);
        if (mapIter != midiEventMap.end()) {
            // Process associated strings
            const std::vector<std::string>& names = mapIter->second;
            for (const auto& name : names) {
                std::cout << "Mapped Event: " << name << std::endl;
            }
        }
    }

    return 0;
}

int main() {
    // Load MIDI mappings from JSON file
    loadMidiMappings("config.json");
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
