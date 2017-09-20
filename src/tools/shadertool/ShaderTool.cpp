/**
 * @file
 */

#include "ShaderTool.h"
#include "io/Filesystem.h"
#include "core/Process.h"
#include "core/String.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"
#include "video/Shader.h"
#include "Generator.h"

// TODO: validate that each $out of the vertex shader has a $in in the fragment shader and vice versa
ShaderTool::ShaderTool(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(filesystem, eventBus, timeProvider, 0) {
	init(ORGANISATION, "shadertool");
}

bool ShaderTool::parseLayout(Layout& layout) {
	if (!_tok.hasNext()) {
		return false;
	}

	std::string token = _tok.next();
	if (token != "(") {
		Log::warn("Unexpected layout syntax - expected {, got %s", token.c_str());
		return false;
	}
	do {
		if (!_tok.hasNext()) {
			return false;
		}
		token = _tok.next();
		Log::trace("token: %s", token.c_str());
		if (token == "std140") {
			layout.blockLayout = BlockLayout::std140;
		} else if (token == "std430") {
			layout.blockLayout = BlockLayout::std430;
		} else if (token == "location") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.location = core::string::toInt(_tok.next());
		} else if (token == "offset") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.offset = core::string::toInt(_tok.next());
		} else if (token == "compontents") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.components = core::string::toInt(_tok.next());
		} else if (token == "index") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.index = core::string::toInt(_tok.next());
		} else if (token == "binding") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.binding = core::string::toInt(_tok.next());
		} else if (token == "xfb_buffer") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackBuffer = core::string::toInt(_tok.next());
		} else if (token == "xfb_offset") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.transformFeedbackOffset = core::string::toInt(_tok.next());
		} else if (token == "vertices") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.tesselationVertices = core::string::toInt(_tok.next());
		} else if (token == "max_vertices") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.maxGeometryVertices = core::string::toInt(_tok.next());
		} else if (token == "origin_upper_left") {
			layout.originUpperLeft = true;
		} else if (token == "pixel_center_integer") {
			layout.pixelCenterInteger = true;
		} else if (token == "early_fragment_tests") {
			layout.earlyFragmentTests = true;
		} else if (token == "primitive_type") {
			core_assert_always(_tok.hasNext() && _tok.next() == "=");
			if (!_tok.hasNext()) {
				return false;
			}
			layout.primitiveType = util::layoutPrimitiveType(_tok.next());
		}
	} while (token != ")");

	return true;
}

