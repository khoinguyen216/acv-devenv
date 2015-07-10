#ifndef ACV_SCRIPT_IF_H
#define ACV_SCRIPT_IF_H

#include <QByteArray>
#include <QObject>


class QTcpServer;
class QTcpSocket;
struct lua_State;

namespace acv {

class script_if : public QObject {
	Q_OBJECT

	long const MAX_LINE_LENGTH = 512;
	long const MAX_CHUNK_LENGTH = 65536;
	char const *USER_PROMPT = "acv > ";
	char const *USER_PROMPT2 = ">> ";

public:
	script_if(QObject *parent = NULL);
	~script_if();

	void start_server(unsigned short port);

private slots:
	void on_newConnection();
	void on_readyRead();
	void on_disconnected();
	void on_inputLine();

private:
	// Lua helper functions
	static int msghandler(lua_State *L);
	bool prompt(int firstline);
	int incomplete(int status);
	int addreturn();
	int docall(int narg, int nres);
	bool load_chunk();
	void push_line(bool firstline);
	int report(int status);
	void stackdump();
	void print_result();

private:
	lua_State *L_ = NULL;
	bool incomplete_ = false;
	char *chunkbuf_ = NULL;
	long chunksz_;
	QTcpServer *tcpserver_ = NULL;
	QTcpSocket *client_ = NULL;
};

}

#endif
