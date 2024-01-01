#include <iostream>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <cstdint>
#include <vector>
#include <map>
#include <fstream>
#include <nlohmann/json.hpp>
#include <xdo.h>

jack_port_t *midi_input_port;

class Xdo
{
public:
	Xdo()
	{
		context = xdo_new(NULL);
	}
	~Xdo()
	{
		xdo_free(context);
	}
	void send_key_down(std::string const & sequence, useconds_t delay = 12000)
	{
		xdo_send_keysequence_window_down(context, CURRENTWINDOW, sequence.c_str(), delay);
	}
	void send_key_up(std::string const & sequence, useconds_t delay = 12000)
	{
		xdo_send_keysequence_window_up(context, CURRENTWINDOW, sequence.c_str(), delay);
	}
	void send_key(std::string const & sequence, useconds_t delay = 12000)
	{
		xdo_send_keysequence_window(context, CURRENTWINDOW, sequence.c_str(), delay);
	}
private:
	xdo_t* context;
} xdomain;

void printBinaryData(jack_midi_event_t rawEvent) {
    std::cout << "raw MIDI: ";
    for (uint8_t i = 0; i < rawEvent.size; i++) {
        for (int j = 7; j >= 0; --j) {
            std::cout << ((rawEvent.buffer[i] >> j) & 1);
        }
        std::cout << " ";
    }
    std::cout << std::endl;
}

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
        const auto& keys = entry[1].get<std::vector<std::string>>();

        std::vector<jack_midi_data_t> midiEventBytes;
        for (int byte : midiEvent) {
            midiEventBytes.push_back(static_cast<jack_midi_data_t>(byte));
        }

        midiEventMap[midiEventBytes] = keys;
    }
}

int process(jack_nframes_t nframes, void *arg) {
    jack_midi_event_t event;
    void *midi_input_buffer = jack_port_get_buffer(midi_input_port, nframes);
    jack_nframes_t event_count = jack_midi_get_event_count(midi_input_buffer);

    for (jack_nframes_t i = 0; i < event_count; ++i) {
        jack_midi_event_get(&event, midi_input_buffer, i);

        printBinaryData(event);

        // Use the first two bytes of the raw MIDI message buffer as the key for the map
        std::vector<jack_midi_data_t> key(event.buffer, event.buffer + 2);

        // Look up MIDI event in the map
        auto mapIter = midiEventMap.find(key);
        if (mapIter != midiEventMap.end()) {
            // Process associated strings
            const std::vector<std::string>& names = mapIter->second;
            for (const auto& name : names) {
                switch (key[0] >> 4) {
                    case 0x8: // noteoff = keyup
                        xdomain.send_key_up(name);
                        break;
                    case 0x9: // noteon = keydown for velocity > 0, else keyup
                        event.buffer[2] > 0 ? xdomain.send_key_down(name) : xdomain.send_key_up(name);
                        break;
                    case 0xB: // control change = keydown for vel 127, keyup for vel 0, ignore the rest
                        switch (event.buffer[2]) {
                            case 0:
                                xdomain.send_key_up(name);
                                break;
                            case 127:
                                xdomain.send_key_down(name);
                                break;
                        }
                        break;
                    case 0xC: // program change = keypress
                        xdomain.send_key(name);
                        break;
                    default:
                        std::cout << "only note on, note off, control change and program change are handled... ignoring key" << std::endl;
                        break;
                }
                std::cout << "Mapped Key: " << name << std::endl;
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
