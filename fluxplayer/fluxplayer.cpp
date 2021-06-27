#include "fluxplayer.h"

#include <QDebug>
#include <QMessageBox>
#include <QSettings>
#include <QFileDialog>

#include "fxaudiodeviceselectdialog.h"
#include "fxconfigurationdialog.h"

#include "bassmix.h"
#include "basswasapi.h"
#include "bassasio.h"


FluxPlayer::FluxPlayer(QWidget *parent)
	: QMainWindow(parent),
	m_playlistModel(this),
	m_players(new QList<FXPlayer*>())
{
	ui.setupUi(this);
	
	this->setWindowTitle("fluxplayer - untitled playlist");

	this->initPlaylist();

	QObject::connect(ui.actionConfiguration, SIGNAL(triggered()), this, SLOT(handleActionConfiguration()));

	QObject::connect(ui.actionAddMedia, SIGNAL(triggered()), this, SLOT(handlePlaylistAddFiles()));
	QObject::connect(ui.actionAddMediaFolder, SIGNAL(triggered()), this, SLOT(handlePlaylistAddFolder()));

	QObject::connect(ui.actionToggleCountdown, SIGNAL(triggered()), ui.playerControlWidget, SLOT(toggleCountdown()));
	QObject::connect(ui.actionPlaylistNumbering, SIGNAL(triggered()), this, SLOT(handleTogglePlaylistNumbering()));

	QObject::connect(&this->m_playlistModel, SIGNAL(statusBarTextChanged(QString)), ui.statusBar, SLOT(showMessage(QString)));
	QObject::connect(&this->m_playlistModel, SIGNAL(attachControlToPlayer(FXPlayer*, bool)), this, SLOT(handleAttachPlayerControl(FXPlayer*, bool)));
	QObject::connect(&this->m_playlistModel, SIGNAL(doEjectPlayer(FXPlayer*)), this, SLOT(handleEjectPlayer(FXPlayer*)));

	this->m_levelTimer = new QTimer(this);
	QObject::connect(this->m_levelTimer, SIGNAL(timeout()), this, SLOT(handleLevelTick()));
	QObject::connect(&this->m_playlistModel, SIGNAL(normalizationChanged(double)), ui.levelMeter, SLOT(handleNormalizationChanged(double)));
	QObject::connect(ui.levelMeter, SIGNAL(volumeRatioChanged(double)), this, SLOT(handleVolumeRatioChanged(double)));
}

FluxPlayer::~FluxPlayer()
{
	delete this->m_levelTimer;
	//delete this->m_playlistModel;
	this->m_playlistModel.setActive(false);

	qDeleteAll(*this->m_players);
	delete this->m_players;

	for (short int i = 0; i < this->m_bassPlugins.size(); ++i)
		BASS_PluginFree(this->m_bassPlugins[i]);

	if (this->m_audioDriver == FXAudioDriver::WASAPI) {
		BASS_WASAPI_Stop(true);
	}
	else if (this->m_audioDriver == FXAudioDriver::ASIO) {
		BASS_ASIO_Stop();
		BASS_ASIO_Free();
	}

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
		FXAudioDeviceSelectDialog selectionDialog(nullptr);
		int dialogResult = selectionDialog.exec();

		if (dialogResult != QDialog::Accepted)
			return false;

		this->m_audioDriver = selectionDialog.getDriver();
		this->m_audioSampleRate = selectionDialog.getSampleRate();
		this->m_audioDeviceIndex = selectionDialog.getOutputDeviceIndex();
		this->m_audioChannelLayout = selectionDialog.getChannelLayout();

		/*this->m_audioDriver = FXAudioDriver::WASAPI;
		this->m_audioSampleRate = 48000;
		this->m_audioDeviceIndex = -1;
		this->m_audioChannelLayout = 2;*/

		//settings.setValue("sampleRate", this->m_sampleRate);
		//settings.setValue("deviceId", this->m_outputDeviceIndex);
		//settings.setValue("channels", this->m_channelLayout);
	//}

	//settings.endGroup();

	if (!this->initBass()) {
		int errorCode = BASS_ErrorGetCode();
		QMessageBox::critical(this, 
			"fluxplayer", 
			"Initialization sequence failed!\n"
			"Audio engine error: " + 
			QString("%2(%1)").arg(errorCode).arg(
				errorCode < sizeof(BassErrorString) / sizeof(*BassErrorString) ? BassErrorString[errorCode] + " " : ""
			) +
			"\n\nApplication will now quit!"
		);
		return false;
	}

	this->initPlayers();

	QObject::connect(ui.playerControlWidget, SIGNAL(playNext()), &this->m_playlistModel, SLOT(handlePlayNext()));
	QObject::connect(ui.playerControlWidget, SIGNAL(playPrev()), &this->m_playlistModel, SLOT(handlePlayPrev()));

	this->m_levelTimer->start(this->METER_UPDATE_PERIOD_MS);

	return true;
}

