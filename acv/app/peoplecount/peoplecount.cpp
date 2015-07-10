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

	module_factory_ = new module_factory();
}

peoplecount::~peoplecount() {
}

void peoplecount::preprocess_cmdargs() {
	QCommandLineParser p;
	// Default options
	auto const& help_opt = p.addHelpOption();
	auto const& ver_opt = p.addVersionOption();

	configure_cmdparser(p);

	if (!p.parse(arguments())) {
		QTextStream(stderr) << p.errorText();
		exit(EXIT_FAILURE);
	}

	if (p.isSet(help_opt)) {
		p.showHelp(EXIT_SUCCESS);
		Q_UNREACHABLE();
	}
	if (p.isSet(ver_opt)) {
		p.showVersion();
		Q_UNREACHABLE();
	}
	if (p.isSet("listmodules")) {
		QTextStream(stdout) << "Available modules\n";
		for (auto const& m : module_factory_->list()) {
			QTextStream(stdout) << "\t" << m << "\n";
		}
		std::exit(EXIT_SUCCESS);
	}
	if (p.isSet("scriptport")) {
		bool ok;
		int port = p.value("scriptport").toInt(&ok);
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
	add_cable("vi.vout", "vo.vin");

	module* vi = module_graph_.get_module("vi");
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
		  	Q_ARG(QString, "source"),
			Q_ARG(QString, "/home/nguyen/clipboard/LevisHK_Cam6_12012014140000"
					".MP4"));
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "recover"),
			Q_ARG(QString, "yes"));
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "recover_interval"),
			Q_ARG(QString, "1"));
	QMetaObject::invokeMethod(vi, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "read_timeout"),
			Q_ARG(QString, "5"));

	module* vo = module_graph_.get_module("vo");
	QMetaObject::invokeMethod(vo, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "dest"),
			Q_ARG(QString, "/home/nguyen/test.avi"));

	module* logic = module_graph_.get_module("logic");
	QMetaObject::invokeMethod(logic, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "mdsize"),
			Q_ARG(QString, "160x120"));
	QMetaObject::invokeMethod(logic, "setopt", Qt::AutoConnection,
			Q_ARG(QString, "cntio"),
			Q_ARG(QString, "0,0,1,0.6;0,0.6,1,1;"));

	QMetaObject::invokeMethod(vo, "start", Qt::AutoConnection);
	QMetaObject::invokeMethod(logic, "start", Qt::AutoConnection);
}

void peoplecount::configure_cmdparser(QCommandLineParser& p) {
	p.setApplicationDescription("KAI Square ACV");
	p.setSingleDashWordOptionMode(
			QCommandLineParser::ParseAsLongOptions);

	p.addOptions({
		{"app", "Application to run"},
		{"listmodules", "List all available modules"},
		{"scriptport", "Start scripting interface over TCP", "port"},
		{"i", "Input video source (file | http | rtsp)", "url"},
		{"recover", "Recover from video source error"}
	});
}

}