#include "Sequencer.h"

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
	// example: [ ["note", 0, 60, 1.0, 1], ["note", 1, 62, 0.9, 1], [1, "preset", 1], [1, "param", "param name", 1.0] ]

	var parse_results;
	JSON::parse(pattern_json, parse_results);
	Array<var> * event_array = parse_results.getArray();

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