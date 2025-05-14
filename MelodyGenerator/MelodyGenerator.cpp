//#include "MelodyGenerator.h"
//
//MelodyGenerator::MelodyGenerator(const std::vector<int>& allowedPC,
//    int minOct, int maxOct,
//    const std::vector<int>& chordPC,
//    uint32_t ppqn, int th)
//    : m_allowed(allowedPC), m_chordPool(chordPC),
//    m_minOct(minOct), m_maxOct(maxOct),
//    m_ppqn(ppqn), m_th(th) {
//}
//
//std::vector<NoteEvent> MelodyGenerator::generate(size_t measures)
//{
//    std::uniform_int_distribution<int> pcDist(0, (int)m_allowed.size() - 1);
//    std::uniform_int_distribution<int> octDist(m_minOct, m_maxOct);
//
//    std::vector<NoteEvent> seq;
//    int prevPitch = -128;
//
//    uint32_t ticksPerBeat = m_ppqn;
//    constexpr uint32_t beats = 4;
//
//    for (size_t m = 0; m < measures; ++m) {
//        /* ---- 1小節ごとにランダムコード選択 ---- */
//        std::vector<int> chord;
//        if (!m_chordPool.empty()) {
//            std::uniform_int_distribution<int> rootDist(0, (int)m_chordPool.size() - 1);
//            int root = m_chordPool[rootDist(m_rng)];
//            chord = { root,
//                      (root + 4) % 12,   // major triad (3rd)
//                      (root + 7) % 12 }; // 5th
//        }
//
//        uint32_t filled = 0;
//        while (filled < beats * ticksPerBeat) {
//            uint32_t len = ticksPerBeat; // 四分固定(例)
//            /* --- 音決定 --- */
//            int pitch;
//            do {
//                int pc = m_allowed[pcDist(m_rng)];
//                int oc = octDist(m_rng);
//                pitch = oc * 12 + pc;
//            } while ((std::abs(pitch - prevPitch) > m_th) ||
//                (!chord.empty() &&   // コード外なら再抽選
//                    std::find(chord.begin(), chord.end(), pitch % 12) == chord.end()));
//
//            prevPitch = pitch;
//            seq.push_back({ (uint8_t)pitch, len, false });
//            filled += len;
//        }
//    }
//    return seq;
//}
