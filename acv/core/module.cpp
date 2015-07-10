#include "module.h"


namespace acv {

module_socket const *module::expose_socket(QString const &s) const {
	if (!socketlist_->contains(s))
		return NULL;
	return &socketlist_->find(s).value();
}

module_options const &module::options() const {
	return *optionlist_;
}

}
