// Goal Friends: a creature collector that lives on the bottom of your screen.
// Win32 host: transparent always-on-top strip, tray icon, reminders, saving.
#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "game.hpp"
#include "gfx.hpp"
#include "sprite.hpp"
#include "ui.hpp"

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT ID_TRAY = 1;
constexpr UINT_PTR TIMER_FRAME = 1;
constexpr UINT_PTR TIMER_SLOW = 2;
constexpr UINT CMD_TOGGLE = 100;
constexpr UINT CMD_QUIT = 101;

struct App {
    HWND hwnd = nullptr;
    HINSTANCE inst = nullptr;
    gfx::Gfx gx;
    gf::GameState game;
    std::unique_ptr<ui::Ui> ui;
    ui::Input in;
    std::string savePath;
    NOTIFYICONDATAW nid = {};
    HICON trayIcon = nullptr;
    UINT taskbarCreated = 0;
    float scale = 1.f;
    RECT work = {};
    bool visible = true;
    bool panelWasOpen = false;
    bool tracking = false;
    bool needSave = false;
    ULONGLONG lastTick = 0, lastSave = 0, lastActivity = 0;
    UINT frameInterval = 66;
    int lastDay = -1;
    int slowCount = 0;
};

App* g_app = nullptr;

std::wstring envVar(const wchar_t* name) {
    const wchar_t* v = _wgetenv(name);
    return v ? v : L"";
}

std::string toUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    const int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), nullptr, 0, nullptr, nullptr);
    std::string s(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, w.data(), static_cast<int>(w.size()), s.data(), n, nullptr, nullptr);
    return s;
}

tu::Time localNow() {
    SYSTEMTIME st;
    GetLocalTime(&st);
    return tu::Time{tu::daysFromCivil(st.wYear, st.wMonth, st.wDay), st.wHour * 60 + st.wMinute};
}

void saveNow(App& a) {
    if (gf::saveToFile(a.game, a.savePath)) {
        a.needSave = false;
        a.lastSave = GetTickCount64();
    }
}

HICON makeTrayIcon(int speciesId) {
    const gf::Sprite s = gf::makeSprite(speciesId, 0);
    const int scale = 2, n = 16 * scale;
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = n;
    bmi.bmiHeader.biHeight = -n;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HDC dc = GetDC(nullptr);
    HBITMAP color = CreateDIBSection(dc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, dc);
    if (!color || !bits) return nullptr;
    auto* px = static_cast<uint32_t*>(bits);
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x) px[y * n + x] = s.at(x / scale, y / scale);
    HBITMAP mask = CreateBitmap(n, n, 1, 1, nullptr);
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = color;
    ii.hbmMask = mask;
    HICON icon = CreateIconIndirect(&ii);
    DeleteObject(color);
    DeleteObject(mask);
    return icon;
}

void addTray(App& a) {
    a.nid = {};
    a.nid.cbSize = sizeof(a.nid);
    a.nid.hWnd = a.hwnd;
    a.nid.uID = ID_TRAY;
    a.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    a.nid.uCallbackMessage = WM_TRAY;
    a.nid.hIcon = a.trayIcon ? a.trayIcon : LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(a.nid.szTip, L"Goal Friends");
    Shell_NotifyIconW(NIM_ADD, &a.nid);
}

void balloon(App& a, const std::string& text) {
    NOTIFYICONDATAW n = a.nid;
    n.uFlags = NIF_INFO;
    n.dwInfoFlags = NIIF_INFO;
    wcscpy_s(n.szInfoTitle, L"Goal Friends");
    const std::wstring w = gfx::widen(text);
    wcsncpy_s(n.szInfo, w.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &n);
}

