//#include "MidiWriter.h"
//#include <fstream>
//
//void MidiWriter::writeVarLen(std::vector<uint8_t>& out, uint32_t v)
//{
//    uint8_t bytes[4];
//    int n = 0;
//    bytes[n++] = v & 0x7F;
//    while ((v >>= 7)) bytes[n++] = 0x80 | (v & 0x7F);
//    while (n--) out.push_back(bytes[n]);
//}
//
//bool MidiWriter::write(const std::string& path,
//    const std::vector<NoteEvent>& ev,
//    int bpm) const
//{
//    std::vector<uint8_t> track;
//
//    /*--- 1) テンポ ---*/
//    track.insert(track.end(), { 0x00, 0xFF, 0x51, 0x03 });
//    uint32_t usec = 60000000 / bpm;
//    track.push_back((usec >> 16) & 0xFF);
//    track.push_back((usec >> 8) & 0xFF);
//    track.push_back((usec) & 0xFF);
//
//    /*--- 2) ノート & 休符 ---*/
//    uint32_t   delta = 0;       // 休符ぶん蓄積
//    uint8_t    channel = 0;
//
//    for (const auto& n : ev)
//    {
//        if (n.isRest)
//        {
//            delta += n.length;    // 休符 → まだ何も書かずに時間だけ進める
//            continue;
//        }
//
//        /* Note On */
//        writeVarLen(track, delta);       // 溜めておいたデルタを書き出す
//        delta = 0;
//
//        track.push_back(0x90 | channel);
//        track.push_back(n.pitch);
//        track.push_back(100);            // velocity
//
//        /* Note Off */
//        writeVarLen(track, n.length);    // 音長ぶん進める
//        track.push_back(0x80 | channel);
//        track.push_back(n.pitch);
//        track.push_back(0);
//    }
//
//    /*--- 3) End of Track ---*/
//    writeVarLen(track, delta);           // 曲末の休符ぶんを反映
//    track.insert(track.end(), { 0xFF, 0x2F, 0x00 });
//
//    /*--- 4) チャンク書き出し ---*/
//
//
//
//    // ---- ヘッダ書き出し ----
//    std::ofstream ofs(path, std::ios::binary);
//    if (!ofs) return false;
//
//    auto w16 = [&](uint16_t v) { ofs.put(v >> 8); ofs.put(v); };
//    auto w32 = [&](uint32_t v) { ofs.put(v >> 24); ofs.put(v >> 16);
//    ofs.put(v >> 8);  ofs.put(v); };
//
//    ofs.write("MThd", 4); w32(6); w16(0); w16(1); w16(m_ppqn);
//    ofs.write("MTrk", 4); w32(static_cast<uint32_t>(track.size()));
//    ofs.write(reinterpret_cast<char*>(track.data()), track.size());
//    return true;
//}
