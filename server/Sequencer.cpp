#include "Sequencer.h"

using namespace std;

Sequencer::Sequencer() : bpm_(120)
{
}

Sequencer::~Sequencer()
{
}

Track* Sequencer::getTrack(unsigned int track)
{
	auto iter = tracks_.find(track);
	if (iter == tracks_.end()) {
		auto insert_ret = tracks_.insert(std::make_pair(track, std::make_unique<Track>(bpm_)));
		if (!insert_ret.second) {
			std::cerr << "Failed to insert new track " << track << std::endl;
			return nullptr;
		}
		iter = insert_ret.first;
	}
	return iter->second.get();
}

void Sequencer::setBpm(double bpm)
{
	for (auto& kv : tracks_) {
		kv.second->setBpm(bpm);
	}
}

void Track::play(std::string pattern_json, Quantize start_point)
{
	// parse json event array
	// array is an ordered array of arrays (representing events)
	// example: [ ["note", 0, 60, 1.0, 1], ["note", 1, 62, 0.9, 1], ["preset", 1, 1], ["param", 1, "param name", 1.0] ]

	var parse_results;
	JSON::parse(pattern_json, parse_results);
	Array<var>* event_array = parse_results.getArray();
	loaded_events_.clear();
	loaded_start_point_ = start_point;
	for (int i = 0; i < event_array->size(); ++i) {
		Array<var>* event_items = event_array->getReference(i).getArray();
		String type = event_items->getReference(0);
		float pos = static_cast<double>(event_items->getReference(1));
		if (type == "note") {
			int note = static_cast<int>(event_items->getReference(2));
			float velocity = static_cast<float>(event_items->getReference(3));
			float length = static_cast <float> (event_items->getReference(4));
			loaded_events_.push_back(make_unique<NoteOnEvent>(pos, note, velocity));
			loaded_events_.push_back(make_unique<NoteOffEvent>(pos+length, note));
		}
		else if (type == "preset") {

		}
		else if (type == "param") {

		}
	}
}

void Track::stop(Quantize stop_point)
{

}

void Track::setBpm(double bpm)
{
	bpm_ = bpm;
	// TODO: reset timer
}

void Track::hiResTimerCallback()
{

}