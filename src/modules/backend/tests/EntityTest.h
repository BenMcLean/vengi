/**
 * @file
 */

#pragma once

#include "app/tests/AbstractTest.h"
#include "backend/entity/ai/AIRegistry.h"
#include "backend/world/MapProvider.h"
#include "backend/world/Map.h"
#include "backend/spawn/SpawnMgr.h"
#include "poi/PoiProvider.h"
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
#include "http/HttpServer.h"

namespace backend {
namespace {

const char *CONTAINER = R"(function init()
local player = attrib.createContainer("PLAYER")
player:addAbsolute("FIELDOFVIEW", 360.0)
player:addAbsolute("HEALTH", 100.0)
player:addAbsolute("STRENGTH", 1.0)
player:addAbsolute("VIEWDISTANCE", 10000.0)

local rabbit = attrib.createContainer("ANIMAL_RABBIT")
rabbit:addAbsolute("FIELDOFVIEW", 360.0)
rabbit:addAbsolute("HEALTH", 100.0)
rabbit:addAbsolute("STRENGTH", 1.0)
rabbit:addAbsolute("VIEWDISTANCE", 10000.0)

local wolf = attrib.createContainer("ANIMAL_WOLF")
wolf:addAbsolute("FIELDOFVIEW", 360.0)
wolf:addAbsolute("HEALTH", 100.0)
wolf:addAbsolute("STRENGTH", 1.0)
wolf:addAbsolute("VIEWDISTANCE", 10000.0)
end)";

}

class EntityTest: public app::AbstractTest {
private:
	using Super = app::AbstractTest;
protected:
	EntityStoragePtr entityStorage;
	network::ProtocolHandlerRegistryPtr protocolHandlerRegistry;
	network::ServerNetworkPtr network;
	network::ServerMessageSenderPtr messageSender;
	AIRegistryPtr registry;
	AILoaderPtr loader;
	attrib::ContainerProviderPtr containerProvider;
	cooldown::CooldownProviderPtr cooldownProvider;
	core::EventBusPtr eventBus;
	io::FilesystemPtr filesystem;
	core::TimeProviderPtr timeProvider;
	persistence::PersistenceMgrPtr persistenceMgr;
	MapProviderPtr mapProvider;
	MapPtr map;
	voxelformat::VolumeCachePtr volumeCache;

	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::ServerSeed, "1");
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::DatabaseMinConnections, "0");
		core::Var::get(cfg::DatabaseMaxConnections, "0");
		voxel::initDefaultPalette();
		entityStorage = std::make_shared<EntityStorage>(_testApp->eventBus());
		protocolHandlerRegistry = core::make_shared<network::ProtocolHandlerRegistry>();
		network = std::make_shared<network::ServerNetwork>(protocolHandlerRegistry, _testApp->eventBus(), _testApp->metric());
		messageSender = std::make_shared<network::ServerMessageSender>(network, _testApp->metric());
		registry = std::make_shared<LUAAIRegistry>();
		loader = std::make_shared<AILoader>(registry);
		containerProvider = core::make_shared<attrib::ContainerProvider>();
		cooldownProvider = std::make_shared<cooldown::CooldownProvider>();
		filesystem = _testApp->filesystem();
		eventBus = _testApp->eventBus();
		volumeCache = std::make_shared<voxelformat::VolumeCache>();
		http::HttpServerPtr httpServer = std::make_shared<http::HttpServer>(_testApp->metric());
		timeProvider = _testApp->timeProvider();
		persistenceMgr = persistence::createPersistenceMgrMock();
		persistence::DBHandlerPtr dbHandler = persistence::createDbHandlerMock();
		// TODO: don't use the DBChunkPersister - but a mock
		core::Factory<backend::DBChunkPersister> chunkPersisterFactory;
		mapProvider = std::make_shared<MapProvider>(filesystem, eventBus, timeProvider,
				entityStorage, messageSender, loader, containerProvider, cooldownProvider,
				persistenceMgr, volumeCache, httpServer, chunkPersisterFactory, dbHandler);
		ASSERT_TRUE(entityStorage->init());
		ASSERT_TRUE(registry->init());
		ASSERT_TRUE(containerProvider->init(CONTAINER));
		ASSERT_TRUE(mapProvider->init()) << "Failed to initialize the map provider";
		map = mapProvider->map(1);
	}

	void TearDown() override {
		entityStorage->shutdown();
		mapProvider->shutdown();
		protocolHandlerRegistry->shutdown();
		network->shutdown();
		registry->shutdown();
		loader->shutdown();
		volumeCache->shutdown();

		entityStorage.reset();
		protocolHandlerRegistry.release();
		network.reset();
		messageSender.reset();
		registry.reset();
		loader.reset();
		containerProvider.release();
		cooldownProvider.reset();
		eventBus.reset();
		filesystem.reset();
		timeProvider.reset();
		persistenceMgr.reset();
		mapProvider.reset();
		map.reset();
		volumeCache.reset();

		Super::TearDown();
	}
};

}
