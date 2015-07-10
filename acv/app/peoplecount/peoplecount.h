#ifndef ACV_PC_H
#define ACV_PC_H

#include "core/app.h"
#include "core/module_factory.h"

#include <QCommandLineParser>


namespace acv {

class script_if;

class peoplecount : public app {
	Q_OBJECT

	char const* PC_VERSION = "1.0.0";

public:
	peoplecount(int& argc, char** argv);
	~peoplecount();

	void preprocess_cmdargs() override;
	void postprocess_cmdargs() override;
	void load_config() override;
	void setup() override;

private:
	void configure_cmdparser(QCommandLineParser& p);

private:
	script_if*		script_;
};

}

#endif //ACV_PC_H
