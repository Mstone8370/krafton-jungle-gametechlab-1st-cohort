#include "RabbitGameUIPanel.h"
#include <EngineLoop.h>
#include <ImGui/imgui_internal.h>
#include <Engine/Contents/GameFramework/RabbitPawn.h>
#include <UObject/UObjectIterator.h>
#include "Engine/Contents/GameFramework/RabbitPlayer.h"
#include <Engine/Engine.h>
#include "World/World.h"
#include "LevelEditor/SLevelEditor.h"
#include "Editor/UnrealEd/EditorViewportClient.h"
#include "GameFramework/RabbitGameMode.h"

inline ImVec2 operator*(const ImVec2& lhs, float rhs) {
    return ImVec2(lhs.x * rhs, lhs.y * rhs);
}
inline ImVec2 operator*(float lhs, const ImVec2& rhs) {
    return ImVec2(lhs * rhs.x, lhs * rhs.y);
}
inline ImVec2 operator+(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x + b.x, a.y + b.y);
}
inline ImVec2 operator-(const ImVec2& a, const ImVec2& b) {
    return ImVec2(a.x - b.x, a.y - b.y);
}

RabbitGameUIPanel::RabbitGameUIPanel()
{
    SetSupportedWorldTypes(EWorldTypeBitFlag::PIE);
}


void RabbitGameUIPanel::ShowBouncingWindow(float DeltaTime)
{
    if (!bShowPictureEndUI)
    {
        return;
    }

    constexpr float downDuration = 1.7f;
    constexpr float waitDuration = 2.5f;
    constexpr float upDuration = 0.8f;

    switch (bounceState)
    {
    case BounceState::Idle:
        bounce.Start(downDuration, &BounceTween::EaseOutBounce);
        bounceState = BounceState::Down;
        std::cout << "간드아앗";
        break;
    case BounceState::Down:
        if (!bounce.IsPlaying()) {
            bounceState = BounceState::Wait;
            waitTimer = 0.0f;
        }
        break;
    case BounceState::Wait:
        waitTimer += DeltaTime;
        if (waitTimer >= waitDuration) {
            bounce.Start(upDuration, &BounceTween::EaseInBack);
            bounceState = BounceState::Up;
        }
        break;
    case BounceState::Up:
        if (!bounce.IsPlaying()) {
            bounceState = BounceState::Done;
        }
        break;
    case BounceState::Done:
        // 애니메이션 종료
        break;
    }

    // 네가 준 코드에서 뷰포트 정보 받아서 중심 계산
    auto ViewPort = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewport()->GetD3DViewport();
    ImVec2 center(
        ViewPort.TopLeftX + ViewPort.Width * 0.5f,
        ViewPort.TopLeftY + ViewPort.Height * 0.4f
    );

    ImVec2 windowSize(388, 197);
    float currentX = 0.0f;

    if (bounceState == BounceState::Down)
    {
        float t = bounce.Update(DeltaTime);
        float startX = ViewPort.TopLeftX - windowSize.x - 100.0f; // 화면 왼쪽 밖
        float endX = center.x; // 화면 중앙
        currentX = startX + (endX - startX) * t;
    }
    else if (bounceState == BounceState::Wait)
    {
        currentX = center.x; // 중앙에서 대기
    }
    else if (bounceState == BounceState::Up)
    {
        float t = bounce.Update(DeltaTime);
        float startX = center.x; // 중앙에서 시작
        float endX = ViewPort.TopLeftX + ViewPort.Width + windowSize.x + 300.0f; // 화면 오른쪽 멀리
        currentX = startX + (endX - startX) * t;
    }
    else if (bounceState == BounceState::Idle)
    {
        currentX = ViewPort.TopLeftX - windowSize.x - 100.0f; // 화면 왼쪽 밖
    }
    else if (bounceState == BounceState::Done)
    {
        currentX = ViewPort.TopLeftX + ViewPort.Width + windowSize.x + 300.0f; // Up과 같은 위치로 고정
    }

    ImVec2 windowPos(currentX - windowSize.x * 0.5f, center.y - windowSize.y * 0.5f);

    ImTextureID textureID = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/PictureEnd.png")->TextureSRV;

    // 방법 1: 완전히 테두리 없는 윈도우
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);  // 테두리 크기 0
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));  // 패딩 제거
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));  // 테두리 색상 투명
    ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));  // 그림자 제거

    ImGui::Begin("📷 PictureEnd", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoBackground);  // 배경 제거 플래그 추가

    ImVec2 imagePos = ImGui::GetWindowPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // SRV 이미지만 그리기
    drawList->AddImage(
        textureID,
        imagePos,
        ImVec2(imagePos.x + windowSize.x, imagePos.y + windowSize.y),
        ImVec2(0, 0),
        ImVec2(1, 1)
    );

    ImGui::End();
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(3);
}

