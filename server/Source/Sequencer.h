#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include "../JuceLibraryCode/JuceHeader.h"

class Event
{
public:
	enum class Type { NOTE_ON, NOTE_OFF, PRESET, PARAM, CONTROL };
	Event(Type type) : type_(type) {}
	Type type_;

	virtual Event* clone() = 0;
};

class NoteOnEvent : public Event
{
public:
	NoteOnEvent(int midi_note, float velocity) : Event(Type::NOTE_ON), midi_note_(midi_note), velocity_(velocity) {}
	int midi_note_;
	float velocity_;

	Event* clone() override { return new NoteOnEvent(midi_note_, velocity_);  }
};

class NoteOffEvent : public Event
{
public:
	NoteOffEvent(int midi_note) : Event(Type::NOTE_OFF), midi_note_(midi_note) {}
	int midi_note_;

	Event* clone() override { return new NoteOffEvent(midi_note_); }
};

class ControlEvent : public Event
{
public:
	ControlEvent(std::string action) : Event(Type::CONTROL), action_(action) {}
	std::string action_;

	Event* clone() override { return new ControlEvent(action_); }
};

/*
class PresetEvent : public Event
{
public:
	PresetEvent(std::string preset) : Event(Type::PRESET), preset_(preset) {}
	std::string preset_;
};

class ParamEvent : public Event
{
public:
	ParamEvent(std::string param, float value) : Event(Type::PARAM), param_(param), value_(value) {}
	std::string param_;
	float value_;
};
*/

enum class Quantize
{
	BEAT,
	BAR
};

typedef std::map<unsigned int, std::unique_ptr<Event>> Events;

struct Pattern
{
	Pattern() : length_(0), cursor_(0) {}

	unsigned int length_;  // in ticks
	unsigned int cursor_;  // in ticks
	Events events_;
};

class Sequencer : public HighResolutionTimer
{
public:
	Sequencer();
	~Sequencer();

	void start();

	// schedule a pattern to be played
	void play(unsigned int track, std::string json);

	// stop the currently playing pattern
	void stop(unsigned int track, std::string json);

	void setBpm(double bpm);
	void setTimeSignature(int num, int den);

	void setMidiMessageCollector(juce::MidiMessageCollector* midi_msg_collector) { midi_msg_collector_ = midi_msg_collector;  }

protected:
	void hiResTimerCallback();

private:
	double bpm_;
	double beat_length_ms_;
	unsigned int time_sig_num_;
	unsigned int time_sig_den_;
	unsigned int ticks_per_whole_;
	unsigned int beat_;
	std::map<unsigned int, std::set<int>> active_notes_;
	std::map<unsigned int, std::unique_ptr<Pattern>> tracks_;
	std::vector<std::pair<unsigned int, std::unique_ptr<Pattern>>> pending_patterns_;
	juce::MidiMessageCollector* midi_msg_collector_;

	double ms_to_beats(double ms);
	double beats_to_ms(double beats);
	double ticks_to_ms(unsigned ticks);
	double getMsToNextBeat();
	
};
