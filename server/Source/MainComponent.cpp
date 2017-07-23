#include "MainComponent.H"
#include "HttpListener.h"
#include <sstream>
#include <functional>

using std::ostringstream;
typedef AudioProcessorGraph::AudioGraphIOProcessor AudioIOProcessor;

struct MainComponent::PluginCreateCallback : public AudioPluginFormat::InstantiationCompletionCallback
{
	PluginCreateCallback(MainComponent* myself, int _track, std::function<void(AudioPluginInstance* instance)> _done_callback)
		: owner(myself), track_num(_track), done_callback(_done_callback)
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
		done_callback(instance);
	}

	MainComponent* owner;
	int track_num;
	std::function<void(AudioPluginInstance* instance)> done_callback;
};

MainComponent::MainComponent(AudioDeviceManager* _deviceManager) 
	: deviceManager(_deviceManager)
{
	formatManager.addDefaultFormats();
	sequencer.start();

	http_listener = std::make_unique<HttpListener>();
	http_listener->startThread();

	doTest();
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

void MainComponent::plugLoadDone(AudioPluginInstance* instance)
{
	if (instance == nullptr) {
		ostringstream ss;
		ss << "Plugin was not loaded!!";
		Logger::writeToLog(ss.str());
		return;
	}
	
	ostringstream ss;
	ss << "Plugin loaded. Current prog: " << instance->getCurrentProgram()
		<< " (" << instance->getProgramName(instance->getCurrentProgram()) << ")"
		<< " num progs: " << instance->getNumPrograms();
	Logger::writeToLog(ss.str());

	ostringstream playss;
	playss << "{ 'track': 1, 'length': 4,"
		   << "'events': [ ['note', 0, 50, 1.0, 1], ['note', 1, 55, 0.9, 1.1] ] }";
	sequencer.play(playss.str());
}

void MainComponent::doTest()
{
	using std::placeholders::_1;
	loadPlugin(1, "C:\\VST\\Synth1 VST.dll", std::bind(&MainComponent::plugLoadDone, this, _1) );
}

void MainComponent::loadPlugin(int track_num, std::string plugin, std::function<void(AudioPluginInstance* instance)> done_callback)
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
			new PluginCreateCallback(this, track_num, done_callback));
	}
	else {
		ostringstream ss;
		ss << "Error: unable to find vst plugin for " << plugin;
		Logger::writeToLog(ss.str());
	}
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
