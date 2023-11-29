/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "voxedit-util/SceneManager.h"
#include "core/ArrayLength.h"

#include "ui/IMGUIApp.h"

namespace voxedit {
class MainWindow;
}

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	voxedit::MainWindow* _mainWindow = nullptr;
	core::DynamicArray<io::FormatDescription> _paletteFormats;

	core::String getSuggestedFilename(const char *extension = nullptr) const;

	// 0 is the default binding
	enum KeyBindings {
		Magicavoxel = 0,
		Blender = 1,
		Vengi = 2,
		Qubicle = 3,
		Max
	};

	void loadKeymap(int keymap) override;
protected:
	void printUsageHeader() const override;

public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	void onRenderUI() override;

	void onDropFile(const core::String& file) override;

	bool allowedToQuit() override;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;

	void toggleScene();
};
