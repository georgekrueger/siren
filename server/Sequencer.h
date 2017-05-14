#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <chrono>
#include "../JuceLibraryCode/JuceHeader.h"

class Event
{
public:
	enum class Type { NOTE_ON, NOTE_OFF, PRESET, PARAM, CONTROL };
	Event(Type type, double pos) : type_(type), pos_(pos) {}
	Type type_;
	double pos_;
};

class NoteOnEvent : public Event
{
public:
	NoteOnEvent(double pos, int midi_note, float velocity) : Event(Type::NOTE_ON, pos), midi_note_(midi_note), velocity_(velocity) {}
	int midi_note_;
	float velocity_;
};

class NoteOffEvent : public Event
{
public:
	NoteOffEvent(double pos, int midi_note) : Event(Type::NOTE_OFF, pos), midi_note_(midi_note) {}
	int midi_note_;
};

class ControlEvent : public Event
{
public:
	ControlEvent(double pos, std::string action) : Event(Type::CONTROL, pos), action_(action) {}
	std::string action_;
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
	BAR
};

typedef std::map<double, std::unique_ptr<Event>> Events;

struct Pattern
{
	Pattern() : length_(0), cursor_(0) {}

	double update(double elapsed, std::vector<std::unique_ptr<Event>>& out_events);

	double length_;
	double cursor_;
	Events events_;
};

class Sequencer : public HighResolutionTimer
{
public:
	Sequencer();
	~Sequencer();

	// schedule a pattern to be played
	void play(unsigned int track, std::string json);

	// stop the currently playing pattern
	void stop(unsigned int track, std::string json);

	void setBpm(double bpm);
	void setTimeSignature(int num, int den);

protected:
	void hiResTimerCallback();

private:
	double bpm_;
	int time_sig_num_;
	int time_sig_den_;
	std::chrono::time_point<std::chrono::steady_clock> timer_start_point_;
	bool next_timer_is_bar;
	std::vector<int> active_notes_;
	std::map<unsigned int, std::unique_ptr<Pattern>> tracks_;
	std::vector<std::pair<unsigned int, std::unique_ptr<Pattern>>> pending_patterns_;

	double us_to_beats(int64 us);
	int64 beats_to_us(double beats);
	int64 getTimeToNextBar();
	void trigger_event(Event* event);
	
};
