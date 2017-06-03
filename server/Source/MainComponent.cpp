#include "MainComponent.H"
#include <sstream>

using std::ostringstream;
typedef AudioProcessorGraph::AudioGraphIOProcessor AudioIOProcessor;

struct MainComponent::PluginCreateCallback : public AudioPluginFormat::InstantiationCompletionCallback
{
	PluginCreateCallback(MainComponent* myself, int _track)
		: owner(myself), track(_track)
	{}

	void completionCallback(AudioPluginInstance* instance, const String& error) override
	{
		if (instance == nullptr)
		{
			AlertWindow::showMessageBox(AlertWindow::WarningIcon,
				TRANS("Failed to create plugin!"), error);
		}
		else
		{
			instance->enableAllBuses();
			uint32 nodeId = owner->graph.addNode(instance)->nodeId;
			owner->graph.addConnection(nodeId, 0, owner->audioOutNodeId, 0);
			owner->graph.addConnection(nodeId, 1, owner->audioOutNodeId, 1);
		}
	}

	MainComponent* owner;
	int track;
};

MainComponent::MainComponent(AudioDeviceManager* _deviceManager) 
	: deviceManager(_deviceManager)
{
	formatManager.addDefaultFormats();
	sequencer.start();
}

MainComponent::Track& MainComponent::getTrack(int track_num)
{
	Track& track = tracks[track_num];
	if (!track.inited) {
		track.graphPlayer.setProcessor(&track.graph);
		deviceManager->addAudioCallback(&track.graphPlayer);
		// create and add midi input and audio output nodes
		track.audioOutNodeId = track.graph.addNode(new AudioIOProcessor(AudioIOProcessor::audioOutputNode))->nodeId;
		track.midiInNodeId = track.graph.addNode(new AudioIOProcessor(AudioIOProcessor::midiInputNode))->nodeId;
		sequencer.setMidiMessageCollector(track_num, &track.graphPlayer.getMidiMessageCollector());
	}
	return track;
}

MainComponent::~MainComponent()
{
	deviceManager->removeAudioCallback(&graphPlayer);

	deleteAllChildren();

	graphPlayer.setProcessor(nullptr);
}

void MainComponent::loadPlugin(int track, std::string plugin)
{
	OwnedArray<PluginDescription> descs;

	VSTPluginFormat vstFormat;
	vstFormat.findAllTypesForFile(descs, plugin);
	if (descs.size() > 0) {
		ostringstream ss;
		ss << "Found vst plugin for " << plugin;
		Logger::writeToLog(ss.str());
	}
	else {
		VST3PluginFormat vst3Format;
		vst3Format.findAllTypesForFile(descs, plugin);
		ostringstream ss;
		ss << "Found vst3 plugin for " << plugin;
		Logger::writeToLog(ss.str());
	}

	if (descs.size() > 0) {
		formatManager.createPluginInstanceAsync(*(descs[0]), graph.getSampleRate(), graph.getBlockSize(),
			new PluginCreateCallback(this, track));
	}
	else {
		ostringstream ss;
		ss << "Error: unable to find vst plugin for " << plugin;
		Logger::writeToLog(ss.str());
	}
}
