//#pragma once
//#include <vector>
//#include <random>
//
//struct NoteEvent {
//    uint8_t pitch;      // MIDI ノート番号
//    uint32_t length;    // Tick 数
//    bool     isRest;    // 休符か
//};
//
//class MelodyGenerator {
//public:
//    MelodyGenerator(const std::vector<int>& allowedPC,  // 0‑11
//        int minOct, int maxOct,
//        const std::vector<int>& chordPC,    // 0‑11
//        uint32_t ppqn, int threshold);
//
//    std::vector<NoteEvent> generate(size_t measures);
//
//private:
//    int m_min, m_max;
//    std::vector<int> m_allowed;
//    std::vector<int> m_chordPool;
//    std::vector<uint32_t> m_lengths;
//    int  m_minOct{}, m_maxOct{}, m_th{};
//    uint32_t m_ppqn;
//    std::mt19937 m_rng{ std::random_device{}() };
//};
