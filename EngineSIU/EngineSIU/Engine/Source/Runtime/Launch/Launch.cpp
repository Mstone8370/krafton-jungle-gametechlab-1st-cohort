#include "Core/HAL/PlatformType.h"
#include "EngineLoop.h"

extern "C"
{
    // NVIDIA 외장 그래픽 강제 사용 설정
    __declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

    // AMD 외장 그래픽 강제 사용 설정
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}

FEngineLoop GEngineLoop;

void RedirectIOToConsole() {
    AllocConsole(); // 콘솔 창 생성

    // 표준 입출력 스트림을 콘솔에 연결
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    std::cout << "Debug Console Initialized!" << std::endl;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
    RedirectIOToConsole();
#endif
    // 사용 안하는 파라미터들
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    UNREFERENCED_PARAMETER(nShowCmd);

    GEngineLoop.Init(hInstance);
    GEngineLoop.Tick();
    GEngineLoop.Exit();

#ifdef _DEBUG
    FreeConsole();
#endif
  
    return 0;
}
