#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include "../JuceLibraryCode/JuceHeader.h"

class Event
{
public:
	enum class Type { NOTE_ON, NOTE_OFF, PRESET, PARAM };
	Event(Type type, float pos) : type_(type), pos_(pos) {}
	Type type_;
	float pos_;
};

class NoteOnEvent : public Event
{
public:
	NoteOnEvent(float pos, int midi_note, float velocity) : Event(Type::NOTE_ON, pos), midi_note_(midi_note), velocity_(velocity) {}
	int midi_note_;
	float velocity_;
};

class NoteOffEvent : public Event
{
public:
	NoteOffEvent(float pos, int midi_note) : Event(Type::NOTE_OFF, pos), midi_note_(midi_note) {}
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

typedef std::vector<std::unique_ptr<Event>> EventList;

enum class Quantize
{
	NOW,
	BEAT,
	BAR
};

class Track : public HighResolutionTimer
{
public:
	Track(double bpm) : curr_event_(0) {}
	~Track() {}

	// schedule a pattern to be played at the next quantize point (beat/bar)
	// current events will keep playing up until the new pattern starts
	void play(std::string pattern_json, Quantize start_point);
	
	// Stop playing all events at the specified stop point
	void stop(Quantize stop_point);
	
	void setBpm(double bpm);

protected:
	void hiResTimerCallback();

private:
	EventList events_;
	EventList loaded_events_;
	Quantize loaded_start_point_;
	size_t curr_event_;
	double bpm_;
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
