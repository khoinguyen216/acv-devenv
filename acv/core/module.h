#ifndef ACV_MODULE_H
#define ACV_MODULE_H

#include <QHash>
#include <QObject>
#include <QString>


namespace acv {

struct module_socket {
	bool dir_out;
	QHash<QString, QString> wires;
};

typedef QHash<QString, module_socket> module_sockets;

enum opt_type {
	OPT_STRING,
	OPT_BOOLEAN,
	OPT_DOUBLE,
	OPT_INT
};

struct module_option {
	QString desc;
	opt_type type;

	static double to_double(QString const& s, double defval) {
		bool ok;
		double v = s.toDouble(&ok);
		return (ok ? v : defval);
	}
	static double to_int(QString const& s, int defval) {
		bool ok;
		int v = s.toInt(&ok);
		return (ok ? v : defval);
	}
	static bool to_bool(QString const& s, bool defval) {
		if (s == "yes")
			return true;
		else if (s == "no")
			return false;
		else
			return defval;
	}
};

typedef QHash<QString, module_option> module_options;

class module : public QObject {
public:
	module() {}
	virtual ~module() {}

	virtual module_socket const* expose_socket(QString const& s) const;
	virtual module_options const& options() const;

public slots:
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void setopt(QString const& o, QString const& v) = 0;

// Disable copy constructor and assignment operator
private:
	module(module const&);
	module& operator=(module const&);

protected:
	module_options* optionlist_;
	module_sockets* socketlist_;
};

template<typename T> module* create_module() { return new T; }

}


#endif //ACV_MODULE_H