void CALLBACK LevelDspProc(HDSP, DWORD, void *buffer, DWORD, void *user) {
	FluxPlayer *parent = reinterpret_cast<FluxPlayer*>(user);
	ebur128_add_frames_float(
		parent->m_r128State,
		reinterpret_cast<float*>(buffer),
		parent->m_audioGranule
	);
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
	DWORD bassFlags = NULL;
	if (this->m_audioChannelLayout == 2)
		bassFlags = BASS_DEVICE_STEREO;
	else if (this->m_audioChannelLayout == 1)
		bassFlags = BASS_DEVICE_MONO;

	int bassDevice = -1;
	if (this->m_audioDriver == FXAudioDriver::DSOUND) {
		bassFlags |= BASS_DEVICE_DSOUND;
		bassDevice = this->m_audioDeviceIndex;
	}

	if (BASS_Init(bassDevice, this->m_audioSampleRate, bassFlags, reinterpret_cast<HWND>(this->winId()), FALSE)) {

		this->m_audioGranule = this->m_audioSampleRate / 100;

		BASS_SetConfig(BASS_CONFIG_FLOATDSP, TRUE);
		BASS_SetConfig(BASS_CONFIG_SRC, 4);

		DWORD mixerFlags = BASS_MIXER_NONSTOP | BASS_SAMPLE_FLOAT;
		if (this->m_audioDriver == FXAudioDriver::WASAPI || this->m_audioDriver == FXAudioDriver::ASIO)
			mixerFlags |= BASS_STREAM_DECODE;

		this->m_mixerHandle = BASS_Mixer_StreamCreate(this->m_audioSampleRate, this->m_audioChannelLayout, mixerFlags);
		if (this->m_mixerHandle == 0)
			return false;

		BASS_ChannelSetAttribute(this->m_mixerHandle, BASS_ATTRIB_GRANULE, this->m_audioGranule);

		// Init R128 Level Meter
		this->m_r128State = ebur128_init(
			this->m_audioChannelLayout,
			this->m_audioSampleRate,
			EBUR128_MODE_S
		);

		BASS_ChannelSetDSP(this->m_mixerHandle, LevelDspProc, this, 0);

		// Init BASS WASAPI
		if (this->m_audioDriver == FXAudioDriver::WASAPI) {
			if (BASS_WASAPI_Init(
				this->m_audioDeviceIndex,
				this->m_audioSampleRate,
				this->m_audioChannelLayout,
				BASS_WASAPI_BUFFER,// | BASS_WASAPI_EXCLUSIVE, // flags
				0.03,
				0,
				WASAPIPROC_BASS,
				reinterpret_cast<void*>(this->m_mixerHandle)
			)) {
				qDebug() << "WASAPI init";

				return BASS_WASAPI_Start();
			}
			else {
				QMessageBox::warning(nullptr,
					"fluxplayer",
					QString("WASAPI Initialization Error: Code %1\nFalling back to DirectSound driver!").arg(BASS_ErrorGetCode())
				);

				BASS_ChannelFlags(this->m_mixerHandle, 0, BASS_STREAM_DECODE);
				this->m_audioDriver = FXAudioDriver::DSOUND;
			}
		}

		// Init BASS ASIO
		else if (this->m_audioDriver == FXAudioDriver::ASIO) {
			if (BASS_ASIO_Init(this->m_audioDeviceIndex, BASS_ASIO_THREAD)) {

				qDebug() << "ASIO OK!";
				BASS_ASIO_ChannelEnableBASS(FALSE, 0, this->m_mixerHandle, TRUE);

				BASS_ASIO_SetRate(this->m_audioSampleRate); // set the device rate

				BASS_ASIO_ChannelSetFormat(FALSE, 0, BASS_ASIO_FORMAT_FLOAT);
				BASS_ASIO_ChannelSetRate(FALSE, 0, this->m_audioSampleRate);

				BASS_ChannelSetAttribute(this->m_mixerHandle, BASS_ATTRIB_MIXER_LATENCY, 0.05);

				return BASS_ASIO_Start(512, 0);
			}
			else {

				QMessageBox::warning(nullptr,
					"fluxplayer",
					QString("ASIO Initialization Error: Code %1\nFalling back to DirectSound driver!").arg(BASS_ASIO_ErrorGetCode())
				);

				BASS_ChannelFlags(this->m_mixerHandle, 0, BASS_STREAM_DECODE);
				this->m_audioDriver = FXAudioDriver::DSOUND;

			}
		}
			
		return BASS_ChannelPlay(this->m_mixerHandle, FALSE);
	}

	return false;
}

