#include "app.h"

#include <cassert>


namespace acv {

bool app::add_module(QString const& id, QString const& modtype) {
	assert(module_factory_ != NULL);

	module* m = module_factory_->create_instance(modtype);
	if (m == NULL)
		return false;
	if (!module_graph_.add_module(id, m)) {
		delete m;
		return false;
	}

	return true;
}

void app::remove_module(QString const &id) {
}

void app::setopt(QString const& id, QString const& o, QString const& v) {

}

bool app::add_cable(QString const &e0, QString const &e1) {
	cable_info c;
	if (!make_cable_from_strings(e0, e1, c))
		return false;

	return module_graph_.add_cable(c);
}
}