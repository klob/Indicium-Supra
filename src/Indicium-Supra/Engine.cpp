#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Indicium/Engine/IndiciumCore.h"
#include "Indicium/Engine/IndiciumDirect3D9.h"
#include "Indicium/Engine/IndiciumDirect3D10.h"
#include "Indicium/Engine/IndiciumDirect3D11.h"
#include "Indicium/Engine/IndiciumDirect3D12.h"

#include "Game/Game.h"


typedef struct _INDICIUM_ENGINE
{
    INDICIUM_D3D_VERSION GameVersion;

    PFN_EVT_INDICIUM_GAME_HOOKED EvtIndiciumGameHooked;

    INDICIUM_D3D9_EVENT_CALLBACKS EventsD3D9;

    INDICIUM_D3D10_EVENT_CALLBACKS EventsD3D10;

    INDICIUM_D3D11_EVENT_CALLBACKS EventsD3D11;

    INDICIUM_D3D12_EVENT_CALLBACKS EventsD3D12;

    HANDLE EngineThread;

    HANDLE EngineCancellationEvent;

} INDICIUM_ENGINE;


INDICIUM_API PINDICIUM_ENGINE IndiciumEngineAlloc(void)
{
    auto engine = static_cast<PINDICIUM_ENGINE>(malloc(sizeof(INDICIUM_ENGINE)));

    if (!engine) {
        return nullptr;
    }

    RtlZeroMemory(engine, sizeof(INDICIUM_ENGINE));

    return engine;
}

INDICIUM_API INDICIUM_ERROR IndiciumEngineInit(PINDICIUM_ENGINE Engine, PFN_EVT_INDICIUM_GAME_HOOKED EvtIndiciumGameHooked)
{
    if (!Engine) {
        return INDICIUM_ERROR_INVALID_ENGINE_HANDLE;
    }

    if (Engine->EventsD3D9.Size != sizeof(INDICIUM_D3D9_EVENT_CALLBACKS)) {
        return INDICIUM_ERROR_INVALID_D3D9_CALLBACKS_SIZE;
    }

    if (Engine->EventsD3D10.Size != sizeof(INDICIUM_D3D10_EVENT_CALLBACKS)) {
        return INDICIUM_ERROR_INVALID_D3D10_CALLBACKS_SIZE;
    }

    if (Engine->EventsD3D11.Size != sizeof(INDICIUM_D3D11_EVENT_CALLBACKS)) {
        return INDICIUM_ERROR_INVALID_D3D11_CALLBACKS_SIZE;
    }

    if (Engine->EventsD3D12.Size != sizeof(INDICIUM_D3D12_EVENT_CALLBACKS)) {
        return INDICIUM_ERROR_INVALID_D3D12_CALLBACKS_SIZE;
    }

    //
    // Callback invoked when initialization finished
    // 
    Engine->EvtIndiciumGameHooked = EvtIndiciumGameHooked;

    //
    // Event to notify engine thread about termination
    // 
    Engine->EngineCancellationEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

    return INDICIUM_ERROR_NONE;
}

INDICIUM_API VOID IndiciumEngineSetD3D9EventCallbacks(PINDICIUM_ENGINE Engine, PINDICIUM_D3D9_EVENT_CALLBACKS Callbacks)
{
    if (Engine) {
        Engine->EventsD3D9 = *Callbacks;
    }
}

INDICIUM_API VOID IndiciumEngineSetD3D10EventCallbacks(PINDICIUM_ENGINE Engine, PINDICIUM_D3D10_EVENT_CALLBACKS Callbacks)
{
    if (Engine) {
        Engine->EventsD3D10 = *Callbacks;
    }
}

INDICIUM_API VOID IndiciumEngineSetD3D11EventCallbacks(PINDICIUM_ENGINE Engine, PINDICIUM_D3D11_EVENT_CALLBACKS Callbacks)
{
    if (Engine) {
        Engine->EventsD3D11 = *Callbacks;
    }
}

INDICIUM_API VOID IndiciumEngineSetD3D12EventCallbacks(PINDICIUM_ENGINE Engine, PINDICIUM_D3D12_EVENT_CALLBACKS Callbacks)
{
    if (Engine) {
        Engine->EventsD3D12 = *Callbacks;
    }
}
