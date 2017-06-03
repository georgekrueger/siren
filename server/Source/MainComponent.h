#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sequencer.h"
#include <vector>

class MainComponent : public Component
{
public:
	MainComponent(AudioDeviceManager* _deviceManager);
	~MainComponent();

	void loadPlugin(int track, std::string plugin);

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
		//AudioProcessorGraph::AudioGraphIOProcessor* audioOutProcessor;
		//AudioProcessorGraph::AudioGraphIOProcessor* midiInProcessor;
		uint32 audioOutNodeId;
		uint32 midiInNodeId;
	};
	std::map<int, Track> tracks;
	Sequencer sequencer;
	struct PluginCreateCallback;

	Track& getTrack(int track_num);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
