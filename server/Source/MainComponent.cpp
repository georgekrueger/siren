#include "MainComponent.H"
#include <sstream>

using std::ostringstream;
typedef AudioProcessorGraph::AudioGraphIOProcessor AudioIOProcessor;

struct MainComponent::PluginCreateCallback : public AudioPluginFormat::InstantiationCompletionCallback
{
	PluginCreateCallback(MainComponent* myself, int _track)
		: owner(myself), track_num(_track)
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
			Track& track = owner->getTrack(track_num);
			uint32 nodeId = track.graph.addNode(instance)->nodeId;
			track.graph.addConnection(nodeId, 0, track.audioOutNodeId, 0);
			track.graph.addConnection(nodeId, 1, track.audioOutNodeId, 1);
			track.graph.addConnection(track.midiInNodeId, 0, nodeId, 0);
			track.graph.addConnection(track.midiInNodeId, 1, nodeId, 1);
		}
	}

	MainComponent* owner;
	int track_num;
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
	for (auto &kv : tracks) {
		deviceManager->removeAudioCallback(&kv.second.graphPlayer);
		kv.second.graphPlayer.setProcessor(nullptr);
	}
	tracks.clear();

	deleteAllChildren();
}

void MainComponent::loadPlugin(int track_num, std::string plugin)
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
		Track& track = getTrack(track_num);
		formatManager.createPluginInstanceAsync(*(descs[0]), track.graph.getSampleRate(), track.graph.getBlockSize(),
			new PluginCreateCallback(this, track_num));
	}
	else {
		ostringstream ss;
		ss << "Error: unable to find vst plugin for " << plugin;
		Logger::writeToLog(ss.str());
	}
}