void RabbitGameUIPanel::Restart()
{
    ClearDeathTimer();
    ResetBounce();
    IsSucceed = false;

    if (ARabbitGameMode* RabbitGameMode = Cast<ARabbitGameMode>(GEngine->ActiveWorld->GetGameMode()))
    {
        RabbitGameMode->Restart();
    }
}

void RabbitGameUIPanel::StartDeathTimer()
{
    bDeathTriggered = true;  // 죽음 트리거
    DeathTimer = 0.0f;       // 타이머 리셋
    bShowDeathUI = false;    // 아직 UI는 보이지 않음
}

void RabbitGameUIPanel::ClearDeathTimer()
{
    bDeathTriggered = false;
    DeathTimer = 0.0f;
    bShowDeathUI = false;
}

void RabbitGameUIPanel::StartSuccessEffect()
{
    IsSucceed = true;
}

void RabbitGameUIPanel::RenderDeathUI()
{
    if (!bDeathTriggered)
    {
        return;
    }
    
    // 죽음이 트리거되었고 아직 UI가 표시되지 않았다면
    if (bDeathTriggered && !bShowDeathUI)
    {
        DeathTimer += FEngineLoop::DeltaTime;

        // 지연 시간이 지나면 UI 표시
        if (DeathTimer >= deathUIDelay)
        {
            bShowDeathUI = true;
        }
    }

    if (!bShowDeathUI)
    {
        return;
    }
    
    auto ViewPort = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewport()->GetD3DViewport();
    // 뷰포트 중심 계산
    ImVec2 centerPos(
        ViewPort.TopLeftX + ViewPort.Width * 0.5f,
        ViewPort.TopLeftY + ViewPort.Height * 0.5f
    );
    // 창 크기 및 위치
    ImVec2 windowSize(1920, 1187);
    ImVec2 windowPos(
        centerPos.x - windowSize.x * 0.5f,
        centerPos.y - windowSize.y * 0.52f
    );
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    // 스타일 설정
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // 완전 투명
    ImGui::Begin("💀 DeathUI", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);
    ImVec2 imagePos = ImGui::GetWindowPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImTextureID textureID = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/DeathBG.png")->TextureSRV;
    // 배경 이미지 그리기
    drawList->AddImage(textureID,
        imagePos,
        ImVec2(imagePos.x + windowSize.x, imagePos.y + windowSize.y),
        ImVec2(0, 0), ImVec2(1, 1)
    );

  
    // 커스텀 이미지 버튼
    ImVec2 buttonSize(250, 100);
    ImVec2 buttonPos(
        imagePos.x + (windowSize.x - buttonSize.x) * 0.5f,
        imagePos.y + windowSize.y - 200.f
    );

    // 버튼 배경 이미지 그리기
    ImTextureID buttonTextureID = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/RestartButton.png")->TextureSRV;

    // 마우스 호버 체크
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = (mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonSize.x &&
        mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonSize.y);

    // 호버 상태에 따른 색상 조정 (선택사항)
    ImU32 buttonTint = isHovered ? IM_COL32(255, 255, 255, 200) : IM_COL32(255, 255, 255, 255);

    drawList->AddImage(buttonTextureID,
        buttonPos,
        ImVec2(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y),
        ImVec2(0, 0), ImVec2(1, 1),
        buttonTint
    );

    // 클릭 감지
    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        Restart();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void RabbitGameUIPanel::RenderSuceessUI()
{
    if (!IsSucceed)
    {
        return;
    }

    auto ViewPort = GEngineLoop.GetLevelEditor()->GetActiveViewportClient()->GetViewport()->GetD3DViewport();
    // 뷰포트 중심 계산
    ImVec2 centerPos(
        ViewPort.TopLeftX + ViewPort.Width * 0.5f,
        ViewPort.TopLeftY + ViewPort.Height * 0.5f
    );
    // 창 크기 및 위치
    ImVec2 windowSize(1920, 1187);
    ImVec2 windowPos(
        centerPos.x - windowSize.x * 0.5f,
        centerPos.y - windowSize.y * 0.52f
    );
    ImGui::SetNextWindowSize(windowSize, ImGuiCond_Always);
    ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always);
    // 스타일 설정
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 15.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // 완전 투명
    ImGui::Begin("💀 DeathUI", nullptr,
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar);
    ImVec2 imagePos = ImGui::GetWindowPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImTextureID textureID = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Success.png")->TextureSRV;
    // 배경 이미지 그리기
    drawList->AddImage(textureID,
        imagePos,
        ImVec2(imagePos.x + windowSize.x, imagePos.y + windowSize.y),
        ImVec2(0, 0), ImVec2(1, 1)
    );


    // 커스텀 이미지 버튼
    ImVec2 buttonSize(250, 100);
    ImVec2 buttonPos(
        imagePos.x + (windowSize.x - buttonSize.x) * 0.5f,
        imagePos.y + windowSize.y - 200.f
    );

    // 버튼 배경 이미지 그리기
    ImTextureID buttonTextureID = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/RestartButton.png")->TextureSRV;

    // 마우스 호버 체크
    ImVec2 mousePos = ImGui::GetMousePos();
    bool isHovered = (mousePos.x >= buttonPos.x && mousePos.x <= buttonPos.x + buttonSize.x &&
        mousePos.y >= buttonPos.y && mousePos.y <= buttonPos.y + buttonSize.y);

    // 호버 상태에 따른 색상 조정 (선택사항)
    ImU32 buttonTint = isHovered ? IM_COL32(255, 255, 255, 200) : IM_COL32(255, 255, 255, 255);

    drawList->AddImage(buttonTextureID,
        buttonPos,
        ImVec2(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y),
        ImVec2(0, 0), ImVec2(1, 1),
        buttonTint
    );

    // 클릭 감지
    if (isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        Restart();
    }
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
}

