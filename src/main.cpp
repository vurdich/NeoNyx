#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "version.h"
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <cwctype>
#include <fstream>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <initializer_list>
#include <wincrypt.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "crypt32.lib")

namespace {
constexpr wchar_t kClassName[] = L"NeoNyxMainWindow";
COLORREF kBg = RGB(16, 17, 27);
COLORREF kPanel = RGB(24, 25, 39);
COLORREF kPanel2 = RGB(31, 32, 49);
COLORREF kPurple = RGB(139, 92, 246);
COLORREF kText = RGB(238, 238, 248);
COLORREF kMuted = RGB(157, 159, 180);
bool g_lightTheme = false;

enum : int {
    ID_SEARCH = 1001, ID_LIST = 1002, ID_ADD = 1003,
    ID_RUN = 1004, ID_REMOVE = 1005, ID_ABOUT = 1006,
    ID_EDIT = 1007, ID_UP = 1008, ID_DOWN = 1009,
    ID_E_NAME = 1101, ID_E_PATH = 1102, ID_E_BROWSE = 1103,
    ID_E_SAVE = 1104, ID_E_CANCEL = 1105, ID_E_CATEGORY = 1106,
    ID_VAULT = 1010, ID_THEME = 1011,
    ID_COLLAPSE = 1012, ID_MEDIA_PREV = 1013, ID_MEDIA_PLAY = 1014,
    ID_MEDIA_NEXT = 1015, ID_SPOTIFY = 1016,
    ID_V_LIST = 1201, ID_V_SERVICE = 1202, ID_V_LOGIN = 1203,
    ID_V_PASSWORD = 1204, ID_V_URL = 1205, ID_V_ADD = 1206,
    ID_V_UPDATE = 1207, ID_V_DELETE = 1208, ID_V_COPY = 1209, ID_V_SHOW = 1210
};

struct AppEntry {
    std::wstring name;
    std::wstring path;
    std::wstring category = L"Программы";
};
struct VaultEntry {
    std::wstring service, login, password, url;
};
std::vector<VaultEntry> g_vault;
HWND g_vaultWindow{}, g_vaultList{}, g_vaultService{}, g_vaultLogin{}, g_vaultPassword{}, g_vaultUrl{};
bool g_revealPassword = false;
std::wstring g_dataDir;

HINSTANCE g_instance{};
HWND g_clockTitle{}, g_musicTitle{};
HWND g_window{}, g_search{}, g_cards{}, g_status{};
bool g_expanded = false;
int g_animStep = 0, g_animStartW = 196, g_animStartH = 44;
bool g_animTargetExpanded = false;
constexpr UINT_PTR kIslandAnimTimer = 9917;
constexpr UINT_PTR kStatusTimer = 9918;
HFONT g_font{}, g_boldFont{}, g_smallFont{};
int g_selected = -1;
int g_scrollY = 0;
HWND g_editor{}, g_editName{}, g_editPath{}, g_editCategory{};
int g_editIndex = -1;
std::vector<AppEntry> g_apps;
std::vector<size_t> g_visible;
std::wstring g_iniPath;

std::wstring AppDataPath() {
    wchar_t path[MAX_PATH]{};
    DWORD n = GetEnvironmentVariableW(L"APPDATA", path, MAX_PATH);
    std::wstring dir = n && n < MAX_PATH ? path : L".";
    dir += L"\\NeoNyx";
    CreateDirectoryW(dir.c_str(), nullptr);
    g_dataDir = dir;
    return dir + L"\\apps.ini";
}

std::wstring DefaultName(const std::wstring& path) {
    std::filesystem::path p(path);
    std::wstring name = p.stem().wstring();
    return name.empty() ? L"New application" : name;
}

void LoadApps() {
    g_iniPath = AppDataPath();
    int count = GetPrivateProfileIntW(L"Apps", L"Count", 0, g_iniPath.c_str());
    count = (std::max)(0, (std::min)(count, 500));
    g_apps.clear();
    for (int i = 0; i < count; ++i) {
        wchar_t key[32]{}, name[512]{}, path[2048]{}, category[128]{};
        wsprintfW(key, L"Name%d", i);
        GetPrivateProfileStringW(L"Apps", key, L"", name, 512, g_iniPath.c_str());
        wsprintfW(key, L"Path%d", i);
        GetPrivateProfileStringW(L"Apps", key, L"", path, 2048, g_iniPath.c_str());
        wsprintfW(key, L"Category%d", i);
        GetPrivateProfileStringW(L"Apps", key, L"Программы", category, 128, g_iniPath.c_str());
        if (path[0]) g_apps.push_back({name[0] ? name : DefaultName(path), path, category[0] ? category : L"Программы"});
    }
}

void SaveApps() {
    WritePrivateProfileStringW(L"Apps", L"Count", std::to_wstring(g_apps.size()).c_str(), g_iniPath.c_str());
    for (size_t i = 0; i < g_apps.size(); ++i) {
        wchar_t key[32]{};
        wsprintfW(key, L"Name%u", (unsigned)i);
        WritePrivateProfileStringW(L"Apps", key, g_apps[i].name.c_str(), g_iniPath.c_str());
        wsprintfW(key, L"Path%u", (unsigned)i);
        WritePrivateProfileStringW(L"Apps", key, g_apps[i].path.c_str(), g_iniPath.c_str());
        wsprintfW(key, L"Category%u", (unsigned)i);
        WritePrivateProfileStringW(L"Apps", key, g_apps[i].category.c_str(), g_iniPath.c_str());
    }
    for (size_t i = g_apps.size(); i < 500; ++i) {
        wchar_t key[32]{};
        wsprintfW(key, L"Name%u", (unsigned)i);
        WritePrivateProfileStringW(L"Apps", key, nullptr, g_iniPath.c_str());
        wsprintfW(key, L"Path%u", (unsigned)i);
        WritePrivateProfileStringW(L"Apps", key, nullptr, g_iniPath.c_str());
    }
}

// Password vault: the complete database is encrypted with Windows DPAPI before writing to disk.
std::wstring VaultFilePath() { return g_dataDir + L"\\vault.dat"; }
void AppendString(std::vector<BYTE>& out, const std::wstring& value) {
    DWORD chars = (DWORD)value.size();
    const BYTE* len = reinterpret_cast<const BYTE*>(&chars);
    out.insert(out.end(), len, len + sizeof(chars));
    const BYTE* data = reinterpret_cast<const BYTE*>(value.data());
    out.insert(out.end(), data, data + chars * sizeof(wchar_t));
}
bool ReadString(const BYTE*& cur, const BYTE* end, std::wstring& value) {
    if (end - cur < (ptrdiff_t)sizeof(DWORD)) return false;
    DWORD chars{}; memcpy(&chars, cur, sizeof(chars)); cur += sizeof(chars);
    if (chars > 32768 || end - cur < (ptrdiff_t)(chars * sizeof(wchar_t))) return false;
    value.assign(reinterpret_cast<const wchar_t*>(cur), chars); cur += chars * sizeof(wchar_t); return true;
}
bool SaveVault() {
    std::vector<BYTE> plain;
    DWORD count = (DWORD)g_vault.size();
    plain.insert(plain.end(), reinterpret_cast<BYTE*>(&count), reinterpret_cast<BYTE*>(&count) + sizeof(count));
    for (const auto& e : g_vault) { AppendString(plain,e.service); AppendString(plain,e.login); AppendString(plain,e.password); AppendString(plain,e.url); }
    DATA_BLOB input{(DWORD)plain.size(), plain.data()}, output{};
    if (!CryptProtectData(&input, L"NeoNyx Password Vault", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return false;
    std::ofstream file(VaultFilePath(), std::ios::binary | std::ios::trunc);
    bool ok = file.good();
    if (ok) file.write(reinterpret_cast<const char*>(output.pbData), output.cbData);
    file.close(); LocalFree(output.pbData);
    SecureZeroMemory(plain.data(), plain.size());
    return ok && file.good();
}
void LoadVault() {
    g_vault.clear();
    std::ifstream file(VaultFilePath(), std::ios::binary);
    if (!file) return;
    std::vector<BYTE> encrypted((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    if (encrypted.empty() || encrypted.size() > 4 * 1024 * 1024) return;
    DATA_BLOB input{(DWORD)encrypted.size(), encrypted.data()}, output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output)) return;
    const BYTE* cur = output.pbData; const BYTE* end = output.pbData + output.cbData;
    if (end - cur >= (ptrdiff_t)sizeof(DWORD)) {
        DWORD count{}; memcpy(&count,cur,sizeof(count)); cur += sizeof(count);
        if (count <= 2000) for (DWORD i=0; i<count; ++i) {
            VaultEntry e;
            if (!ReadString(cur,end,e.service) || !ReadString(cur,end,e.login) || !ReadString(cur,end,e.password) || !ReadString(cur,end,e.url)) { g_vault.clear(); break; }
            g_vault.push_back(std::move(e));
        }
    }
    SecureZeroMemory(output.pbData, output.cbData); LocalFree(output.pbData);
}
void ClearVaultFields() {
    if (g_vaultService) SetWindowTextW(g_vaultService,L"");
    if (g_vaultLogin) SetWindowTextW(g_vaultLogin,L"");
    if (g_vaultPassword) SetWindowTextW(g_vaultPassword,L"");
    if (g_vaultUrl) SetWindowTextW(g_vaultUrl,L"");
    g_revealPassword=false; if (g_vaultPassword) SendMessageW(g_vaultPassword,EM_SETPASSWORDCHAR,(WPARAM)L'*',0);
}
void RefreshVaultList() {
    if (!g_vaultList) return;
    SendMessageW(g_vaultList,LB_RESETCONTENT,0,0);
    for (const auto& e:g_vault) SendMessageW(g_vaultList,LB_ADDSTRING,0,(LPARAM)e.service.c_str());
}
void SelectVaultEntry() {
    int i=(int)SendMessageW(g_vaultList,LB_GETCURSEL,0,0); if(i<0 || i>=(int)g_vault.size()) return;
    const auto& e=g_vault[(size_t)i]; SetWindowTextW(g_vaultService,e.service.c_str());
    SetWindowTextW(g_vaultLogin,e.login.c_str()); SetWindowTextW(g_vaultPassword,e.password.c_str());
    SetWindowTextW(g_vaultUrl,e.url.c_str()); g_revealPassword=false;
    SendMessageW(g_vaultPassword,EM_SETPASSWORDCHAR,(WPARAM)L'*',0); InvalidateRect(g_vaultPassword,nullptr,TRUE);
}
std::wstring ReadControl(HWND h) { int n=GetWindowTextLengthW(h); std::wstring v((size_t)n+1,L'\0'); if(n) GetWindowTextW(h,v.data(),n+1); v.resize((size_t)n); return v; }
void SaveVaultEntry(bool update) {
    VaultEntry e{ReadControl(g_vaultService),ReadControl(g_vaultLogin),ReadControl(g_vaultPassword),ReadControl(g_vaultUrl)};
    if(e.service.empty() || e.password.empty()) { MessageBoxW(g_vaultWindow,L"Укажи название сервиса и пароль.",L"Password Vault",MB_OK|MB_ICONWARNING); return; }
    int i=(int)SendMessageW(g_vaultList,LB_GETCURSEL,0,0);
    if(update && i>=0 && i<(int)g_vault.size()) g_vault[(size_t)i]=std::move(e); else g_vault.push_back(std::move(e));
    if(!SaveVault()) { MessageBoxW(g_vaultWindow,L"Не удалось сохранить зашифрованное хранилище.",L"Password Vault",MB_OK|MB_ICONERROR); return; }
    RefreshVaultList(); SendMessageW(g_vaultList,LB_SETCURSEL,(WPARAM)(update && i>=0 ? i : (int)g_vault.size()-1),0);
}
void CopyVaultPassword() {
    std::wstring value=ReadControl(g_vaultPassword); if(value.empty()) return;
    if(!OpenClipboard(g_vaultWindow)) return; EmptyClipboard();
    HGLOBAL mem=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));
    if(mem) { void* ptr=GlobalLock(mem); memcpy(ptr,value.c_str(),(value.size()+1)*sizeof(wchar_t)); GlobalUnlock(mem); SetClipboardData(CF_UNICODETEXT,mem); }
    CloseClipboard();
}
void ToggleTheme() {
    g_lightTheme=!g_lightTheme;
    if(g_lightTheme) { kBg=RGB(239,240,248); kPanel=RGB(250,250,255); kPanel2=RGB(226,227,240); kText=RGB(35,35,52); kMuted=RGB(94,96,120); }
    else { kBg=RGB(16,17,27); kPanel=RGB(24,25,39); kPanel2=RGB(31,32,49); kText=RGB(238,238,248); kMuted=RGB(157,159,180); }
    WritePrivateProfileStringW(L"Settings",L"LightTheme",g_lightTheme?L"1":L"0",g_iniPath.c_str());
    if(g_window) InvalidateRect(g_window,nullptr,TRUE); if(g_cards) InvalidateRect(g_cards,nullptr,TRUE);
}

void SendMediaKey(WORD key) {
    INPUT input[2]{};
    input[0].type = INPUT_KEYBOARD; input[0].ki.wVk = key;
    input[1].type = INPUT_KEYBOARD; input[1].ki.wVk = key; input[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, input, sizeof(INPUT));
}

void UpdateStatus() {
    if (!g_status) return;
    SYSTEMTIME st{}; GetLocalTime(&st);
    MEMORYSTATUSEX mem{}; mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    wchar_t text[192]{};
    wsprintfW(text, L"%02u:%02u  •  %02u.%02u.%04u     ОЗУ: ",
        st.wHour, st.wMinute, st.wDay, st.wMonth, st.wYear);
    std::wstring status = text + std::to_wstring(mem.dwMemoryLoad) + L"% занято";
    SetWindowTextW(g_status, status.c_str());
}

void SetFont(HWND h, HFONT font) { SendMessageW(h, WM_SETFONT, (WPARAM)font, TRUE); }

void AddButton(HWND parent, int id, const wchar_t* label, int x, int y, int w, int h, bool primary=false) {
    HWND b = CreateWindowExW(0, L"BUTTON", label,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        x, y, w, h, parent, (HMENU)(INT_PTR)id, g_instance, nullptr);
    SetPropW(b, L"NeoNyxPrimary", (HANDLE)(INT_PTR)(primary ? 1 : 0));
    SetFont(b, g_boldFont);
}

void UpdateCardsScroll() {
    if (!g_cards) return;
    RECT client{};
    GetClientRect(g_cards, &client);
    const int width = client.right - client.left;
    const int columns = width >= 760 ? 3 : width >= 500 ? 2 : 1;
    const int cardHeight = 116;
    const int gap = 14;
    const int rows = (int(g_visible.size()) + columns - 1) / columns;
    const int contentHeight = rows ? 18 + rows * (cardHeight + gap) : 0;
    SCROLLINFO si{sizeof(si)};
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = (std::max)(0, contentHeight - 1);
    si.nPage = (UINT)(std::max)(1L, client.bottom - client.top);
    int maxPos = (std::max)(0, contentHeight - (int)si.nPage);
    g_scrollY = (std::max)(0, (std::min)(g_scrollY, maxPos));
    si.nPos = g_scrollY;
    SetScrollInfo(g_cards, SB_VERT, &si, TRUE);
    InvalidateRect(g_cards, nullptr, TRUE);
}

void RefreshList() {
    if (!g_cards) return;
    g_visible.clear();
    wchar_t query[256]{};
    if (g_search) GetWindowTextW(g_search, query, 256);
    std::wstring q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::towlower);
    for (size_t i = 0; i < g_apps.size(); ++i) {
        std::wstring hay = g_apps[i].name + L" " + g_apps[i].path + L" " + g_apps[i].category;
        std::transform(hay.begin(), hay.end(), hay.begin(), ::towlower);
        if (!q.empty() && hay.find(q) == std::wstring::npos) continue;
        g_visible.push_back(i);
    }
    if (g_selected >= 0 && std::find(g_visible.begin(), g_visible.end(), (size_t)g_selected) == g_visible.end())
        g_selected = -1;
    UpdateCardsScroll();
}

