#include "Sequencer.h"

using namespace std;

double Pattern::update(double elapsed, vector<unique_ptr<Event>>& out_events)
{
	cursor_ += elapsed;
	// just in case. shouldn't be necessary since there is an end-of-pattern event
	if (cursor_ > length_) {
		cursor_ = 0;
	}

	if (events_.empty() || length_ == 0) {
		return -1;
	}

	double thresh = 0.001;
	bool done = false;
	while (!done) {
		for (auto& kv : events_) {
			double event_pos = kv.first;
			auto& event = kv.second;
			if (event_pos < cursor_ - thresh) {
				continue;
			}
			if (event_pos <= cursor_ + thresh) {
				if (event->type_ == Event::Type::CONTROL && 
					static_cast<ControlEvent*>(event.get())->action_ == "pattern_end")
				{
					cursor_ = 0;
					break;
				}
				out_events.push_back(unique_ptr<Event>(new Event(*event)));
			}
			else {
				done = true;
				break;
			}
		}
	}

	auto it = events_.lower_bound(cursor_);
	if (it == events_.end()) {
		return -1.0;
	}
	return it->first - cursor_;
}

Sequencer::Sequencer() : bpm_(120), time_sig_num_(4), time_sig_den_(4), next_timer_is_bar(false)
{
}

Sequencer::~Sequencer()
{
}

void Sequencer::play(unsigned int track, std::string json)
{
	// parse json event array
	// array is an ordered array of arrays (representing events)
	// example: 
	// {
    // 'length': 4,
    // 'quantize': 'bar',
    // 'events' : [["note", 0, 60, 1.0, 1], ["note", 1, 62, 0.9, 1], ["preset", 1, 1], ["param", 1, "param name", 1.0]]
    // }

	auto new_pattern = make_unique<Pattern>();
	auto pattern_start = Quantize::BAR;
	var parse_results;
	JSON::parse(json, parse_results);
	DynamicObject* obj = parse_results.getDynamicObject();
	if (obj->hasProperty("length")) {
		new_pattern->length_ = static_cast<double>(obj->getProperty("length"));
	}
	if (obj->hasProperty("quantize")) {
		String start_quantize = obj->getProperty("quantize").toString();
		if (start_quantize == "bar") {
			pattern_start = Quantize::BAR;
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
			new_pattern->events_.insert(make_pair(pos, make_unique<NoteOnEvent>(pos, note, velocity)));
			new_pattern->events_.insert(make_pair(pos, make_unique<NoteOffEvent>(pos+length, note)));
		}
		else if (type == "preset") {
		}
		else if (type == "param") {
		}
	}

	auto pattern_end_event = make_unique<ControlEvent>(new_pattern->length_, "pattern_end");
	new_pattern->events_.insert(make_pair(new_pattern->length_, move(pattern_end_event)));

	pending_patterns_.push_back(make_pair(track, move(new_pattern)));

	if (!isTimerRunning()) {
		startTimer(static_cast<int>(getTimeToNextBar() / 1000));
		next_timer_is_bar = true;
	}
}

void Sequencer::stop(unsigned int track, std::string json)
{
	//tracks_[track] = make_unique<Pattern>();
	// TODO: stop at the end of bar
}

double Sequencer::us_to_beats(int64 us)
{
	double beats_per_us = bpm_ / 60 / 1000000;
	return (us * beats_per_us);
}

int64 Sequencer::beats_to_us(double beats)
{
	double bps = bpm_ / 60;
	return static_cast<int64>(beats * (1000000 / bps));
}

int64 Sequencer::getTimeToNextBar()
{
	chrono::time_point<chrono::steady_clock> begin;
	chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
	int64 us_since_begin = chrono::duration_cast<std::chrono::microseconds>(now - begin).count();
	double bar_length = time_sig_num_ / time_sig_den_;
	int64 bar_length_us = beats_to_us(bar_length);
	int64 next_bar_us = ((us_since_begin / bar_length_us) + 1) * bar_length_us;
	int64 us_to_next_bar = next_bar_us - us_since_begin;
	return us_to_next_bar;
}

void Sequencer::setBpm(double bpm)
{
	bpm_ = bpm;
}

void Sequencer::setTimeSignature(int num, int den)
{
	time_sig_num_ = num;
	time_sig_den_ = den;
}

void Sequencer::hiResTimerCallback()
{
	bool is_bar = next_timer_is_bar;
	next_timer_is_bar = false;

	// add any pending patterns
	if (is_bar) {
		for (auto& pat : pending_patterns_) {
			tracks_[pat.first] = move(pat.second);
		}
		pending_patterns_.clear();
	}

	// update tracks
	double next = us_to_beats(getTimeToNextBar());
	next_timer_is_bar = true;
	vector<unique_ptr<Event>> events;
	chrono::time_point<chrono::steady_clock> now = chrono::steady_clock::now();
	int64 us_elapsed = chrono::duration_cast<chrono::microseconds>(now - timer_start_point_).count();
	double elapsed = us_to_beats(us_elapsed);
	for (auto& kv : tracks_) {
		auto time_to_next = kv.second->update(elapsed, events);
		if (time_to_next < next) {
			next = time_to_next;
			next_timer_is_bar = false;
		}
	}

	// trigger events
	for (auto& ev : events) {
		if (ev->type_ == Event::Type::NOTE_ON) {
			active_notes_.insert(static_cast<NoteOnEvent*>(ev.get())->midi_note_);
		}
		if (ev->type_ == Event::Type::NOTE_OFF) {
			active_notes_.erase(static_cast<NoteOffEvent*>(ev.get())->midi_note_);
		}
		trigger_event(ev.get());
	}

	startTimer(static_cast<int>(beats_to_us(next) / 1000));
	timer_start_point_ = chrono::steady_clock::now();
}

void Sequencer::trigger_event(Event* event)
{
	cout << "event triggered. type: " << (int)event->type_ << " pos: " << event->pos_ << endl;
}
