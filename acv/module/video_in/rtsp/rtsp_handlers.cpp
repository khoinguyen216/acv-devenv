#include "rtsp_handlers.h"

#include <QDebug>

#include <liveMedia.hh>

#include "DecoderSink.h"
#include "MPRTSPClient.h"


void after_describe(RTSPClient* client, int result, char* resultstr)
{
	auto cl		= static_cast<MPRTSPClient *>(client);
	auto& env	= cl->envir();

	do {
		auto& env = client->envir();
		if (result != 0) {
			delete[] resultstr;
			break;
		}

		// Create a media session from the SDP
		auto const sdp = resultstr;
		cl->session_ = MediaSession::createNew(env, sdp);
		delete[] resultstr;
		if (cl->session_ == nullptr || !cl->session_->hasSubsessions())
			break;
		cl->iter_ = new MediaSubsessionIterator(*cl->session_);
		setup_next_subsession(cl);
		return;
	} while (0);

	// Shutdown the stream
	shutdown(cl);
}

void after_setup(RTSPClient* client, int result, char* resultstr)
{
	auto cl		= static_cast<MPRTSPClient *>(client);
	auto& env	= cl->envir();
	auto& subs	= cl->subsession_;

	do {
		if (result != 0) break;
		if (strcmp(subs->mediumName(), "video") == 0) {
			subs->sink = DecoderSink::createNew(env, *subs,
												cl->parent_, cl->url());
		}
		if (subs->sink == 0) break;
		subs->miscPtr = cl;
		subs->sink->startPlaying(*subs->readSource(), subsession_after_playing,
								 subs);
		// Set up a handler for RTCP BYE
		if (subs->rtcpInstance() != 0) {
			subs->rtcpInstance()->setByeHandler(subsession_bye_handler, subs);
		}
	} while (0);
	delete[] resultstr;

	setup_next_subsession(cl);
}

void after_play(RTSPClient* client, int result, char* resultstr)
{
	auto cl		= static_cast<MPRTSPClient *>(client);
	auto& env	= cl->envir();
	bool ok		= false;

	do {
		if (result != 0) {
			// Could not start playing the stream
			break;
		}
		ok = true;
	} while (0);
	delete[] resultstr;
}

void subsession_after_playing(void* data)
{
	MediaSubsession* subs = static_cast<MediaSubsession *>(data);
	MPRTSPClient* cl = static_cast<MPRTSPClient *>(subs->miscPtr);

	Medium::close(subs->sink);
	subs->sink = nullptr;

	// Check whether all other subsessions have already been closed
	MediaSession& sess = subs->parentSession();
	MediaSubsessionIterator iter(sess);
	while ((subs = iter.next()) != nullptr) {
		if (subs->sink != nullptr)
			return;
	}

	shutdown(cl);
}

void subsession_bye_handler(void* data)
{
	subsession_after_playing(data);
}

void stream_timer_handler(void* data)
{

}

void setup_next_subsession(RTSPClient* client)
{
	auto cl = static_cast<MPRTSPClient *>(client);
	auto& env = cl->envir();
	auto& sess = cl->session_;
	auto& subs = cl->subsession_;

	subs = cl->iter_->next();
	if (subs != 0) {
		if (subs->initiate()) {
			cl->sendSetupCommand(*subs, after_setup, False, True);
		} else {
			// Give up and move on to the next subsession
			setup_next_subsession(cl);
		}
	}

	// We have finished setting up all subsessions
	// Now send START to the server
	if (sess->absStartTime() != 0) {
		// The stream is indexed by absolute time
		cl->sendPlayCommand(*sess, after_play,
							sess->absStartTime(), sess->absEndTime());
	} else {
		cl->duration_ = sess->playEndTime() - sess->playStartTime();
		cl->sendPlayCommand(*sess, after_play);
	}
}

void shutdown(RTSPClient* client, int exit_code)
{
	auto cl = static_cast<MPRTSPClient *>(client);

	// Close all open sinks
	bool has_active_subsessions = false;
	if (cl->session_ != nullptr) {
		MediaSubsessionIterator iter(*cl->session_);
		MediaSubsession* subs;
		while ((subs = iter.next()) != nullptr) {
			if (subs->sink != nullptr) {
				Medium::close(subs->sink);
				subs->sink = nullptr;
				// Disable BYE handler
				if (subs->rtcpInstance() != nullptr) {
					subs->rtcpInstance()->setByeHandler(nullptr, nullptr);
				}
				has_active_subsessions = true;
			}
		}

		if (has_active_subsessions) {
			 cl->sendTeardownCommand(*cl->session_, nullptr);;
		}
	}
}
