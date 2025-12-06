#pragma once

#include "UObject/ObjectMacros.h"
#include "UObject/ObjectTypes.h"

#include "UnrealEd/EditorPanel.h"
#include <RabbitCamera.h>

// 트윈 상태 구조체
struct BounceTween {
    float Elapsed = 0.0f;
    float Duration = 1.0f;
    bool Playing = false;

    void Reset() {
        Elapsed = 0.0f;
        Duration = 1.0f;
        Playing = false;
        CurrentEase = &BounceTween::EaseOutBounce; // 기본값으로 초기화 (선택사항)
    }

    float EaseOutBounce(float t) {
        if (t < (1 / 2.75f)) {
            return 7.5625f * t * t;
        }
        else if (t < (2 / 2.75f)) {
            t -= 1.5f / 2.75f;
            return 7.5625f * t * t + 0.75f;
        }
        else if (t < (2.5f / 2.75f)) {
            t -= 2.25f / 2.75f;
            return 7.5625f * t * t + 0.9375f;
        }
        else {
            t -= 2.625f / 2.75f;
            return 7.5625f * t * t + 0.984375f;
        }
    }

    float EaseInBack(float t) {
        const float s = 1.70158f;
        return t * t * ((s + 1) * t - s);
    }

    // Ease 함수 포인터 타입 정의
    using EaseFunc = float(BounceTween::*)(float);

    EaseFunc CurrentEase = &BounceTween::EaseOutBounce;

    void Start(float duration = 1.0f, EaseFunc easeFunc = &BounceTween::EaseOutBounce) {
        Duration = duration;
        Elapsed = 0.0f;
        Playing = true;
        CurrentEase = easeFunc;
    }

    float Update(float deltaTime) {
        if (!Playing) return 1.0f;

        Elapsed += deltaTime;
        float t = std::min(Elapsed / Duration, 1.0f);
        if (t >= 1.0f) Playing = false;

        return (this->*CurrentEase)(t);
    }

    bool IsPlaying() const { return Playing; }
};


enum class BounceState
{
    Down,
    Wait,
    Up,
    Idle,
    Done
};


class RabbitGameUIPanel : public UEditorPanel
{
public:
    RabbitGameUIPanel();
    
    void Render() override;
    void ShowBouncingWindow(float deltaTime);
    void Restart();
    
    void StartDeathTimer();
    void ClearDeathTimer();
    
    void StartSuccessEffect();
    bool IsSucceed=false;

    void RenderDeathUI();
    void RenderSuceessUI();
    void OnPictureEndUI();
private:
    virtual void OnResize(HWND hWnd) override;
    
    void RenderGallery();
    
    bool RegisterPlayerCamera();
    
    void RenderCameraCool();

    void ResetBounce();

   
    
    float Width;
    float Height;

    bool bShowPictureEndUI=false;
    // 전역 또는 static 변수
    BounceTween bounce;
    bool hasStarted = false;
    BounceState bounceState = BounceState::Idle;
    float waitTimer = 3.0f;
    bool bShowDeathUI = false;
    bool bDeathTriggered = false;  // 죽음이 트리거되었는지
    float deathUIDelay = 4.5f;     // 지연 시간 (초)
    float deathTimer = 0.0f;       // 현재 타이머
    float DeathTimer = 0.0f;
    std::shared_ptr<RabbitCamera> PlayerCam;

    bool showLargeView = false;
    int32 selectedPhotoIndex = -1;
    const FRenderTargetResource* selectedPicture = nullptr;

};