void RabbitGameUIPanel::OnResize(HWND hWnd)
{
    RECT ClientRect;
    GetClientRect(hWnd, &ClientRect);
    Width = static_cast<float>(ClientRect.right - ClientRect.left);
    Height = static_cast<float>(ClientRect.bottom - ClientRect.top);
}


void RabbitGameUIPanel::RenderGallery()
{
    TArray<FRenderTargetRHI*> Pictures = PlayerCam->GetPicturesRHI();

    // 각 사진의 작은 아이콘 SRV를 담을 변수 (직접 연결하세요)
    auto Icon1 = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Slave.png")->TextureSRV;
    auto Icon2 = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Lab.png")->TextureSRV;
    auto Icon3 = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Carrot.png")->TextureSRV;
    auto Icon4 = (ImTextureID)FEngineLoop::ResourceManager.GetTexture(L"Assets/Texture/Carrot.png")->TextureSRV;
    ImTextureID OverlayIcons[4] = { Icon1,Icon2,Icon3,Icon4 };

    constexpr float THUMBNAIL_SIZE = 128.0f;
    constexpr float spacing = 15.0f;
    constexpr int32 MaxSlots = 4;
    constexpr float totalWidth = (THUMBNAIL_SIZE * MaxSlots) + (spacing * (MaxSlots - 1)) + 50.0f;
    constexpr float panelHeight = THUMBNAIL_SIZE + 30.0f;

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 panelPos = ImVec2(
        viewport->WorkPos.x + (viewport->WorkSize.x - totalWidth) * 0.5f,
        viewport->WorkPos.y + viewport->WorkSize.y - panelHeight - 5.0f // 바닥에서 살짝 띄움
    );

    ImGui::SetNextWindowPos(panelPos, ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(totalWidth, panelHeight));
    ImGui::Begin("PhotoSlots", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);

    for (int32 slotIdx = 0; slotIdx < MaxSlots; ++slotIdx)
    {
        ImGui::PushID(slotIdx);

        const FRenderTargetRHI* picturePtr = (slotIdx < Pictures.Num()) ? Pictures[slotIdx] : nullptr;
        ImTextureID baseTexture = picturePtr && picturePtr->SRV
            ? reinterpret_cast<ImTextureID>(picturePtr->SRV)
            : OverlayIcons[slotIdx]; // 없으면 아이콘으로 대체

        if (baseTexture)
        {
            if (ImGui::ImageButton("##thumbnail", baseTexture, ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
            {
                if (picturePtr && picturePtr->SRV)
                {
                    showLargeView = true;
                    selectedPhotoIndex = slotIdx;
                    selectedPicture = picturePtr;
                }
            }
        }
        else
        {
            // 아이콘도 없을 경우 빈 배경 (선택사항)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            ImGui::Button("##empty", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
            ImGui::PopStyleColor();
        }

        ImGui::PopID();

        if (slotIdx < MaxSlots - 1)
        {
            ImGui::SameLine(0.0f, spacing);
        }
    }


    ImGui::End();

    // 확대 보기 모달
    if (showLargeView && selectedPicture && selectedPicture->SRV)
    {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Picture Viewer", &showLargeView, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize))
        {
            D3D11_TEXTURE2D_DESC Desc;
            selectedPicture->Texture2D->GetDesc(&Desc);

            const float TextureWidth = static_cast<float>(Desc.Width);
            const float TextureHeight = static_cast<float>(Desc.Height);

            ImVec2 available = ImGui::GetContentRegionAvail();

            float AspectRatio = TextureWidth / TextureHeight;

            float DisplayWidth = available.x;
            float DisplayHeight = DisplayWidth / AspectRatio;
            if (DisplayHeight > available.y)
            {
                DisplayHeight = available.y;
                DisplayWidth = DisplayHeight * AspectRatio;
            }
            if (DisplayWidth > available.x)
            {
                DisplayWidth = available.x;
                DisplayHeight = DisplayWidth / AspectRatio;
            }

            float cursorX = (available.x - DisplayWidth) * 0.5f;
            float cursorY = (available.y - DisplayHeight) * 0.5f;

            ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPos().x + cursorX, ImGui::GetCursorPos().y + cursorY));
            
            ImGui::Image(reinterpret_cast<ImTextureID>(selectedPicture->SRV), ImVec2(DisplayWidth, DisplayHeight));

            if (ImGui::Button("Close"))
            {
                showLargeView = false;
                selectedPicture = nullptr;
                selectedPhotoIndex = -1;
            }
        }
        ImGui::End();
    }
}



