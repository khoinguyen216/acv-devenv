#ifndef MPRTSPCLIENT_H
#define MPRTSPCLIENT_H

#include <liveMedia.hh>
#include <UsageEnvironment.hh>


class MPRTSPClient : public RTSPClient {
public:
	static MPRTSPClient* createNew(UsageEnvironment& env,
									char const*	url,
									void*		parent,
									int			verbose = 0,
									char const*	appname = 0,
									portNumBits	httptunnelport = 0);
protected:
	MPRTSPClient(UsageEnvironment& env,
				char const*	url,
				void*		parent,
				int			verbose,
				char const*	appname,
				portNumBits	httptunnelport);
	~MPRTSPClient();

public:
	void*						parent_		= 0;
	double						duration_	= 0;
	MediaSession*				session_	= 0;
	MediaSubsession*			subsession_	= 0;
	MediaSubsessionIterator*	iter_		= 0;
};

#endif
