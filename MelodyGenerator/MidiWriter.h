//#pragma once
//#include <vector>
//#include <string>
//#include "MelodyGenerator.h"
//
//enum {
//	ID_GBX_NOTE = 2000,   // ���K�`�F�b�N�{�b�N�X (�����f�B�p)
//	ID_GBX_CHORD = 2100,   // ���K�`�F�b�N�{�b�N�X (�R�[�h�p)
//	ID_EDT_MIN_OCT = 2200,
//	ID_EDT_MAX_OCT = 2201,
//	ID_BTN_GEN = 2300,
//};
//
//static const wchar_t* NOTE_JP[12] = {
//	L"�h",L"�h��",L"��",L"����",L"�~",L"�t�@",L"�t�@��",L"�\",
//	L"�\��",L"��",L"����",L"�V"
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
