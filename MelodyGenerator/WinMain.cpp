/*=====================================================================
  Win32 C++ MIDI Melody Generator  — config.ini 対応
  (May 2025 fixed : freeze-free, unique-name OK)
  ────────────────────────────────────────────────────────────────────
  • INI 仕様
	  [Melody]          Do=1  Do#=1  … Si=1/0
	  [Length]          note=1,2,4,8,16,32   rest=8,16,32
	  [Option]          minOct, maxOct, bpm, threshold
=====================================================================*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <shlwapi.h>
#include <vector>
#include <string>
#include <fstream>
#include <random>
#include <algorithm>
#include <set>

#pragma comment(lib,"Comctl32.lib")
#pragma comment(lib,"Shlwapi.lib")

/* ───────── GUI ID ───────── */
enum
{
	ID_MEL_NOTE = 3000,   // 3000-3011
	ID_LEN_NOTE = 3200,   // 3200-3205
	ID_LEN_REST = 3300,   // 3300-3305
	ID_ED_MINOC = 3400,
	ID_ED_MAXOC,
	ID_ED_BPM,
	ID_ED_THRESH,
	ID_BTN_GEN = 3500
};

/* ───────── 定数表 ───────── */
static const wchar_t* JP_NOTE[12] = { L"Do", L"Do#", L"Re", L"Re#", L"Mi", L"Fa", L"Fa#", L"So", L"So#", L"La", L"La#", L"Si" };
static const wchar_t* NOTE_KEY[12] = { L"Do", L"Do#", L"Re", L"Re#", L"Mi", L"Fa", L"Fa#", L"So", L"So#", L"La", L"La#", L"Si" };

struct LenDef { const wchar_t* txt; std::uint32_t tick; int denom; };

static const LenDef LENDEF[6] =
{
	{L"全", 1920, 1},
	{L"2",  960,  2},
	{L"4",  480,  4},
	{L"8",  240,  8},
	{L"16", 120,  16},
	{L"32", 60,   32}
};

constexpr int LEN_DEN[6] = { 1, 2, 4, 8, 16, 32 };
constexpr std::uint32_t PPQN = 480;

/* ───────── 設定 ───────── */
struct AppConfig
{
	int  minOct = 3, maxOct = 6, bpm = 120, threshold = 7;
	std::uint16_t melMask = 0xFFF;  // 12bit
	std::uint16_t lenMask = 0x3F;   // 6bit
	std::uint16_t restMask = 0x00;   // 6bit
};

/* config.ini パス */
static std::wstring IniPath()
{
	wchar_t buf[MAX_PATH];
	GetModuleFileNameW(nullptr, buf, MAX_PATH);
	PathRemoveFileSpecW(buf);
	PathAppendW(buf, L"config.ini");

	return buf;
}

/* INI 読み込み */
static void LoadConfig(AppConfig& c)
{
	wchar_t buf[256]{};
	auto path = IniPath().c_str();

	/* Melody */
	for (int i = 0; i < 12; ++i)
	{
		int v = GetPrivateProfileIntW(L"Melody", NOTE_KEY[i], -1, path);

		if (v == 0)
			c.melMask &= ~(1u << i);
		else if (v == 1)
			c.melMask |= (1u << i);
	}

	/* Length → mask */
	auto parse = [&](const wchar_t* key)->std::uint16_t
		{
			GetPrivateProfileStringW(L"Length", key, L"", buf, 256, path);
			std::uint16_t m = 0;
			wchar_t* ctx = nullptr;

			for (wchar_t* p = wcstok_s(buf, L", ", &ctx); p; p = wcstok_s(nullptr, L", ", &ctx))
			{
				int v = _wtoi(p);

				for (int i = 0; i < 6; ++i)
					if (v == LEN_DEN[i])
						m |= (1u << i);
			}
			return m;
		};

	if (auto lm = parse(L"note"))
		c.lenMask = lm;

	if (auto rm = parse(L"rest"))
		c.restMask = rm;

	/* Option */
	c.minOct = GetPrivateProfileIntW(L"Option", L"minOct", c.minOct, path);
	c.maxOct = GetPrivateProfileIntW(L"Option", L"maxOct", c.maxOct, path);
	c.bpm = GetPrivateProfileIntW(L"Option", L"bpm", c.bpm, path);
	c.threshold = GetPrivateProfileIntW(L"Option", L"threshold", c.threshold, path);
}

