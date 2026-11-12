// Goal Friends: a creature collector that lives on the bottom of your screen.
// Win32 host: transparent always-on-top strip, tray icon, reminders, saving.
#include <windows.h>
#include <shellapi.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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
constexpr int HOTKEY_ID = 1;

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
    bool hotkeyOk = false;
    ULONGLONG panelClosedSince = 0;
    bool trimmed = false;
    // diagnostics (GF_TRACE): average cost of drawing and of presenting a frame
    long long perfFrames = 0;
    double perfRenderMs = 0, perfPresentMs = 0;
    ULONGLONG perfStart = 0;
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
    int minute = st.wHour * 60 + st.wMinute;
    if (const char* h = std::getenv("GF_HOUR")) minute = static_cast<int>(std::atof(h) * 60.0) % 1440;  // test override: fake the hour of day
    return tu::Time{tu::daysFromCivil(st.wYear, st.wMonth, st.wDay), minute};
}

void saveNow(App& a) {
    if (gf::saveToFile(a.game, a.savePath)) {
        a.needSave = false;
        a.lastSave = GetTickCount64();
    }
}

void addTray(App& a) {
    a.nid = {};
    a.nid.cbSize = sizeof(a.nid);
    a.nid.hWnd = a.hwnd;
    a.nid.uID = ID_TRAY;
    a.nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    a.nid.uCallbackMessage = WM_TRAY;
    a.nid.hIcon = a.trayIcon ? a.trayIcon : LoadIcon(nullptr, IDI_APPLICATION);   // the app icon (resource 1), see app.rc
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

// The area the strip lives in. With an auto-hiding taskbar the work area reaches the very bottom of the screen, and a window
// that covers the last pixel row stops the mouse from touching the edge, so the taskbar can never slide up. In that case we
// stop one pixel short of the screen edge.
void refreshWorkArea(App& a) {
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &a.work, 0);
    APPBARDATA abd = {};
    abd.cbSize = sizeof(abd);
    const bool autoHide = (SHAppBarMessage(ABM_GETSTATE, &abd) & ABS_AUTOHIDE) != 0;
    if (autoHide && a.work.bottom >= GetSystemMetrics(SM_CYSCREEN)) a.work.bottom = GetSystemMetrics(SM_CYSCREEN) - 1;
}

void setFrameInterval(App& a, UINT ms) {
    if (a.frameInterval == ms) return;
    a.frameInterval = ms;
    SetTimer(a.hwnd, TIMER_FRAME, ms, nullptr);
}

