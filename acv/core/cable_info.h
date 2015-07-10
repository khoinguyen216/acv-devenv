#ifndef ACV_CABLE_INFO_H
#define ACV_CABLE_INFO_H

#include <QString>


namespace acv{
class cable_info {
public:
	cable_info() {}
	cable_info(QString const& mod_a, QString const& socket_a,
			   QString const& mod_b, QString const& socket_b)
			: mod_a_(mod_a), socket_a_(socket_a),
			  mod_b_(mod_b), socket_b_(socket_b)
	{
	}
	cable_info& operator=(cable_info const& b)
	{
		mod_a_	= b.mod_a();
		socket_a_	= b.socket_a();
		mod_b_	= b.mod_b();
		socket_b_	= b.socket_b();
		return *this;
	}

	QString const& mod_a() const { return mod_a_; }
	QString const& socket_a() const { return socket_a_; }
	QString const& mod_b() const { return mod_b_; }
	QString const& socket_b() const { return socket_b_; }

private:
	QString mod_a_;
	QString socket_a_;
	QString mod_b_;
	QString socket_b_;
};

bool make_cable_from_strings(QString const& end0, QString const& end1,
							 cable_info& c);
}

#endif //ACV_CABLE_INFO_H