bool ShaderTool::parse(const std::string& buffer, bool vertex) {
	bool uniformBlock = false;

	simplecpp::DUI dui;
	simplecpp::OutputList outputList;
	std::vector<std::string> files;
	std::stringstream f(buffer);
	simplecpp::TokenList rawtokens(f, files, _shaderfile, &outputList);
	std::map<std::string, simplecpp::TokenList*> included = simplecpp::load(rawtokens, files, dui, &outputList);
	simplecpp::TokenList output(files);
	simplecpp::preprocess(output, rawtokens, files, included, dui, &outputList);

	simplecpp::Location loc(files);
	std::stringstream comment;
	UniformBlock block;

	_tok.init(&output);

	while (_tok.hasNext()) {
		const std::string token = _tok.next();
		Log::trace("token: %s", token.c_str());
		std::vector<Variable>* v = nullptr;
		if (token == "$in") {
			if (vertex) {
				v = &_shaderStruct.attributes;
			} else {
				// TODO: use this to validate that each $out of the vertex shader has a $in in the fragment shader
				//v = &_shaderStruct.varyings;
			}
		} else if (token == "$out") {
			if (vertex) {
				v = &_shaderStruct.varyings;
			} else {
				v = &_shaderStruct.outs;
			}
		} else if (token == "layout") {
			// there can be multiple layouts per definition since GL 4.2 (or ARB_shading_language_420pack)
			// that's why we only reset the layout after we finished parsing the variable and/or the
			// uniform buffer. The last defined value for the mutually-exclusive qualifiers or for numeric
			// qualifiers prevails.
			if (!parseLayout(block.layout)) {
				Log::warn("Could not parse layout");
			}
		} else if (token == "buffer") {
			Log::warn("SSBO not supported");
		} else if (token == "uniform") {
			v = &_shaderStruct.uniforms;
		} else if (uniformBlock) {
			if (token == "}") {
				uniformBlock = false;
				Log::trace("End of uniform block: %s", block.name.c_str());
				_shaderStruct.uniformBlocks.push_back(block);
				core_assert_always(_tok.next() == ";");
			} else {
				_tok.prev();
			}
		}

		if (v == nullptr && !uniformBlock) {
			continue;
		}

		if (!_tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get type");
			return false;
		}
		std::string type = _tok.next();
		Log::trace("token: %s", type.c_str());
		if (!_tok.hasNext()) {
			Log::error("Failed to parse the shader, could not get variable name for type %s", type.c_str());
			return false;
		}
		while (type == "highp" || type == "mediump" || type == "lowp" || type == "precision") {
			Log::trace("token: %s", type.c_str());
			if (!_tok.hasNext()) {
				Log::error("Failed to parse the shader, could not get type");
				return false;
			}
			type = _tok.next();
		}
		std::string name = _tok.next();
		Log::trace("token: %s", name.c_str());
		// uniform block
		if (name == "{") {
			block.name = type;
			block.members.clear();
			block.layout = Layout();
			Log::trace("Found uniform block: %s", type.c_str());
			uniformBlock = true;
			continue;
		}
		const Variable::Type typeEnum = util::getType(type, _tok.line());
		const bool isArray = _tok.peekNext() == "[";
		int arraySize = 0;
		if (isArray) {
			_tok.next();
			const std::string& number = _tok.next();
			core_assert_always(_tok.next() == "]");
			core_assert_always(_tok.next() == ";");
			arraySize = core::string::toInt(number);
			if (arraySize == 0) {
				arraySize = -1;
				Log::warn("Could not determine array size for %s (%s)", name.c_str(), number.c_str());
			}
		}
		// TODO: multi dimensional arrays are only supported in glsl >= 5.50
		if (uniformBlock) {
			block.members.push_back(Variable{typeEnum, name, arraySize});
		} else {
			auto findIter = std::find_if(v->begin(), v->end(), [&] (const Variable& var) {return var.name == name;});
			if (findIter == v->end()) {
				v->push_back(Variable{typeEnum, name, arraySize});
			} else {
				Log::warn("Found duplicate variable %s (%s versus %s)",
						name.c_str(), util::resolveTypes(findIter->type).ctype, util::resolveTypes(typeEnum).ctype);
			}
		}
	}
	if (uniformBlock) {
		Log::error("Parsing error - still inside a uniform block");
		return false;
	}
	return true;
}

core::AppState ShaderTool::onConstruct() {
	registerArg("--glslang").setShort("-g").setDescription("Path to glslangvalidator binary").setMandatory();
	registerArg("--shader").setShort("-s").setDescription("The base name of the shader to create the c++ bindings for").setMandatory();
	registerArg("--shadertemplate").setShort("-t").setDescription("The shader template file").setMandatory();
	registerArg("--buffertemplate").setShort("-b").setDescription("The uniform buffer template file").setMandatory();
	registerArg("--namespace").setShort("-n").setDescription("Namespace to generate the source in").setDefaultValue("shader");
	registerArg("--shaderdir").setShort("-d").setDescription("Directory to load the shader from").setDefaultValue("shaders/");
	registerArg("--sourcedir").setDescription("Directory to generate the source in").setMandatory();
	Log::trace("Set some shader config vars to let the validation work");
	core::Var::get(cfg::ClientGamma, "2.2", core::CV_SHADER);
	core::Var::get(cfg::ClientShadowMap, "true", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadow, "false", core::CV_SHADER);
	core::Var::get(cfg::ClientDebugShadowMapCascade, "false", core::CV_SHADER);
	return Super::onConstruct();
}

