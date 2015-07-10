#include "MPRTSPClient.h"

#include <Groupsock.hh>


MPRTSPClient* MPRTSPClient::createNew(UsageEnvironment& env,
						char const*	url,
						void*		parent,
						int			verbose,
						char const*	appname,
						portNumBits	httptunnelport)
{
	NetAddress a;
	return new MPRTSPClient(env, url, parent, verbose, appname, httptunnelport);
}

MPRTSPClient::MPRTSPClient(UsageEnvironment& env,
							char const*	url,
							void*		parent,
							int			verbose,
							char const*	appname,
							portNumBits	httptunnelport) :
	RTSPClient(env, url, verbose, appname, httptunnelport, -1),
	parent_(parent)
{
}

MPRTSPClient::~MPRTSPClient()
{
}
