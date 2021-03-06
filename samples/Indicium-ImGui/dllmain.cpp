/*
MIT License

Copyright (c) 2018 Benjamin H�glinger

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "dllmain.h"

// 
// MinHook
// 
#include <MinHook.h>

// 
// STL
// 
#include <mutex>

// 
// POCO
// 
#include <Poco/Message.h>
#include <Poco/Logger.h>
#include <Poco/FileChannel.h>
#include <Poco/AutoPtr.h>
#include <Poco/PatternFormatter.h>
#include <Poco/FormattingChannel.h>
#include <Poco/Path.h>

using Poco::Message;
using Poco::Logger;
using Poco::FileChannel;
using Poco::AutoPtr;
using Poco::PatternFormatter;
using Poco::FormattingChannel;
using Poco::Path;

// 
// ImGui includes
// 
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_dx10.h>
#include <imgui_impl_dx11.h>

t_WindowProc OriginalDefWindowProc = nullptr;
t_WindowProc OriginalWindowProc = nullptr;
PINDICIUM_ENGINE engine = nullptr;

/**
 * \fn  BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
 *
 * \brief   DLL main entry point. Only Indicium engine initialization or shutdown should happen
 *          here to avoid deadlocks.
 *
 * \author  Benjamin "Nefarius" H�glinger
 * \date    16.06.2018
 *
 * \param   hInstance   The instance handle.
 * \param   dwReason    The call reason.
 * \param   parameter3  Unused.
 *
 * \return  TRUE on success, FALSE otherwise (will abort loading the library).
 */
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID)
{
    //
    // We don't need to get notified in thread attach- or detachments
    // 
    DisableThreadLibraryCalls(static_cast<HMODULE>(hInstance));

    INDICIUM_D3D9_EVENT_CALLBACKS d3d9;
    INDICIUM_D3D9_EVENT_CALLBACKS_INIT(&d3d9);
    d3d9.EvtIndiciumD3D9PrePresent = EvtIndiciumD3D9Present;
    d3d9.EvtIndiciumD3D9PreReset = EvtIndiciumD3D9Reset;
    d3d9.EvtIndiciumD3D9PrePresentEx = EvtIndiciumD3D9PresentEx;
    d3d9.EvtIndiciumD3D9PreResetEx = EvtIndiciumD3D9ResetEx;

    INDICIUM_D3D10_EVENT_CALLBACKS d3d10;
    INDICIUM_D3D10_EVENT_CALLBACKS_INIT(&d3d10);
    d3d10.EvtIndiciumD3D10PrePresent = EvtIndiciumD3D10Present;
    d3d10.EvtIndiciumD3D10PreResizeTarget = EvtIndiciumD3D10ResizeTarget;

    INDICIUM_D3D11_EVENT_CALLBACKS d3d11;
    INDICIUM_D3D11_EVENT_CALLBACKS_INIT(&d3d11);
    d3d11.EvtIndiciumD3D11PrePresent = EvtIndiciumD3D11Present;
    d3d11.EvtIndiciumD3D11PreResizeTarget = EvtIndiciumD3D11ResizeTarget;

    INDICIUM_ERROR err;

    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:

        if (!engine)
        {
            //
            // Get engine handle
            // 
            engine = IndiciumEngineAlloc();

            //
            // Register render pipeline callbacks
            // 
            IndiciumEngineSetD3D9EventCallbacks(engine, &d3d9);
            IndiciumEngineSetD3D10EventCallbacks(engine, &d3d10);
            IndiciumEngineSetD3D11EventCallbacks(engine, &d3d11);

            err = IndiciumEngineInit(engine, EvtIndiciumGameHooked);
        }

        break;
    case DLL_PROCESS_DETACH:

        if (engine)
        {
            IndiciumEngineShutdown(engine, EvtIndiciumGameUnhooked);
            IndiciumEngineFree(engine);
        }

        break;
    default:
        break;
    }

    return TRUE;
}

/**
 * \fn  void EvtIndiciumGameHooked(const INDICIUM_D3D_VERSION GameVersion)
 *
 * \brief   Gets called when the games' rendering pipeline has successfully been hooked and the
 *          rendering callbacks are about to get fired. The detected version of the used
 *          rendering objects is reported as well.
 *
 * \author  Benjamin "Nefarius" H�glinger
 * \date    16.06.2018
 *
 * \param   GameVersion The detected DirectX/Direct3D version.
 */
