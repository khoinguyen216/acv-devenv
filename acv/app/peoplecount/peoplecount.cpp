#include "peoplecount.h"

#include <cassert>
#include <cstdlib>

#include <QDebug>
#include <QTextStream>

#include "core/script_if.h"


namespace acv {

peoplecount::peoplecount(int& argc, char** argv) : app(argc, argv) {
	setObjectName("peoplecount");

	setOrganizationName("KAI Square Pte Ltd");
	setOrganizationDomain("kaisquare.com");
	setApplicationName("peoplecount");
	setApplicationVersion(PC_VERSION);

	parser_ = new QCommandLineParser();
	module_factory_ = new module_factory();
}

peoplecount::~peoplecount() {
}

void peoplecount::preprocess_cmdargs() {
	// Default options
	auto const& help_opt = parser_->addHelpOption();
	auto const& ver_opt = parser_->addVersionOption();

	configure_cmdparser();

	if (!parser_->parse(arguments())) {
		QTextStream(stderr) << parser_->errorText();
		exit(EXIT_FAILURE);
	}

	if (parser_->isSet(help_opt)) {
		parser_->showHelp(EXIT_SUCCESS);
		Q_UNREACHABLE();
	}
	if (parser_->isSet(ver_opt)) {
		parser_->showVersion();
		Q_UNREACHABLE();
	}
	if (parser_->isSet("listmodules")) {
		QTextStream(stdout) << "Available modules\n";
		for (auto const& m : module_factory_->list()) {
			QTextStream(stdout) << "\t" << m << "\n";
		}
		std::exit(EXIT_SUCCESS);
	}
	if (parser_->isSet("scriptport")) {
		bool ok;
		int port = parser_->value("scriptport").toInt(&ok);
		if (ok && (port > 1024 && port < 65535)) {
			script_ = new script_if(this);
			script_->start_server(port);
		} else {
			QTextStream(stdout) << "Invalid script port\n";
			std::exit(EXIT_FAILURE);
		}
	}
}

void peoplecount::postprocess_cmdargs() {
	module* vi = module_graph_.get_module("vi");
	module* vo = module_graph_.get_module("vo");
	module* logic = module_graph_.get_module("logic");

	if (parser_->isSet("i")) {
		QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
								  Q_ARG(QString, "source"),
								  Q_ARG(QString, parser_->value("i")));
	} else {
		parser_->showHelp(EXIT_FAILURE);
		Q_UNREACHABLE();
	}

	if (parser_->isSet("o")) {
		QString outvid = QString("%1/out.avi").arg(parser_->value("o"));
		QMetaObject::invokeMethod(vo, "setopt", Qt::AutoConnection,
								  Q_ARG(QString, "dest"),
								  Q_ARG(QString, outvid));
	}

	if (parser_->isSet("cntio")) {
		QMetaObject::invokeMethod(logic, "setopt", Qt::AutoConnection,
								  Q_ARG(QString, "cntio"),
								  Q_ARG(QString, parser_->value("cntio")));
	} else {
		parser_->showHelp(EXIT_FAILURE);
		Q_UNREACHABLE();
	}

	if (parser_->isSet("ss")) {

	}

	delete parser_;
}

void peoplecount::load_config() {

}

void peoplecount::setup() {
	add_module("vi", "video_in");
	add_module("vo", "video_out");
	add_module("logic", "pc_logic");

	add_cable("vi.status", "logic.vi_status");
	add_cable("vi.vout", "logic.vin");
	add_cable("logic.vi_control", "vi.control");
	add_cable("logic.viz", "vo.vin");

	module* vi = module_graph_.get_module("vi");
	module* vo = module_graph_.get_module("vo");
	module* logic = module_graph_.get_module("logic");

	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "recover"),
			Q_ARG(QString, "yes"));
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "recover_interval"),
			Q_ARG(QString, "1"));
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "read_timeout"),
			Q_ARG(QString, "5"));

	QMetaObject::invokeMethod(vo, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "size"),
			Q_ARG(QString, "320x240"));

	QMetaObject::invokeMethod(logic, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "mdsize"),
			Q_ARG(QString, "160x120"));
}

void peoplecount::start_main_modules() {
	module* vi = module_graph_.get_module("vi");
	module* vo = module_graph_.get_module("vo");
	module* logic = module_graph_.get_module("logic");

	QMetaObject::invokeMethod(vi, "start", Qt::AutoConnection);
	QMetaObject::invokeMethod(vo, "start", Qt::AutoConnection);
	QMetaObject::invokeMethod(logic, "start", Qt::AutoConnection);
}

void peoplecount::configure_cmdparser() {
	parser_->setApplicationDescription("KAI Square ACV");
	parser_->setSingleDashWordOptionMode(
			QCommandLineParser::ParseAsLongOptions);

	parser_->addOptions({
		{"app", "Application to run"},
		{"listmodules", "List all available modules"},
		{"scriptport", "Start scripting interface over TCP", "port"},
		{"i", "Input video source (file | http | rtsp)", "url"},
		{"o", "Output directory", "dirpath"},
		{"cntio", "Cntio parameter", "param"},
		{"ss", "Save segmentation"},
		{"recover", "Recover from video source error"}
	});
}

}