int SelectedAppIndex() { return g_selected; }

void SelectVisibleCard(int row) {
    if (row < 0 || row >= (int)g_visible.size()) return;
    g_selected = (int)g_visible[(size_t)row];
    InvalidateRect(g_cards, nullptr, FALSE);
}

void RunSelected() {
    int index = SelectedAppIndex();
    if (index < 0) {
        MessageBoxW(g_window, L"Сначала выбери приложение в списке.", L"NeoNyx", MB_OK | MB_ICONINFORMATION);
        return;
    }
    const auto& app = g_apps[(size_t)index];
    if (GetFileAttributesW(app.path.c_str()) == INVALID_FILE_ATTRIBUTES) {
        MessageBoxW(g_window, L"Файл не найден. Возможно, приложение было перемещено.", L"NeoNyx", MB_OK | MB_ICONWARNING);
        return;
    }
    HINSTANCE result = ShellExecuteW(g_window, L"open", app.path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32)
        MessageBoxW(g_window, L"Не удалось запустить приложение.", L"NeoNyx", MB_OK | MB_ICONERROR);
}

void AddApplication() {
    wchar_t file[MAX_PATH]{};
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_window;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Программы (*.exe)\0*.exe\0Все файлы (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = L"Добавить приложение в NeoNyx";
    if (!GetOpenFileNameW(&ofn)) return;

    std::wstring full = file;
    auto duplicate = std::find_if(g_apps.begin(), g_apps.end(), [&](const AppEntry& a) {
        return _wcsicmp(a.path.c_str(), full.c_str()) == 0;
    });
    if (duplicate != g_apps.end()) {
        MessageBoxW(g_window, L"Это приложение уже добавлено.", L"NeoNyx", MB_OK | MB_ICONINFORMATION);
        return;
    }
    g_apps.push_back({DefaultName(full), full});
    SaveApps();
    RefreshList();
}

void RemoveSelected() {
    int index = SelectedAppIndex();
    if (index < 0) {
        MessageBoxW(g_window, L"Выбери приложение, которое нужно удалить из списка.", L"NeoNyx", MB_OK | MB_ICONINFORMATION);
        return;
    }
    std::wstring message = L"Убрать «" + g_apps[(size_t)index].name + L"» из NeoNyx?\nСам файл программы не будет удалён.";
    if (MessageBoxW(g_window, message.c_str(), L"Подтверждение", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    g_apps.erase(g_apps.begin() + index);
    g_selected = -1;
    SaveApps();
    RefreshList();
}

void DrawButton(const DRAWITEMSTRUCT* dis);


void BrowseEditorPath() {
    if (!g_editPath) return;
    wchar_t file[MAX_PATH]{};
    GetWindowTextW(g_editPath, file, MAX_PATH);
    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_editor;
    ofn.lpstrFile = file;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = L"Программы (*.exe)\0*.exe\0Все файлы (*.*)\0*.*\0";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_EXPLORER;
    ofn.lpstrTitle = L"Выбрать EXE-файл";
    if (GetOpenFileNameW(&ofn)) SetWindowTextW(g_editPath, file);
}

void CloseEditor(bool save) {
    if (!g_editor) return;
    if (save && g_editIndex >= 0 && g_editIndex < (int)g_apps.size()) {
        wchar_t name[256]{}, path[2048]{};
        GetWindowTextW(g_editName, name, 256);
        GetWindowTextW(g_editPath, path, 2048);
        if (!name[0] || !path[0]) {
            MessageBoxW(g_editor, L"Заполни название и путь к EXE-файлу.", L"NeoNyx", MB_OK | MB_ICONWARNING);
            return;
        }
        if (GetFileAttributesW(path) == INVALID_FILE_ATTRIBUTES) {
            MessageBoxW(g_editor, L"Указанный файл не найден.", L"NeoNyx", MB_OK | MB_ICONWARNING);
            return;
        }
        g_apps[(size_t)g_editIndex].name = name;
        g_apps[(size_t)g_editIndex].path = path;
        int categoryIndex = (int)SendMessageW(g_editCategory, CB_GETCURSEL, 0, 0);
        const wchar_t* categories[] = {L"Игры", L"Программы", L"Другое"};
        if (categoryIndex >= 0 && categoryIndex < 3) g_apps[(size_t)g_editIndex].category = categories[categoryIndex];
        SaveApps();
    }
    HWND old = g_editor;
    g_editor = nullptr;
    EnableWindow(g_window, TRUE);
    DestroyWindow(old);
    SetForegroundWindow(g_window);
    RefreshList();
}

LRESULT CALLBACK VaultProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_CREATE: {
        auto label=[&](const wchar_t* t,int x,int y){ HWND h=CreateWindowW(L"STATIC",t,WS_CHILD|WS_VISIBLE,x,y,90,22,hwnd,nullptr,g_instance,nullptr); SetFont(h,g_smallFont); };
        label(L"Записи:",18,18);
        g_vaultList=CreateWindowExW(WS_EX_CLIENTEDGE,L"LISTBOX",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|LBS_NOTIFY|WS_VSCROLL,18,44,205,360,hwnd,(HMENU)ID_V_LIST,g_instance,nullptr); SetFont(g_vaultList,g_font);
        int x=245;
        label(L"Сервис / сайт:",x,18); g_vaultService=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,x,42,330,28,hwnd,(HMENU)ID_V_SERVICE,g_instance,nullptr);
        label(L"Логин:",x,82); g_vaultLogin=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,x,106,330,28,hwnd,(HMENU)ID_V_LOGIN,g_instance,nullptr);
        label(L"Пароль:",x,146); g_vaultPassword=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_PASSWORD|ES_AUTOHSCROLL,x,170,330,28,hwnd,(HMENU)ID_V_PASSWORD,g_instance,nullptr);
        label(L"Адрес сайта:",x,210); g_vaultUrl=CreateWindowExW(WS_EX_CLIENTEDGE,L"EDIT",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,x,234,330,28,hwnd,(HMENU)ID_V_URL,g_instance,nullptr);
        HWND show=CreateWindowW(L"BUTTON",L"Показать пароль",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_AUTOCHECKBOX,x,278,150,26,hwnd,(HMENU)ID_V_SHOW,g_instance,nullptr); SetFont(show,g_smallFont);
        AddButton(hwnd,ID_V_ADD,L"Добавить",x,322,100,38,true); AddButton(hwnd,ID_V_UPDATE,L"Обновить",x+110,322,100,38);
        AddButton(hwnd,ID_V_COPY,L"Копировать",x+220,322,110,38); AddButton(hwnd,ID_V_DELETE,L"Удалить",18,420,95,36);
        for(HWND h:{g_vaultService,g_vaultLogin,g_vaultPassword,g_vaultUrl}) SetFont(h,g_font);
        RefreshVaultList(); return 0;
    }
    case WM_COMMAND:
        switch(LOWORD(wp)) {
        case ID_V_LIST: if(HIWORD(wp)==LBN_SELCHANGE) SelectVaultEntry(); return 0;
        case ID_V_ADD: SaveVaultEntry(false); return 0;
        case ID_V_UPDATE: SaveVaultEntry(true); return 0;
        case ID_V_COPY: CopyVaultPassword(); return 0;
        case ID_V_SHOW: g_revealPassword=!g_revealPassword; SendMessageW(g_vaultPassword,EM_SETPASSWORDCHAR,g_revealPassword?0:(WPARAM)L'*',0); InvalidateRect(g_vaultPassword,nullptr,TRUE); return 0;
        case ID_V_DELETE: { int i=(int)SendMessageW(g_vaultList,LB_GETCURSEL,0,0); if(i>=0 && i<(int)g_vault.size() && MessageBoxW(hwnd,L"Удалить выбранную запись?",L"Password Vault",MB_YESNO|MB_ICONQUESTION)==IDYES) { g_vault.erase(g_vault.begin()+i); SaveVault(); RefreshVaultList(); ClearVaultFields(); } return 0; }
        }
        break;
    case WM_CTLCOLORSTATIC: { HDC dc=(HDC)wp; SetBkMode(dc,TRANSPARENT); SetTextColor(dc,kMuted); return (LRESULT)GetStockObject(NULL_BRUSH); }
    case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: { HDC dc=(HDC)wp; SetTextColor(dc,kText); SetBkColor(dc,kPanel2); static HBRUSH b=CreateSolidBrush(RGB(31,32,49)); return (LRESULT)b; }
    case WM_ERASEBKGND: { RECT r{}; GetClientRect(hwnd,&r); HBRUSH b=CreateSolidBrush(kPanel); FillRect((HDC)wp,&r,b); DeleteObject(b); return 1; }
    case WM_DRAWITEM: DrawButton((const DRAWITEMSTRUCT*)lp); return TRUE;
    case WM_CLOSE:
        g_vaultWindow=nullptr; g_vaultList=g_vaultService=g_vaultLogin=g_vaultPassword=g_vaultUrl=nullptr;
        for (auto& entry : g_vault) if (!entry.password.empty()) SecureZeroMemory(entry.password.data(), entry.password.size()*sizeof(wchar_t));
        g_vault.clear(); EnableWindow(g_window,TRUE); DestroyWindow(hwnd); SetForegroundWindow(g_window); return 0;
    }
    return DefWindowProcW(hwnd,msg,wp,lp);
}
void OpenVault() {
    if(g_vaultWindow) { SetForegroundWindow(g_vaultWindow); return; }
    LoadVault();
    g_vaultWindow=CreateWindowExW(WS_EX_DLGMODALFRAME,L"NeoNyxVaultWindow",L"NeoNyx — Password Vault",WS_CAPTION|WS_SYSMENU|WS_SIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,620,520,g_window,nullptr,g_instance,nullptr);
    if(!g_vaultWindow) return; EnableWindow(g_window,FALSE); ShowWindow(g_vaultWindow,SW_SHOW); UpdateWindow(g_vaultWindow);
}

