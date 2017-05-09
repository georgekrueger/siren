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

	/*double beat_unit = 1 / time_sig_den_;
	for (int beat = 0; beat < time_sig_num_; ++beat) {
		double beat_pos = beat * beat_unit;
		auto beat_event = make_unique<ControlEvent>(beat_pos, "beat");
		loaded_pattern_->events_.insert(make_pair(beat_pos, move(beat_event)));
	}*/

	// insert bar events
	double bar_unit = time_sig_num_ / time_sig_den_;
	int num_bars = static_cast<int>(loaded_pattern_->length_ / bar_unit);
	for (int bar = 0; bar < num_bars; ++bar) {
		double bar_pos = bar * bar_unit;
		auto bar_event = make_unique<ControlEvent>(bar_pos, "bar");
		loaded_pattern_->events_.insert(make_pair(bar_pos, move(bar_event)));
	}

	// insert end of pattern event
	auto pattern_end_event = make_unique<ControlEvent>(loaded_pattern_->length_, "pattern_end");
	loaded_pattern_->events_.insert(make_pair(loaded_pattern_->length_, move(pattern_end_event)));

	if (!isTimerRunning() && loaded_pattern_->start_ == Quantize::BAR) {
		// set timer to start at next bar
		int64 time_to_next_bar = getTimeToNextBar();
		startTimer(time_to_next_bar / 1000);
		timer_start_point_ = chrono::steady_clock::now();
	}
	else {
		update();
	}
}

double Track::us_to_beats(int64 us)
{
	double beats_per_us = bpm_ / 60 / 1000000;
	return (us * beats_per_us);
}

int64 Track::beats_to_us(double beats)
{
	double bps = bpm_ / 60;
	return static_cast<int64>(beats * (1000000 / bps));
}

int64 Track::getTimeToNextBar()
{
	chrono::time_point<chrono::steady_clock> begin;
	chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
	int64 us_since_begin = chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
	double bar_length = time_sig_num_ / time_sig_den_;
	int64 bar_length_us = beats_to_us(bar_length);
	return ((us_since_begin / bar_length_us) + 1) * bar_length_us;
}

void Track::stop(std::string json)
{

}

void Track::setBpm(double bpm)
{
	bpm_ = bpm;
	update();
}

void Track::setTimeSignature(int num, int den)
{
	time_sig_num_ = num;
	time_sig_den_ = den;
	update();
}

void Track::hiResTimerCallback()
{
	
}

void trigger_event(Event* event)
{
}

void Track::update()
{
	if (isTimerRunning()) {
		// update cursor position
		chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
		int64 us_elapsed = chrono::duration_cast<chrono::microseconds>(now - timer_start_point_).count();
		cursor_pos_ += beats_to_us(us_elapsed);
	}
	stopTimer();

	auto it = active_pattern_->events_.lower_bound(cursor_pos_);
	while (it != active_pattern_->events_.end() && it->second->pos_ <= cursor_pos_ + 0.001) {
		trigger_event(it->second.get());
		++it;
	}
	int64 time_to_next_event = 0;

	startTimer(time_to_next_event / 1000);
	timer_start_point_ = chrono::steady_clock::now();
}