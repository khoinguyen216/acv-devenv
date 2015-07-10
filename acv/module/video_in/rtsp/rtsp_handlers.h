#ifndef RTSP_HANDLERS_H
#define RTSP_HANDLERS_H


class RTSPClient;

// RTSP response handlers
void after_describe(RTSPClient* client, int result, char* resultstr);
void after_setup(RTSPClient* client, int result, char* resultstr);
void after_play(RTSPClient* client, int result, char* resultstr);

// Other events
void subsession_after_playing(void* data);
void subsession_bye_handler(void* data);
void stream_timer_handler(void* data);
// Called at the end of a stream's expected duration if it has not already
// signaled its end using RTCP BYE

// Processing functions
void setup_next_subsession(RTSPClient* client);
void shutdown(RTSPClient* client, int exit_code = 1);



#endif