LRESULT CALLBACK EditorProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        auto makeLabel = [&](const wchar_t* text, int y) {
            HWND h = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE,
                22, y, 110, 24, hwnd, nullptr, g_instance, nullptr);
            SetFont(h, g_font);
        };
        makeLabel(L"Название:", 24);
        g_editName = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            132, 20, 390, 30, hwnd, (HMENU)ID_E_NAME, g_instance, nullptr);
        SetFont(g_editName, g_font);
        makeLabel(L"Путь к EXE:", 70);
        g_editPath = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            132, 66, 300, 30, hwnd, (HMENU)ID_E_PATH, g_instance, nullptr);
        SetFont(g_editPath, g_font);
        HWND browse = CreateWindowW(L"BUTTON", L"Обзор...",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
            442, 64, 80, 34, hwnd, (HMENU)ID_E_BROWSE, g_instance, nullptr);
        SetFont(browse, g_boldFont);
        makeLabel(L"Категория:", 112);
        g_editCategory = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST,
            132, 108, 220, 120, hwnd, (HMENU)ID_E_CATEGORY, g_instance, nullptr);
        SetFont(g_editCategory, g_font);
        SendMessageW(g_editCategory, CB_ADDSTRING, 0, (LPARAM)L"Игры");
        SendMessageW(g_editCategory, CB_ADDSTRING, 0, (LPARAM)L"Программы");
        SendMessageW(g_editCategory, CB_ADDSTRING, 0, (LPARAM)L"Другое");
        AddButton(hwnd, ID_E_SAVE, L"Сохранить", 322, 158, 105, 38, true);
        AddButton(hwnd, ID_E_CANCEL, L"Отмена", 437, 158, 85, 38);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_E_BROWSE: BrowseEditorPath(); return 0;
        case ID_E_SAVE: CloseEditor(true); return 0;
        case ID_E_CANCEL: CloseEditor(false); return 0;
        }
        break;
    case WM_CLOSE: CloseEditor(false); return 0;
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp; SetBkMode(dc, TRANSPARENT); SetTextColor(dc, kMuted);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wp; SetTextColor(dc, kText); SetBkColor(dc, kPanel2);
        static HBRUSH b = CreateSolidBrush(kPanel2); return (LRESULT)b;
    }
    case WM_ERASEBKGND: {
        RECT r{}; GetClientRect(hwnd, &r); HBRUSH b = CreateSolidBrush(kPanel);
        FillRect((HDC)wp, &r, b); DeleteObject(b); return 1;
    }
    case WM_DRAWITEM: DrawButton((const DRAWITEMSTRUCT*)lp); return TRUE;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

