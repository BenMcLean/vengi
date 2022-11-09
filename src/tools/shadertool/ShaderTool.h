/**
 * @file
 */

#pragma once

#include "app/CommandlineApp.h"
#include "Types.h"
#include "core/collection/List.h"

/**
 * @brief This tool validates the GLSL shaders and generates c++ code for them.
 *
 * @ingroup Tools
 */
class ShaderTool: public app::CommandlineApp {
private:
	using Super = app::CommandlineApp;
protected:
	ShaderStruct _shaderStruct;
	core::String _namespaceSrc;
	core::String _sourceDirectory;
	core::String _postfix;
	core::String _shaderDirectory;
	core::String _headerTemplateFile;
	core::String _sourceTemplateFile;
	core::String _glslangValidatorBin;
	core::String _uniformBufferTemplateFile;
	core::String _constantsTemplateFile;
	core::String _shaderfile;
	core::String _shaderpath;
	core::List<core::String> _includes;
	core::List<core::String> _includeDirs;

	bool parse(const core::String& filename, const core::String& src, bool vertex);
	void validate(const core::String& name);
	std::pair<core::String, bool> getSource(const core::String& file) const;
	bool printInfo();
public:
	ShaderTool(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider);

	app::AppState onConstruct() override;
	app::AppState onRunning() override;
};
