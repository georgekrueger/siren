#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sequencer.h"
#include <vector>

class HttpListener;

class MainComponent : public Component
{
public:
	MainComponent(AudioDeviceManager* _deviceManager);
	~MainComponent();

	void loadPlugin(int track, std::string plugin, std::function<void(AudioPluginInstance* instance)> done_callback);

	//void paint(Graphics& g);
	//void mouseDown(const MouseEvent& e);
	//void resized();

private:
	AudioDeviceManager* deviceManager;
	AudioPluginFormatManager formatManager;
	struct Track
	{
		Track() : inited(false) {}
		bool inited;
		AudioProcessorPlayer graphPlayer;
		AudioProcessorGraph graph;
		uint32 audioOutNodeId;
		uint32 midiInNodeId;
	};
	std::map<int, Track> tracks;
	Sequencer sequencer;
	std::unique_ptr<HttpListener> http_listener;
	struct PluginCreateCallback;

	Track& getTrack(int track_num);
	void doTest();

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
