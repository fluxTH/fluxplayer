#include "fluxplayer.h"

#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

#include "fxaudiodeviceselectdialog.h"
#include "fxconfigurationdialog.h"

#include "bassmix.h"
#include "basswasapi.h"

FluxPlayer::FluxPlayer(QWidget *parent)
	: QMainWindow(parent),
	m_players(new QList<FXPlayer*>())
{
	ui.setupUi(this);
	
	this->initPlaylist();

	QObject::connect(ui.actionConfiguration, SIGNAL(triggered()), this, SLOT(handleActionConfiguration()));

	QObject::connect(ui.actionAddMedia, SIGNAL(triggered()), this, SLOT(handlePlaylistAddFiles()));
	QObject::connect(ui.actionAddMediaFolder, SIGNAL(triggered()), this, SLOT(handlePlaylistAddFolder()));

	QObject::connect(ui.actionToggleCountdown, SIGNAL(triggered()), ui.playerControlWidget, SLOT(toggleCountdown()));
	QObject::connect(ui.actionPlaylistNumbering, SIGNAL(triggered()), this, SLOT(handleTogglePlaylistNumbering()));

	QObject::connect(this->m_playlistModel, SIGNAL(attachControlToPlayer(FXPlayer*, bool)), this, SLOT(handleAttachPlayerControl(FXPlayer*, bool)));
	QObject::connect(this->m_playlistModel, SIGNAL(doEjectPlayer(FXPlayer*)), this, SLOT(handleEjectPlayer(FXPlayer*)));
}

FluxPlayer::~FluxPlayer()
{
	delete this->m_playlistModel;

	qDeleteAll(*this->m_players);
	delete this->m_players;

	for (short int i = 0; i < this->m_bassPlugins.size(); ++i)
		BASS_PluginFree(this->m_bassPlugins[i]);

	if (this->m_audioDriver == FXAudioDriver::WASAPI)
		BASS_WASAPI_Stop(true);

	BASS_StreamFree(this->m_mixerHandle);
	BASS_Free();
}

bool FluxPlayer::initialize()
{
	//QSettings settings("config.ini", QSettings::IniFormat);
	//settings.beginGroup("audio");

	//this->m_sampleRate = settings.value("sampleRate", -1).toInt();
	//this->m_outputDeviceIndex = settings.value("deviceId", -2).toInt();
	//this->m_channelLayout = settings.value("channels", -1).toInt();

	//if (this->m_sampleRate == -1 || this->m_outputDeviceIndex == -2 || m_channelLayout == -1) {
		/*FXAudioDeviceSelectDialog dialog(nullptr);
		int dialog_result = dialog.exec();

		if (dialog_result != QDialog::Accepted)
			return false;*/

		this->m_audioDriver = FXAudioDriver::WASAPI;//dialog.getDriver();
		this->m_audioSampleRate = 48000;//dialog.getSampleRate();
		this->m_audioDeviceIndex = -1;//dialog.getOutputDeviceIndex();
		this->m_audioChannelLayout = 2;//dialog.getChannelLayout();

		//settings.setValue("sampleRate", this->m_sampleRate);
		//settings.setValue("deviceId", this->m_outputDeviceIndex);
		//settings.setValue("channels", this->m_channelLayout);
	//}

	//settings.endGroup();

	if (!this->initBass()) {
		QMessageBox::critical(nullptr, 
			"fluxplayer", 
			"Initialization sequence failed!\n" 
			"Audio engine error: " + QString::number(BASS_ErrorGetCode())
		);
		return false;
	}

	this->initPlayers();

	QObject::connect(ui.playerControlWidget, SIGNAL(playNext()), this->m_playlistModel, SLOT(handlePlayNext()));
	QObject::connect(ui.playerControlWidget, SIGNAL(playPrev()), this->m_playlistModel, SLOT(handlePlayPrev()));

	return true;
}