void renderFrame(App& a) {
    if (!a.visible) {
        a.ui->backgroundTick(static_cast<long long>(GetTickCount64()), localNow());
        if (a.ui->takeDirty()) a.needSave = true;
        for (const auto& b : a.ui->takeBalloons()) balloon(a, b);
        return;
    }
    const ULONGLONG nowMs = GetTickCount64();
    double dt = a.lastTick ? static_cast<double>(nowMs - a.lastTick) / 1000.0 : 0.0;
    a.lastTick = nowMs;
    dt = std::min(dt, 0.1);

    const int pw = a.work.right - a.work.left;
    const float logicalW = static_cast<float>(pw) / a.scale;
    const float maxH = static_cast<float>(a.work.bottom - a.work.top) / a.scale;
    auto clearInput = [&a] {
        a.in.pressed = false;
        a.in.wheel = 0;
        a.in.typed.clear();
        a.in.paste.clear();
        a.in.backspace = a.in.enter = a.in.escape = false;
    };

    LARGE_INTEGER q0, q1, q2, qf;
    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    int ph = 0;
    float logicalH = 0;
    // A click can open or close a panel in the middle of a frame, which changes the window height. Draw that frame again
    // at the new height right away instead of presenting a stale one and resizing on the next timer tick (that was the jitter).
    for (int pass = 0; pass < 2; ++pass) {
        logicalH = std::min(a.ui->desiredHeight(), maxH);
        ph = static_cast<int>(logicalH * a.scale + 0.5f);
        if (!a.gx.resize(pw, ph)) return;
        a.gx.setScale(a.scale);
        if (!a.gx.begin()) return;
        a.ui->frame(a.gx, logicalW, logicalH, a.in, pass == 0 ? dt : 0.0, static_cast<long long>(nowMs), localNow());
        a.gx.end();
        const float after = std::min(a.ui->desiredHeight(), maxH);
        if (pass == 1 || static_cast<int>(after * a.scale + 0.5f) == ph) break;
        if (a.in.my >= 0) a.in.my += after - logicalH;   // the window grew or shrank at its top edge: keep the cursor where it was on screen
        clearInput();   // the click was already used by the first pass
    }
    QueryPerformanceCounter(&q1);

    HDC screen = GetDC(nullptr);
    POINT dst = {a.work.left, a.work.bottom - ph};
    SIZE size = {pw, ph};
    POINT src = {0, 0};
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(a.hwnd, screen, &dst, &size, a.gx.dc(), &src, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
    QueryPerformanceCounter(&q2);
    a.perfFrames++;
    a.perfRenderMs += 1000.0 * static_cast<double>(q1.QuadPart - q0.QuadPart) / static_cast<double>(qf.QuadPart);
    a.perfPresentMs += 1000.0 * static_cast<double>(q2.QuadPart - q1.QuadPart) / static_cast<double>(qf.QuadPart);
    if (const char* path = std::getenv("GF_TRACE")) {
        const ULONGLONG now2 = GetTickCount64();
        if (!a.perfStart) a.perfStart = now2;
        if (now2 - a.perfStart >= 10000 && a.perfFrames > 0) {
            if (FILE* f = std::fopen(path, "a")) {
                std::fprintf(f, "perf: %.1f fps, draw %.2f ms/frame, present %.2f ms/frame (%dx%d px)\n", a.perfFrames * 1000.0 / static_cast<double>(now2 - a.perfStart),
                             a.perfRenderMs / static_cast<double>(a.perfFrames), a.perfPresentMs / static_cast<double>(a.perfFrames), pw, ph);
                std::fclose(f);
            }
            a.perfFrames = 0; a.perfRenderMs = a.perfPresentMs = 0; a.perfStart = now2;
        }
    }

    clearInput();

    const bool open = a.ui->panelOpen();
    if (open != a.panelWasOpen) {
        a.panelWasOpen = open;
        setNoActivate(a, !open);
        if (open) { SetForegroundWindow(a.hwnd); SetFocus(a.hwnd); }
    }
    if (a.ui->takeDirty()) a.needSave = true;
    for (const auto& b : a.ui->takeBalloons()) balloon(a, b);

    // Frame rate follows need: fast right after input, medium while something moves or a panel is open, slow when the
    // strip is just sitting there.
    const bool recent = (nowMs - a.lastActivity) < 2500;
    UINT want;
    if (recent) want = open ? 33 : 66;
    else if (open) want = a.ui->effectsActive() ? 50 : 100;       // static panels barely need redrawing
    else want = a.ui->animating() ? 100 : 500;                    // walking friends: 10 fps (steps stay under one cell per frame); sitting still: 2 fps
    setFrameInterval(a, want);
}

// "Hidden" means a fully transparent, click-through frame. The window itself stays alive, so the global hotkey and
// timers keep working exactly as when it is visible.
void presentBlank(App& a) {
    const int pw = a.work.right - a.work.left;
    const int ph = static_cast<int>(ui::Ui::kStrip * a.scale);
    if (!a.gx.resize(pw, ph)) return;
    a.gx.setScale(a.scale);
    if (!a.gx.begin()) return;
    a.gx.end();
    HDC screen = GetDC(nullptr);
    POINT dst = {a.work.left, a.work.bottom - ph};
    SIZE size = {pw, ph};
    POINT src = {0, 0};
    BLENDFUNCTION bf = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};
    UpdateLayeredWindow(a.hwnd, screen, &dst, &size, a.gx.dc(), &src, 0, &bf, ULW_ALPHA);
    ReleaseDC(nullptr, screen);
}