void EvtIndiciumGameHooked(const INDICIUM_D3D_VERSION GameVersion)
{
    std::string logfile("%TEMP%\\Indicium-ImGui.Plugin.log");

    AutoPtr<FileChannel> pFileChannel(new FileChannel);
    pFileChannel->setProperty("path", Poco::Path::expand(logfile));
    AutoPtr<PatternFormatter> pPF(new PatternFormatter);
    pPF->setProperty("pattern", "%Y-%m-%d %H:%M:%S.%i %s [%p]: %t");
    AutoPtr<FormattingChannel> pFC(new FormattingChannel(pPF, pFileChannel));

    Logger::root().setChannel(pFC);

    auto& logger = Logger::get(__func__);

    logger.information("Loading ImGui plugin");

    logger.information("Initializing hook engine...");

    MH_STATUS status = MH_Initialize();

    if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED)
    {
        logger.fatal("Couldn't initialize hook engine: %lu", (ULONG)status);
    }

    logger.information("Hook engine initialized");

    // Setup Dear ImGui binding
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls

    // Setup style
    ImGui::StyleColorsDark();
}

/**
 * \fn  void EvtIndiciumGameUnhooked()
 *
 * \brief   Gets called when all core engine hooks have been released. At this stage it is save
 *          to remove our own additional hooks and shut down the hooking sub-system as well.
 *
 * \author  Benjamin "Nefarius" H�glinger
 * \date    16.06.2018
 */
void EvtIndiciumGameUnhooked()
{
    auto& logger = Logger::get(__func__);

    if (MH_DisableHook(MH_ALL_HOOKS) != MH_OK)
    {
        logger.fatal("Couldn't disable hooks, host process might crash");
        return;
    }

    logger.information("Hooks disabled");

    if (MH_Uninitialize() != MH_OK)
    {
        logger.fatal("Couldn't shut down hook engine, host process might crash");
        return;
    }
}

#pragma region D3D9(Ex)

