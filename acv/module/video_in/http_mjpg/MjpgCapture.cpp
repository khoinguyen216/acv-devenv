#include "MjpgCapture.h"

using namespace std;
using namespace cv;

#ifdef _WIN32
#include <ws2tcpip.h>
#include <windows.h>
#endif

void PRINTERR(std::string msg)
{
#ifdef vca_nix
	fprintf(stderr,"%s",("\x1B[1;31m(1;31)"+msg+"\x1b[0m\n").c_str());
#endif


#ifdef vca_win

	HANDLE hConsoleErr;
	hConsoleErr = GetStdHandle(STD_ERROR_HANDLE);
	SetConsoleTextAttribute(hConsoleErr, FOREGROUND_RED|FOREGROUND_INTENSITY);

	fprintf(stderr,(msg+"\n").c_str());
	SetConsoleTextAttribute(hConsoleErr, FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_GREEN);
#endif
}


std::vector<std::string> &zssplit(const std::string &s, char delim,
								  std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while(getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}
std::vector<std::string> zssplit(const std::string &s, char delim) {
	std::vector<std::string> elems;
	return zssplit(s, delim, elems);
}
static const char kBase64Chars[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
static const char kBase64CharsReversed[128] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 62, 0, 0, 52,
	53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5,
	6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
	25, 0, 0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37,
	38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 0, 0, 0, 0, 0
};

std::string Base64Encode(const std::string &orig)
{
	int size = orig.size();
	int encoded_size = (size * 4 + 2) / 3;
	std::string encoded(encoded_size, '\x00');
	const unsigned char *src = (unsigned char *)orig.data();
	std::string::iterator dest = encoded.begin();
	while (size > 0) {
		*dest++ = kBase64Chars[(src[0] >> 2)];
		*dest++ = kBase64Chars[((src[0] << 4) | (--size ? (src[1] >> 4) : 0)) & 63];
		if (size) {
			*dest++ = kBase64Chars[((src[1] << 2) | (--size ? (src[2] >> 6) : 0)) & 63];
			if (size) {
				*dest++ = kBase64Chars[src[2] & 63];
				--size;
			}
		}
		src += 3;
	}
	//padding
	if(encoded.size()%4!=0)
	{
		switch(encoded.size()%4)
		{
		case 3:encoded+="=";
		case 2:encoded+="=";
		case 1:encoded+="=";
		}
	}
	return encoded;
}
MjpgCapture::MjpgCapture(std::string _url)
{
	url=_url;
	skip=true;
	imgready=false;
	ff=false;
	readbytes=-2;
	isInited=false;
	i=0;
}

void MjpgCapture::init()
{

	skip=true;
	imgready=false;
	ff=false;
	readbytes=-2;
	isInited=false;
	i=0;

	memset( ca,0, _httploop_BUFSIZE);
	int frameno=0;
	cv::Mat frame;
	string _url=url;
	//string url = "http://localhost:8080/frame.mjpg";
	//cout<<"using "+url<<endl;
	_url.replace(_url.find("http://"), 7, "");
	std::string pass="";
	std::vector<std::string> upsplit= zssplit(_url,'@');
	if(upsplit.size()>2)
	{
		throw std::string("malformed url");
	}

	if(upsplit.size()==2)
	{
		pass = upsplit[0];
		_url = upsplit[1];
	}
	std::vector<std::string> splitted = zssplit(_url,'/');
	std::string host=splitted[0];

	std::vector<std::string> hostplusport = zssplit(host,':');
	_url.replace(_url.find(splitted[0]), splitted[0].size(), "");
	std::string path = _url;
	std::string hoststr = hostplusport[0];
	std::string portstr ="80";
	if(hostplusport.size()>1) portstr = hostplusport[1];

	sockfd=INVALID_SOCKET;
	struct addrinfo hints, *servinfo, *p;
	int err;

#ifdef vca_win
	WSADATA data;
	WSAStartup(MAKEWORD(2,0), &data);
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if((err = getaddrinfo(hoststr.c_str(), portstr.c_str(), &hints, &servinfo)) != 0)
	{
#ifdef vca_win
		WSACleanup();
#endif
		throw std::string("error " + tostring(err));
	}

	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
			p->ai_protocol)) == -1) {
				printf("net: socket");
				//PRINTERR("net: socket");
				continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			closesocket(sockfd);
			printf("net: connect");
			//PRINTERR("net: connect");
			continue;
		}
		break;
	}

	freeaddrinfo(servinfo);
	std::string request = "GET " + path + " HTTP/1.1\r\n";

	if(!pass.empty())
	{
		request += "Authorization: Basic " + Base64Encode(pass) +"\r\n";
	}
	request+="\r\n";
	if (p == NULL) {
#ifdef vca_win
		WSACleanup();
#endif
		throw std::string("net: failed to connect");
	}



#ifdef vca_win
	int32_t timeout = 3000;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));
#else
	struct timeval tv;
	tv.tv_sec  = 3;
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif


#ifdef vca_win
	int bytecnt = send(sockfd,request.c_str(),request.size(),0);
#endif
#ifdef vca_nix
	int bytecnt = write(sockfd,request.c_str(),request.size());
#endif
	if(bytecnt==-1)
	{
#ifdef vca_win
		WSACleanup();
#endif
		throw std::string("net: socket send error");
	}
	isInited=true;
}

MjpgCapture& MjpgCapture::operator >>(Mat& out)
{
	if(!isInited)
		init();

	if(!isInited)
	{
		out=Mat();
	}else
	{
		while(1)
		{

			uchar c;
			c = 0;
			if (readbytes!=0 && readbytes!=-1)
			{
				if(i>=readbytes)
				{
#ifdef vca_win
					readbytes=recv(sockfd,ca,_httploop_BUFSIZE,0);
#endif
#ifdef vca_nix
					readbytes=::read(sockfd,ca,_httploop_BUFSIZE);
#endif
					i=0;
				}

				if (readbytes==0)
				{
					int dfdfdf=0;
					dfdfdf+=1;
				}

				for(;i<readbytes;i++)
				{
					c=ca[i];
					if(ff && c==0xd8)
					{
						skip=false;
						data.push_back((uchar)0xff);
					}
					if(ff && c==0xd9)
					{
						imgready=true;
						data.push_back((uchar)0xd9);
						skip=true;
					}
					ff=c==0xff;
					if(!skip)
					{
						data.push_back(c);
					}
					if(imgready)
					{
						if(data.size()!=0)
						{
							cv::Mat data_mat(data);

							cv::Mat im(imdecode(data_mat,1));
							out=im;
						}else
						{
							printf("warning");
						}
						imgready=false;
						skip=true;
						data.clear();
						return *this;
					}
				}
			}
			else
			{
#ifdef vca_win
				WSACleanup();
#endif
				out=Mat();
				isInited=false;
				printf("net: recv error\n");
				return *this;
				//throw std::string("net: recv error");
			}

		}

	}


}

MjpgCapture::~MjpgCapture()
{


#ifdef vca_win
	WSACleanup();
#endif
}