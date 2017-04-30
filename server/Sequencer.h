#pragma once

#include <string>
#include <vector>
#include <memory>

class Event
{
public:
	enum class Type { NOTE_ON, NOTE_OFF, PRESET, PARAM };
	Event(Type type) : type_(type) {}
	Type type_;
};

class NoteOnEvent : public Event
{
public:
	NoteOnEvent(int midi_note, float velocity) : Event(Type::NOTE_ON), midi_note_(midi_note), velocity_(velocity) {}
	int midi_note_;
	float velocity_;
};

class NoteOffEvent : public Event
{
public:
	NoteOffEvent(int midi_note) : Event(Type::NOTE_OFF), midi_note_(midi_note) {}
	int midi_note_;
};

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

class Track
{
public:
	Track() {}
	

private:
	std::vector<std::unique_ptr<Event>> events_;
	std::vector<std::unique_ptr<Event>> pending_events_;
};

class Sequencer
{
public:
	Sequencer() {}
	~Sequencer() {}

private:
	int bpm_;
	
};
