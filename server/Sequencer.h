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
	enum class Type { NOTE_ON, NOTE_OFF, PRESET, PARAM };
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
	NOW,
	BEAT,
	BAR,
	PATTERN
};

typedef std::map<double, std::unique_ptr<Event>> Events;

struct Pattern
{
	Pattern() : start_(Quantize::BAR), length_(4) {}

	Quantize start_;
	double length_;
	Events events_;
};

class Track : public HighResolutionTimer
{
public:
	Track(double bpm) : cursor_pos_(0), bpm_(bpm) {}
	~Track() {}

	// schedule a pattern to be played
	void play(std::string json);
	
	void stop(std::string json);
	
	void setBpm(double bpm);

protected:
	void hiResTimerCallback();

private:
	std::unique_ptr<Pattern> active_pattern_;
	std::unique_ptr<Pattern> loaded_pattern_;
	double bpm_;
	double cursor_pos_;
	std::chrono::time_point<std::chrono::steady_clock> timer_start_point_;

	void resetTimer();
};

class Sequencer
{
public:
	Sequencer();
	~Sequencer();

	/**
	* Get the track if it exists or create it and return it if it doesn't exist
	* Note: Track numbers start at 1
	*/
	Track* getTrack(unsigned int track);

	void setBpm(double bpm);

private:
	double bpm_;
	std::map<unsigned int, std::unique_ptr<Track>> tracks_;
	
};
