/**
 * @file
 */
#include "TestTraze.h"
#include "core/SharedPtr.h"
#include "command/Command.h"
#include "testcore/TestAppMain.h"
#include "voxel/MaterialColor.h"
#include "voxel/Region.h"
#include "voxel/RawVolumeWrapper.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace {
const int PlayFieldVolume = 0;
const int FloorVolume = 1;
const int FontSize = 48;
}

TestTraze::TestTraze(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider), _protocol(eventBus), _voxelFontRender(FontSize, 4, voxel::VoxelFont::OriginUpperLeft), _soundMgr(filesystem) {
	init(ORGANISATION, "testtraze");
	setRenderAxis(false);
	setRelativeMouseMode(false);
	_allowRelativeMouseMode = false;
	_eventBus->subscribe<traze::NewGridEvent>(*this);
	_eventBus->subscribe<traze::NewGamesEvent>(*this);
	_eventBus->subscribe<traze::PlayerListEvent>(*this);
	_eventBus->subscribe<traze::TickerEvent>(*this);
	_eventBus->subscribe<traze::SpawnEvent>(*this);
	_eventBus->subscribe<traze::BikeEvent>(*this);
	_eventBus->subscribe<traze::ScoreEvent>(*this);
}

const core::String& TestTraze::playerName(traze::PlayerId playerId) const {
	return player(playerId).name;
}

const traze::Player& TestTraze::player(traze::PlayerId playerId) const {
	for (auto& p : _players) {
		if (p.id == playerId) {
			return p;
		}
	}
	static traze::Player player;
	return player;
}

app::AppState TestTraze::onConstruct() {
	app::AppState state = Super::onConstruct();
	_framesPerSecondsCap->setVal(60.0f);
	core::Var::get("mosquitto_host", "traze.iteratec.de");
	core::Var::get("mosquitto_port", "1883");
	_name = core::Var::get("name", "noname_testtraze");
	command::Command::registerCommand("join", [&] (const command::CmdArgs& args) { _protocol.join(_name->strVal()); });
	command::Command::registerCommand("bail", [&] (const command::CmdArgs& args) { _protocol.bail(); });
	command::Command::registerCommand("left", [&] (const command::CmdArgs& args) { _protocol.steer(traze::BikeDirection::W); });
	command::Command::registerCommand("right", [&] (const command::CmdArgs& args) { _protocol.steer(traze::BikeDirection::E); });
	command::Command::registerCommand("forward", [&] (const command::CmdArgs& args) { _protocol.steer(traze::BikeDirection::N); });
	command::Command::registerCommand("backward", [&] (const command::CmdArgs& args) { _protocol.steer(traze::BikeDirection::S); });
	command::Command::registerCommand("players", [&] (const command::CmdArgs& args) {
		for (const auto& p : _players) {
			Log::info("%s", p.name.c_str());
		}
	});
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
	_rawVolumeRenderer.construct();
	_messageQueue.construct();
	_soundMgr.construct();
	return state;
}

void TestTraze::sound(const char *soundId) {
	if (_soundMgr.play(-1, soundId, glm::ivec3(0), false) < 0) {
		Log::warn("Failed to play sound %s", soundId);
	}
}

app::AppState TestTraze::onInit() {
	app::AppState state = Super::onInit();
	if (state != app::AppState::Running) {
		return state;
	}
	if (!voxel::initDefaultMaterialColors()) {
		Log::error("Failed to initialize the palette data");
		return app::AppState::InitFailure;
	}
	if (!_protocol.init()) {
		Log::error("Failed to init protocol");
		return app::AppState::InitFailure;
	}
	if (!_rawVolumeRenderer.init()) {
		Log::error("Failed to initialize the raw volume renderer");
		return app::AppState::InitFailure;
	}
	if (!_messageQueue.init()) {
		Log::error("Failed to init message queue");
		return app::AppState::InitFailure;
	}
	if (!_voxelFontRender.init()) {
		Log::error("Failed to init voxel font");
		return app::AppState::InitFailure;
	}
	if (!_soundMgr.init()) {
		Log::error("Failed to init sound manager");
		return app::AppState::InitFailure;
	}

	camera().setWorldPosition(glm::vec3(0.0f, 50.0f, 84.0f));
	_logLevelVar->setVal(core::string::toString(SDL_LOG_PRIORITY_INFO));
	Log::init();

	_textCamera.setMode(video::CameraMode::Orthogonal);
	_textCamera.setNearPlane(-10.0f);
	_textCamera.setFarPlane(10.0f);
	_textCamera.init(glm::ivec2(0), frameBufferDimension(), windowDimension());
	_textCamera.update(0.0);

	_voxelFontRender.setViewProjectionMatrix(_textCamera.viewProjectionMatrix());

	return state;
}

void TestTraze::onEvent(const traze::NewGamesEvent& event) {
	_games = event.get();
	Log::debug("Got %i games", (int)_games.size());
	// there are some points were we assume a limited amount of games...
	if (_games.size() >= UCHAR_MAX) {
		Log::warn("Too many games found - reducing them");
		_games.resize(UCHAR_MAX - 1);
	}
	// TODO: this doesn't work if the instanceName changed (new game added, old game removed...)
	if (_games.empty() || _currentGameIndex > (int8_t)_games.size()) {
		_protocol.unsubscribe();
		_currentGameIndex = -1;
	} else if (!_games.empty() && _currentGameIndex == -1) {
		Log::info("Select first game");
		_currentGameIndex = 0;
	}
}

