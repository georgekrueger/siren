#include "MainComponent.H"

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
			AudioProcessorGraph::Node* node = owner->graph.addNode(instance);
		}
	}

	MainComponent* owner;
	int track;
};

MainComponent::MainComponent(AudioDeviceManager* _deviceManager) 
	: deviceManager(_deviceManager), graphPlayer(false)
{
	formatManager.addDefaultFormats();
	graphPlayer.setProcessor(&graph);
	deviceManager->addAudioCallback(&graphPlayer);

	sequencer.setMidiMessageCollector(&graphPlayer.getMidiMessageCollector());
	sequencer.start();
}

MainComponent::~MainComponent()
{
	deviceManager->removeAudioCallback(&graphPlayer);

	deleteAllChildren();

	graphPlayer.setProcessor(nullptr);
}

void MainComponent::loadPlugin(int track, std::string plugin)
{
	PluginDescription* desc;

	if (desc != nullptr)
	{
		formatManager.createPluginInstanceAsync(*desc, graph.getSampleRate(), graph.getBlockSize(),
			new PluginCreateCallback(this, track));
	}
}