void EvtIndiciumD3D9Present(
    LPDIRECT3DDEVICE9   pDevice,
    const RECT          *pSourceRect,
    const RECT          *pDestRect,
    HWND                hDestWindowOverride,
    const RGNDATA       *pDirtyRegion
)
{
    static auto& logger = Logger::get(__func__);
    static auto initialized = false;
    static std::once_flag init;

    std::call_once(init, [&](LPDIRECT3DDEVICE9 pd3dDevice)
    {
        D3DDEVICE_CREATION_PARAMETERS params;

        auto hr = pd3dDevice->GetCreationParameters(&params);
        if (FAILED(hr))
        {
            logger.error("Couldn't get creation parameters from device");
            return;
        }

        ImGui_ImplDX9_Init(params.hFocusWindow, pd3dDevice);

        logger.information("ImGui (DX9) initialized");

        HookWindowProc(params.hFocusWindow);

        initialized = true;

    }, pDevice);

    if (initialized)
    {
        ImGui_ImplDX9_NewFrame();
        RenderScene();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
}

void EvtIndiciumD3D9Reset(
    LPDIRECT3DDEVICE9       pDevice,
    D3DPRESENT_PARAMETERS   *pPresentationParameters
)
{
    //
    // TODO: more checks and testing!
    // 
    ImGui_ImplDX9_InvalidateDeviceObjects();
    ImGui_ImplDX9_CreateDeviceObjects();
}

void EvtIndiciumD3D9PresentEx(
    LPDIRECT3DDEVICE9EX     pDevice,
    const RECT              *pSourceRect,
    const RECT              *pDestRect,
    HWND                    hDestWindowOverride,
    const RGNDATA           *pDirtyRegion,
    DWORD                   dwFlags
)
{
    static auto& logger = Logger::get(__func__);
    static auto initialized = false;
    static std::once_flag init;

    std::call_once(init, [&](LPDIRECT3DDEVICE9EX pd3dDevice)
    {
        D3DDEVICE_CREATION_PARAMETERS params;

        auto hr = pd3dDevice->GetCreationParameters(&params);
        if (FAILED(hr))
        {
            logger.error("Couldn't get creation parameters from device");
            return;
        }

        ImGui_ImplDX9_Init(params.hFocusWindow, pd3dDevice);

        logger.information("ImGui (DX9Ex) initialized");

        HookWindowProc(params.hFocusWindow);

        initialized = true;

    }, pDevice);

    if (initialized)
    {
        ImGui_ImplDX9_NewFrame();
        RenderScene();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    }
}

void EvtIndiciumD3D9ResetEx(
    LPDIRECT3DDEVICE9EX     pDevice,
    D3DPRESENT_PARAMETERS   *pPresentationParameters,
    D3DDISPLAYMODEEX        *pFullscreenDisplayMode
)
{
    //
    // TODO: more checks and testing!
    // 
    ImGui_ImplDX9_InvalidateDeviceObjects();
    ImGui_ImplDX9_CreateDeviceObjects();
}

#pragma endregion

#pragma region D3D10

void EvtIndiciumD3D10Present(
    IDXGISwapChain  *pSwapChain,
    UINT            SyncInterval,
    UINT            Flags
)
{
    static auto& logger = Logger::get(__func__);
    static auto initialized = false;
    static std::once_flag init;

    std::call_once(init, [&](IDXGISwapChain *pChain)
    {
        logger.information("Grabbing device and context pointers");

        ID3D10Device *pDevice;
        if (FAILED(D3D10_DEVICE_FROM_SWAPCHAIN(pChain, &pDevice)))
        {
            logger.error("Couldn't get device from swapchain");
            return;
        }

        DXGI_SWAP_CHAIN_DESC sd;
        pChain->GetDesc(&sd);

        logger.information("Initializing ImGui");

        ImGui_ImplDX10_Init(sd.OutputWindow, pDevice);

        logger.information("ImGui (DX10) initialized");

        HookWindowProc(sd.OutputWindow);

        initialized = true;

    }, pSwapChain);

    if (initialized)
    {
        ImGui_ImplDX10_NewFrame();
        RenderScene();
        ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
    }
}

void EvtIndiciumD3D10ResizeTarget(
    IDXGISwapChain          *pSwapChain,
    const DXGI_MODE_DESC    *pNewTargetParameters
)
{
    //
    // TODO: more checks and testing!
    // 
    ImGui_ImplDX10_InvalidateDeviceObjects();
    ImGui_ImplDX10_CreateDeviceObjects();
}

#pragma endregion

#pragma region D3D11

void EvtIndiciumD3D11Present(
    IDXGISwapChain  *pSwapChain,
    UINT            SyncInterval,
    UINT            Flags
)
{
    static auto& logger = Logger::get(__func__);
    static auto initialized = false;
    static std::once_flag init;

    static ID3D11DeviceContext *pContext;
    static ID3D11RenderTargetView *mainRenderTargetView;

    std::call_once(init, [&](IDXGISwapChain *pChain)
    {
        logger.information("Grabbing device and context pointers");

        ID3D11Device *pDevice;
        if (FAILED(D3D11_DEVICE_CONTEXT_FROM_SWAPCHAIN(pChain, &pDevice, &pContext)))
        {
            logger.error("Couldn't get device and context from swapchain");
            return;
        }

        ID3D11Texture2D* pBackBuffer;
        pChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
        pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
        pBackBuffer->Release();

        DXGI_SWAP_CHAIN_DESC sd;
        pChain->GetDesc(&sd);

        logger.information("Initializing ImGui");

        ImGui_ImplDX11_Init(sd.OutputWindow, pDevice, pContext);

        logger.information("ImGui (DX11) initialized");

        HookWindowProc(sd.OutputWindow);

        initialized = true;

    }, pSwapChain);

    if (initialized)
    {
        ImGui_ImplDX11_NewFrame();
        pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
        RenderScene();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }
}

void EvtIndiciumD3D11ResizeTarget(
    IDXGISwapChain          *pSwapChain,
    const DXGI_MODE_DESC    *pNewTargetParameters
)
{
    //
    // TODO: more checks and testing!
    // 
    ImGui_ImplDX11_InvalidateDeviceObjects();
    ImGui_ImplDX11_CreateDeviceObjects();
}

#pragma endregion

#pragma region WNDPROC Hooking

void HookWindowProc(HWND hWnd)
{
    auto& logger = Logger::get(__func__);

    MH_STATUS ret;

    if ((ret = MH_CreateHook(
        &DefWindowProcW,
        &DetourDefWindowProc,
        reinterpret_cast<LPVOID*>(&OriginalDefWindowProc))
        ) != MH_OK)
    {
        logger.error("Couldn't create hook for DefWindowProcW: %lu", static_cast<ULONG>(ret));
        return;
    }

    if (ret == MH_OK && MH_EnableHook(&DefWindowProcW) != MH_OK)
    {
        logger.error("Couldn't enable DefWindowProcW hook");
    }   

    if ((ret = MH_CreateHook(
        &DefWindowProcA,
        &DetourDefWindowProc,
        reinterpret_cast<LPVOID*>(&OriginalDefWindowProc))
        ) != MH_OK)
    {
        logger.error("Couldn't create hook for DefWindowProcA: %lu", static_cast<ULONG>(ret));
        return;
    }

    if (ret == MH_OK && MH_EnableHook(&DefWindowProcA) != MH_OK)
    {
        logger.error("Couldn't enable DefWindowProcW hook");
    }

    auto lptrWndProc = reinterpret_cast<t_WindowProc>(GetWindowLongPtr(hWnd, GWLP_WNDPROC));

    if (MH_CreateHook(lptrWndProc, &DetourWindowProc, reinterpret_cast<LPVOID*>(&OriginalWindowProc)) != MH_OK)
    {
        logger.warning("Couldn't create hook for GWLP_WNDPROC");
        return;
    }

    if (MH_EnableHook(lptrWndProc) != MH_OK)
    {
        logger.error("Couldn't enable GWLP_WNDPROC hook");
    }
}

LRESULT WINAPI DetourDefWindowProc(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    static std::once_flag flag;
    std::call_once(flag, []() {Logger::get("DetourDefWindowProc").information("++ DetourDefWindowProc called"); });

    ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

    return OriginalDefWindowProc(hWnd, Msg, wParam, lParam);
}

LRESULT WINAPI DetourWindowProc(
    _In_ HWND hWnd,
    _In_ UINT Msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
)
{
    static std::once_flag flag;
    std::call_once(flag, []() {Logger::get("DetourWindowProc").information("++ DetourWindowProc called"); });

    ImGui_ImplWin32_WndProcHandler(hWnd, Msg, wParam, lParam);

    return OriginalWindowProc(hWnd, Msg, wParam, lParam);
}

#pragma endregion

#pragma region Main content rendering

void RenderScene()
{
    static std::once_flag flag;
    std::call_once(flag, []() {Logger::get("RenderScene").information("++ RenderScene called"); });

    static bool show_overlay = true;

    if (show_overlay)
    {
        ImGui::ShowMetricsWindow();

        {
            ImGui::SetNextWindowPos(ImVec2(1400, 60));
            ImGui::Begin(
                "Some plots =)",
                nullptr,
                ImGuiWindowFlags_AlwaysAutoResize
            );

            static bool animate = true;
            ImGui::Checkbox("Animate", &animate);

            static float arr[] = { 0.6f, 0.1f, 1.0f, 0.5f, 0.92f, 0.1f, 0.2f };
            ImGui::PlotLines("Frame Times", arr, IM_ARRAYSIZE(arr));

            // Create a dummy array of contiguous float values to plot
            // Tip: If your float aren't contiguous but part of a structure, you can pass a pointer to your first float and the sizeof() of your structure in the Stride parameter.
            static float values[90] = { 0 };
            static int values_offset = 0;
            static float refresh_time = 0.0f;
            if (!animate || refresh_time == 0.0f)
                refresh_time = ImGui::GetTime();
            while (refresh_time < ImGui::GetTime()) // Create dummy data at fixed 60 hz rate for the demo
            {
                static float phase = 0.0f;
                values[values_offset] = cosf(phase);
                values_offset = (values_offset + 1) % IM_ARRAYSIZE(values);
                phase += 0.10f*values_offset;
                refresh_time += 1.0f / 60.0f;
            }
            ImGui::PlotLines("Lines", values, IM_ARRAYSIZE(values), values_offset, "avg 0.0", -1.0f, 1.0f, ImVec2(0, 80));
            ImGui::PlotHistogram("Histogram", arr, IM_ARRAYSIZE(arr), 0, NULL, 0.0f, 1.0f, ImVec2(0, 80));

            // Use functions to generate output
            // FIXME: This is rather awkward because current plot API only pass in indices. We probably want an API passing floats and user provide sample rate/count.
            struct Funcs
            {
                static float Sin(void*, int i) { return sinf(i * 0.1f); }
                static float Saw(void*, int i) { return (i & 1) ? 1.0f : -1.0f; }
            };
            static int func_type = 0, display_count = 70;
            ImGui::Separator();
            ImGui::PushItemWidth(100); ImGui::Combo("func", &func_type, "Sin\0Saw\0"); ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::SliderInt("Sample count", &display_count, 1, 400);
            float(*func)(void*, int) = (func_type == 0) ? Funcs::Sin : Funcs::Saw;
            ImGui::PlotLines("Lines", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
            ImGui::PlotHistogram("Histogram", func, NULL, display_count, 0, NULL, -1.0f, 1.0f, ImVec2(0, 80));
            ImGui::Separator();

            // Animate a simple progress bar
            static float progress = 0.0f, progress_dir = 1.0f;
            if (animate)
            {
                progress += progress_dir * 0.4f * ImGui::GetIO().DeltaTime;
                if (progress >= +1.1f) { progress = +1.1f; progress_dir *= -1.0f; }
                if (progress <= -0.1f) { progress = -0.1f; progress_dir *= -1.0f; }
            }

            // Typically we would use ImVec2(-1.0f,0.0f) to use all available width, or ImVec2(width,0.0f) for a specified width. ImVec2(0.0f,0.0f) uses ItemWidth.
            ImGui::ProgressBar(progress, ImVec2(0.0f, 0.0f));
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
            ImGui::Text("Progress Bar");

            float progress_saturated = (progress < 0.0f) ? 0.0f : (progress > 1.0f) ? 1.0f : progress;
            char buf[32];
            sprintf(buf, "%d/%d", (int)(progress_saturated * 1753), 1753);
            ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), buf);

            ImGui::End();
        }
    }

    static auto pressedPast = false, pressedNow = false;
    if (GetAsyncKeyState(VK_F12) & 0x8000)
    {
        pressedNow = true;
    }
    else
    {
        pressedPast = false;
        pressedNow = false;
    }

    if (!pressedPast && pressedNow)
    {
        show_overlay = !show_overlay;

        pressedPast = true;
    }

    if (show_overlay) {
        ImGui::Render();
    }
}