void TestTraze::onEvent(const traze::BikeEvent& event) {
	const traze::Bike& bike = event.get();
	Log::debug("Received bike event for player %u (%i:%i)",
			bike.playerId, bike.currentLocation.x, bike.currentLocation.y);
}

void TestTraze::onEvent(const traze::TickerEvent& event) {
	const traze::Ticker& ticker = event.get();
	const core::String& fraggerName = playerName(ticker.fragger);
	const core::String& casualtyName = playerName(ticker.casualty);
	switch (ticker.type) {
	case traze::TickerType::Frag:
		if (ticker.fragger == _protocol.playerId()) {
			sound("you_win");
			_messageQueue.message("You fragged %s", casualtyName.c_str());
		} else if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
			_messageQueue.message("You were fragged by %s", fraggerName.c_str());
		} else {
			_messageQueue.message("%s fragged %s", fraggerName.c_str(), casualtyName.c_str());
		}
		break;
	case traze::TickerType::Suicide:
		if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
		} else {
			sound("suicide");
		}
		_messageQueue.message("%s committed suicide", fraggerName.c_str());
		break;
	case traze::TickerType::Collision:
		if (ticker.casualty == (int)_protocol.playerId()) {
			sound("you_lose");
		} else if (ticker.fragger == _protocol.playerId()) {
			sound("you_win");
		} else {
			sound("collision");
		}
		_messageQueue.message("%s - collision with another player", fraggerName.c_str());
		break;
	default:
		break;
	}
}

void TestTraze::onEvent(const traze::ScoreEvent& event) {
	const traze::Score& score = event.get();
	Log::debug("Received score event with %i entries", (int)score.size());
}

void TestTraze::onEvent(const traze::SpawnEvent& event) {
	const traze::Spawn& spawn = event.get();
	Log::debug("Spawn at position %i:%i", spawn.position.x, spawn.position.y);
	if (spawn.own) {
		_spawnPosition = spawn.position;
		_spawnTime = _nowSeconds;
		sound("join");
	}
}