void FluxPlayer::initPlayers()
{
	qDebug() << "Initializing players...";

	for (unsigned short int i = 0; i < this->PLAYER_COUNT;  i++) {
		unsigned short int index = this->createPlayer(this->PLAYER_NAME_MAP[i]);
		FXPlayer *player = this->m_players->value(index);

		//temp
		QObject::connect(player, SIGNAL(bassSyncPosNext()), &this->m_playlistModel, SLOT(handleFadeNext()));

		QObject::connect(player, SIGNAL(ejected(FXPlayer*)), &this->m_playlistModel, SLOT(handlePlayerEjected(FXPlayer*)));

		QObject::connect(player, SIGNAL(trackPlayed(FXPlayer*)), ui.playerControlWidget, SLOT(handlePlayerPlayed(FXPlayer*)));
		QObject::connect(player, SIGNAL(trackStopped(FXPlayer*)), ui.playerControlWidget, SLOT(handlePlayerStopped(FXPlayer*)));
	}

	this->m_playlistModel.setPlayerContainer(this->m_players);
}

void FluxPlayer::initPlaylist()
{
	ui.playlistView->setModel(&this->m_playlistModel);
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

void FluxPlayer::handleLevelTick()
{
	float levels[2];
	float period = this->METER_UPDATE_PERIOD_MS / 1000.0;
	DWORD flags = NULL;

	if (this->m_audioDriver == FXAudioDriver::WASAPI) {
		BASS_WASAPI_GetLevelEx(levels, period, flags);
	}
	else if (this->m_audioDriver == FXAudioDriver::ASIO) {
		levels[0] = BASS_ASIO_ChannelGetLevel(FALSE, 0);
		levels[1] = BASS_ASIO_ChannelGetLevel(FALSE, 1);
	}
	else {
		BASS_ChannelGetLevelEx(this->m_mixerHandle, levels, period, flags);
	}

	double out;
	ebur128_loudness_momentary(this->m_r128State, &out);

	ui.levelMeter->setLevels(levels);
	ui.levelMeter->setShortTermLevelDecibel(out);


	ui.levelMeter->render();
}

void FluxPlayer::handlePlaylistAddFiles()
{
	QFileDialog dialog(this, "Add media file to playlist...");
	dialog.setFileMode(QFileDialog::ExistingFiles);

	QString audioExtensions = "*.";
	audioExtensions += AUDIO_EXTS.join(" *.");

	dialog.setNameFilter(
		QString("Audio Files (%1);;").arg(audioExtensions) +
		"All Files (*)"
	);

	if (dialog.exec()) {
		QStringList selected = dialog.selectedFiles();
		foreach (QString filepath, selected) {
			this->m_playlistModel.addTrack(filepath);
		}
		this->m_playlistModel.loadPlayers();
	}
	
}

void FluxPlayer::handlePlaylistAddFolder()
{
	QFileDialog dialog(this, "Add media folder to playlist...");
	dialog.setFileMode(QFileDialog::Directory);

	if (dialog.exec()) {
		QDir directory(dialog.selectedFiles().first());

		QStringList extFilter;
		foreach (QString ext, AUDIO_EXTS)
			extFilter << QString("*.%1").arg(ext);

		QStringList files = directory.entryList(extFilter, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);
		foreach (QString filename, files) {
			this->m_playlistModel.addTrack(directory.filePath(filename));
		}
		this->m_playlistModel.loadPlayers();
	}

}

void FluxPlayer::handleVolumeRatioChanged(double volume)
{
	if (this->m_audioDriver == FXAudioDriver::WASAPI) {
		BASS_WASAPI_SetVolume(BASS_WASAPI_VOL_SESSION, volume);
	}
	else if (this->m_audioDriver == FXAudioDriver::ASIO) {
		BASS_ASIO_ChannelSetVolume(FALSE, -1, volume);
	} 
	else {
		BASS_SetVolume(volume);
	}
}