bool RabbitGameUIPanel::RegisterPlayerCamera()
{
    bool Registered = false;

    for (auto Rabbit : TObjectRange<ARabbitPlayer>())
    {
        if (Rabbit->GetRabbitCamera())
        {
            PlayerCam = Rabbit->GetRabbitCamera();
            Registered = true;
        }
    }

    return Registered;
}

void RabbitGameUIPanel::RenderCameraCool()
{

    float progress = 1.0f - (PlayerCam->GetCameraCoolTime() / PlayerCam->GetCameraCoolTimeInit());

    ImVec2 screen_size = ImGui::GetIO().DisplaySize;
    float radius = 60.0f;
    ImVec2 padding(40.0f, 40.0f);
    ImVec2 center = ImVec2(radius + padding.x, screen_size.y - radius - padding.y);

    ImDrawList* draw_list = ImGui::GetBackgroundDrawList(); // 또는 ForegroundDrawList

    // 배경 원
    draw_list->AddCircle(center, radius, IM_COL32(100, 100, 100, 255), 64, 2.0f);

    // 진행 원호
    draw_list->PathClear();
    int segments = 64;
    for (int i = 0; i <= segments * progress; ++i)
    {
        float angle = (i / (float)segments) * 2 * IM_PI - IM_PI / 2;
        draw_list->PathLineTo(center + ImVec2(cosf(angle), sinf(angle)) * radius);
    }
    draw_list->PathStroke(IM_COL32(100, 255, 100, 255), false, 4.0f);

    // 텍스트 (남은 시간)
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.1fs", PlayerCam->GetCameraCoolTime());
    ImVec2 text_size = ImGui::CalcTextSize(buffer);
    draw_list->AddText(center - text_size * 0.5f, IM_COL32_WHITE, buffer);


}

void RabbitGameUIPanel::ResetBounce()
{
    bounce.Reset();
    bounceState = BounceState::Idle;
    waitTimer = 0.0f;
    bShowPictureEndUI = false;
}

void RabbitGameUIPanel::OnPictureEndUI()
{
    bShowPictureEndUI = true;
}

void RabbitGameUIPanel::Render()
{

    if (!RegisterPlayerCamera())
    {
        return;
    }

    RenderDeathUI();
    RenderCameraCool();
    RenderGallery();
    RenderSuceessUI();

    ShowBouncingWindow(FEngineLoop::DeltaTime);
}