/* INI 書き出し */
static void SaveConfig(const AppConfig& c)
{
	auto path = IniPath().c_str();

	auto wnum = [&](const wchar_t* s, const wchar_t* k, int v)
		{
			wchar_t t[16]{};
			_itow_s(v, t, 10);
			WritePrivateProfileStringW(s, k, t, path);
		};

	for (int i = 0; i < 12; ++i)
		wnum(L"Melody", NOTE_KEY[i], (c.melMask >> i) & 1);

	auto list = [&](std::uint16_t m)->std::wstring
		{
			std::wstring s{};
			for (int i = 0; i < 6; ++i)
				if (m & (1u << i))
				{
					if (!s.empty())
						s += L",";

					s += std::to_wstring(LEN_DEN[i]);
				}
			return s;
		};
	WritePrivateProfileStringW(L"Length", L"note", list(c.lenMask).c_str(), path);
	WritePrivateProfileStringW(L"Length", L"rest", list(c.restMask).c_str(), path);

	wnum(L"Option", L"minOct", c.minOct);
	wnum(L"Option", L"maxOct", c.maxOct);
	wnum(L"Option", L"bpm", c.bpm);
	wnum(L"Option", L"threshold", c.threshold);
}

/* ───────── NoteEvent ───────── */
struct NoteEvent { std::uint8_t pitch; std::uint32_t len; bool rest; };

/* ───────── MelodyGenerator ───────── */
class MelodyGenerator
{
public:
	MelodyGenerator(const std::vector<int>& pcs, int minOc, int maxOc,
		const std::vector<int>& chd, const std::vector<std::uint32_t>& nL,
		const std::vector<std::uint32_t>& rL, std::uint32_t p, int t) :
		allow(pcs), chordPool(chd), nLen(nL), rLen(rL), minO(minOc), maxO(maxOc), th(t), ppqn(p)
	{}

	std::vector<NoteEvent> generate(size_t bars)
	{
		std::mt19937 rng{ std::random_device{}() };
		std::uniform_int_distribution<int>    ocDist(minO, maxO);
		std::uniform_int_distribution<size_t> nLenDist(0, nLen.size() - 1);
		std::uniform_int_distribution<size_t> rLenDist(0, rLen.empty() ? 0 : rLen.size() - 1);

		auto pick = [&](const std::vector<int>& v)
			{ return std::uniform_int_distribution<size_t>(0, v.size() - 1); };

		std::vector<NoteEvent> seq;
		int prev = -1;
		const std::uint32_t barTick = ppqn * 4;

		for (size_t bar = 0; bar < bars; ++bar)
		{
			/* コード・プール作成 */
			std::vector<int> chord;
			if (!chordPool.empty())
			{
				int root = chordPool[pick(chordPool)(rng)];
				chord = { root,(root + 4) % 12,(root + 7) % 12 };
				std::sort(chord.begin(), chord.end());
			}

			std::vector<int> pool;
			std::set_intersection(allow.begin(), allow.end(), chord.begin(), chord.end(), std::back_inserter(pool));
			const std::vector<int>& use = pool.empty() ? allow : pool;
			auto useDist = pick(use);
			std::uint32_t filled = 0, retry = 0;

			while (filled < barTick)
			{
				bool rest = !rLen.empty() &&
					(std::uniform_int_distribution<int>(0, int(nLen.size() + rLen.size() - 1))(rng) >= int(nLen.size()));

				std::uint32_t len = rest ? rLen[rLenDist(rng)] : nLen[nLenDist(rng)];

				if (filled + len > barTick)
				{
					if (++retry > 32)
						break;

					continue;
				}

				retry = 0;
				if (rest)
				{
					seq.push_back({ 0,len,true });
					filled += len;
					continue;
				}

				int pitch = 0;
				if (prev < 0)
					pitch = use[useDist(rng)] + ocDist(rng) * 12;
				else
				{
					int localTh = th;
					bool ok = false;

					for (int a = 0; a < 128 && !ok; ++a)
					{
						pitch = use[useDist(rng)] + ocDist(rng) * 12;

						if (std::abs(pitch - prev) <= localTh)
							ok = true;
					}
					if (!ok)
					{
						localTh = 12;

						for (int a = 0; a < 128 && !ok; ++a)
						{
							pitch = use[useDist(rng)] + ocDist(rng) * 12;
							if (std::abs(pitch - prev) <= localTh) ok = true;
						}

						if (!ok)
							pitch = prev;
					}
				}
				pitch = std::clamp(pitch, 0, 127);
				prev = pitch;
				seq.push_back({ std::uint8_t(pitch),len,false });
				filled += len;
			}
		}
		return seq;
	}

private:
	std::vector<int> allow, chordPool;
	std::vector<std::uint32_t> nLen, rLen;
	int minO, maxO, th; std::uint32_t ppqn;
};