void slowTick(App& a) {
    refreshWorkArea(a);   // picks up taskbar auto-hide being switched on or off
    const tu::Time now = localNow();
    if (now.day != a.lastDay) {
        gf::sweep(a.game, now);
        a.lastDay = now.day;
        a.needSave = true;
    }
    {
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
    // Hand idle memory back to Windows a while after the panel closes.
    if (a.ui->panelOpen()) { a.panelClosedSince = 0; a.trimmed = false; }
    else if (!a.panelClosedSince) a.panelClosedSince = GetTickCount64();
    else if (!a.trimmed && GetTickCount64() - a.panelClosedSince > 15000) {
        SetProcessWorkingSetSize(GetCurrentProcess(), static_cast<SIZE_T>(-1), static_cast<SIZE_T>(-1));
        a.trimmed = true;
    }
}

void toggleVisible(App& a) {
    a.visible = !a.visible;
    a.in = ui::Input{};  // drop any held input
    if (a.visible) {
        a.lastTick = 0;
        renderFrame(a);
    } else {
        presentBlank(a);
        setFrameInterval(a, 1000);  // only a once-a-second check for a finishing focus session
    }
}

// Ctrl+Shift+O-circumflex toggles the window. The key is found from the installed keyboard layouts, so it lands on
// the right physical key (the key next to L on a Slovak keyboard); if no layout has the character we fall back to
// the key that holds it on a Slovak layout (VK_OEM_1).
void registerHotkey(App& a) {
    UINT vk = VK_OEM_1;  // the Slovak key for the o-circumflex
    const int n = GetKeyboardLayoutList(0, nullptr);
    std::vector<HKL> layouts(static_cast<size_t>(std::max(n, 0)));
    if (n > 0) GetKeyboardLayoutList(n, layouts.data());
    // Use the character's key from an installed layout, but only a direct, non-letter key. Dead-key or AltGr
    // combinations, and plain letters, would make a clumsy or colliding global shortcut.
    auto direct = [](HKL l, UINT& out) {
        const SHORT r = VkKeyScanExW(L'\u00f4', l);
        if (r == -1) return false;
        const UINT v = static_cast<UINT>(LOBYTE(r));
        const int mods = HIBYTE(r);
        if ((mods & ~1) != 0 || (v >= 0x30 && v <= 0x5A) || v == 0) return false;
        out = v;
        return true;
    };
    bool found = false;
    for (HKL l : layouts)
        if (LOWORD(reinterpret_cast<UINT_PTR>(l)) == 0x041B && direct(l, vk)) { found = true; break; }  // Slovak first
    for (HKL l : layouts) {
        if (found) break;
        found = direct(l, vk);
    }
    a.hotkeyOk = RegisterHotKey(a.hwnd, HOTKEY_ID, MOD_CONTROL | MOD_SHIFT | MOD_NOREPEAT, vk) != 0;
    if (const char* path = std::getenv("GF_TRACE")) {  // optional diagnostics
        if (FILE* f = std::fopen(path, "a")) {
            std::fprintf(f, "hotkey vk=0x%02x ok=%d err=%lu layouts=%d\n", vk, a.hotkeyOk ? 1 : 0, static_cast<unsigned long>(GetLastError()), n);
            std::fclose(f);
        }
    }
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
            if (!a->visible) return 0;
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
            if (!a->visible) return 0;
            a->in.mx = static_cast<float>(static_cast<short>(LOWORD(lp))) / a->scale;
            a->in.my = static_cast<float>(static_cast<short>(HIWORD(lp))) / a->scale;
            a->in.down = a->in.pressed = true;
            SetCapture(hwnd);   // keep receiving mouse moves while dragging the panel outside the window
            a->lastActivity = GetTickCount64();
            if (a->panelWasOpen) SetFocus(hwnd);
            renderFrame(*a);  // respond immediately
            return 0;
        case WM_LBUTTONUP:
            a->in.down = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            return 0;
        case WM_MOUSEWHEEL:
            a->in.wheel += static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / WHEEL_DELTA;
            a->lastActivity = GetTickCount64();
            return 0;
        case WM_MOUSEACTIVATE:
            return a->panelWasOpen ? MA_ACTIVATE : MA_NOACTIVATE;
        case WM_CHAR: {
            if (!a->visible) return 0;
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
        case WM_HOTKEY:
            if (const char* path = std::getenv("GF_TRACE")) { if (FILE* f = std::fopen(path, "a")) { std::fprintf(f, "WM_HOTKEY received\n"); std::fclose(f); } }
            if (wp == static_cast<WPARAM>(HOTKEY_ID) && !std::getenv("GF_HOTKEY_NOHIDE")) toggleVisible(*a);
            return 0;
        case WM_TRAY:
            if (lp == WM_LBUTTONUP) toggleVisible(*a);
            else if (lp == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU menu = CreatePopupMenu();
                AppendMenuW(menu, MF_STRING, CMD_TOGGLE, a->visible ? L"Hide Goal Friends (Ctrl+Shift+\u00f4)" : L"Show Goal Friends (Ctrl+Shift+\u00f4)");
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
            UnregisterHotKey(hwnd, HOTKEY_ID);
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
    wc.hIcon = LoadIconW(inst, MAKEINTRESOURCEW(1));
    wc.hIconSm = app.trayIcon;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"GoalFriendsStrip";
    RegisterClassExW(&wc);

    const int pw = app.work.right - app.work.left;
    const int ph = static_cast<int>(ui::Ui::kStrip * app.scale);
    app.hwnd = CreateWindowExW(WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, wc.lpszClassName, L"Goal Friends",
                               WS_POPUP, app.work.left, app.work.bottom - ph, pw, ph, nullptr, nullptr, inst, nullptr);
    if (!app.hwnd) return 1;

    app.trayIcon = static_cast<HICON>(LoadImageW(inst, MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    app.taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
    addTray(app);

    registerHotkey(app);
    if (!app.hotkeyOk) balloon(app, "Couldn't register Ctrl+Shift+\u00d4 (another app may be using it). Use the tray icon to hide or show the window.");
    ShowWindow(app.hwnd, SW_SHOWNA);
    SetTimer(app.hwnd, TIMER_FRAME, app.frameInterval, nullptr);
    SetTimer(app.hwnd, TIMER_SLOW, 5000, nullptr);
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