void OpenEditor() {
    int index = SelectedAppIndex();
    if (index < 0) {
        MessageBoxW(g_window, L"Выбери приложение для редактирования.", L"NeoNyx", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (g_editor) return;
    g_editIndex = index;
    g_editor = CreateWindowExW(WS_EX_DLGMODALFRAME, L"NeoNyxEditorWindow", L"Редактировать приложение",
        WS_CAPTION | WS_SYSMENU, CW_USEDEFAULT, CW_USEDEFAULT, 550, 255,
        g_window, nullptr, g_instance, nullptr);
    if (!g_editor) return;
    SetWindowTextW(g_editName, g_apps[(size_t)index].name.c_str());
    SetWindowTextW(g_editPath, g_apps[(size_t)index].path.c_str());
    const std::wstring& cat = g_apps[(size_t)index].category;
    int catIndex = cat == L"Игры" ? 0 : cat == L"Другое" ? 2 : 1;
    SendMessageW(g_editCategory, CB_SETCURSEL, catIndex, 0);
    EnableWindow(g_window, FALSE);
    ShowWindow(g_editor, SW_SHOW);
    UpdateWindow(g_editor);
    SetFocus(g_editName);
}

void MoveSelected(int direction) {
    int index = SelectedAppIndex();
    if (index < 0) return;
    int next = index + direction;
    if (next < 0 || next >= (int)g_apps.size()) return;
    std::swap(g_apps[(size_t)index], g_apps[(size_t)next]);
    SaveApps();
    g_selected = next;
    RefreshList();
    InvalidateRect(g_cards, nullptr, TRUE);
}

void DrawButton(const DRAWITEMSTRUCT* dis) {
    bool primary = (INT_PTR)GetPropW(dis->hwndItem, L"NeoNyxPrimary") == 1;
    bool hot = (dis->itemState & ODS_HOTLIGHT) != 0;
    bool down = (dis->itemState & ODS_SELECTED) != 0;
    COLORREF fill = primary ? (down ? RGB(103, 64, 207) : hot ? RGB(157, 112, 255) : kPurple)
                            : (down ? RGB(45, 44, 68) : hot ? RGB(49, 48, 72) : kPanel2);
    HBRUSH brush = CreateSolidBrush(fill);
    RECT r = dis->rcItem;
    HBRUSH old = (HBRUSH)SelectObject(dis->hDC, brush);
    RoundRect(dis->hDC, r.left, r.top, r.right, r.bottom, 10, 10);
    SelectObject(dis->hDC, old);
    DeleteObject(brush);
    SetBkMode(dis->hDC, TRANSPARENT);
    SetTextColor(dis->hDC, kText);
    wchar_t text[128]{};
    GetWindowTextW(dis->hwndItem, text, 128);
    DrawTextW(dis->hDC, text, -1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if (dis->itemState & ODS_FOCUS) {
        InflateRect(&r, -5, -5);
        DrawFocusRect(dis->hDC, &r);
    }
}

struct CardRect { RECT rect; int appIndex; };

std::vector<CardRect> GetCardRects(HWND hwnd) {
    RECT client{};
    GetClientRect(hwnd, &client);
    int width = client.right - client.left;
    int columns = width >= 760 ? 3 : width >= 500 ? 2 : 1;
    int gap = 14, margin = 18, cardHeight = 116;
    int cardWidth = (std::max)(180, (width - margin * 2 - gap * (columns - 1)) / columns);
    std::vector<CardRect> cards;
    cards.reserve(g_visible.size());
    for (size_t i = 0; i < g_visible.size(); ++i) {
        int col = (int)(i % columns), row = (int)(i / columns);
        int x = margin + col * (cardWidth + gap);
        int y = margin + row * (cardHeight + gap) - g_scrollY;
        RECT r{x, y, x + cardWidth, y + cardHeight};
        cards.push_back({r, (int)g_visible[i]});
    }
    return cards;
}

void DrawCard(HDC dc, const CardRect& card) {
    bool selected = card.appIndex == g_selected;
    RECT r = card.rect;
    COLORREF fill = selected ? RGB(47, 37, 76) : RGB(27, 28, 44);
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, selected ? 2 : 1, selected ? kPurple : RGB(48, 49, 68));
    HGDIOBJ oldBrush = SelectObject(dc, brush);
    HGDIOBJ oldPen = SelectObject(dc, pen);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, 16, 16);
    SelectObject(dc, oldBrush); SelectObject(dc, oldPen);
    DeleteObject(brush); DeleteObject(pen);

    const AppEntry& app = g_apps[(size_t)card.appIndex];
    SHFILEINFOW sfi{};
    if (SHGetFileInfoW(app.path.c_str(), 0, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_LARGEICON)) {
        DrawIconEx(dc, r.left + 16, r.top + 22, sfi.hIcon, 42, 42, 0, nullptr, DI_NORMAL);
        DestroyIcon(sfi.hIcon);
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, kText);
    HFONT oldFont = (HFONT)SelectObject(dc, g_boldFont);
    RECT nameRect{r.left + 72, r.top + 22, r.right - 12, r.top + 58};
    DrawTextW(dc, app.name.c_str(), -1, &nameRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, g_smallFont);
    SetTextColor(dc, kMuted);
    RECT pathRect{r.left + 16, r.top + 76, r.right - 14, r.bottom - 10};
    std::wstring details = app.category + L"  •  " + app.path;
    DrawTextW(dc, details.c_str(), -1, &pathRect, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(dc, oldFont);
}

LRESULT CALLBACK CardsProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT client{}; GetClientRect(hwnd, &client);
        HBRUSH bg = CreateSolidBrush(kBg);
        FillRect(dc, &client, bg); DeleteObject(bg);
        auto cards = GetCardRects(hwnd);
        for (const auto& card : cards) {
            if (card.rect.bottom < 0 || card.rect.top > client.bottom) continue;
            DrawCard(dc, card);
        }
        if (g_visible.empty()) {
            SetBkMode(dc, TRANSPARENT); SetTextColor(dc, kMuted);
            SelectObject(dc, g_font);
            const wchar_t* message = L"Пока нет приложений. Нажми «+ Добавить», чтобы начать.";
            DrawTextW(dc, message, -1, &client, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        auto cards = GetCardRects(hwnd);
        for (size_t i = 0; i < cards.size(); ++i) {
            if (PtInRect(&cards[i].rect, pt)) {
                SetFocus(hwnd);
                SelectVisibleCard((int)i);
                return 0;
            }
        }
        g_selected = -1; InvalidateRect(hwnd, nullptr, FALSE); return 0;
    }
    case WM_LBUTTONDBLCLK: {
        POINT pt{GET_X_LPARAM(lp), GET_Y_LPARAM(lp)};
        auto cards = GetCardRects(hwnd);
        for (size_t i = 0; i < cards.size(); ++i) {
            if (PtInRect(&cards[i].rect, pt)) {
                SelectVisibleCard((int)i); RunSelected(); return 0;
            }
        }
        return 0;
    }
    case WM_VSCROLL: {
        SCROLLINFO si{sizeof(si)}; si.fMask = SIF_ALL; GetScrollInfo(hwnd, SB_VERT, &si);
        int pos = si.nPos;
        switch (LOWORD(wp)) {
        case SB_LINEUP: pos -= 36; break;
        case SB_LINEDOWN: pos += 36; break;
        case SB_PAGEUP: pos -= (int)si.nPage; break;
        case SB_PAGEDOWN: pos += (int)si.nPage; break;
        case SB_THUMBTRACK: pos = si.nTrackPos; break;
        case SB_TOP: pos = si.nMin; break;
        case SB_BOTTOM: pos = si.nMax; break;
        }
        si.fMask = SIF_POS; si.nPos = pos; SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
        g_scrollY = GetScrollPos(hwnd, SB_VERT); InvalidateRect(hwnd, nullptr, TRUE); return 0;
    }
    case WM_MOUSEWHEEL:
        SendMessageW(hwnd, WM_VSCROLL, GET_WHEEL_DELTA_WPARAM(wp) > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
        return 0;
    case WM_SIZE: UpdateCardsScroll(); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

BOOL CALLBACK SetChildVisibility(HWND child, LPARAM show) {
    ShowWindow(child, show ? SW_SHOW : SW_HIDE);
    return TRUE;
}

void ApplyWindowRoundness(HWND hwnd, int width, int height, int radius) {
    HRGN region = CreateRoundRectRgn(0, 0, width + 1, height + 1, radius, radius);
    if (region) SetWindowRgn(hwnd, region, TRUE);
}

void PositionIsland(HWND hwnd, bool expanded) {
    int width = expanded ? 980 : 196;
    int height = expanded ? 700 : 44;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int x = (screenWidth - width) / 2;
    int y = 8;
    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
    ApplyWindowRoundness(hwnd, width, height, expanded ? 28 : 44);
}

void SetExpanded(HWND hwnd, bool expanded) {
    if (g_expanded == expanded) return;
    g_expanded = expanded;
    g_animTargetExpanded = expanded;
    RECT current{}; GetWindowRect(hwnd, &current);
    g_animStartW = current.right - current.left;
    g_animStartH = current.bottom - current.top;
    g_animStep = 0;
    EnumChildWindows(hwnd, SetChildVisibility, expanded ? TRUE : FALSE);
    if (expanded) SetForegroundWindow(hwnd);
    SetTimer(hwnd, kIslandAnimTimer, 12, nullptr);
    InvalidateRect(hwnd, nullptr, TRUE);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_window = hwnd;
        g_font = CreateFontW(17, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_boldFont = CreateFontW(17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
        g_smallFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");

        HWND title = CreateWindowW(L"STATIC", L"NeoNyx", WS_CHILD | WS_VISIBLE,
            28, 18, 280, 34, hwnd, nullptr, g_instance, nullptr);
        SetFont(title, g_boldFont);
        HWND subtitle = CreateWindowW(L"STATIC", L"Твоя библиотека приложений",
            WS_CHILD | WS_VISIBLE, 30, 52, 360, 24, hwnd, nullptr, g_instance, nullptr);
        SetFont(subtitle, g_smallFont);
        g_status = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE,
            30, 78, 560, 20, hwnd, nullptr, g_instance, nullptr);
        SetFont(g_status, g_smallFont);
        UpdateStatus();
        SetTimer(hwnd, kStatusTimer, 1000, nullptr);
        AddButton(hwnd, ID_COLLAPSE, L"− Свернуть", 830, 18, 120, 36);
        g_clockTitle = CreateWindowW(L"STATIC", L"ВРЕМЯ И ДАТА", WS_CHILD | WS_VISIBLE, 30, 108, 180, 20, hwnd, nullptr, g_instance, nullptr);
        SetFont(g_clockTitle, g_boldFont);
        g_musicTitle = CreateWindowW(L"STATIC", L"♫  Сейчас играет: управление мультимедиа Windows", WS_CHILD | WS_VISIBLE, 30, 132, 560, 24, hwnd, nullptr, g_instance, nullptr);
        SetFont(g_musicTitle, g_font);
        AddButton(hwnd, ID_MEDIA_PREV, L"Пред.", 650, 112, 58, 38);
        AddButton(hwnd, ID_MEDIA_PLAY, L"▶ / ⏸", 716, 112, 78, 38, true);
        AddButton(hwnd, ID_MEDIA_NEXT, L"След.", 802, 112, 58, 38);
        AddButton(hwnd, ID_SPOTIFY, L"Spotify", 868, 112, 82, 38);

        g_search = CreateWindowExW(0, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL,
            30, 194, 590, 36, hwnd, (HMENU)ID_SEARCH, g_instance, nullptr);
        SetFont(g_search, g_font);

        AddButton(hwnd, ID_ADD, L"+ Добавить", 650, 190, 145, 42, true);
        AddButton(hwnd, ID_RUN, L"Запустить", 805, 190, 145, 42, true);
        AddButton(hwnd, ID_VAULT, L"Пароли", 650, 58, 105, 36);
        AddButton(hwnd, ID_THEME, L"Тема", 765, 58, 85, 36);

        g_cards = CreateWindowExW(WS_EX_CLIENTEDGE, L"NeoNyxCardsWindow", L"",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL,
            30, 250, 920, 310, hwnd, (HMENU)ID_LIST, g_instance, nullptr);

        AddButton(hwnd, ID_EDIT, L"Редактировать", 220, 580, 145, 42);
        AddButton(hwnd, ID_UP, L"↑", 375, 580, 55, 42);
        AddButton(hwnd, ID_DOWN, L"↓", 440, 580, 55, 42);
        AddButton(hwnd, ID_REMOVE, L"Убрать", 505, 580, 105, 42);
        AddButton(hwnd, ID_ABOUT, L"О программе", 805, 580, 145, 42);
        RefreshList();
        EnumChildWindows(hwnd, SetChildVisibility, FALSE);
        return 0;
    }
    case WM_SIZE: {
        int w = LOWORD(lp), h = HIWORD(lp);
        if (g_search) MoveWindow(g_search, 30, 194, (std::max)(250, w - 390), 36, TRUE);
        if (g_clockTitle) MoveWindow(g_clockTitle, 30, 108, 180, 20, TRUE);
        if (g_musicTitle) MoveWindow(g_musicTitle, 30, 132, w - 370, 24, TRUE);
        if (GetDlgItem(hwnd, ID_MEDIA_PREV)) MoveWindow(GetDlgItem(hwnd, ID_MEDIA_PREV), w - 330, 112, 58, 38, TRUE);
        if (GetDlgItem(hwnd, ID_MEDIA_PLAY)) MoveWindow(GetDlgItem(hwnd, ID_MEDIA_PLAY), w - 264, 112, 78, 38, TRUE);
        if (GetDlgItem(hwnd, ID_MEDIA_NEXT)) MoveWindow(GetDlgItem(hwnd, ID_MEDIA_NEXT), w - 176, 112, 58, 38, TRUE);
        if (GetDlgItem(hwnd, ID_SPOTIFY)) MoveWindow(GetDlgItem(hwnd, ID_SPOTIFY), w - 112, 112, 82, 38, TRUE);
        HWND add = GetDlgItem(hwnd, ID_ADD), run = GetDlgItem(hwnd, ID_RUN);
        if (add) MoveWindow(add, w - 330, 190, 145, 42, TRUE);
        if (run) MoveWindow(run, w - 175, 190, 145, 42, TRUE);
        if (GetDlgItem(hwnd, ID_VAULT)) MoveWindow(GetDlgItem(hwnd, ID_VAULT), w - 330, 58, 105, 36, TRUE);
        if (GetDlgItem(hwnd, ID_THEME)) MoveWindow(GetDlgItem(hwnd, ID_THEME), w - 215, 58, 85, 36, TRUE);
        if (g_cards) MoveWindow(g_cards, 30, 250, (std::max)(300, w - 60), (std::max)(180, h - 330), TRUE);
        HWND edit = GetDlgItem(hwnd, ID_EDIT), up = GetDlgItem(hwnd, ID_UP);
        HWND down = GetDlgItem(hwnd, ID_DOWN), remove = GetDlgItem(hwnd, ID_REMOVE);
        HWND about = GetDlgItem(hwnd, ID_ABOUT);
        HWND collapse = GetDlgItem(hwnd, ID_COLLAPSE);
        if (collapse) MoveWindow(collapse, w - 150, 18, 120, 36, TRUE);
        if (edit) MoveWindow(edit, 30, h - 70, 145, 42, TRUE);
        if (up) MoveWindow(up, 185, h - 70, 55, 42, TRUE);
        if (down) MoveWindow(down, 250, h - 70, 55, 42, TRUE);
        if (remove) MoveWindow(remove, 315, h - 70, 105, 42, TRUE);
        if (about) MoveWindow(about, w - 175, h - 70, 145, 42, TRUE);
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case ID_ADD: AddApplication(); return 0;
        case ID_RUN: RunSelected(); return 0;
        case ID_REMOVE: RemoveSelected(); return 0;
        case ID_EDIT: OpenEditor(); return 0;
        case ID_UP: MoveSelected(-1); return 0;
        case ID_DOWN: MoveSelected(1); return 0;
        case ID_VAULT: OpenVault(); return 0;
        case ID_THEME: ToggleTheme(); return 0;
        case ID_COLLAPSE: SetExpanded(hwnd, false); return 0;
        case ID_MEDIA_PREV: SendMediaKey(VK_MEDIA_PREV_TRACK); return 0;
        case ID_MEDIA_PLAY: SendMediaKey(VK_MEDIA_PLAY_PAUSE); return 0;
        case ID_MEDIA_NEXT: SendMediaKey(VK_MEDIA_NEXT_TRACK); return 0;
        case ID_SPOTIFY: ShellExecuteW(hwnd, L"open", L"spotify:", nullptr, nullptr, SW_SHOWNORMAL); return 0;
        case ID_ABOUT:
            MessageBoxW(hwnd, L"NeoNyx\n\nНативная версия для Windows.\nCreated by Lunexxy",
                L"О программе NeoNyx", MB_OK | MB_ICONINFORMATION);
            return 0;
        case ID_SEARCH:
            if (HIWORD(wp) == EN_CHANGE) RefreshList();
            return 0;
        }
        break;
    case WM_TIMER:
        if (wp == kStatusTimer) { UpdateStatus(); InvalidateRect(hwnd, nullptr, FALSE); return 0; }
        if (wp == kIslandAnimTimer) {
            ++g_animStep;
            const double t = (std::min)(1.0, g_animStep / 15.0);
            const double eased = 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
            const int targetW = g_animTargetExpanded ? 980 : 196;
            const int targetH = g_animTargetExpanded ? 700 : 44;
            const int width = g_animStartW + (int)((targetW - g_animStartW) * eased);
            const int height = g_animStartH + (int)((targetH - g_animStartH) * eased);
            const int x = (GetSystemMetrics(SM_CXSCREEN) - width) / 2;
            SetWindowPos(hwnd, HWND_TOPMOST, x, 8, width, height, SWP_NOACTIVATE);
            ApplyWindowRoundness(hwnd, width, height, g_animTargetExpanded ? 28 : 44);
            if (t >= 1.0) KillTimer(hwnd, kIslandAnimTimer);
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (!g_expanded) SetExpanded(hwnd, true);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps{};
        HDC dc = BeginPaint(hwnd, &ps);
        RECT r{}; GetClientRect(hwnd, &r);
        if (!g_expanded) {
            HBRUSH bg = CreateSolidBrush(RGB(8, 9, 15));
            HPEN edge = CreatePen(PS_SOLID, 1, RGB(48, 42, 72));
            HGDIOBJ oldBrush = SelectObject(dc, bg);
            HGDIOBJ oldPen = SelectObject(dc, edge);
            RoundRect(dc, 0, 0, r.right, r.bottom, 44, 44);
            SelectObject(dc, oldBrush); SelectObject(dc, oldPen);
            DeleteObject(bg); DeleteObject(edge);
            HBRUSH dot = CreateSolidBrush(kPurple);
            HGDIOBJ old = SelectObject(dc, dot);
            Ellipse(dc, 18, 16, 30, 28);
            SelectObject(dc, old); DeleteObject(dot);
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(245, 243, 255));
            HFONT font = g_boldFont ? g_boldFont : (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            HGDIOBJ oldFont = SelectObject(dc, font);
            RECT label{40, 0, r.right - 12, r.bottom};
            DrawTextW(dc, L"NeoNyx", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
            wchar_t clockText[16]{}; SYSTEMTIME now{}; GetLocalTime(&now);
            wsprintfW(clockText, L"%02u:%02u", now.wHour, now.wMinute);
            RECT clockRect{r.right - 62, 0, r.right - 16, r.bottom};
            DrawTextW(dc, clockText, -1, &clockRect, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
            SelectObject(dc, oldFont);
        } else {
            HBRUSH bg = CreateSolidBrush(kBg);
            FillRect(dc, &r, bg);
            DeleteObject(bg);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DRAWITEM:
        DrawButton((const DRAWITEMSTRUCT*)lp);
        return TRUE;
    case WM_CTLCOLORSTATIC: {
        HDC dc = (HDC)wp;
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, (HWND)lp == GetDlgItem(hwnd, ID_SEARCH) ? kText : kMuted);
        return (LRESULT)GetStockObject(NULL_BRUSH);
    }
    case WM_CTLCOLOREDIT: {
        HDC dc = (HDC)wp;
        SetTextColor(dc, kText);
        SetBkColor(dc, kPanel2);
        static HBRUSH editBrush = CreateSolidBrush(kPanel2);
        return (LRESULT)editBrush;
    }
    case WM_ERASEBKGND: {
        RECT r{}; GetClientRect(hwnd, &r);
        HBRUSH b = CreateSolidBrush(g_expanded ? kBg : RGB(8, 9, 15));
        FillRect((HDC)wp, &r, b);
        DeleteObject(b);
        return 1;
    }
    case WM_DESTROY:
        KillTimer(hwnd, kStatusTimer);
        if (g_font) DeleteObject(g_font);
        if (g_boldFont) DeleteObject(g_boldFont);
        if (g_smallFont) DeleteObject(g_smallFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    g_instance = instance;
    INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_LISTVIEW_CLASSES};
    InitCommonControlsEx(&icc);
    LoadApps();
    g_lightTheme = GetPrivateProfileIntW(L"Settings",L"LightTheme",0,g_iniPath.c_str()) != 0;
    if(g_lightTheme) { kBg=RGB(239,240,248); kPanel=RGB(250,250,255); kPanel2=RGB(226,227,240); kText=RGB(35,35,52); kMuted=RGB(94,96,120); }

    WNDCLASSEXW cardsClass{sizeof(cardsClass)};
    cardsClass.style = CS_DBLCLKS;
    cardsClass.lpfnWndProc = CardsProc;
    cardsClass.hInstance = instance;
    cardsClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    cardsClass.hbrBackground = CreateSolidBrush(kBg);
    cardsClass.lpszClassName = L"NeoNyxCardsWindow";
    if (!RegisterClassExW(&cardsClass)) return 1;

    WNDCLASSEXW vaultClass{sizeof(vaultClass)};
    vaultClass.lpfnWndProc = VaultProc;
    vaultClass.hInstance = instance;
    vaultClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    vaultClass.hbrBackground = CreateSolidBrush(kPanel);
    vaultClass.lpszClassName = L"NeoNyxVaultWindow";
    RegisterClassExW(&vaultClass);

    WNDCLASSEXW editorClass{sizeof(editorClass)};
    editorClass.lpfnWndProc = EditorProc;
    editorClass.hInstance = instance;
    editorClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    editorClass.hbrBackground = CreateSolidBrush(kPanel);
    editorClass.lpszClassName = L"NeoNyxEditorWindow";
    RegisterClassExW(&editorClass);

    WNDCLASSEXW wc{sizeof(wc)};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = CreateSolidBrush(kBg);
    wc.lpszClassName = kClassName;
    if (!RegisterClassExW(&wc)) return 1;

    HWND hwnd = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW, kClassName, L"NeoNyx",
        WS_POPUP, 0, 8, 196, 44,
        nullptr, nullptr, instance, nullptr);
    if (!hwnd) return 1;
    PositionIsland(hwnd, false);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(hwnd);

    // Quietly check GitHub Releases for a newer installer.
    wchar_t modulePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    PathRemoveFileSpecW(modulePath);
    std::wstring updaterPath = std::wstring(modulePath) + L"\\update.ps1";
    std::wstring command = L"powershell.exe -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File \"" +
        updaterPath + L"\" \"" NEONYX_VERSION L"\"";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW; startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, modulePath, &startup, &process)) {
        CloseHandle(process.hThread); CloseHandle(process.hProcess);
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