/* ───────── MidiWriter ───────── */
class MidiWriter
{
public:
	explicit MidiWriter(std::uint32_t p) :ppqn(p) {}

	bool write(const std::wstring& fn, const std::vector<NoteEvent>& ev, int bpm) const
	{
		if (ev.empty())
			return false;

		std::vector<std::uint8_t> trk =
		{
			0x00,0xFF,0x51,0x03,
			std::uint8_t((60000000 / bpm) >> 16),
			std::uint8_t((60000000 / bpm) >> 8),
			std::uint8_t(60000000 / bpm)
		};

		std::uint32_t acc = 0;
		for (auto& n : ev)
		{
			if (n.rest)
			{
				acc += n.len;
				continue;
			}

			wVar(trk, acc);
			acc = 0;
			trk.insert(trk.end(), { 0x90,n.pitch,100 });
			wVar(trk, n.len);
			trk.insert(trk.end(), { 0x80,n.pitch,0 });
		}
		wVar(trk, acc);
		trk.insert(trk.end(), { 0xFF,0x2F,0x00 });

		std::ofstream f(fn, std::ios::binary);
		if (!f) return false;

		auto w16 = [&](std::uint16_t v)
			{
				f.put(v >> 8);
				f.put(v);
			};
		auto w32 = [&](std::uint32_t v)
			{
				f.put(v >> 24);
				f.put(v >> 16);
				f.put(v >> 8);
				f.put(v);
			};

		f.write("MThd", 4);

		w32(6);
		w16(0);
		w16(1);
		w16(ppqn);

		f.write("MTrk", 4);
		w32(std::uint32_t(trk.size()));
		f.write(reinterpret_cast<char*>(trk.data()), trk.size());

		return true;
	}
private:
	std::uint32_t ppqn;
	static void wVar(std::vector<std::uint8_t>& v, std::uint32_t val)
	{
		std::uint8_t b[5]{};
		int n = 0;
		b[n++] = val & 0x7F;

		while (val >>= 7)
			b[n++] = 0x80 | (val & 0x7F);

		while (n--)
			v.push_back(b[n]);
	}
};

