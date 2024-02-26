/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "command/CommandHandler.h"
#include "core/Var.h"
#include "scenegraph/SceneGraphNode.h"

namespace video {
class Camera;
}
namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
class SceneGraphNodeCamera;
}

namespace voxedit {

struct ModelNodeSettings;

class SceneGraphPanel : public ui::Panel {
private:
	core::VarPtr _animationSpeedVar;
	core::VarPtr _hideInactive;
	bool _showNodeDetails = true;
	bool _hasFocus = false;
	bool _popupDragAndDrop = false;
	core::String _propertyKey;
	core::String _propertyValue;
	int _dragDropSourceNodeId = InvalidNodeId;
	int _dragDropTargetNodeId = InvalidNodeId;

	void registerPopups();
	void detailView(scenegraph::SceneGraphNode &node);
	void recursiveAddNodes(video::Camera &camera, const scenegraph::SceneGraph &sceneGraph,
							  scenegraph::SceneGraphNode &node, command::CommandExecutionListener &listener,
							  int depth, int referencedNodeId);
	/**
	 * @return @c true if the property was handled with a special ui input widget - @c false if it should just be a
	 * normal text input field
	 */
	bool handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key, const core::String &value);
public:
	PANEL_CLASS(SceneGraphPanel)
	bool _popupNewModelNode = false;
	bool init();
	void update(video::Camera &camera, const char *title, ModelNodeSettings *modelNodeSettings,
				command::CommandExecutionListener &listener);
	bool hasFocus() const;
};

}
