//#include "WavePlayer.h"
//#include <mmsystem.h>
//#include <cmath>
//#pragma comment(lib, "winmm.lib")
//
//double WavePlayer::noteToFreq(int midi)
//{
//    return 440.0 * std::pow(2.0, (midi - 69) / 12.0);
//}
//
//void WavePlayer::synthNote(std::vector<int16_t>& pcm,
//    double f, double sec, uint32_t sr)
//{
//    const double twoPi = 6.28318530718;
//    size_t N = static_cast<size_t>(sec * sr);
//    size_t start = pcm.size();
//    pcm.resize(start + N);
//
//    for (size_t i = 0; i < N; ++i) {
//        double t = static_cast<double>(i) / sr;
//        // 0.2ms の線形フェードイン／アウトでクリック防止
//        double env = 1.0;
//        double fade = 0.0002;
//        if (t < fade)                env = t / fade;
//        else if (t > sec - fade)     env = (sec - t) / fade;
//
//        double s = std::sin(twoPi * f * t) * env;
//        pcm[start + i] = static_cast<int16_t>(s * 32767);
//    }
//}
//
//bool WavePlayer::play(const std::vector<NoteEvent>& seq,
//    uint32_t bpm, uint32_t ppqn,
//    uint32_t sr)
//{
//    const double secPerTick = 60.0 / (bpm * ppqn);
//
//    std::vector<int16_t> pcm;
//    pcm.reserve(sr * 10);          // 目安
//
//    for (auto& n : seq) {
//        double sec = n.length * secPerTick;
//        if (n.isRest)
//            pcm.resize(pcm.size() + static_cast<size_t>(sec * sr), 0);
//        else
//            synthNote(pcm, noteToFreq(n.pitch), sec, sr);
//    }
//
//    // ---- waveOut 再生 ----
//    WAVEFORMATEX fmt{};
//    fmt.wFormatTag = WAVE_FORMAT_PCM;
//    fmt.nChannels = 1;
//    fmt.nSamplesPerSec = sr;
//    fmt.wBitsPerSample = 16;
//    fmt.nBlockAlign = fmt.nChannels * fmt.wBitsPerSample / 8;
//    fmt.nAvgBytesPerSec = fmt.nSamplesPerSec * fmt.nBlockAlign;
//
//    HWAVEOUT hwo;
//    if (waveOutOpen(&hwo, WAVE_MAPPER, &fmt, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR)
//        return false;
//
//    WAVEHDR hdr{};
//    hdr.lpData = reinterpret_cast<LPSTR>(pcm.data());
//    hdr.dwBufferLength = static_cast<DWORD>(pcm.size() * sizeof(int16_t));
//
//    if (waveOutPrepareHeader(hwo, &hdr, sizeof(hdr)) != MMSYSERR_NOERROR) {
//        waveOutClose(hwo); return false;
//    }
//    waveOutWrite(hwo, &hdr, sizeof(hdr));
//    // 再生完了を待機
//    while (waveOutUnprepareHeader(hwo, &hdr, sizeof(hdr)) == WAVERR_STILLPLAYING)
//        Sleep(10);
//
//    waveOutClose(hwo);
//    return true;
//}
