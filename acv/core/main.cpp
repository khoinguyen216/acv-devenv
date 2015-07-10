#include <cstdlib>
#include <cstdio>
#include <cstring>

#include "app/peoplecount/peoplecount.h"


int main(int argc, char** argv) {
	int i;
	for (i = 0; i < argc; ++i)
		if (strcmp(argv[i], "--app") == 0)
			break;

	if (i >= argc - 1) {
		fprintf(stderr, "Usage: acv --app <appname>\n");
		return EXIT_FAILURE;
	}

	char const* appname = argv[i + 1];
	acv::app* app;
	if (strcmp(appname, "peoplecount") == 0) {
		app = new acv::peoplecount(argc, argv);
	} else {
		fprintf(stderr, "Error: no application with name %s found\n", appname);
		return EXIT_FAILURE;
	}

	app->preprocess_cmdargs();
	app->load_config();
	app->setup();
	app->postprocess_cmdargs();

	int retcode = app->exec();
	delete app;
	return retcode;
}