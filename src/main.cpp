#include "webrtc/webrtc.hpp"
#include <iostream>
#include <alsa/asoundlib.h>
#include <signal.h>

using namespace std;

#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_SIZE 160 // 10ms at 16kHz

volatile sig_atomic_t running = 1;

void signal_handler(int signum) {
    running = 0;
}

int main() {
    // 设置信号处理
    signal(SIGINT, signal_handler);

    // 创建并初始化VAD
    void* vad = WebRtcVad_Create();
    if (vad == nullptr) {
        cerr << "Failed to create VAD instance" << endl;
        return EXIT_FAILURE;
    }

    if (WebRtcVad_Init(vad) != 0) {
        cerr << "Failed to initialize VAD" << endl;
        WebRtcVad_Free(vad);
        return EXIT_FAILURE;
    }

    if (WebRtcVad_set_mode(vad, 1) != 0) {
        cerr << "Failed to set VAD mode" << endl;
        WebRtcVad_Free(vad);
        return EXIT_FAILURE;
    }

    // 打开ALSA录音设备
    snd_pcm_t* pcm_handle;
    snd_pcm_hw_params_t* params;
    int rc;
    unsigned int actual_rate = SAMPLE_RATE;
    int dir;
    snd_pcm_uframes_t frames = FRAME_SIZE;

    rc = snd_pcm_open(&pcm_handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        cerr << "Unable to open PCM device: " << snd_strerror(rc) << endl;
        WebRtcVad_Free(vad);
        return EXIT_FAILURE;
    }

    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);
    snd_pcm_hw_params_set_access(pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(pcm_handle, params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(pcm_handle, params, CHANNELS);
    snd_pcm_hw_params_set_rate_near(pcm_handle, params, &actual_rate, &dir);
    snd_pcm_hw_params_set_period_size_near(pcm_handle, params, &frames, &dir);

    rc = snd_pcm_hw_params(pcm_handle, params);
    if (rc < 0) {
        cerr << "Unable to set HW parameters: " << snd_strerror(rc) << endl;
        snd_pcm_close(pcm_handle);
        WebRtcVad_Free(vad);
        return EXIT_FAILURE;
    }

    if (actual_rate != SAMPLE_RATE) {
        cerr << "Warning: Actual sample rate (" << actual_rate << ") differs from requested rate (" << SAMPLE_RATE << ")" << endl;
    }

    int16_t buffer[FRAME_SIZE];
    while (running) {
        rc = snd_pcm_readi(pcm_handle, buffer, FRAME_SIZE);
        if (rc == -EPIPE) {
            cerr << "Overrun occurred" << endl;
            snd_pcm_prepare(pcm_handle);
            continue;
        } else if (rc < 0) {
            cerr << "Error from read: " << snd_strerror(rc) << endl;
            break;
        } else if (rc != static_cast<int>(FRAME_SIZE)) {
            cerr << "Short read, read " << rc << " frames" << endl;
            continue;
        }

        int ret = WebRtcVad_Process(vad, actual_rate, buffer, FRAME_SIZE);
        if (ret == -1) {
            cerr << "VAD processing failed" << endl;
            break;
        }
        cout << (ret == 1 ? "Speech detected" : "No speech detected") << endl;
    }

    cout << "Cleaning up..." << endl;
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
    WebRtcVad_Free(vad);

    return EXIT_SUCCESS;
}