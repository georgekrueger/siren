#include "Sequencer.h"

using namespace std;

Sequencer::Sequencer() : time_sig_num_(4), time_sig_den_(4)
{
	setBpm(120);
}

Sequencer::~Sequencer()
{
}

void Sequencer::start()
{
	if (!isTimerRunning()) {
		beat_ = 0;
		startTimer(static_cast<int>(getMsToNextBeat()));
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
		new_pattern->length_in_beats_ = static_cast<int>(obj->getProperty("length"));
	}
	if (obj->hasProperty("quantize")) {
		String start_quantize = obj->getProperty("quantize").toString();
		if (start_quantize == "bar") {
			pattern_start = Quantize::BAR;
		}
		else if (start_quantize == "beat") {
			pattern_start = Quantize::BEAT;
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
			new_pattern->events_.insert(make_pair(pos, make_unique<NoteOnEvent>(note, velocity)));
			new_pattern->events_.insert(make_pair(pos + length, make_unique<NoteOffEvent>(note)));
		}
		else if (type == "preset") {
		}
		else if (type == "param") {
		}
	}

	//auto pattern_end_event = make_unique<ControlEvent>("pattern_end");
	//new_pattern->events_.insert(make_pair(new_pattern->length_in_beats_, move(pattern_end_event)));

	pending_patterns_.push_back(make_pair(track, move(new_pattern)));
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

double Sequencer::getMsToNextBeat()
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
	if (beat_ == 0) {
		// special case to handle first time timer is started
		++beat_;
		// load pending patterns
		for (auto& pat : pending_patterns_) {
			tracks_[pat.first] = move(pat.second);
		}
		pending_patterns_.clear();
		startTimer(static_cast<int>(getMsToNextBeat()));
		return;
	}

	// update tracks
	double now = Time::getMillisecondCounterHiRes();
	double cursor_inc = 1 / time_sig_den_;
	for (auto& track : tracks_) {
		Pattern* pattern = track.second.get();
		auto it = pattern->events_.lower_bound(pattern->cursor_);
		while (it != pattern->events_.end() && it->first < pattern->cursor_ + cursor_inc)
		{
			if (it->second->type_ != Event::Type::CONTROL) {
				double event_offset_ms = beats_to_ms(it->first - pattern->cursor_);
				if (it->second->type_ == Event::Type::NOTE_ON) {
					NoteOnEvent* note_on = static_cast<NoteOnEvent*>(it->second.get());
					active_notes_.insert(note_on->midi_note_);
					MidiMessage m(MidiMessage::noteOn(1, note_on->midi_note_, note_on->velocity_));
					m.setTimeStamp((now + event_offset_ms) * 0.001);
					midi_msg_collector_->addMessageToQueue(m);
				}
				if (it->second->type_ == Event::Type::NOTE_OFF) {
					NoteOffEvent* note_off = static_cast<NoteOffEvent*>(it->second.get());
					active_notes_.erase(note_off->midi_note_);
					MidiMessage m(MidiMessage::noteOff(1, note_off->midi_note_));
					m.setTimeStamp((now + event_offset_ms) * 0.001);
					midi_msg_collector_->addMessageToQueue(m);
				}
			}
			++it;
		}
		pattern->cursor_ += cursor_inc;
	}

	// if at the end of the bar, load any pending patterns
	if (beat_ == time_sig_num_) {
		for (auto& pat : pending_patterns_) {
			tracks_[pat.first] = move(pat.second);
		}
		pending_patterns_.clear();
	}

	++beat_;
	if (beat_ > time_sig_num_) {
		beat_ = 1;
	}
	startTimer(static_cast<int>(getMsToNextBeat()));
}