core::AppState ShaderTool::onRunning() {
	const std::string glslangValidatorBin = getArgVal("--glslang");
	const std::string shaderfile          = getArgVal("--shader");
	_shaderTemplateFile                   = getArgVal("--shadertemplate");
	_uniformBufferTemplateFile            = getArgVal("--buffertemplate");
	_namespaceSrc                         = getArgVal("--namespace");
	_shaderDirectory                      = getArgVal("--shaderdir");
	_sourceDirectory                      = getArgVal("--sourcedir",
			_filesystem->basePath() + "src/modules/" + _namespaceSrc + "/");

	if (!core::string::endsWith(_shaderDirectory, "/")) {
		_shaderDirectory = _shaderDirectory + "/";
	}

	Log::debug("Using glslangvalidator binary: %s", glslangValidatorBin.c_str());
	Log::debug("Using %s as output directory", _sourceDirectory.c_str());
	Log::debug("Using %s as namespace", _namespaceSrc.c_str());
	Log::debug("Using %s as shader directory", _shaderDirectory.c_str());

	Log::debug("Preparing shader file %s", shaderfile.c_str());
	_shaderfile = std::string(core::string::extractFilename(shaderfile.c_str()));
	Log::debug("Preparing shader file %s", _shaderfile.c_str());
	const std::string fragmentFilename = _shaderfile + FRAGMENT_POSTFIX;
	const io::FilesystemPtr& fs = filesystem();
	const bool changedDir = fs->pushDir(std::string(core::string::extractPath(shaderfile.c_str())));
	const std::string fragmentBuffer = fs->load(fragmentFilename);
	if (fragmentBuffer.empty()) {
		Log::error("Could not load %s", fragmentFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string vertexFilename = _shaderfile + VERTEX_POSTFIX;
	const std::string vertexBuffer = fs->load(vertexFilename);
	if (vertexBuffer.empty()) {
		Log::error("Could not load %s", vertexFilename.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string geometryFilename = _shaderfile + GEOMETRY_POSTFIX;
	const std::string geometryBuffer = fs->load(geometryFilename);

	const std::string computeFilename = _shaderfile + COMPUTE_POSTFIX;
	const std::string computeBuffer = fs->load(computeFilename);

	video::Shader shader;

	const std::string& fragmentSrcSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, false);
	const std::string& vertexSrcSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, false);

	_shaderStruct.filename = _shaderfile;
	_shaderStruct.name = _shaderfile;
	parse(fragmentSrcSource, false);
	if (!geometryBuffer.empty()) {
		const std::string& geometrySrcSource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, false);
		parse(geometrySrcSource, false);
	}
	if (!computeBuffer.empty()) {
		const std::string& computeSrcSource = shader.getSource(video::ShaderType::Compute, computeBuffer, false);
		parse(computeSrcSource, false);
	}
	parse(vertexSrcSource, true);

	for (const auto& block : _shaderStruct.uniformBlocks) {
		Log::debug("Found uniform block %s with %i members", block.name.c_str(), int(block.members.size()));
	}
	for (const auto& v : _shaderStruct.uniforms) {
		Log::debug("Found uniform of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.attributes) {
		Log::debug("Found attribute of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.varyings) {
		Log::debug("Found varying of type %i with name %s", int(v.type), v.name.c_str());
	}
	for (const auto& v : _shaderStruct.outs) {
		Log::debug("Found out var of type %i with name %s", int(v.type), v.name.c_str());
	}

	const std::string& templateShader = fs->load(_shaderTemplateFile);
	const std::string& templateUniformBuffer = fs->load(_uniformBufferTemplateFile);
	if (!shadertool::generateSrc(templateShader, templateUniformBuffer, _shaderStruct, filesystem(), _namespaceSrc, _sourceDirectory, _shaderDirectory)) {
		Log::error("Failed to generate shader source for %s", _shaderfile.c_str());
		_exitCode = 1;
		return core::AppState::Cleanup;
	}

	const std::string& fragmentSource = shader.getSource(video::ShaderType::Fragment, fragmentBuffer, true);
	const std::string& vertexSource = shader.getSource(video::ShaderType::Vertex, vertexBuffer, true);
	const std::string& geometrySource = shader.getSource(video::ShaderType::Geometry, geometryBuffer, true);
	const std::string& computeSource = shader.getSource(video::ShaderType::Compute, computeBuffer, true);

	if (changedDir) {
		fs->popDir();
	}

	const std::string& writePath = fs->homePath();
	Log::debug("Writing shader file %s to %s", _shaderfile.c_str(), writePath.c_str());
	std::string finalFragmentFilename = _appname + "-" + fragmentFilename;
	std::string finalVertexFilename = _appname + "-" + vertexFilename;
	std::string finalGeometryFilename = _appname + "-" + geometryFilename;
	std::string finalComputeFilename = _appname + "-" + computeFilename;
	fs->write(finalFragmentFilename, fragmentSource);
	fs->write(finalVertexFilename, vertexSource);
	if (!geometrySource.empty()) {
		fs->write(finalGeometryFilename, geometrySource);
	}
	if (!computeSource.empty()) {
		fs->write(finalComputeFilename, computeSource);
	}

	Log::debug("Validating shader file %s", _shaderfile.c_str());

	std::vector<std::string> fragmentArgs;
	fragmentArgs.push_back(writePath + finalFragmentFilename);
	int fragmentValidationExitCode = core::Process::exec(glslangValidatorBin, fragmentArgs);

	std::vector<std::string> vertexArgs;
	vertexArgs.push_back(writePath + finalVertexFilename);
	int vertexValidationExitCode = core::Process::exec(glslangValidatorBin, vertexArgs);

	int geometryValidationExitCode = 0;
	if (!geometrySource.empty()) {
		std::vector<std::string> geometryArgs;
		geometryArgs.push_back(writePath + finalGeometryFilename);
		geometryValidationExitCode = core::Process::exec(glslangValidatorBin, geometryArgs);
	}
	int computeValidationExitCode = 0;
	if (!computeSource.empty()) {
		std::vector<std::string> computeArgs;
		computeArgs.push_back(writePath + finalComputeFilename);
		computeValidationExitCode = core::Process::exec(glslangValidatorBin, computeArgs);
	}

	if (fragmentValidationExitCode != 0) {
		Log::error("Failed to validate fragment shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), writePath.c_str(), finalFragmentFilename.c_str());
		_exitCode = fragmentValidationExitCode;
	} else if (vertexValidationExitCode != 0) {
		Log::error("Failed to validate vertex shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), writePath.c_str(), finalVertexFilename.c_str());
		_exitCode = vertexValidationExitCode;
	} else if (geometryValidationExitCode != 0) {
		Log::error("Failed to validate geometry shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), writePath.c_str(), finalGeometryFilename.c_str());
		_exitCode = geometryValidationExitCode;
	} else if (computeValidationExitCode != 0) {
		Log::error("Failed to validate compute shader");
		Log::warn("%s %s%s", glslangValidatorBin.c_str(), writePath.c_str(), finalComputeFilename.c_str());
		_exitCode = computeValidationExitCode;
	}

	return core::AppState::Cleanup;
}

int main(int argc, char *argv[]) {
	const core::EventBusPtr eventBus = std::make_shared<core::EventBus>();
	const io::FilesystemPtr filesystem = std::make_shared<io::Filesystem>();
	const core::TimeProviderPtr timeProvider = std::make_shared<core::TimeProvider>();
	ShaderTool app(filesystem, eventBus, timeProvider);
	return app.startMainLoop(argc, argv);
}
