#include "Sequencer.h"

using namespace std;

Sequencer::Sequencer() : bpm_(120)
{
	chrono::time_point<chrono::steady_clock> tp = chrono::steady_clock::now();
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

void Track::play(std::string json)
{
	// parse json event array
	// array is an ordered array of arrays (representing events)
	// example: 
	// {
    // 'length': 4,
    // 'quantize': 'bar',
    // 'events' : [["note", 0, 60, 1.0, 1], ["note", 1, 62, 0.9, 1], ["preset", 1, 1], ["param", 1, "param name", 1.0]]
    // }
	// 

	loaded_pattern_ = make_unique<Pattern>();

	var parse_results;
	JSON::parse(json, parse_results);
	DynamicObject* obj = parse_results.getDynamicObject();
	if (obj->hasProperty("length")) {
		loaded_pattern_->length_ = static_cast<double>(obj->getProperty("length"));
	}
	if (obj->hasProperty("quantize")) {
		String start_quantize = obj->getProperty("quantize").toString();
		if (start_quantize == "bar") {
			loaded_pattern_->start_ = Quantize::BAR;
		}
		if (start_quantize == "pattern") {
			loaded_pattern_->start_ = Quantize::PATTERN;
		}
	}
	Array<var>* event_array = parse_results.getArray();
	for (int i = 0; i < event_array->size(); ++i) {
		Array<var>* event_items = event_array->getReference(i).getArray();
		String type = event_items->getReference(0);
		double pos = static_cast<double>(event_items->getReference(1));
		if (type == "note") {
			int note = static_cast<int>(event_items->getReference(2));
			float velocity = static_cast<float>(event_items->getReference(3));
			float length = static_cast <float> (event_items->getReference(4));
			loaded_pattern_->events_.insert(make_pair(pos, make_unique<NoteOnEvent>(pos, note, velocity)));
			loaded_pattern_->events_.insert(make_pair(pos, make_unique<NoteOffEvent>(pos+length, note)));
		}
		else if (type == "preset") {
		}
		else if (type == "param") {
		}
	}

	resetTimer();
}

void Track::resetTimer()
{

}

void Track::stop(std::string json)
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