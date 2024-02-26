/**
 * @file
 */

#pragma once

#include "ui/Panel.h"
#include "core/String.h"

namespace voxedit {

class LSystemPanel : public ui::Panel {
private:
	struct LSystemData {
		core::String axiom = "F";
		core::String rulesStr = R"({
	F
	(67)F+[!+F-F-F(37)L]-[!-F+F+F(142)L]>[!F<F<F(128)L]<[!<F>F>F(123)L]
})";
		float angle = 22.5f;
		float length = 12.0f;
		float width = 4.0f;
		float widthIncrement = 1.5f;
		int iterations = 2;
		float leavesRadius = 8.0f;
	};
	LSystemData _lsystemData;

public:
	PANEL_CLASS(LSystemPanel)
	bool init();
	void update(const char *title);
	void shutdown();
};

}
