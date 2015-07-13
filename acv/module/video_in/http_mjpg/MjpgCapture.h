#ifndef MJPGCAPTURE_H_
#define MJPGCAPTURE_H_
#define WIN32_LEAN_AND_MEAN
#if defined(unix)        || defined(__unix)      || defined(__unix__) \
	|| defined(linux)       || defined(__linux)     || defined(__linux__) \
	|| defined(sun)         || defined(__sun) \
	|| defined(BSD)         || defined(__OpenBSD__) || defined(__NetBSD__) \
	|| defined(__FreeBSD__) || defined __DragonFly__ \
	|| defined(sgi)         || defined(__sgi) \
	|| defined(__MACOSX__)  || defined(__APPLE__) \
	|| defined(__CYGWIN__)
#define vca_nix
#endif


#if defined(_MSC_VER) || defined(WIN32)  || defined(_WIN32) || defined(__WIN32__) \
	|| defined(WIN64)    || defined(_WIN64) || defined(__WIN64__)
#define vca_win
#endif

#ifdef vca_win
#pragma comment (lib, "Ws2_32.lib")

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <cstdint>
#endif
#include "stdio.h"
#ifdef vca_nix
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET  ((SOCKET)-1)
static inline int closesocket(int fd) { return close(fd); }
#endif


#include "opencv2/opencv.hpp"



void PRINTERR(std::string msg);
std::vector<std::string> zssplit(const std::string &s, char delim);
std::string Base64Encode(const std::string &orig);

template <class T>
	inline std::string tostring (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}

class MjpgCapture
{
public:
	std::string url;
	bool isInited;
	SOCKET sockfd;
	std::vector<char> data;
	bool skip;
	bool imgready;
	bool ff;
	int i;
	long long readbytes;
#define _httploop_BUFSIZE 10240
	char ca[_httploop_BUFSIZE];
	MjpgCapture() {};
	MjpgCapture(std::string _url);
	void init();
	~MjpgCapture();
	MjpgCapture& operator >>(cv::Mat& out);
};

#endif

