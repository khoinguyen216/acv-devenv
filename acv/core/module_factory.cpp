#include "module_factory.h"

#include "module/video_in/video_in.h"
#include "module/video_out/video_out.h"
#include "module/pc_logic/pc_logic.h"


namespace acv {

module_factory::module_factory() {
	map_["video_in"] = &create_module<video_in>;
	map_["video_out"] = &create_module<video_out>;
	map_["pc_logic"] = &create_module<pc_logic>;
}

module *module_factory::create_instance(QString const&m) {
	auto it = get_map().find(m);
	if (it == get_map().end())
		return NULL;
	return it.value()();
}

QStringList module_factory::list() {
	return get_map().keys();
}

bool module_factory::register_module(QString const &m,
									 module_factory::module_factory_func f) {
	if (get_map().contains(m))
		return false;
	get_map()[m] = f;
	return true;
}

}