bool FluxPlayer::initBass()
{
	// Load BASS plugins
	for (short int i = 0; i < this->BASS_PLUGINS.size(); ++i) {
		HPLUGIN plugin = BASS_PluginLoad(QString("%1.dll").arg(this->BASS_PLUGINS[i]).toStdString().c_str(), NULL);
		if (plugin != 0) {
			qInfo() << "Loaded" << this->BASS_PLUGINS[i] << "plugin successfully.";
			this->m_bassPlugins.append(plugin);
		}
		else {
			QMessageBox::warning(nullptr,
				"fluxplayer",
				"Audio plugin failed to load!\n"
				"Plugin \"" + this->BASS_PLUGINS[i] + "\" failed to load, player functionality maybe decreased."
			);
		}
	}

	// Init BASS
	DWORD bassFlags;
	if (this->m_audioChannelLayout == 2)
		bassFlags = BASS_DEVICE_STEREO;
	else if (this->m_audioChannelLayout == 1)
		bassFlags = BASS_DEVICE_MONO;

	if (this->m_audioDriver == FXAudioDriver::DSOUND)
		bassFlags |= BASS_DEVICE_DSOUND;

	int bassDevice = this->m_audioDriver == FXAudioDriver::WASAPI ? -1 : this->m_audioDeviceIndex;

	if (BASS_Init(bassDevice, this->m_audioSampleRate, bassFlags, reinterpret_cast<HWND>(this->winId()), FALSE)) {
		DWORD mixerFlags = BASS_MIXER_NONSTOP | BASS_SAMPLE_FLOAT;
		if (this->m_audioDriver == FXAudioDriver::WASAPI /* Add ASIO here too */)
			mixerFlags |= BASS_STREAM_DECODE;

		this->m_mixerHandle = BASS_Mixer_StreamCreate(this->m_audioSampleRate, this->m_audioChannelLayout, mixerFlags);
		if (this->m_mixerHandle == 0)
			return false;

		if (this->m_audioDriver == FXAudioDriver::WASAPI) {
			if (BASS_WASAPI_Init(
				this->m_audioDeviceIndex,
				this->m_audioSampleRate,
				this->m_audioChannelLayout,
				NULL,//BASS_WASAPI_EXCLUSIVE, // flags
				0.04,
				0,
				WASAPIPROC_BASS,
				reinterpret_cast<void*>(this->m_mixerHandle)
			)) {
				qDebug() << "WASAPI init";

				return BASS_WASAPI_Start();
			}
			else {
				qDebug() << "WASAPI error" << BASS_ErrorGetCode();
				qDebug() << "Fallback";
			}
		}
			
		return BASS_ChannelPlay(this->m_mixerHandle, FALSE);

	}

	return false;
}

void FluxPlayer::initPlayers()
{
	qDebug() << "Initializing players...";

	for (short int i = 0; i < this->PLAYER_COUNT;  i++) {
		short int index = this->createPlayer(this->PLAYER_NAME_MAP[i]);
		FXPlayer *player = this->m_players->value(index);

		//temp
		QObject::connect(player, SIGNAL(bassSyncPosNext()), this->m_playlistModel, SLOT(handleFadeNext()));

		QObject::connect(player, SIGNAL(ejected(FXPlayer*)), this->m_playlistModel, SLOT(handlePlayerEjected(FXPlayer*)));

		QObject::connect(player, SIGNAL(trackPlayed(FXPlayer*)), ui.playerControlWidget, SLOT(handlePlayerPlayed(FXPlayer*)));
		QObject::connect(player, SIGNAL(trackStopped(FXPlayer*)), ui.playerControlWidget, SLOT(handlePlayerStopped(FXPlayer*)));
	}

	this->m_playlistModel->setPlayerContainer(this->m_players);
}

void FluxPlayer::initPlaylist()
{
	this->m_playlistModel = new FXPlaylistModel(this);
	ui.playlistView->setModel(this->m_playlistModel);
}

short int FluxPlayer::createPlayer(const char &name)
{
	FXPlayer *player = new FXPlayer(this, name);
	player->attachToMixer(&this->m_mixerHandle);
	this->m_players->append(player);
	return this->m_players->indexOf(player);
}

bool FluxPlayer::deletePlayer(short int &index)
{
	if (0 <= index && index < this->m_players->size()) {
		delete this->m_players->takeAt(index);

		return true;
	}
	return false;
}

bool FluxPlayer::deletePlayer(FXPlayer *player)
{
	short int index = this->m_players->indexOf(player);
	return this->deletePlayer(index);
}

FXAudioDriver* FluxPlayer::getAudioDriver()
{
	return &this->m_audioDriver;
}

// Slots
void FluxPlayer::handleAttachPlayerControl(FXPlayer *player, bool forced) {
	ui.playerControlWidget->attachPlayer(player, forced);
}

void FluxPlayer::handleEjectPlayer(FXPlayer *player)
{
	player->eject();
}

void FluxPlayer::handleTogglePlaylistNumbering()
{

}

void FluxPlayer::handleActionConfiguration()
{
	FXConfigurationDialog config_dialog(this);

	config_dialog.exec();
}

void FluxPlayer::handlePlaylistAddFiles()
{
	QFileDialog dialog(this, "Add media file to playlist...");
	dialog.setFileMode(QFileDialog::ExistingFiles);

	QString audioExtensions = "*.";
	audioExtensions += this->AUDIO_EXTS.join(" *.");

	dialog.setNameFilter(
		QString("Audio Files (%1);;").arg(audioExtensions) +
		"All Files (*)"
	);

	if (dialog.exec()) {
		QStringList selected = dialog.selectedFiles();
		foreach (QString filepath, selected) {
			this->m_playlistModel->addTrack(filepath);
		}
		this->m_playlistModel->loadPlayers();
	}
	
}

void FluxPlayer::handlePlaylistAddFolder()
{
	QFileDialog dialog(this, "Add media folder to playlist...");
	dialog.setFileMode(QFileDialog::Directory);

	if (dialog.exec()) {
		QDir directory(dialog.selectedFiles().first());

		QStringList extFilter;
		foreach (QString ext, this->AUDIO_EXTS)
			extFilter << QString("*.%1").arg(ext);

		QStringList files = directory.entryList(extFilter, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
		foreach(QString filename, files) {
			this->m_playlistModel->addTrack(directory.filePath(filename));
		}
		this->m_playlistModel->loadPlayers();
	}

}