#pragma endregion

#pragma region ImGui-specific (taken from their examples unmodified)

bool ImGui_ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None)
    {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        ::SetCursor(NULL);
    }
    else
    {
        // Hardware cursor type
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor)
        {
        case ImGuiMouseCursor_Arrow:        win32_cursor = IDC_ARROW; break;
        case ImGuiMouseCursor_TextInput:    win32_cursor = IDC_IBEAM; break;
        case ImGuiMouseCursor_ResizeAll:    win32_cursor = IDC_SIZEALL; break;
        case ImGuiMouseCursor_ResizeEW:     win32_cursor = IDC_SIZEWE; break;
        case ImGuiMouseCursor_ResizeNS:     win32_cursor = IDC_SIZENS; break;
        case ImGuiMouseCursor_ResizeNESW:   win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:   win32_cursor = IDC_SIZENWSE; break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

// Allow compilation with old Windows SDK. MinGW doesn't have default _WIN32_WINNT/WINVER versions.
#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif

// Process Win32 mouse/keyboard inputs. 
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinations when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.
IMGUI_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    switch (msg)
    {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) button = 0;
        if (msg == WM_RBUTTONUP) button = 1;
        if (msg == WM_MBUTTONUP) button = 2;
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return 0;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
        return 0;
    case WM_MOUSEMOVE:
        io.MousePos.x = (signed short)(lParam);
        io.MousePos.y = (signed short)(lParam >> 16);
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 256)
            io.KeysDown[wParam] = 1;
        return 0;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 256)
            io.KeysDown[wParam] = 0;
        return 0;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if (wParam > 0 && wParam < 0x10000)
            io.AddInputCharacter((unsigned short)wParam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImGui_ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    }
    return 0;
}

#pragma endregion
