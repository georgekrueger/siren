#include "Sequencer.h"

using namespace std;

Sequencer::Sequencer() : time_sig_num_(4), time_sig_den_(4), next_timer_is_bar(false)
{
	setBpm(120);
}

Sequencer::~Sequencer()
{
}

void Sequencer::start()
{
	if (!isTimerRunning()) {

	}
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

double Sequencer::ms_to_beats(double ms)
{
	return (ms / beat_length_ms_);
}

double Sequencer::beats_to_ms(double beats)
{
	return beats * beat_length_ms_;
}

double Sequencer::getTimeToNextBar()
{
	double now = Time::getMillisecondCounterHiRes();
	double next_beat_ms = ( static_cast<unsigned long>((now + 20) / beat_length_ms_) + 1) * beat_length_ms_;
	return next_beat_ms - now;
}

void Sequencer::setBpm(double bpm)
{
	bpm_ = bpm;
	beat_length_ms_ = 60000 / bpm;
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
