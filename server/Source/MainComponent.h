#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Sequencer.h"

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
	AudioProcessorPlayer graphPlayer;
	AudioProcessorGraph graph;
	AudioProcessorGraph::AudioGraphIOProcessor* audioOutProcessor;
	uint32 audioOutNodeId;
	Sequencer sequencer;
	struct PluginCreateCallback;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
