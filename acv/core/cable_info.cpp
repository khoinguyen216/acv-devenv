#include "cable_info.h"

#include <QStringList>


namespace acv {
bool make_cable_from_strings(QString const& end0, QString const& end1,
							 cable_info& c) {
	auto parts0 = end0.split(".");
	auto parts1 = end1.split(".");
	if (parts0.count() != 2 || parts1.count() != 2) {
		return false;
	} else {
		c = cable_info(parts0[0], parts0[1], parts1[0], parts1[1]);
		return true;
	}
}
}