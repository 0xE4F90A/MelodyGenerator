//#pragma once
//#include <vector>
//#include <string>
//#include "MelodyGenerator.h"
//
//enum {
//	ID_GBX_NOTE = 2000,   // 音階チェックボックス (メロディ用)
//	ID_GBX_CHORD = 2100,   // 音階チェックボックス (コード用)
//	ID_EDT_MIN_OCT = 2200,
//	ID_EDT_MAX_OCT = 2201,
//	ID_BTN_GEN = 2300,
//};
//
//static const wchar_t* NOTE_JP[12] = {
//	L"ド",L"ド♯",L"レ",L"レ♯",L"ミ",L"ファ",L"ファ♯",L"ソ",
//	L"ソ♯",L"ラ",L"ラ♯",L"シ"
//};
//
//class MidiWriter {
//public:
//
//
//
//	MidiWriter(uint32_t ppqn) : m_ppqn(ppqn) {}
//	bool write(const std::string& path,
//		const std::vector<NoteEvent>& events,
//		int bpm) const;
//
//private:
//	static void writeVarLen(std::vector<uint8_t>& out, uint32_t v);
//	uint32_t m_ppqn;
//};