void TestTraze::onEvent(const traze::NewGridEvent& event) {
	core::SharedPtr<voxel::RawVolume> v = event.get();
	if (_spawnTime > 0.0 && _nowSeconds - _spawnTime < 4.0) {
		const voxel::Voxel voxel = voxel::createRandomColorVoxel(voxel::VoxelType::Generic);
		v->setVoxel(glm::ivec3(_spawnPosition.y, 0, _spawnPosition.x), voxel);
		v->setVoxel(glm::ivec3(_spawnPosition.y, 1, _spawnPosition.x), voxel);
	}
	voxel::RawVolume* volume = _rawVolumeRenderer.volume(PlayFieldVolume);
	voxel::Region dirtyRegion;
	if (volume == nullptr || volume->region() != v->region()) {
		volume = new voxel::RawVolume(v.get());
		delete _rawVolumeRenderer.setVolume(PlayFieldVolume, volume);
		dirtyRegion = v->region();

		voxel::RawVolume* floor = new voxel::RawVolume(dirtyRegion);
		const voxel::Region& region = floor->region();
		const voxel::Voxel voxel = voxel::createColorVoxel(voxel::VoxelType::Dirt, 0);
		// ground
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
				floor->setVoxel(x, region.getLowerY(), z, voxel);
			}
		}
		// walls
		for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			floor->setVoxel(region.getLowerX(), region.getLowerY() + 1, z, voxel);
			floor->setVoxel(region.getUpperX(), region.getLowerY() + 1, z, voxel);
			floor->setVoxel(region.getLowerX(), region.getLowerY() + 2, z, voxel);
			floor->setVoxel(region.getUpperX(), region.getLowerY() + 2, z, voxel);
		}
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			floor->setVoxel(x, region.getLowerY() + 1, region.getLowerZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 1, region.getUpperZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 2, region.getLowerZ(), voxel);
			floor->setVoxel(x, region.getLowerY() + 2, region.getUpperZ(), voxel);
		}
		delete _rawVolumeRenderer.setVolume(FloorVolume, floor);
		if (!_rawVolumeRenderer.extractRegion(FloorVolume, region)) {
			Log::error("Failed to extract the volume");
		}
	} else {
		voxel::RawVolumeWrapper wrapper(volume);
		const voxel::Region& region = volume->region();
		const glm::ivec3& mins = region.getLowerCorner();
		const glm::ivec3& maxs = region.getUpperCorner();
		glm::ivec3 p;
		for (int x = mins.x; x <= maxs.x; ++x) {
			p.x = x;
			for (int y = mins.y; y <= maxs.y; ++y) {
				p.y = y;
				for (int z = mins.z; z <= maxs.z; ++z) {
					p.z = z;
					wrapper.setVoxel(p, v->voxel(p));
				}
			}
		}
		dirtyRegion = wrapper.dirtyRegion();
	}
	const glm::mat4& translate = glm::translate(-volume->region().getCenter());
	const glm::mat4& rotateY = glm::rotate(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	const glm::mat4& rotateX = glm::rotate(glm::radians(25.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::mat4 model = rotateX * rotateY * translate;
	_rawVolumeRenderer.setModelMatrix(PlayFieldVolume, model);
	_rawVolumeRenderer.setModelMatrix(FloorVolume, model);
	if (!_rawVolumeRenderer.extractRegion(PlayFieldVolume, dirtyRegion)) {
		Log::error("Failed to extract the volume");
	}
}

void TestTraze::onEvent(const traze::PlayerListEvent& event) {
	_players = event.get();
	_maxLength = 200;
	for (const traze::Player& p : _players) {
		_maxLength = core_max(_maxLength, _voxelFontRender.stringWidth(p.name.c_str(), p.name.size()) + 60);
	}
}

app::AppState TestTraze::onRunning() {
	_rawVolumeRenderer.update();
	app::AppState state = Super::onRunning();
	if (!_protocol.connected()) {
		if (_nextConnectTime <= 0.0) {
			_nextConnectTime = 3.0;
			_protocol.connect();
		} else {
			_nextConnectTime -= deltaFrameSeconds();
		}
	} else if (_currentGameIndex != -1) {
		_protocol.subscribe(_games[_currentGameIndex]);
	}
	_messageQueue.update(_deltaFrameSeconds);
	_soundMgr.setListenerPosition(camera().worldPosition());
	_soundMgr.update();
	return state;
}

app::AppState TestTraze::onCleanup() {
	_voxelFontRender.shutdown();
	_soundMgr.shutdown();
	const core::DynamicArray<voxel::RawVolume*>& old = _rawVolumeRenderer.shutdown();
	for (voxel::RawVolume* v : old) {
		delete v;
	}
	_protocol.shutdown();
	_messageQueue.shutdown();
	return Super::onCleanup();
}

void TestTraze::onRenderUI() {
	if (ImGui::BeginCombo("GameInfo", _currentGameIndex == -1 ? "" : _games[_currentGameIndex].name.c_str(), 0)) {
		for (size_t i = 0u; i < (size_t)_games.size(); ++i) {
			const traze::GameInfo& game = _games[i];
			const bool selected = _currentGameIndex == (int)i;
			if (ImGui::Selectable(game.name.c_str(), selected)) {
				_currentGameIndex = i;
			}
			if (selected) {
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}
	ImGui::InputVarString("Name", _name);
	if (!_protocol.joined() && ImGui::Button("Join")) {
		_protocol.join(_name->strVal());
	}
	if (_protocol.joined() && ImGui::Button("Leave")) {
		_protocol.bail();
	}
	ImGui::Checkbox("Render board", &_renderBoard);
	ImGui::Checkbox("Render player names", &_renderPlayerNames);
	Super::onRenderUI();
}

void TestTraze::doRender() {
	if (_renderBoard) {
		_rawVolumeRenderer.render(camera());
	}

	const glm::ivec2& dim = frameBufferDimension();
	_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(dim.x / 3, 0.0f, 0.0f)));
	int messageOffset = 0;
	_messageQueue.visitMessages([&] (int64_t /*remainingMillis*/, const core::String& msg) {
		_voxelFontRender.text(glm::ivec3(0.0f, (float)messageOffset, 0.0f), core::Color::White, "%s", msg.c_str());
		messageOffset += _voxelFontRender.lineHeight();
	});
	_voxelFontRender.swapBuffers();
	_voxelFontRender.render();

	if (!_protocol.connected()) {
		const char* connecting = "Connecting";
		const int w = _voxelFontRender.stringWidth(connecting, SDL_strlen(connecting));
		_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(dim.x / 2 - w / 2, dim.y / 2 - _voxelFontRender.lineHeight() / 2, 0.0f)));
		const glm::ivec3 pos(0, 0, 0);
		_voxelFontRender.text(pos, core::Color::Red, "%s", connecting);
		_voxelFontRender.text(glm::ivec3(pos.x, pos.y + _voxelFontRender.lineHeight(), pos.z), core::Color::Red, ".");
	} else if (_renderPlayerNames) {
		_voxelFontRender.setModelMatrix(glm::translate(glm::vec3(20.0f, 20.0f, 0.0f)));
		int yOffset = 0;
		_voxelFontRender.text(glm::ivec3(0, yOffset, 0), core::Color::White, "%i Players", (int)_players.size());
		yOffset += _voxelFontRender.lineHeight();
		for (const traze::Player& p : _players) {
			_voxelFontRender.text(glm::ivec3(0, yOffset, 0), p.color, "* %s", p.name.c_str());
			_voxelFontRender.text(glm::ivec3(_maxLength, yOffset, 0), p.color, "%i/%i", p.frags, p.owned);
			yOffset += _voxelFontRender.lineHeight();
		}
	}

	_voxelFontRender.swapBuffers();
	_voxelFontRender.render();
}

TEST_APP(TestTraze)