/* ───────── ユニーク名生成（FIXED）───────── */
static std::wstring MakeUniqueName(const std::wstring& base, const std::wstring& ext)
{
	std::wstring dir = IniPath();
	PathRemoveFileSpecW(dir.data());           // 末尾削除
	dir.resize(wcslen(dir.c_str()));           // 長さを詰める！

	std::wstring name{};
	for (int n = 0;; ++n)
	{
		name = dir + L"\\" + base;

		if (n > 0)
			name += L"(" + std::to_wstring(n) + L")";

		name += ext;

		if (!PathFileExistsW(name.c_str()))
			break;
	}
	return name;
}

/* ───────── MsgBox ───────── */
static int mbox(HWND w, const wchar_t* t, const wchar_t* c, UINT f)
{
	return MessageBoxW(w, t, c, f | MB_SETFOREGROUND | MB_APPLMODAL);
}

/* ───────── WndProc ───────── */
static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM wP, LPARAM lP)
{
	static AppConfig cfg;
	switch (m)
	{
	case WM_CREATE:
	{
		INITCOMMONCONTROLSEX icc{ sizeof(icc),ICC_STANDARD_CLASSES };
		InitCommonControlsEx(&icc);

		CreateWindowW(L"STATIC", L"メロディ", WS_CHILD | WS_VISIBLE, 10, 10, 100, 20, h, nullptr, nullptr, nullptr);
		const int BW = 58;

		for (int i = 0; i < 12; ++i)
			CreateWindowW(L"BUTTON", JP_NOTE[i], WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
				10 + (i % 6) * (BW + 5), 35 + (i / 6) * 25, BW, 20, h, (HMENU)(ID_MEL_NOTE + i), nullptr, nullptr);

		struct
		{
			const wchar_t* t;
			int id;
			int y;
			const wchar_t* d;
		}

		ed[] =
		{
			{L"最小Oct", ID_ED_MINOC,  10, L"3"  },
			{L"最大Oct", ID_ED_MAXOC,  35, L"6"  },
			{L"BPM",     ID_ED_BPM,    60, L"120"},
			{L"閾値",    ID_ED_THRESH, 85, L"7"  }
		};

		for (auto& e : ed)
		{
			CreateWindowW(L"STATIC", e.t, WS_CHILD | WS_VISIBLE, 400, e.y, 60, 20, h, nullptr, nullptr, nullptr);
			CreateWindowW(L"EDIT", e.d, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 460, e.y, 40, 22, h, (HMENU)e.id, nullptr, nullptr);
		}

		CreateWindowW(L"STATIC", L"音符長", WS_CHILD | WS_VISIBLE, 400, 115, 60, 20, h, nullptr, nullptr, nullptr);
		CreateWindowW(L"STATIC", L"休符長", WS_CHILD | WS_VISIBLE, 460, 115, 60, 20, h, nullptr, nullptr, nullptr);

		for (int i = 0; i < 6; ++i)
		{
			int y = 140 + i * 22;
			CreateWindowW(L"BUTTON", LENDEF[i].txt, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 400, y, 45, 20, h, (HMENU)(ID_LEN_NOTE + i), nullptr, nullptr);
			CreateWindowW(L"BUTTON", LENDEF[i].txt, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 460, y, 45, 20, h, (HMENU)(ID_LEN_REST + i), nullptr, nullptr);
		}

		CreateWindowW(L"BUTTON", L"生成 → 保存", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 360, 280, 120, 35, h, (HMENU)ID_BTN_GEN, nullptr, nullptr);

		CreateWindowW(L"STATIC", L"Author: 0xE4F90A", WS_CHILD | WS_VISIBLE, 10, 310, 125, 20, h, nullptr, nullptr, nullptr);

		if (GetFileAttributesW(IniPath().c_str()) != INVALID_FILE_ATTRIBUTES)
			LoadConfig(cfg);
		else
			SaveConfig(cfg);

		/* UI 反映 */
		for (int i = 0; i < 12; ++i)
			CheckDlgButton(h, ID_MEL_NOTE + i, (cfg.melMask >> i) & 1 ? BST_CHECKED : BST_UNCHECKED);

		for (int i = 0; i < 6; ++i)
		{
			CheckDlgButton(h, ID_LEN_NOTE + i, (cfg.lenMask >> i) & 1 ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(h, ID_LEN_REST + i, (cfg.restMask >> i) & 1 ? BST_CHECKED : BST_UNCHECKED);
		}

		SetDlgItemInt(h, ID_ED_MINOC, cfg.minOct, TRUE);
		SetDlgItemInt(h, ID_ED_MAXOC, cfg.maxOct, TRUE);
		SetDlgItemInt(h, ID_ED_BPM, cfg.bpm, FALSE);
		SetDlgItemInt(h, ID_ED_THRESH, cfg.threshold, FALSE);

		return 0;
	}
	case WM_COMMAND:
		if (LOWORD(wP) == ID_BTN_GEN)
		{
			std::vector<int> melPC{};
			cfg.melMask = 0;

			for (int i = 0; i < 12; ++i)
				if (IsDlgButtonChecked(h, ID_MEL_NOTE + i) == BST_CHECKED)
				{
					melPC.push_back(i);
					cfg.melMask |= (1u << i);
				}

			if (melPC.empty())
			{
				mbox(h, L"音階が 0 個です", L"エラー", MB_ICONERROR);
				return 0;
			}

			cfg.lenMask = cfg.restMask = 0;
			std::vector<std::uint32_t> nLen, rLen;
			for (int i = 0; i < 6; ++i)
			{
				if (IsDlgButtonChecked(h, ID_LEN_NOTE + i) == BST_CHECKED)
				{
					nLen.push_back(LENDEF[i].tick);
					cfg.lenMask |= (1u << i);
				}

				if (IsDlgButtonChecked(h, ID_LEN_REST + i) == BST_CHECKED)
				{
					rLen.push_back(LENDEF[i].tick);
					cfg.restMask |= (1u << i);
				}
			}

			if (nLen.empty())
			{
				mbox(h, L"音符長が 0 個です", L"エラー", MB_ICONERROR);
				return 0;
			}

			BOOL ok;
			cfg.minOct = GetDlgItemInt(h, ID_ED_MINOC, &ok, TRUE);
			cfg.maxOct = GetDlgItemInt(h, ID_ED_MAXOC, &ok, TRUE);
			cfg.bpm = GetDlgItemInt(h, ID_ED_BPM, &ok, FALSE);
			cfg.threshold = GetDlgItemInt(h, ID_ED_THRESH, &ok, FALSE);

			if (cfg.minOct > cfg.maxOct)
			{
				mbox(h, L"最小/最大オクターブが逆転", L"エラー", MB_ICONERROR);
				return 0;
			}

			SaveConfig(cfg);
			MelodyGenerator gen(melPC, cfg.minOct, cfg.maxOct, {}, nLen, rLen, PPQN, cfg.threshold);
			auto ev = gen.generate(8);

			std::wstring outPath = MakeUniqueName(L"generate", L".mid");

			if (MidiWriter(PPQN).write(outPath, ev, cfg.bpm))
				mbox(h, (L"保存完了: " + outPath).c_str(), L"完了", MB_OK);
			else
				mbox(h, L"保存に失敗しました", L"エラー", MB_ICONERROR);

		}
		return 0;

	case WM_DESTROY: PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(h, m, wP, lP);
}

/* ───────── WinMain ───────── */
int WINAPI wWinMain(_In_ HINSTANCE hi, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int nc)
{
	const wchar_t* cls = L"MIDI_GEN_APP";

	WNDCLASSW wc{};
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hi;
	wc.lpszClassName = cls;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	RegisterClassW(&wc);

	HWND h = CreateWindowExW(0, cls, L"Melody Generator 1.0",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, 580, 380, nullptr, nullptr, hi, nullptr);

	ShowWindow(h, nc);
	UpdateWindow(h);

	MSG msg{};
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
	return 0;
}
