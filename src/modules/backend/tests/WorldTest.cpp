/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "backend/world/World.h"
#include "backend/world/MapProvider.h"
#include "network/ProtocolHandlerRegistry.h"
#include "backend/network/ServerNetwork.h"
#include "backend/network/ServerMessageSender.h"
#include "cooldown/CooldownProvider.h"
#include "attrib/ContainerProvider.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/entity/ai/AILoader.h"
#include "backend/entity/EntityStorage.h"
#include "voxel/MaterialColor.h"
#include "voxelformat/VolumeCache.h"
#include "persistence/tests/Mocks.h"
#include "io/Filesystem.h"

namespace backend {

class WorldTest: public app::AbstractTest {
public:
	EntityStoragePtr _entityStorage;
	network::ProtocolHandlerRegistryPtr _protocolHandlerRegistry;
	network::ServerNetworkPtr _network;
	network::ServerMessageSenderPtr _messageSender;
	AILoaderPtr _loader;
	attrib::ContainerProviderPtr _containerProvider;
	cooldown::CooldownProviderPtr _cooldownProvider;
	AIRegistryPtr _aiRegistry;
	MapProviderPtr _mapProvider;
	persistence::PersistenceMgrPtr _persistenceMgr;
	voxelformat::VolumeCachePtr _volumeCache;
	http::HttpServerPtr _httpServer;

	void SetUp() override {
		app::AbstractTest::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		voxel::initDefaultPalette();
		_entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		_protocolHandlerRegistry = core::make_shared<network::ProtocolHandlerRegistry>();
		_network = std::make_shared<network::ServerNetwork>(_protocolHandlerRegistry, _testApp->eventBus(), _testApp->metric());
		_messageSender = std::make_shared<network::ServerMessageSender>(_network, _testApp->metric());
		_aiRegistry = std::make_shared<LUAAIRegistry>();
		_loader = std::make_shared<AILoader>(_aiRegistry);
		_containerProvider = core::make_shared<attrib::ContainerProvider>();
		const core::String& attributes = _testApp->filesystem()->load("test-attributes.lua");
		_cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		_persistenceMgr = persistence::createPersistenceMgrMock();
		_volumeCache = std::make_shared<voxelformat::VolumeCache>();
		_httpServer = std::make_shared<http::HttpServer>(_testApp->metric());
		persistence::DBHandlerPtr dbHandler = persistence::createDbHandlerMock();
		core::Factory<backend::DBChunkPersister> chunkPersisterFactory;
		_mapProvider = std::make_shared<MapProvider>(_testApp->filesystem(), _testApp->eventBus(), _testApp->timeProvider(),
				_entityStorage, _messageSender, _loader, _containerProvider, _cooldownProvider,
				_persistenceMgr, _volumeCache, _httpServer, chunkPersisterFactory, dbHandler);
		ASSERT_TRUE(_entityStorage->init());
		ASSERT_TRUE(_containerProvider->init(attributes)) << _containerProvider->error();
	}

	void TearDown() override {
		_entityStorage->shutdown();
		_protocolHandlerRegistry->shutdown();
		_network->shutdown();
		_loader->shutdown();
		_volumeCache->shutdown();
		_mapProvider->shutdown();

		_entityStorage.reset();
		_protocolHandlerRegistry.release();
		_network.reset();
		_messageSender.reset();
		_loader.reset();
		_containerProvider.release();
		_cooldownProvider.reset();
		_volumeCache.reset();
		_persistenceMgr.reset();
		_mapProvider.reset();

		app::AbstractTest::TearDown();
	}
};

#define create(name) \
	World name(_mapProvider, _aiRegistry, _testApp->eventBus(), _testApp->filesystem(), _testApp->metric());

TEST_F(WorldTest, testInitShutdown) {
	create(world);
	ASSERT_TRUE(world.init());
	world.shutdown();
}

TEST_F(WorldTest, testUpdate) {
	create(world);
	ASSERT_TRUE(world.init());
	world.update(0ul);
	world.shutdown();
}

#undef create

}
