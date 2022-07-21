/**
 * @file
 */

#include "DatabaseTool.h"
#include "io/Filesystem.h"
#include "core/StringUtil.h"
#include "core/Common.h"
#include "core/Tokenizer.h"
#include "Util.h"
#include "Parser.h"
#include "Mapping.h"
#include "Generator.h"
#include "Table.h"

DatabaseTool::DatabaseTool(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider) :
		Super(metric, filesystem, eventBus, timeProvider) {
	init(ORGANISATION, "databasetool");
	_initialLogLevel = SDL_LOG_PRIORITY_WARN;
}

bool DatabaseTool::generateSrc() const {
	Log::debug("Generate database bindings for %s", _targetFile.c_str());
	core::String header;
	header.reserve(65535);
	header += "#pragma once\n\n";

	const core::String& dir = core::string::extractPath(_targetFile);
	bool error = false;
	for (const auto& i : _tables) {
		const databasetool::Table& table = i.second;
		core::String src;
		if (!databasetool::generateClassForTable(table, src)) {
			error = true;
			continue;
		}
		header += "#include \"";
		header += table.classname;
		header += ".h\"\n";
		const core::String filename = dir + table.classname + ".h";
		if (!filesystem()->syswrite(filename, src)) {
			error = true;
			continue;
		}
	}
	if (error) {
		return false;
	}
	return filesystem()->syswrite(_targetFile, header);
}

bool DatabaseTool::validateOperators(const databasetool::Table& table) const {
	for (const auto& fe : table.fields) {
		const persistence::Field& field = fe.second;
		if (field.updateOperator == persistence::Operator::SET) {
			continue;
		}
		if (databasetool::isString(field)) {
			Log::error("Table '%s': Invalid operator for string based field '%s'",
					table.name.c_str(), field.name.c_str());
			return true;
		}
	}
	return false;
}

bool DatabaseTool::validateForeignKeys(const databasetool::Table& table) const {
	bool error = false;
	const persistence::ForeignKeys& foreignKeys = table.foreignKeys;
	for (const auto& fke : foreignKeys) {
		const persistence::ForeignKey& fk = fke.second;
		const auto i = _tables.find(fk.table + "_" + table.classname);
		if (i == _tables.end()) {
			Log::debug("Table '%s': Could not find referenced table in this definition", table.name.c_str());
			continue;
		}
		const databasetool::Table& ref = i->second;
		const auto fi = table.fields.find(fke.first);
		if (fi == table.fields.end()) {
			error = true;
			Log::error("Table '%s': Specified field '%s' is not part of the table '%s'",
					table.name.c_str(), fke.first.c_str(), table.name.c_str());
			continue;
		}

		const auto ri = ref.fields.find(fk.field);
		if (ri == ref.fields.end()) {
			error = true;
			Log::error("Table '%s': Referenced field '%s' is not part of the table '%s'",
					table.name.c_str(), fk.field.c_str(), ref.name.c_str());
			continue;
		}

		if (ri->second.type != fi->second.type) {
			error = true;
			Log::error("Table '%s': Type of field '%s' doesn't match the field of the referenced field of the table '%s'",
					table.name.c_str(), fk.field.c_str(), ref.name.c_str());
			continue;
		}

		if ((ri->second.contraintMask & core::enumVal(persistence::ConstraintType::PRIMARYKEY)) == 0u) {
			if ((ri->second.contraintMask & core::enumVal(persistence::ConstraintType::NOTNULL)) == 0u) {
				error = true;
				Log::error("Table '%s': Referenced field '%s' in table '%s' isn't a primary key and can be null",
						table.name.c_str(), fk.field.c_str(), fk.table.c_str());
				continue;
			}
			Log::warn("Table '%s': Referenced field '%s' in table '%s' isn't a primary key",
					table.name.c_str(), fk.field.c_str(), fk.table.c_str());
		}
	}
	return error;
}

bool DatabaseTool::validate() const {
	bool error = false;
	for (const auto& entry : _tables) {
		const databasetool::Table& table = entry.second;
		error |= validateForeignKeys(table);
		error |= validateOperators(table);
	}
	return !error;
}

bool DatabaseTool::parse(const core::String& buffer) {
	core::Tokenizer tok(buffer.c_str(), " \t\n", "(){},;");
	while (tok.hasNext()) {
		const core::String& token = tok.next();
		if (token == "table") {
			if (!tok.hasNext()) {
				Log::error("Expected table name");
				return false;
			}
			const core::String& tablename = tok.next();
			databasetool::Table table;
			table.name = tablename;
			table.classname = core::string::upperCamelCase(tablename + "Model");
			if (!databasetool::parseTable(tok, table)) {
				return false;
			}
			_tables.insert(std::make_pair(tablename + "_" + table.classname, table));
		}
	}
	return validate();
}

app::AppState DatabaseTool::onConstruct() {
	registerArg("--tablefile").setShort("-t").setDescription("The path to the table to file").setMandatory();
	registerArg("--outfile").setShort("-o").setDescription("The file that should be generated").setMandatory();
	return Super::onConstruct();
}

app::AppState DatabaseTool::onRunning() {
	_tableFile    = getArgVal("--tablefile");
	_targetFile   = getArgVal("--outfile");

	Log::debug("Preparing table file %s", _tableFile.c_str());
	const core::String& buf = filesystem()->load(_tableFile);
	if (buf.empty()) {
		Log::error("Could not load %s", _tableFile.c_str());
		return app::AppState::InitFailure;
	}

	if (!parse(buf.c_str())) {
		return app::AppState::InitFailure;
	}
	if (!generateSrc()) {
		return app::AppState::InitFailure;
	}

	return app::AppState::Cleanup;
}

CONSOLE_APP(DatabaseTool)
