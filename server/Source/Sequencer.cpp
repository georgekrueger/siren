#include "Sequencer.h"
#include <sstream>

using namespace std;

const unsigned int ticks_per_beat = 1000;

Sequencer::Sequencer()
{
	setBpm(120);
	setTimeSignature(4, 4);
}

Sequencer::~Sequencer()
{
	stopTimer();
}

void Sequencer::start()
{
	if (!isTimerRunning()) {
		beat_ = 0;
		startTimer(static_cast<int>(getMsToNextBeat()));
	}
}

void Sequencer::play(std::string json)
{
	// parse json event array
	// array is an ordered array of arrays (representing events)
	// example: 
	// {
	// 'track': 1,
    // 'length': 4,
    // 'quantize': 'bar',
    // 'events' : [["note", 0, 60, 1.0, 1], ["note", 1, 62, 0.9, 1], ["preset", 1, 1], ["param", 1, "param name", 1.0]]
    // }

	auto new_pattern = make_unique<Pattern>();
	auto pattern_start = Quantize::BAR;
	var parse_results;
	juce::Result parse_ret = JSON::parse(json, parse_results);
	if (parse_ret.failed()) {
		Logger::writeToLog(parse_ret.getErrorMessage());
		return;
	}
	DynamicObject* obj = parse_results.getDynamicObject();
	if (obj->hasProperty("length")) {
		new_pattern->length_ = static_cast<int>(obj->getProperty("length")) * ticks_per_whole_;
	}
	int track = -1;
	if (!obj->hasProperty("track")) {
		return;
	}
	track = static_cast<int>(obj->getProperty("track"));

	if (obj->hasProperty("quantize")) {
		String start_quantize = obj->getProperty("quantize").toString();
		if (start_quantize == "bar") {
			pattern_start = Quantize::BAR;
		}
		else if (start_quantize == "beat") {
			pattern_start = Quantize::BEAT;
		}
	}
	Array<var>* event_array = obj->getProperty("events").getArray();
	for (int i = 0; i < event_array->size(); ++i) {
		Array<var>* event_items = event_array->getReference(i).getArray();
		String type = event_items->getReference(0);
		double pos = static_cast<double>(event_items->getReference(1));
		if (type == "note") {
			int note = static_cast<int>(event_items->getReference(2));
			float velocity = static_cast<float>(event_items->getReference(3));
			float length = static_cast <float> (event_items->getReference(4));
			unsigned int length_in_ticks = static_cast<unsigned int>(length * ticks_per_whole_);
			unsigned int pos_in_ticks = static_cast<unsigned int>(pos * ticks_per_whole_);
			new_pattern->events_.insert(make_pair(pos_in_ticks, make_unique<NoteOnEvent>(note, velocity)));
			new_pattern->events_.insert(make_pair(pos_in_ticks + length_in_ticks, make_unique<NoteOffEvent>(note)));
			std::ostringstream ss;
			ss << "Sequencer add note " << note << " vel " << velocity << " len " << length;
			Logger::writeToLog(ss.str());
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

double Sequencer::ticks_to_ms(unsigned ticks)
{
	return beats_to_ms(ticks / ticks_per_beat);
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
	ticks_per_whole_ = ticks_per_beat * time_sig_den_;
}

void Sequencer::setMidiMessageCollector(int track_num, juce::MidiMessageCollector* midi_msg_collector)
{
	midi_msg_collectors_[track_num] = midi_msg_collector;
}

void Sequencer::hiResTimerCallback()
{
	//Logger::writeToLog("timer callback");
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
	unsigned int cursor_inc = ticks_per_beat;
	for (auto& track : tracks_) {
		unsigned int track_num = track.first;
		Pattern* pattern = track.second.get();
		if (pattern->length_ == 0) {
			continue;
		}
		auto event_it = pattern->events_.lower_bound(pattern->cursor_);
		while (event_it != pattern->events_.end() && event_it->first < pattern->cursor_ + cursor_inc)
		{
			if (event_it->second->type_ != Event::Type::CONTROL) {
				double event_offset_ms = ticks_to_ms(event_it->first - pattern->cursor_);
				if (event_it->second->type_ == Event::Type::NOTE_ON) {
					NoteOnEvent* note_on = static_cast<NoteOnEvent*>(event_it->second.get());
					active_notes_[track.first].insert(note_on->midi_note_);
					MidiMessage m(MidiMessage::noteOn(1, note_on->midi_note_, note_on->velocity_));
					m.setTimeStamp((now + event_offset_ms) * 0.001);
					midi_msg_collectors_[track_num]->addMessageToQueue(m);
				}
				if (event_it->second->type_ == Event::Type::NOTE_OFF) {
					NoteOffEvent* note_off = static_cast<NoteOffEvent*>(event_it->second.get());
					active_notes_[track.first].erase(note_off->midi_note_);
					MidiMessage m(MidiMessage::noteOff(1, note_off->midi_note_));
					m.setTimeStamp((now + event_offset_ms) * 0.001);
					midi_msg_collectors_[track_num]->addMessageToQueue(m);
				}
			}
			++event_it;
		}
		pattern->cursor_ += cursor_inc;
		if (pattern->cursor_ >= pattern->length_) {
			pattern->cursor_ -= pattern->length_;
		}
	}

	// if at the end of the bar, load any pending patterns
	if (beat_ == time_sig_num_) {
		for (auto& pat : pending_patterns_) {
			// turn off any active notes that have not finished yet for existing pattern
			unsigned int track_num = pat.first;
			for (int note_num : active_notes_[track_num]) {
				MidiMessage m(MidiMessage::noteOff(1, note_num));
				m.setTimeStamp((now + ticks_to_ms(cursor_inc - 1)) * 0.001);
				midi_msg_collectors_[track_num]->addMessageToQueue(m);
			}
			tracks_[track_num] = move(pat.second);
		}
		pending_patterns_.clear();
	}

	++beat_;
	if (beat_ > time_sig_num_) {
		beat_ = 1;
	}
	startTimer(static_cast<int>(getMsToNextBeat()));
}

