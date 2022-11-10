/**
 * @file
 */

#include "ComputeShaderTool.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include "io/Filesystem.h"
#include "core/TimeProvider.h"
#include "compute/Shader.h"
#include "Generator.h"
#include "Parser.h"
#include "Util.h"
#include "util/IncludeUtil.h"
#include "core/String.h"
#include <simplecpp.h>

ComputeShaderTool::ComputeShaderTool(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, timeProvider) {
	init(ORGANISATION, "computeshadertool");
	_initialLogLevel = SDL_LOG_PRIORITY_WARN;
}

ComputeShaderTool::~ComputeShaderTool() {
}

bool ComputeShaderTool::parse(const core::String& buffer) {
	return computeshadertool::parse(buffer, _computeFilename, _kernels, _structs, _constants);
}

app::AppState ComputeShaderTool::onConstruct() {
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("compute");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	registerArg("-I").setDescription("Add additional include dir");
	return Super::onConstruct();
}

core::Pair<core::String, bool> ComputeShaderTool::getSource(const core::String& file) const {
	const io::FilesystemPtr& fs = filesystem();

	const core::Pair<core::String, bool>& retIncludes = util::handleIncludes(file, fs->load(file), _includeDirs);
	core::String src = retIncludes.first;
	int level = 0;
	bool success = retIncludes.second;
	while (core::string::contains(src, "#include")) {
		const core::Pair<core::String, bool>& ret = util::handleIncludes(file, src, _includeDirs);
		src = ret.first;
		success &= ret.second;
		++level;
		if (level >= 10) {
			Log::warn("Abort shader include loop for %s", file.c_str());
			break;
		}
	}
	return {src, success};
}

app::AppState ComputeShaderTool::onRunning() {
	const core::String shaderfile = getArgVal("--shader");
	if (shaderfile.empty()) {
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	_shaderTemplateFile = getArgVal("--shadertemplate");
	if (_shaderTemplateFile.empty()) {
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	_namespaceSrc = getArgVal("--namespace");
	if (_namespaceSrc.empty()) {
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	_shaderDirectory = getArgVal("--shaderdir");
	if (_shaderDirectory.empty()) {
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	_sourceDirectory = getArgVal("--sourcedir", core::string::path(_filesystem->basePath(), "src", "modules", _namespaceSrc));
	_postfix = getArgVal("--postfix", "");

	// handle include dirs
	_includeDirs.insert(".");
	int index = 0;
	for (;;) {
		const core::String& dir = getArgVal("-I", "", &index);
		if (dir.empty()) {
			break;
		}
		_includeDirs.insert(dir);
	}

	_shaderDirectory = core::string::sanitizeDirPath(_shaderDirectory);
	_sourceDirectory = core::string::sanitizeDirPath(_sourceDirectory);
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_computeFilename = shaderfile + COMPUTE_POSTFIX;
	const bool changedDir = filesystem()->pushDir(core::string::extractPath(shaderfile));
	const core::Pair<core::String, bool>& computeBuffer = getSource(_computeFilename);
	if (computeBuffer.first.empty() || !computeBuffer.second) {
		Log::error("Could not load %s", _computeFilename.c_str());
		_exitCode = 127;
		return app::AppState::Cleanup;
	}

	compute::Shader shader;
	const core::String& computeSrcSource = shader.getSource(computeBuffer.first, false);

	_name = core::string::extractFilename(shaderfile.c_str());
	if (!parse(computeSrcSource)) {
		_exitCode = 1;
		return app::AppState::Cleanup;
	}
	const core::String& templateShader = filesystem()->load(_shaderTemplateFile);
	if (!computeshadertool::generateSrc(filesystem(), templateShader, _name, _namespaceSrc, _shaderDirectory, _sourceDirectory, _kernels, _structs, _constants, _postfix, computeBuffer.first)) {
		_exitCode = 100;
		return app::AppState::Cleanup;
	}

	const core::String& computeSource = shader.getSource(computeBuffer.first, true);

	if (changedDir) {
		filesystem()->popDir();
	}

	Log::debug("Writing shader file %s to %s", shaderfile.c_str(), filesystem()->homePath().c_str());
	core::String finalComputeFilename = _appname + "-" + _computeFilename;
	filesystem()->write(finalComputeFilename, computeSource);

	return app::AppState::Cleanup;
}

CONSOLE_APP(ComputeShaderTool)