void setNoActivate(App& a, bool noActivate) {
    LONG_PTR ex = GetWindowLongPtrW(a.hwnd, GWL_EXSTYLE);
    LONG_PTR want = noActivate ? (ex | WS_EX_NOACTIVATE) : (ex & ~static_cast<LONG_PTR>(WS_EX_NOACTIVATE));
    if (want != ex) {
        SetWindowLongPtrW(a.hwnd, GWL_EXSTYLE, want);
        SetWindowPos(a.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    }
}

void refreshWorkArea(App& a) { SystemParametersInfoW(SPI_GETWORKAREA, 0, &a.work, 0); }

void setFrameInterval(App& a, UINT ms) {
    if (a.frameInterval == ms) return;
    a.frameInterval = ms;
    SetTimer(a.hwnd, TIMER_FRAME, ms, nullptr);
}

void renderFrame(App& a) {
    if (!a.visible) return;
    const ULONGLONG nowMs = GetTickCount64();
    double dt = a.lastTick ? static_cast<double>(nowMs - a.lastTick) / 1000.0 : 0.0;
    a.lastTick = nowMs;
    dt = std::min(dt, 0.1);

    const int pw = a.work.right - a.work.left;
    const float logicalW = static_cast<float>(pw) / a.scale;
    const float maxH = static_cast<float>(a.work.bottom - a.work.top) / a.scale;
    const float logicalH = std::min(a.ui->desiredHeight(), maxH);
    const int ph = static_cast<int>(logicalH * a.scale + 0.5f);
    if (!a.gx.resize(pw, ph)) return;
    a.gx.setScale(a.scale);

    if (!a.gx.begin()) return;
    a.ui->frame(a.gx, logicalW, logicalH, a.in, dt, static_cast<long long>(nowMs), localNow());
    a.gx.end();

    HDC screen = GetDC(nullptr);
    POINT dst = {a.work.left, a.work.bottom - ph};
    SIZE size = {pw, ph};
    POINT src = {0, 0};
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(a.hwnd, screen, &dst, &size, a.gx.dc(), &src, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, screen);

    a.in.pressed = false;
    a.in.wheel = 0;
    a.in.typed.clear();
    a.in.paste.clear();
    a.in.backspace = a.in.enter = a.in.escape = false;

    const bool open = a.ui->panelOpen();
    if (open != a.panelWasOpen) {
        a.panelWasOpen = open;
        setNoActivate(a, !open);
        if (open) { SetForegroundWindow(a.hwnd); SetFocus(a.hwnd); }
    }
    if (a.ui->takeDirty()) a.needSave = true;
    for (const auto& b : a.ui->takeBalloons()) balloon(a, b);

    const bool active = open || (nowMs - a.lastActivity) < 2500;
    setFrameInterval(a, active ? 33 : 66);
}

void slowTick(App& a) {
    const tu::Time now = localNow();
    if (now.day != a.lastDay) {
        gf::sweep(a.game, now);
        a.lastDay = now.day;
        a.needSave = true;
    }
    if (++a.slowCount % 5 == 0) {
        const auto msgs = gf::pollReminders(a.game, now);
        if (!msgs.empty()) {
            std::string text;
            if (msgs.size() == 1) text = msgs[0].text;
            else {
                text = std::to_string(msgs.size()) + " tasks need you: ";
                for (size_t i = 0; i < msgs.size() && i < 3; ++i) text += (i ? ", " : "") + msgs[i].text;
            }
            balloon(a, text);
            a.ui->toast(text);
            a.ui->reminderFired();
            a.needSave = true;
        }
        SetWindowPos(a.hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    if (a.needSave && GetTickCount64() - a.lastSave > 3000) saveNow(a);
}

void toggleVisible(App& a) {
    a.visible = !a.visible;
    ShowWindow(a.hwnd, a.visible ? SW_SHOWNA : SW_HIDE);
    if (a.visible) renderFrame(a);
}

void pasteClipboard(App& a) {
    if (!OpenClipboard(a.hwnd)) return;
    if (HANDLE h = GetClipboardData(CF_UNICODETEXT)) {
        if (const wchar_t* p = static_cast<const wchar_t*>(GlobalLock(h))) {
            std::wstring s(p);
            GlobalUnlock(h);
            for (auto& c : s) if (c == L'\r' || c == L'\n' || c == L'\t') c = L' ';
            if (s.size() > 120) s.resize(120);
            a.in.paste += s;
        }
    }
    CloseClipboard();
}

LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    App* a = g_app;
    if (!a) return DefWindowProcW(hwnd, msg, wp, lp);
    if (msg == a->taskbarCreated && a->taskbarCreated) { addTray(*a); return 0; }
    switch (msg) {
        case WM_MOUSEMOVE: {
            a->in.mx = static_cast<float>(static_cast<short>(LOWORD(lp))) / a->scale;
            a->in.my = static_cast<float>(static_cast<short>(HIWORD(lp))) / a->scale;
            a->lastActivity = GetTickCount64();
            if (!a->tracking) {
                TRACKMOUSEEVENT t = {sizeof(t), TME_LEAVE, hwnd, 0};
                TrackMouseEvent(&t);
                a->tracking = true;
            }
            return 0;
        }
        case WM_MOUSELEAVE:
            a->in.mx = a->in.my = -1;
            a->tracking = false;
            return 0;
        case WM_LBUTTONDOWN:
            a->in.mx = static_cast<float>(static_cast<short>(LOWORD(lp))) / a->scale;
            a->in.my = static_cast<float>(static_cast<short>(HIWORD(lp))) / a->scale;
            a->in.down = a->in.pressed = true;
            a->lastActivity = GetTickCount64();
            if (a->panelWasOpen) SetFocus(hwnd);
            renderFrame(*a);  // respond immediately
            return 0;
        case WM_LBUTTONUP:
            a->in.down = false;
            return 0;
        case WM_MOUSEWHEEL:
            a->in.wheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
            a->lastActivity = GetTickCount64();
            return 0;
        case WM_MOUSEACTIVATE:
            return a->panelWasOpen ? MA_ACTIVATE : MA_NOACTIVATE;
        case WM_CHAR: {
            const wchar_t c = static_cast<wchar_t>(wp);
            if (c == 8) a->in.backspace = true;
            else if (c == 13) a->in.enter = true;
            else if (c == 27) a->in.escape = true;
            else if (c == 22) pasteClipboard(*a);
            else if (c >= 32) a->in.typed += c;
            a->lastActivity = GetTickCount64();
            renderFrame(*a);
            return 0;
        }
        case WM_TIMER:
            if (wp == TIMER_FRAME) renderFrame(*a);
            else if (wp == TIMER_SLOW) slowTick(*a);
            return 0;
        case WM_DISPLAYCHANGE:
        case WM_SETTINGCHANGE:
            refreshWorkArea(*a);
            return 0;
        case WM_TRAY:
            if (lp == WM_LBUTTONUP) toggleVisible(*a);
            else if (lp == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING, CMD_TOGGLE, a->visible ? L"Hide Goal Friends" : L"Show Goal Friends");
                AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
                AppendMenuW(menu, MF_STRING, CMD_QUIT, L"Quit");
                SetForegroundWindow(hwnd);
                TrackPopupMenu(menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
                PostMessageW(hwnd, WM_NULL, 0, 0);
                DestroyMenu(menu);
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wp) == CMD_TOGGLE) toggleVisible(*a);
            else if (LOWORD(wp) == CMD_QUIT) DestroyWindow(hwnd);
            return 0;
        case WM_QUERYENDSESSION:
            return TRUE;
        case WM_ENDSESSION:
            saveNow(*a);
            return 0;
        case WM_DESTROY:
            saveNow(*a);
            Shell_NotifyIconW(NIM_DELETE, &a->nid);
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

int WINAPI WinMain(HINSTANCE inst, HINSTANCE, LPSTR, int) {
    SetProcessDPIAware();
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"GoalFriends.SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxW(nullptr, L"Goal Friends is already running (look for its tray icon).", L"Goal Friends", MB_OK | MB_ICONINFORMATION);
        return 0;
    }

    App app;
    g_app = &app;
    app.inst = inst;

    // Save file in %APPDATA%\GoalFriends\save.json
    std::wstring dir = envVar(L"APPDATA");
    if (dir.empty()) dir = L".";
    dir += L"\\GoalFriends";
    CreateDirectoryW(dir.c_str(), nullptr);
    app.savePath = toUtf8(dir + L"\\save.json");
    if (!gf::loadFromFile(app.savePath, app.game)) {
        // Keep a corrupt file around instead of overwriting it.
        const std::wstring p = dir + L"\\save.json";
        if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) MoveFileExW(p.c_str(), (p + L".bad").c_str(), MOVEFILE_REPLACE_EXISTING);
        app.game = gf::GameState{};
        app.game.rngSeed = static_cast<uint32_t>(GetTickCount64() ^ 0x9E3779B9u);
    }
    gf::sweep(app.game, localNow());
    app.lastDay = localNow().day;

    if (!app.gx.init()) {
        MessageBoxW(nullptr, L"Could not start Direct2D.", L"Goal Friends", MB_OK | MB_ICONERROR);
        return 1;
    }
    HDC sdc = GetDC(nullptr);
    app.scale = static_cast<float>(GetDeviceCaps(sdc, LOGPIXELSX)) / 96.f;
    ReleaseDC(nullptr, sdc);
    refreshWorkArea(app);
    app.ui = std::make_unique<ui::Ui>(app.game);

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"GoalFriendsStrip";
    RegisterClassExW(&wc);

    const int pw = app.work.right - app.work.left;
    const int ph = static_cast<int>(ui::Ui::kStrip * app.scale);
    app.hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName, L"Goal Friends",
                               WS_POPUP, app.work.left, app.work.bottom - ph, pw, ph, nullptr, nullptr, inst, nullptr);
    if (!app.hwnd) return 1;

    app.trayIcon = makeTrayIcon(0);
    app.taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    addTray(app);

    ShowWindow(app.hwnd, SW_SHOWNA);
    SetTimer(app.hwnd, TIMER_FRAME, app.frameInterval, nullptr);
    SetTimer(app.hwnd, TIMER_SLOW, 1000, nullptr);
    app.lastSave = GetTickCount64();
    renderFrame(app);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    if (app.trayIcon) DestroyIcon(app.trayIcon);
    ReleaseMutex(mutex);
    CloseHandle(mutex);
    return static_cast<int>(msg.wParam);
}
